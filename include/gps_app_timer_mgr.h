/******************************************************************************
* File name: 
*	gps_app_timer_mgr.h
*
* Description: 
*	The header file for the interface of timers for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.14
*
******************************************************************************/
#ifndef _GPS_APP_TIMER_MGR_H_
#define _GPS_APP_TIMER_MGR_H_

#include "nucleus.h"
#include "kal_release.h"

typedef void* GPSAppTimer_t;
typedef UNSIGNED GPSAppTimerID_t;
typedef UNSIGNED GPSAppTimerTick_t;
typedef void (* GPSAppTimerProc_t)( GPSAppTimerID_t Id );

#define GENERAL_GPSAPP_TIMERID	(0xffffffff)

GPSAppTimer_t GPSAppTimer_Create(
	GPSAppTimerID_t Id,
	GPSAppTimerProc_t Proc,
	GPSAppTimerTick_t InitTime,
	GPSAppTimerTick_t RepeatTime,
	kal_bool AutoStart
	);
void GPSAppTimer_Start(GPSAppTimer_t Timer);
void GPSAppTimer_Reset(
	GPSAppTimer_t Timer,
	GPSAppTimerProc_t Proc,
	GPSAppTimerTick_t InitTime,
	GPSAppTimerTick_t RepeatTime,
	kal_bool AutoStart
	);
void GPSAppTimer_Stop(
	GPSAppTimer_t Timer,
	kal_bool AutoDestroy
	);
void GPSAppTimer_Destroy(GPSAppTimer_t Timer);

#endif //_GPS_APP_TIMER_MGR_H_

