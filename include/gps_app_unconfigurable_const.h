/******************************************************************************
* File name: 
*	gps_app_unconfigurable_const.h
*
* Description: 
*	This header file defines unconfigurable constants for the GPS App module.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.30
*
******************************************************************************/
#ifndef _GPS_APP_UNCONFIGURABLE_CONST_H_
#define _GPS_APP_UNCONFIGURABLE_CONST_H_

/******************************************************************************
*	Software relative
******************************************************************************/

/********************************************************** 
 * Feature switch macro
 **********************************************************/
//#define GPS_SEPARATE_USER_PASSWOR
#define GPS_CHECK_PIN21_STATUS
#define GPS_FETION_SUPPORT
//#define GPS_POWERON_PHONECALL
#define GPS_DEFENCE_FEATURE
//#define GPS_CHECK_POWERONTIMES
#define GPS_BACKUPLOAD_DAEMON
#define GPS_POSITION_MONITOR
#define GPS_RATE_MONITOR
//#define GPS_GSG_SUPPORT
#define GPS_ITRACK_FORMAT


//#define GPS_REPORT_NEBR_ARFCN
//#define GPS_VIB_4SOSGPRSUPLDSUC
//#define GPS_VIB_4SOSCALLCONNECT
#define GPS_VIB_4SOSCALLSTARTING

#define VIBRATION_SENSOR_SUPPORT
#ifdef VIBRATION_SENSOR_SUPPORT
#define VIBRATION_SENSOR_EINT
#endif

#define GPS_MOTOPWRCUT_FEATURE
//#define GPS_NVRAM_TASK

//#define GPS_MOD_SWAUTOPOWER

#define GPS_SMS_UNICODE_SUPPORT

/********************************************************** 
 * Const macro define
 **********************************************************/
/*
GPS locate SMS message data buffer length.
It should be equal to SMSAL_MAX_MSG_LEN which is defined in 
ps\l4\include\smsal_l4_defs.h
*/
#define GPSLOCATE_SMS_MSGDATA_BUFFER_LEN		160

/*
GPS locate phone number buffer length
It should be equal to MAX_CC_ADDR_LEN, which is defined in 
ps\l4\include\l4c_aux_struct.h
*/
#define GPSLOCATE_PHONE_NUMBER_BUFFER_LEN		41

/*
GPS locate network PLMN buffer length.
It should be equal to MAX_PLMN_LEN+1.
MAX_PLMN_LEN is defined in ps\l4\include\l4c_common_enum.h
*/
#define GPSLOCATE_NWPLMN_BUFFER_LEN				7


#define GPSLOCATE_IMEI_BUFFER_LEN				16	// IMEI string length 15

/* 
 * Value to identify two important parameters of a phone number:
 * 1. Type of Number
 * 2. Numbering Plan
 * See section 10.5.4.7 of GSM 04.08. 
 */
#define GPS_UNKNOWN_TON_UNKNOWN_NPI 	128 ///< Unknown Type of Number and unknown Numbering Plan 
#define GPS_UNKNOWN_TON_ISDN_NPI 		129	///< Unknown Type of Number and ISDN Numbering Plan 
#define GPS_INTERNA_TON_ISDN_NPI 		145	///< International number and ISDN Numbering Plan 

#define	GPS_INTERNATIONAL_CODE			'+'	///< '+' at the beginning
											///< of a number represents an international
											///< phone number


/********************************************************** 
 * Type enum
 **********************************************************/


/******************************************************************************
*	Hardware relative
******************************************************************************/

#endif //_GPS_APP_UNCONFIGURABLE_CONST_H_

