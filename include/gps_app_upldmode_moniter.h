/******************************************************************************
* File name: 
*	gps_app_upldmode_moniter.h
*
* Description:
*	header file for GPS Upload mode monitor
*
* Author:
*	Robbie Cao
*
* Date:
*	2008-2-28 15:10
******************************************************************************/

#ifndef _GPS_APP_UPLDMODE_MONITER_H
#define _GPS_APP_UPLDMODE_MONITER_H

#include "kal_release.h"
#include "gpio_sw.h"
#include "eint.h"

#include "gps_app_unconfigurable_const.h"

#define GPS_GPRS_UPLDMODE_GPIO			29

void GPS_UpldModeMonitorInit(void);
void GPS_UpldModeMonitorStart(void);
void GPS_UpldModeMonitorStop(void);

#endif /* _GPS_APP_UPLDMODE_MONITER_H */

