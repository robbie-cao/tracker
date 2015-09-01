/******************************************************************************
* File name: 
*	gps_app_vibration_sensor.c
*
* Description: 
*	This module provide the interface of vibration sensor for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.05.13
*
******************************************************************************/
#include "kal_release.h"
#include "gpio_sw.h"
#include "eint.h"

#include "gps_app_timer_mgr.h"
#include "gps_app_nvram_mgr.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_vibration_sensor.h"
#include "gps_app.h"

#ifdef VIBRATION_SENSOR_SUPPORT

#define _VIBSENSOR_DBG_	(0)

/******************************************************************************
* Local constants definition
******************************************************************************/

/******************************************************************************
* Global variables
******************************************************************************/
static GPSAppTimer_t gGPSVibSensorTimer = NULL;

#ifndef VIBRATION_SENSOR_EINT
static kal_uint8 gGPSVibSensorPollingInterval = 0;
static kal_uint16 gGPSVibSensorSilenceThreshold = 0;
static char gGPSVibSensorPreviousIOValue;
static GPSLocateVibSensorStatus_t gGPSVibSensorCurrState;
static kal_uint16 gSilenceCounter;

/******************************************************************************
* Static functions
******************************************************************************/
static void GPSLocateVibSensorExpireProc(GPSAppTimerID_t Id);

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSLocateVibSensorInit
*
* Usage:
*	Init vibration sensor monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateVibSensorInit(void)
{
}

/******************************************************************************
* Function:
*	GPSLocateVibSensorStart
*
* Usage:
*	Start vibration sensor monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateVibSensorStart(void)
{
	kal_bool bResult;
	kal_uint8 SilenceThreshold;

	if (gGPSVibSensorTimer != NULL)
	{
		//has already started, do nothing
		return;
	}
	bResult = GPSLocateNvramReadRecord(
				GPS_NVRAM_RECID_VIBSENSOR_POLLINGINTERVAL,
				&gGPSVibSensorPollingInterval,
				sizeof(kal_uint8)
				);
	if (bResult != KAL_TRUE)
	{
		return;
	}
	bResult = GPSLocateNvramReadRecord(
				GPS_NVRAM_RECID_VIBSENSOR_SILENCETHRESHOLD,
				&SilenceThreshold,
				sizeof(kal_uint8)
				);
	if (bResult != KAL_TRUE)
	{
		return;
	}
	/*
	NOTE: 
		unit of SilenceThreshold is minute
		unit of gGPSVibSensorPollingInterval is 50ms
		so, the following convert is needed.
	*/
	gGPSVibSensorSilenceThreshold = (SilenceThreshold*60*1000)/(gGPSVibSensorPollingInterval*50);
	gGPSVibSensorTimer = GPSAppTimer_Create(
								GENERAL_GPSAPP_TIMERID,
								GPSLocateVibSensorExpireProc,
								gGPSVibSensorPollingInterval*KAL_TICKS_50_MSEC,
								gGPSVibSensorPollingInterval*KAL_TICKS_50_MSEC,
								KAL_TRUE
								);
	/*
	Configurate GPIO: input, pull-up/down disable
	*/
	GPIO_ModeSetup(GPS_VIBRATION_SENSOR_GPIO, 0);
	GPIO_InitIO(INPUT, GPS_VIBRATION_SENSOR_GPIO);
	GPIO_PullenSetup(GPS_VIBRATION_SENSOR_GPIO, KAL_FALSE);
	//get the initial IO value
	gGPSVibSensorPreviousIOValue = GPIO_ReadIO(GPS_VIBRATION_SENSOR_GPIO);
	//initialize the state
	gGPSVibSensorCurrState = GPSLOCATE_VIBSENSORSTATUS_SILENCE;
	//initialize silence counter
	gSilenceCounter = 0;
}

