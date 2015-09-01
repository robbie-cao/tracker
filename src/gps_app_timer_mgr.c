/******************************************************************************
* File name: 
*	gps_app_timer_mgr.c
*
* Description: 
*	This module provide the interface of timers for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.14
*
******************************************************************************/
#if defined(GPS_LOCATE_SUPPORT)

#include "kal_release.h"
#include "app_buff_alloc.h"

#include "gps_app_timer_mgr.h"

/******************************************************************************
* Local constants definition
******************************************************************************/
#define GPSAPPTIMERNAMEPREFIX 	("GTMGR")

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSAppTimer_Create
*
* Usage:
*	Create a timer
*
* Parameters:
*	Id
*	Proc
*	InitTime
*	RepeatTime
*	AutoStart
*
* Return:
*	GPSAppTimer_t
******************************************************************************/
GPSAppTimer_t GPSAppTimer_Create(
	GPSAppTimerID_t Id,
	GPSAppTimerProc_t Proc,
	GPSAppTimerTick_t InitTime,
	GPSAppTimerTick_t RepeatTime,
	kal_bool AutoStart
	)
{
	GPSAppTimer_t t;
	STATUS status;
	char name[ NU_MAX_NAME+1 ];

	ASSERT(Proc != NULL);
	t = (GPSAppTimer_t)get_ctrl_buffer(sizeof(NU_TIMER));
	if (Id == GENERAL_GPSAPP_TIMERID)
	{
		static unsigned char tmpId = 0xff;
		Id = tmpId--;
	}
	sprintf( name, "%s%d", GPSAPPTIMERNAMEPREFIX, (unsigned char)Id );
	status = NU_Create_Timer((NU_TIMER*)t, name, Proc, Id, InitTime, 
		RepeatTime, (AutoStart == KAL_TRUE ? NU_ENABLE_TIMER : NU_DISABLE_TIMER));

	ASSERT( status == NU_SUCCESS );
	return t;
}

/******************************************************************************
* Function:
*	GPSAppTimer_Start
*
* Usage:
*	Start a timer
*
* Parameters:
*	Timer
*
* Return:
*	None
******************************************************************************/
void GPSAppTimer_Start(GPSAppTimer_t Timer)
{
	STATUS status;
	
	ASSERT(Timer != NULL);
	status = NU_Control_Timer((NU_TIMER*)Timer, NU_ENABLE_TIMER);
	ASSERT( status == NU_SUCCESS );
}

/******************************************************************************
* Function:
*	GPSAppTimer_Reset
*
* Usage:
*	Reset a timer
*
* Parameters:
*	Timer
*
* Return:
*	None
******************************************************************************/
void GPSAppTimer_Reset(
	GPSAppTimer_t Timer,
	GPSAppTimerProc_t Proc,
	GPSAppTimerTick_t InitTime,
	GPSAppTimerTick_t RepeatTime,
	kal_bool AutoStart
	)
{
	STATUS status;

	ASSERT(Timer != NULL);
	status = NU_Control_Timer((NU_TIMER*)Timer, NU_DISABLE_TIMER);
	ASSERT( status == NU_SUCCESS );
	status = NU_Reset_Timer((NU_TIMER*)Timer, Proc, InitTime, RepeatTime,
		(AutoStart == KAL_TRUE ? NU_ENABLE_TIMER : NU_DISABLE_TIMER));
	ASSERT( status == NU_SUCCESS );
}

/******************************************************************************
* Function:
*	GPSAppTimer_Stop
*
* Usage:
*	Stop a timer
*
* Parameters:
*	Timer
*	AutoDestroy
*
* Return:
*	None
******************************************************************************/
void GPSAppTimer_Stop(
	GPSAppTimer_t Timer,
	kal_bool AutoDestroy
	)
{
	STATUS status;

	ASSERT(Timer != NULL);
	status = NU_Control_Timer((NU_TIMER*)Timer, NU_DISABLE_TIMER);
	ASSERT( status == NU_SUCCESS );
	if (AutoDestroy == KAL_TRUE)
	{
		status = NU_Delete_Timer((NU_TIMER*)Timer);
		ASSERT( status == NU_SUCCESS );
		free_ctrl_buffer(Timer);
	}
}

/******************************************************************************
* Function:
*	GPSAppTimer_Destroy
*
* Usage:
*	Destroy a timer
*
* Parameters:
*	Timer
*
* Return:
*	None
******************************************************************************/
void GPSAppTimer_Destroy(GPSAppTimer_t Timer)
{
	STATUS status;

	ASSERT(Timer != NULL);
	status = NU_Delete_Timer((NU_TIMER*)Timer);
	ASSERT( status == NU_SUCCESS );
	free_ctrl_buffer(Timer);
}

#endif //GPS_LOCATE_SUPPORT

