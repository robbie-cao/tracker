/******************************************************************************
* File name: 
*	gps_app_sq_wave.h
*
* Description: 
*	The header file for square wave generator of GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.14
*
******************************************************************************/
#ifndef _GPS_APP_SQ_WAVE_H_
#define _GPS_APP_SQ_WAVE_H_

#if defined(GPS_LOCATE_SUPPORT)

#include "kal_release.h"

void GSG_Init(void);
kal_bool GSG_Start(void);
kal_bool GSG_Stop(void);

void GSG_WidthSquareWave(kal_uint16 width);

#endif // defined(GPS_LOCATE_SUPPORT)

#endif //_GPS_APP_SQ_WAVE_H_