/******************************************************************************
* Function:
*	GPSLocateVibSensorStop
*
* Usage:
*	Stop vibration sensor monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateVibSensorStop(void)
{
	if (gGPSVibSensorTimer == NULL)
	{
		//has not yet started, do nothing
		return;
	}
	GPSAppTimer_Stop(gGPSVibSensorTimer, KAL_TRUE);
	gGPSVibSensorTimer = NULL;
}

static void GPSLocateVibSensorExpireProc(GPSAppTimerID_t Id)
{
	char CurrIOValue;

	CurrIOValue = GPIO_ReadIO(GPS_VIBRATION_SENSOR_GPIO);
	#if _VIBSENSOR_DBG_
	trace_printf("VSEP: CIO=%d, PIO=%d", CurrIOValue, gGPSVibSensorPreviousIOValue);
	trace_printf("VSEP: CS=%d", gGPSVibSensorCurrState);
	trace_printf("VSEP: SC=%d, ST=%d", gSilenceCounter, gGPSVibSensorSilenceThreshold);
	#endif //_VIBSENSOR_DBG_
	if (CurrIOValue != gGPSVibSensorPreviousIOValue)
	{
		///////// status changed ///////////
		// 1. save the current IO value as the previous one
		gGPSVibSensorPreviousIOValue = CurrIOValue;
		// 2. clear the silence counter
		gSilenceCounter = 0;
		// 3. check and post vibration message if needed
		if (gGPSVibSensorCurrState == GPSLOCATE_VIBSENSORSTATUS_SILENCE)
		{
			//post vibration message
			GPSLocateVibSensorStatusIndStruct_t *pStatusInd;
			pStatusInd = (GPSLocateVibSensorStatusIndStruct_t*)construct_local_para(sizeof(GPSLocateVibSensorStatusIndStruct_t), TD_UL);
			pStatusInd->Status = GPSLOCATE_VIBSENSORSTATUS_VIBRATING;
			SendMsg2GPSApp(
				stack_int_get_active_module_id(),
				MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND,
				(void*)pStatusInd
				);
			#if _VIBSENSOR_DBG_
			trace_printf("VSEP: V");
			#endif //_VIBSENSOR_DBG_
			//change state
			gGPSVibSensorCurrState = GPSLOCATE_VIBSENSORSTATUS_VIBRATING;
		}
	}
	else
	{
		////////// status not changed ///////////////
		gSilenceCounter++;
		if (gGPSVibSensorCurrState == GPSLOCATE_VIBSENSORSTATUS_VIBRATING 
			&& gSilenceCounter >= gGPSVibSensorSilenceThreshold)
		{
			//////// consider as silence ///////////

			//post silence message
			GPSLocateVibSensorStatusIndStruct_t *pStatusInd;
			pStatusInd = (GPSLocateVibSensorStatusIndStruct_t*)construct_local_para(sizeof(GPSLocateVibSensorStatusIndStruct_t), TD_UL);
			pStatusInd->Status = GPSLOCATE_VIBSENSORSTATUS_SILENCE;
			SendMsg2GPSApp(
				stack_int_get_active_module_id(),
				MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND,
				(void*)pStatusInd
				);
			#if _VIBSENSOR_DBG_
			trace_printf("VSEP: S");
			#endif //_VIBSENSOR_DBG_
			//change state
			gGPSVibSensorCurrState = GPSLOCATE_VIBSENSORSTATUS_SILENCE;
		}
	}
}

#else	/* VIBRATION_SENSOR_EINT */

/******************************************************************************
 * Local variables
 ******************************************************************************/
//current interrupt sensitive polarity
static kal_bool gbCurrentPolarity;	// KAL_FALSE: low level; KAL_TRUE: high level
static kal_bool gGPSVibSensorStart = KAL_FALSE;

/******************************************************************************
 * Static functions
 ******************************************************************************/
static void GPSLocateVibSensorHISR(void);
static void GPSLocateVibSensorExpireProc(GPSAppTimerID_t Id);

/******************************************************************************
* Function:
*	GPSLocateVibSensorInit
*
* Usage:
*	Init vibration sensor monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateVibSensorInit(void)
{
#if 0
	/*
	Configurate GPIO: input, pull-up/down disable
	*/
	GPIO_ModeSetup(GPS_VIBRATION_SENSOR_GPIO, 0);
	GPIO_InitIO(INPUT, GPS_VIBRATION_SENSOR_GPIO);
	GPIO_PullenSetup(GPS_VIBRATION_SENSOR_GPIO, KAL_FALSE);
	// config gpio 31 as above because of eint 1 is connect to gpio 31 as a temp solution
#endif

	EINT_Set_HW_Debounce(GPS_VIBSENSOR_EINT_NO, 1);
	gbCurrentPolarity = KAL_FALSE;
	EINT_Registration(GPS_VIBSENSOR_EINT_NO, 
					KAL_TRUE, 
					gbCurrentPolarity, 
					GPSLocateVibSensorHISR, 
					KAL_TRUE
					);
	//Mask this EINT after registration, unmask it before using it
	EINT_Mask(GPS_VIBSENSOR_EINT_NO);
}

