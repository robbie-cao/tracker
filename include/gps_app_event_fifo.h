/******************************************************************************
* File name: 
*	gps_app_event_fifo.h
*
* Description:
*	Header file of Event FIFO for GPS locate application
*
* Author:
*	Hongji Zhou
*
* Date:
*	2007-12-01
******************************************************************************/
#ifndef _GPS_APP_EVENT_FIFO_H_
#define _GPS_APP_EVENT_FIFO_H_

#include "kal_release.h"
#include "gps_app_unconfigurable_const.h"

/*
GPS locate event operation enum
*/
enum {
	GPS_APP_EVENT_OP_UNKNOWN,

	GPS_APP_EVENT_OP_GPRSUPLOAD,

	GPS_APP_EVENT_OP_TOTAL
};

typedef struct
{
	kal_uint8 OpCode;
	void* LocalPara;
} GPSAppEvent_t;

void GPSAppEventFifoInit(void);
kal_bool GPSAppEventFifoPush(const GPSAppEvent_t* Event, 
									kal_bool AutoFreeLocalPara);
kal_bool GPSAppEventFifoPop(void);
kal_bool GPSAppEventFifoIsEmpty(void);
kal_bool GPSAppEventFifoIsFull(void);
GPSAppEvent_t* GPSAppEventFifoCurrItem(void);

#endif //_GPS_APP_EVENT_FIFO_H_

