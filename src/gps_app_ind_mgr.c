/******************************************************************************
* File name: 
*	gps_app_ind_mgr.c
*
* Description: 
*	This module provide the interface of indicators (LED) for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.10
*
******************************************************************************/
#if defined(GPS_LOCATE_SUPPORT)

#include "kal_release.h"
#include "gpio_sw.h"
#include "nucleus.h"

#include "gps_app_ind_mgr.h"
#include "gps_app_configurable_const.h"

/******************************************************************************
* Local constants definition
******************************************************************************/

/******************************************************************************
* Local data structures definition
******************************************************************************/
typedef struct
{
	kal_bool IsFlashing;
	//current status of the indicator
	kal_bool Status;		//KAL_TRUE : On, KAL_FALSE : Off
	NU_TIMER Timer;
	kal_uint16 DurationOn;
	kal_uint16 DurationOff;
} GPSLocateIndFlashCtrlBlk_t;

/******************************************************************************
* Global variables
******************************************************************************/
#if defined(M100)
static const char GPS_LOC_IND_GPIO[GPS_LOC_IND_Total] =
{
	GPS_NETWORK_IND_GPIO,
	GPS_BATTERY_IND_GPIO
};

static GPSLocateIndFlashCtrlBlk_t gGPSLocateIndFlashCtrlBlk[GPS_LOC_IND_Total];
static kal_semid gGPSLocateIndSem = NULL;
#endif //M100
/******************************************************************************
* Static functions
******************************************************************************/
static void GPSLocateLEDIndicatorTimeOutHandler(UNSIGNED Indicator);

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSLocateLEDIndicatorInit
*
* Usage:
*	Initialize LED indicators
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateLEDIndicatorInit(void)
{
#if defined(M100)
	kal_uint8 i;
	char TName[NU_MAX_NAME+1];
	STATUS status;

	if (gGPSLocateIndSem != NULL)
	{
		//has been initialized
		//do nothing, return error directly
		return;
	}
	//create semaphore
	gGPSLocateIndSem = kal_create_sem("GPSIND SEM", 1);
	ASSERT((gGPSLocateIndSem!=NULL));

	for (i=0; i<GPS_LOC_IND_Total; i++)
	{
		gGPSLocateIndFlashCtrlBlk[i].IsFlashing = KAL_FALSE;
		gGPSLocateIndFlashCtrlBlk[i].Status = KAL_FALSE;
		gGPSLocateIndFlashCtrlBlk[i].DurationOn = gGPSLocateIndFlashCtrlBlk[i].DurationOff = 0;
		sprintf(TName, "GINDT%d", i);
		status = NU_Create_Timer(&(gGPSLocateIndFlashCtrlBlk[i].Timer), 
						TName, 
						GPSLocateLEDIndicatorTimeOutHandler, 
						i, 
						0, 
						0,
						NU_DISABLE_TIMER
						);
		ASSERT(status == NU_SUCCESS);
	}
#endif //M100
}

/******************************************************************************
* Function:
*	GPSLocateLEDIndicatorAlwaysOn
*
* Usage:
*	Drive the indicator always on/off
*
* Parameters:
*	Indicator - the indicator ID
*	OnOff - KAL_TRUE : turn on, KAL_FALSE : turn off
*
* Return:
*	KAL_TRUE : successful
*	KAL_FALSE : fail
******************************************************************************/
kal_bool GPSLocateLEDIndicatorAlwaysOn(
	GPSLocateIndicator_t Indicator,
	kal_bool OnOff
	)
{
#if defined(M100)
	if (Indicator >= GPS_LOC_IND_Total)
	{
		//invalid parameter
		return KAL_FALSE;
	}
	ASSERT((gGPSLocateIndSem!=NULL));
	//take the semaphore
	kal_take_sem(gGPSLocateIndSem, KAL_INFINITE_WAIT);
	if (gGPSLocateIndFlashCtrlBlk[Indicator].IsFlashing == KAL_TRUE)
	{	//the indicator LED is flashing
		//stop the timer
		NU_Control_Timer(&(gGPSLocateIndFlashCtrlBlk[Indicator].Timer), 
						NU_DISABLE_TIMER);
		gGPSLocateIndFlashCtrlBlk[Indicator].IsFlashing = KAL_FALSE;
	}
	//release semaphore
	kal_give_sem(gGPSLocateIndSem);
	GPIO_WriteIO(
			(OnOff == KAL_TRUE ? GPS_LED_TURN_ON : GPS_LED_TURN_OFF), 
			GPS_LOC_IND_GPIO[Indicator]
			);
	return KAL_TRUE;
#else
	return KAL_TRUE;
#endif //M100
}