/******************************************************************************
* Function:
*	GPSLocateVibSensorStart
*
* Usage:
*	Start vibration sensor monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateVibSensorStart(void)
{
	if (gGPSVibSensorStart == KAL_TRUE) return;
	gGPSVibSensorStart = KAL_TRUE;
	/*
	* Enabling the eint to detect the vibration sensor status
	*/
	EINT_UnMask(GPS_VIBSENSOR_EINT_NO);

	if (gGPSVibSensorTimer != NULL)
	{
		GPSAppTimer_Reset(gGPSVibSensorTimer, 
						GPSLocateVibSensorExpireProc,
						GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
						GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
						KAL_TRUE
						);
	}
	else
	{
		gGPSVibSensorTimer = GPSAppTimer_Create(
									GENERAL_GPSAPP_TIMERID,
									GPSLocateVibSensorExpireProc,
									GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
									GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
									KAL_TRUE
									);
	}
}

/******************************************************************************
* Function:
*	GPSLocateVibSensorStop
*
* Usage:
*	Stop vibration sensor monitor
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSLocateVibSensorStop(void)
{
	if (gGPSVibSensorStart == KAL_FALSE) return;
	gGPSVibSensorStart = KAL_FALSE;
	//Mask this EINT to stop detecting the vibration sensor status
	EINT_Mask(GPS_VIBSENSOR_EINT_NO);

	if (gGPSVibSensorTimer != NULL)
	{
		GPSAppTimer_Stop(gGPSVibSensorTimer, KAL_TRUE);
		gGPSVibSensorTimer = NULL;
	}
}

static void GPSLocateVibSensorHISR(void)
{
	//disable this interrupt
	/*
	* Hongji fix me:
	*	This external interrupt has already been disabled by EINT_LISR before activating this 
	*	HISR. So don't disable it one more time.
	*/
	//EINT_Mask(GPS_VIBSENSOR_EINT_NO);
	if (gGPSVibSensorStart != KAL_TRUE)
	{
		//change the sensitive polarity
		gbCurrentPolarity = !gbCurrentPolarity;
		EINT_Set_Polarity(GPS_VIBSENSOR_EINT_NO, gbCurrentPolarity);

		return;
	}

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPSLocateVibSensorStatusIndStruct_t *pStatusInd;

		//post vibration message
		pStatusInd = (GPSLocateVibSensorStatusIndStruct_t*)construct_local_para(sizeof(GPSLocateVibSensorStatusIndStruct_t), TD_UL);
		pStatusInd->Status = GPSLOCATE_VIBSENSORSTATUS_VIBRATING;
		SendMsg2GPSApp(
			stack_int_get_active_module_id(),
			MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND,
			(void*)pStatusInd
			);
	}

	if (gGPSVibSensorTimer != NULL)
	{
		GPSAppTimer_Reset(gGPSVibSensorTimer, 
						GPSLocateVibSensorExpireProc,
						GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
						GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
						KAL_TRUE
						);
	}
	else
	{
		gGPSVibSensorTimer = GPSAppTimer_Create(
									GENERAL_GPSAPP_TIMERID,
									GPSLocateVibSensorExpireProc,
									GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
									GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT*KAL_TICKS_1_MIN,
									KAL_TRUE
									);
	}

	//change the sensitive polarity
	gbCurrentPolarity = !gbCurrentPolarity;
	EINT_Set_Polarity(GPS_VIBSENSOR_EINT_NO, gbCurrentPolarity);

	//enable this interrupt
	//EINT_UnMask(GPS_VIBSENSOR_EINT_NO);
}

static void GPSLocateVibSensorExpireProc(GPSAppTimerID_t Id)
{
	GPSLocateVibSensorStatusIndStruct_t *pStatusInd;

	if (gGPSVibSensorStart != KAL_TRUE) return;
	//post silence message
	pStatusInd = (GPSLocateVibSensorStatusIndStruct_t*)construct_local_para(sizeof(GPSLocateVibSensorStatusIndStruct_t), TD_UL);
	pStatusInd->Status = GPSLOCATE_VIBSENSORSTATUS_SILENCE;
	SendMsg2GPSApp(
		stack_int_get_active_module_id(),
		MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND,
		(void*)pStatusInd
		);
}

#endif	/* VIBRATION_SENSOR_EINT */

#endif //VIBRATION_SENSOR_SUPPORT

