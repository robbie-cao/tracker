/******************************************************************************
* File name: 
*	gps_app_defence_moniter.c
*
* Description:
*	GPS defence monitor
*
* Author:
*	Robbie Cao
*
* Date:
*	2008-2-28 15:10
******************************************************************************/
#include "gps_app_defence_moniter.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_timer_mgr.h"

#ifdef GPS_DEFENCE_FEATURE
#ifndef VIBRATION_SENSOR_EINT
/******************************************************************************
 * Local constants definition
 ******************************************************************************/
//#define GPS_DEFENCE_ONLYONCE


/******************************************************************************
 * Local variables
 ******************************************************************************/
//current interrupt sensitive polarity
static kal_bool gbCurrentPolarity;	// KAL_FALSE: low level; KAL_TRUE: high level


/******************************************************************************
 * Global variables
 ******************************************************************************/


/******************************************************************************
 * Static functions
 ******************************************************************************/
static void GPS_DefenceMonitorHISR(void);

/******************************************************************************
 * Functions implementation
 ******************************************************************************/

/******************************************************************************
* Function:
*	GPS_DefenceMonitorInit
*
* Usage:
*	Initialize 
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPS_DefenceMonitorInit(void)
{
	EINT_Set_HW_Debounce(GPS_DEFENCE_EINT_NO, 50);
	gbCurrentPolarity = KAL_FALSE;
	EINT_Registration(GPS_DEFENCE_EINT_NO, 
					KAL_TRUE, 
					gbCurrentPolarity, 
					GPS_DefenceMonitorHISR, 
#ifdef GPS_DEFENCE_ONLYONCE
					KAL_FALSE
#else
					KAL_TRUE
#endif
					);
	//Mask this EINT after registration, unmask it before using it
	EINT_Mask(GPS_DEFENCE_EINT_NO);
	/*
	* Hongji fix me:
	*	This external interrupt has already been enabled in EINT_Registration().
	*	So enabling it again is not necessary.
	*/
	//EINT_UnMask(GPS_DEFENCE_EINT_NO);
}

/******************************************************************************
* Function:
*	GPS_DefenceMonitorHISR
*
* Usage:
*	HISR for the defence trigger interrupt 
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
static void GPS_DefenceMonitorHISR(void)
{
	//disable this interrupt
	/*
	* Hongji fix me:
	*	This external interrupt has already been disabled by EINT_LISR before activating this 
	*	HISR. So don't disable it one more time.
	*/
	//EINT_Mask(GPS_DEFENCE_EINT_NO);

	if (!GPS_APP_DefenceOn())
	{
		//change the sensitive polarity
		gbCurrentPolarity = !gbCurrentPolarity;
		EINT_Set_Polarity(GPS_DEFENCE_EINT_NO, gbCurrentPolarity);

		return;
	}

	if (gbCurrentPolarity == KAL_FALSE)
	{
	#if 0
		// defence alarm on
		GPS_APP_EnDefenceAlarm();	
	#else
		SendMsg2GPSApp(MOD_EINT_HISR, MSG_ID_GPSLOCATE_TRIGGER_DEFENCE, NULL);
	#endif
	}
#if !(defined GPS_DEFENCE_ONLYONCE)
	else
	{
		// defence alarm off
		GPS_APP_DisDefenceAlarm();
	}

	//change the sensitive polarity
	gbCurrentPolarity = !gbCurrentPolarity;
	EINT_Set_Polarity(GPS_DEFENCE_EINT_NO, gbCurrentPolarity);
#endif

	//enable this interrupt
	/*
	* Hongji fix me:
	*	This external interrupt has been registered as AUTO_UNMASK in GPS_DefenceMonitorInit().
	*	After this HISR processing, it will be enabled automatically. Therefore, it is no need to enable
	*	it specially here.
	*/
	//EINT_UnMask(GPS_DEFENCE_EINT_NO);
}

#else /* VIBRATION_SENSOR_EINT */

static GPSAppTimer_t gGPSDefenceTimer = NULL;
static kal_uint8 gGPSDefencePollingInterval = 0;
static kal_uint8 gGPSDefenceCurrState = 0xff;

/******************************************************************************
* Static functions
******************************************************************************/
static void GPS_DefenceExpireProc(GPSAppTimerID_t Id);

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPS_DefenceInit
*
* Usage:
*	Init defence monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPS_DefenceMonitorInit(void)
{
}

/******************************************************************************
* Function:
*	GPS_DefenceStart
*
* Usage:
*	Start defence monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPS_APP_EnableDefence(void)
{
	GPIO_WriteIO(1, GPS_DEFENCE_EN_GPIO);

	if (gGPSDefenceTimer != NULL)
	{
		//has already started, do nothing
		return;
	}

	gGPSDefencePollingInterval = 1; // unit: 50 ms
	/*
	NOTE: 
		unit of gGPSDefencePollingInterval is 50ms
	*/
	gGPSDefenceTimer = GPSAppTimer_Create(
								GENERAL_GPSAPP_TIMERID,
								GPS_DefenceExpireProc,
								gGPSDefencePollingInterval*KAL_TICKS_50_MSEC,
								gGPSDefencePollingInterval*KAL_TICKS_50_MSEC,
								KAL_TRUE
								);
	/*
	Configurate GPIO: input, pull-up/down disable
	*/
	GPIO_ModeSetup(GPS_DEFENCE_TIGGER_GPIO, 0);
	GPIO_InitIO(INPUT, GPS_DEFENCE_TIGGER_GPIO);
	GPIO_PullenSetup(GPS_DEFENCE_TIGGER_GPIO, KAL_FALSE);
}

/******************************************************************************
* Function:
*	GPS_DefenceStop
*
* Usage:
*	Stop defence monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPS_APP_DisableDefence(void)
{
	GPIO_WriteIO(0, GPS_DEFENCE_EN_GPIO);
	gGPSDefenceCurrState = 0xff;

	if (gGPSDefenceTimer == NULL)
	{
		//has not yet started, do nothing
		return;
	}
	GPSAppTimer_Stop(gGPSDefenceTimer, KAL_TRUE);
	gGPSDefenceTimer = NULL;
}

void GPS_APP_DefenceReset(void)
{
	GPIO_WriteIO(0, GPS_DEFENCE_EN_GPIO);
	GPIO_WriteIO(0, GPS_DEFENCE_ALARM_GPIO);
	gGPSDefenceCurrState = 0xff;
	if (gGPSDefenceTimer == NULL)
	{
		//has not yet started, do nothing
		return;
	}
	GPSAppTimer_Stop(gGPSDefenceTimer, KAL_TRUE);
	gGPSDefenceTimer = NULL;
}

static void GPS_DefenceExpireProc(GPSAppTimerID_t Id)
{
	char CurrIOValue;

	if (!GPS_APP_DefenceOn())
	{
		return;
	}

	CurrIOValue = GPIO_ReadIO(GPS_DEFENCE_TIGGER_GPIO);
	if (CurrIOValue == gGPSDefenceCurrState) return;

	if (!CurrIOValue)
	{
		SendMsg2GPSApp(stack_int_get_active_module_id(), MSG_ID_GPSLOCATE_TRIGGER_DEFENCE, NULL);
	}
#if 0
	else
	{
		// defence alarm off
		GPS_APP_DisDefenceAlarm();
	}
#endif
	gGPSDefenceCurrState = CurrIOValue;
}


#endif /* VIBRATION_SENSOR_EINT */
#endif /* GPS_DEFENCE_FEATURE */