/******************************************************************************
* Function:
*	GPSLocateLEDIndicatorFlash
*
* Usage:
*	Drive the indicator flash
*
* Parameters:
*	Indicator - the indicator ID
*	OnOff - KAL_TRUE : turn on, KAL_FALSE : turn off
*	DurationOn (in ticks) - the duration of the LED on. When OnOff is KAL_FALSE, ignore this
*						parameter.
*	DurationOff (in ticks) - the duration of the LED off. When OnOff is KAL_FALSE, ignore this
*						parameter.
*
* Return:
*	KAL_TRUE : successful
*	KAL_FALSE : fail
******************************************************************************/
kal_bool GPSLocateLEDIndicatorFlash(
	GPSLocateIndicator_t Indicator,
	kal_bool OnOff, 
	kal_uint16 DurationOn,
	kal_uint16 DurationOff
	)
{
#if defined(M100)
	if (Indicator >= GPS_LOC_IND_Total 
		|| (OnOff == KAL_TRUE && (DurationOn == 0 || DurationOff == 0)))
	{
		//invalid parameter
		return KAL_FALSE;
	}
	ASSERT((gGPSLocateIndSem!=NULL));
	//take the semaphore
	kal_take_sem(gGPSLocateIndSem, KAL_INFINITE_WAIT);
	if (gGPSLocateIndFlashCtrlBlk[Indicator].IsFlashing == KAL_TRUE)
	{
		//stop the timer firstly
		NU_Control_Timer(&(gGPSLocateIndFlashCtrlBlk[Indicator].Timer), 
						NU_DISABLE_TIMER);
	}
	//turn off/on the LED
	GPIO_WriteIO(OnOff == KAL_TRUE ? GPS_LED_TURN_ON : GPS_LED_TURN_OFF,
				GPS_LOC_IND_GPIO[Indicator]);
	//save flashing status
	gGPSLocateIndFlashCtrlBlk[Indicator].IsFlashing = OnOff;
	//save the status of the LED
	gGPSLocateIndFlashCtrlBlk[Indicator].Status = OnOff;
	if (OnOff == KAL_TRUE)
	{				/*	turn on	*/
		//save control data
		gGPSLocateIndFlashCtrlBlk[Indicator].DurationOn = DurationOn;
		gGPSLocateIndFlashCtrlBlk[Indicator].DurationOff = DurationOff;
		//start timer
		NU_Reset_Timer(&(gGPSLocateIndFlashCtrlBlk[Indicator].Timer), 
					GPSLocateLEDIndicatorTimeOutHandler, 
					DurationOn, 
					0,
					NU_ENABLE_TIMER
					);
	}
	//release semaphore
	kal_give_sem(gGPSLocateIndSem);
	return KAL_TRUE;
#else
	return KAL_TRUE;
#endif //M100
}

static void GPSLocateLEDIndicatorTimeOutHandler(UNSIGNED Indicator)
{
#if defined(M100)
	ASSERT(Indicator < GPS_LOC_IND_Total);
	
	if (gGPSLocateIndFlashCtrlBlk[Indicator].IsFlashing == KAL_FALSE
		|| gGPSLocateIndFlashCtrlBlk[Indicator].DurationOn == 0 
		|| gGPSLocateIndFlashCtrlBlk[Indicator].DurationOff == 0)
	{
		//invalid parameters
		//ASSERT(0);
		return;
	}
	//stop the timer firstly
	NU_Control_Timer(&(gGPSLocateIndFlashCtrlBlk[Indicator].Timer), 
					NU_DISABLE_TIMER);
	//inverse the status
	gGPSLocateIndFlashCtrlBlk[Indicator].Status = !(gGPSLocateIndFlashCtrlBlk[Indicator].Status);
	//turn off/on the LED
	GPIO_WriteIO(
		gGPSLocateIndFlashCtrlBlk[Indicator].Status == KAL_TRUE ? GPS_LED_TURN_ON : GPS_LED_TURN_OFF,
		GPS_LOC_IND_GPIO[Indicator]);
	//re-start timer
	NU_Reset_Timer(&(gGPSLocateIndFlashCtrlBlk[Indicator].Timer), 
				GPSLocateLEDIndicatorTimeOutHandler, 
				gGPSLocateIndFlashCtrlBlk[Indicator].Status == KAL_TRUE ? \
				gGPSLocateIndFlashCtrlBlk[Indicator].DurationOn : gGPSLocateIndFlashCtrlBlk[Indicator].DurationOff, 
				0,
				NU_ENABLE_TIMER
				);
#endif //M100
}

#endif //defined(GPS_LOCATE_SUPPORT)

