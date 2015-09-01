/******************************************************************************
* File name: 
*	gps_app_vibration_sensor.h
*
* Description: 
*	This header file defines APIs for vibration sensor for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.05.13
*
******************************************************************************/
#ifndef _GPS_APP_VIBRATION_SENSOR_H_
#define _GPS_APP_VIBRATION_SENSOR_H_

// external interrupt number for vibration sensor
#define GPS_VIBSENSOR_EINT_NO			1	// EINT1


void GPSLocateVibSensorInit(void);
void GPSLocateVibSensorStart(void);
void GPSLocateVibSensorStop(void);

#endif //_GPS_APP_VIBRATION_SENSOR_H_

