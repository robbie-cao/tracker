/******************************************************************************
* File name: 
*	gps_app_defence_moniter.h
*
* Description:
*	header file for GPS defence monitor
*
* Author:
*	Robbie Cao
*
* Date:
*	2008-2-28 15:10
******************************************************************************/

#ifndef _GPS_APP_DEFENCE_MONITER_H
#define _GPS_APP_DEFENCE_MONITER_H

#include "kal_release.h"
#include "gpio_sw.h"
#include "eint.h"

#include "gps_app_unconfigurable_const.h"

#ifdef GPS_DEFENCE_FEATURE
#ifndef VIBRATION_SENSOR_EINT
// external interrupt number for moniter defence
#define GPS_DEFENCE_EINT_NO				1	// EINT1

#define GPS_DEFENCE_EN_GPIO				32
#define GPS_DEFENCE_ALARM_GPIO			33

#define GPS_APP_DefenceReset()			{	GPIO_WriteIO(0, GPS_DEFENCE_EN_GPIO);		\
											GPIO_WriteIO(0, GPS_DEFENCE_ALARM_GPIO);	\
											EINT_Mask(GPS_DEFENCE_EINT_NO);				\
										}
#define GPS_APP_EnableDefence()			{	GPIO_WriteIO(1, GPS_DEFENCE_EN_GPIO);		\
											EINT_UnMask(GPS_DEFENCE_EINT_NO);			\
										}
#define GPS_APP_DisableDefence()		{	GPIO_WriteIO(0, GPS_DEFENCE_EN_GPIO);		\
											EINT_Mask(GPS_DEFENCE_EINT_NO);				\
										}
#define GPS_APP_DefenceOn()				GPIO_ReadIO(GPS_DEFENCE_EN_GPIO)
#define GPS_APP_EnDefenceAlarm()		GPIO_WriteIO(1, GPS_DEFENCE_ALARM_GPIO)
#define GPS_APP_DisDefenceAlarm()		GPIO_WriteIO(0, GPS_DEFENCE_ALARM_GPIO)
#define GPS_APP_DefenceAlarmOn()		GPIO_ReadIO(GPS_DEFENCE_ALARM_GPIO)

#else

#define GPS_DEFENCE_TIGGER_GPIO			31
#define GPS_DEFENCE_EN_GPIO				32
#define GPS_DEFENCE_ALARM_GPIO			33

#define GPS_APP_DefenceOn()				GPIO_ReadIO(GPS_DEFENCE_EN_GPIO)
#define GPS_APP_EnDefenceAlarm()		GPIO_WriteIO(1, GPS_DEFENCE_ALARM_GPIO)
#define GPS_APP_DisDefenceAlarm()		GPIO_WriteIO(0, GPS_DEFENCE_ALARM_GPIO)
#define GPS_APP_DefenceAlarmOn()		GPIO_ReadIO(GPS_DEFENCE_ALARM_GPIO)

void GPS_APP_DefenceReset(void);
void GPS_APP_EnableDefence(void);
void GPS_APP_DisableDefence(void);

#endif

void GPS_DefenceMonitorInit(void);
#endif

#endif /* _GPS_APP_DEFENCE_MONITER_H */

