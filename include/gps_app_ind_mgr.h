/******************************************************************************
* File name: 
*	gps_app_ind_mgr.h
*
* Description: 
*	The header file for the interface of indicators (LED) for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.10
*
******************************************************************************/
#ifndef _GPS_APP_IND_MGR_H_
#define _GPS_APP_IND_MGR_H_
#if defined(GPS_LOCATE_SUPPORT)

#include "kal_release.h"

typedef enum
{
	GPS_LOC_IND_NETWORK = 0,		//LED for network
	GPS_LOC_IND_BATTERY,			//LED for battery

	GPS_LOC_IND_Total
} GPSLocateIndicator_t;

typedef enum
{
	GPS_LOC_IND_STATUS_ON,
	GPS_LOC_IND_STATUS_OFF,

	GPS_LOC_IND_STATUS_Invalid
} GSPLocateIndStatus_t;

void GPSLocateLEDIndicatorInit(void);
kal_bool GPSLocateLEDIndicatorAlwaysOn(
	GPSLocateIndicator_t Indicator,
	kal_bool OnOff
	);
kal_bool GPSLocateLEDIndicatorFlash(
	GPSLocateIndicator_t Indicator,
	kal_bool OnOff, 
	kal_uint16 DurationOn,
	kal_uint16 DurationOff
	);

#endif //defined(GPS_LOCATE_SUPPORT) && defined(M100)
#endif //_GPS_APP_IND_MGR_H_

