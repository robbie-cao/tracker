/******************************************************************************
* File name: 
*	gps_app_configurable_const.h
*
* Description: 
*	This header file defines configurable constants for the GPS App module.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.30
*
******************************************************************************/
#ifndef _GPS_APP_CONFIGURABLE_CONST_H_
#define _GPS_APP_CONFIGURABLE_CONST_H_

#include "gps_app_unconfigurable_const.h"		// referring feature switch macro


/******************************************************************************
*	Software relative
******************************************************************************/

/*
GPS module frame data buffer length
The current definition is just a statistical value.
If needed, it should be adjusted according to some GPS data information which 
frame data length is larger than the current value.
*/
#define GPSLOCATE_GPS_FRAME_DATA_BUFFER_LEN		128

/*
GPS locate pre-saved number buffer length
number length is 20
*/
#define GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN	17

/*
GPS locate pre-saved password buffer length
password length is 4
*/
#define GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN	5

/*
GPS locate user number maximum amount
*/
#define GPSLOCATE_USER_NUMBER_MAX				3

/*
GPS locate user password maximum amount
*/
#define GPSLOCATE_USER_PASSWORD_MAX				GPSLOCATE_USER_NUMBER_MAX

/*
GPS locate pre-saved APN buffer length
number length is 36
*/
#define GPSLOCATE_PRESAVED_APN_BUFFER_LEN		32

#define GPSLOCATE_PRESAVED_ITRACKMSG_LEN		56

/*
GPS locate pre-saved APN user buffer length
number length is 20
*/
#define GPSLOCATE_PRESAVED_APNUSER_BUFFER_LEN	GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN

/*
GPS locate pre-saved APN password buffer length
number length is 20
*/
#define GPSLOCATE_PRESAVED_APNPWD_BUFFER_LEN	GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN

// Ping-Pong buffer size
#define GPS_PP_BUFFER_SIZE				(50)

#define GPS_IND_NWSTATUS_INTV			8	// second
#define GPS_IND_CALLCONNING_INTV		1	// second
#define GPS_IND_CALLCONNED_INTV			3	// second

#define GSG_WAVE_WIDTH					(KAL_TICKS_100_MSEC+1)
//wave table size
#define GSG_WAVE_TABLE_SIZE				(20)

#define GPS_SEPARATOR_SIGN_1			'#'
#define GPS_SEPARATOR_SIGN_2			'&'
#define GPS_SEPARATOR_SIGN_3			'*'
#define GPS_SEPARATOR_SIGN_4			'%'
#define GPS_SEPARATOR_SIGN_5			'@'

#ifdef GPS_CHECK_PIN21_STATUS
#define GPS_PIN21_STATUS_HIGH			GPIO_ReadIO(GPS_BEARER_SWITCH_GPIO)
#define GPS_VALID_SEPARATOR_SIGN		(GPS_PIN21_STATUS_HIGH ? 	\
										 GPS_SEPARATOR_SIGN_3 : 	\
										 GPS_SEPARATOR_SIGN_1)
#define GPS_INVALID_SEPARATOR_SIGN		(GPS_PIN21_STATUS_HIGH ? 	\
										 GPS_SEPARATOR_SIGN_4 : 	\
										 GPS_SEPARATOR_SIGN_2)
#else
#define GPS_VALID_SEPARATOR_SIGN		GPS_SEPARATOR_SIGN_1
#define GPS_INVALID_SEPARATOR_SIGN		GPS_SEPARATOR_SIGN_2
#endif

#ifdef GPS_CHECK_POWERONTIMES
#define GPS_POWERON_TIMES_MAX			1000
#endif

#define GPS_LOWBAT_WARN_TIMES			3

//for code protector
#ifdef CODEPROTECTOR_FS8816_SUPPORT

//key, 8 bytes
#define CODEPROTECTOR_KEY	\
	{0x63, 0x66, 0x75, 0x7B, 0x82, 0x81, 0x81, 0x98}
	//{0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}

//FS8816 slave address
#define FS8816_SLVADDR	(0x62)

#endif //CODEPROTECTOR_FS8816_SUPPORT

#define GPSAPP_DATABACKUP_FILES_MAX		100

/*
* Key monitor, keypad message trigger method definition
*
* 0 - trigger message when key up
* 1 - trigger message before key up
*/
#define GPSLOCATE_KEYMONITOR_KPDMSG_TRIGGER	1

#define GPSLOCATE_VIBSENSOR_POLLINGINTERVAL_DEFAULT	1	// 50 ms, the unit is 50ms
#define GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT	5	// 5 min, the unit is 1min

/*
* Indicate unaccepted MT call or not
*
* 0 - don't indicate
* 1 - indicate
*/
#define GPSLOCATE_INDICATE_UNACCEPTED_MTCALL	(0)

//the microphone gain under mt call monitor
#define GPS_MTCALL_MONITOR_MIC_GAIN	(200)

/******************************************************************************
*	Hardware relative
******************************************************************************/
#define GPS_BATTERY_IND_GPIO			2
#define GPS_NETWORK_IND_GPIO			4
#define GPS_BEARER_SWITCH_GPIO			0
#define GPS_MODULE_ENALBE_GPIO			1
#define GPS_SMS_MODE_GPIO				6
#define GPS_SQWAVE_GPIO					30

#define GPS_LED_TURN_ON					1
#define GPS_LED_TURN_OFF				0

//GSG output GPIO slot
#define GSG_GPIO_SLOT					(14)	//use GPIO30

//for code protector
#ifdef CODEPROTECTOR_FS8816_SUPPORT

/*
* FS8816 power control
* 0 - disable
* 1 - enable
*/
#define FS8816_PWR_CTRL	(1)

/*
* FS8816 power control GPIO slot configuration
*/
#if FS8816_PWR_CTRL
#define FS8816_PWR_CTRL_GPIO	(1)	//same as GPS module
#endif //FS8816_PWR_CTRL

/*
* FS8816 reset control
* 0 - disable
* 1 - enable
*/
#define FS8816_RST_CTRL		(1)

/*
* FS8816 reset control GPIO slot configuration
*/
#if FS8816_RST_CTRL
#define FS8816_RST_CTRL_GPIO	(29)
#endif //FS8816_PWR_CTRL

#endif //CODEPROTECTOR_FS8816_SUPPORT

#ifdef VIBRATION_SENSOR_SUPPORT
//GPIO for vibration sensor
#define GPS_VIBRATION_SENSOR_GPIO		(31)
#endif //VIBRATION_SENSOR_SUPPORT

#endif //_GPS_APP_CONFIGURABLE_CONST_H_

