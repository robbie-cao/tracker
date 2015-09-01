/******************************************************************************
* File name: 
*	gps_app_upldmode_moniter.c
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
#include "gps_app_upldmode_moniter.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_timer_mgr.h"
#include "gps_app_data.h"
#include "gps_app_settings.h"

static GPSAppTimer_t gGPSUpldModeTimer = NULL;
static kal_uint8 gGPSUpldCurrMode = 0xff;

/******************************************************************************
* Static functions
******************************************************************************/
static void GPS_UpldModeMonitorProc(GPSAppTimerID_t Id);

void GPS_UpldModeMonitorInit(void)
{
	/*
	Configurate GPIO: input, pull-up/down disable
	*/
	GPIO_ModeSetup(GPS_GPRS_UPLDMODE_GPIO, 0);
	GPIO_InitIO(INPUT, GPS_GPRS_UPLDMODE_GPIO);
	GPIO_PullenSetup(GPS_GPRS_UPLDMODE_GPIO, KAL_FALSE);
}

void GPS_UpldModeMonitorStart(void)
{
	if (gGPSUpldModeTimer == NULL)
	{
		gGPSUpldModeTimer = GPSAppTimer_Create(
								GENERAL_GPSAPP_TIMERID,
								GPS_UpldModeMonitorProc,
								KAL_TICKS_5_SEC,
								KAL_TICKS_5_SEC,
								KAL_TRUE
								);
	}
	else
	{
		GPSAppTimer_Reset(
					gGPSUpldModeTimer,
					GPS_UpldModeMonitorProc,
					KAL_TICKS_5_SEC,
					KAL_TICKS_5_SEC,
					KAL_TRUE
					);
	}
}

void GPS_UpldModeMonitorStop(void)
{
	if (gGPSUpldModeTimer == NULL)
	{
		//has not yet started, do nothing
		return;
	}
	GPSAppTimer_Stop(gGPSUpldModeTimer, KAL_TRUE);
	gGPSUpldModeTimer = NULL;
}

static void GPS_UpldModeMonitorProc(GPSAppTimerID_t Id)
{
	char CurrIOValue;

	if (gps_curr_bearer_type != GPS_BEARER_GPRS)
	{
		return;
	}

	CurrIOValue = GPIO_ReadIO(GPS_GPRS_UPLDMODE_GPIO);
	if (CurrIOValue == gGPSUpldCurrMode) return;

	SendMsg2GPSApp(stack_int_get_active_module_id(), MSG_ID_GPSLOCATE_SWITCHGPRSUPLD, NULL);
	gGPSUpldCurrMode = CurrIOValue;
}

