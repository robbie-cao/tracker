/******************************************************************************
 * File name: gps_app_settings.h
 * Author: Robbie Cao
 * Date: 2008-3-11 11:17
 ******************************************************************************/

#ifndef _GPS_APP_SETTINGS_H
#define _GPS_APP_SETTINGS_H

#include "kal_release.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_nema.h"
#include "parser.h"

#define GPS_PRESAVED_NUMBER_TOTAL		GPSLOCATE_USER_NUMBER_MAX

typedef enum
{
	GPS_BEARER_SMS,
	GPS_BEARER_GPRS,
	GPS_BEARER_CSD,
	GPS_BEARER_WIFI,
	GPS_BEARER_TOTAL
} GPS_Bearer_Type_t;

typedef enum
{
	GPS_SMS_MODE_P2P,
	GPS_SMS_MODE_SC,
	GPS_SMS_MODE_TOTAL
} GPS_SMS_Mode_Type_t;

/*
 *
 */
typedef enum {
	GPS_PRESAVED_NUMBER_TYPE_USER,
	GPS_PRESAVED_NUMBER_TYPE_SERVICE,
	GPS_PRESAVED_NUMBER_TYPE_TOTAL
} GPS_Presaved_Number_Type_t;

/*
 * Presaved phone number structure
 */
typedef struct {
	kal_uint8 	index;
	char 		number[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
	char 		passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
} GPS_Saved_Number_t;


/*************************************************************************
 * GPS settings on non-volatile memory macro and function prototype
 *************************************************************************/

/** Read presaved request base station number from non-volatile memory */
Result_t GPS_APP_ReadBSNumber(kal_uint8 *pValue);

/** Write request base station number to non-volatile memory */
Result_t GPS_APP_WriteBSNumber(const kal_uint8 *pValue);

/** Read presaved timing value setting from non-volatile memory */
Result_t GPS_APP_ReadTimingValue(kal_uint8 *pValue);

/** Write timing value to non-volatile memory */
Result_t GPS_APP_WriteTimingValue(const kal_uint8 *pValue);

/** Read presaved default timing value setting from non-volatile memory */
Result_t GPS_APP_ReadDefaultTimingValue(kal_uint8 *pValue);

/** Write default timing value to non-volatile memory */
Result_t GPS_APP_WriteDefaultTimingValue(const kal_uint8 *pValue);

/** Read presaved user number from non-volatile memory */
Result_t GPS_APP_ReadUserNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			char 		*pNumber);

/** Write user number to non-volatile memory */
Result_t GPS_APP_WriteUserNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pNumber);

/** Read presaved user password from non-volatile memory */
Result_t GPS_APP_ReadUserPassword(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			char 		*pPasswd);

/** Write user password to non-volatile memory */
Result_t GPS_APP_WriteUserPassword(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pPasswd);

/** Read presaved service number from non-volatile memory */
Result_t GPS_APP_ReadServiceNumber(char *pNumber);

/** Write service number to non-volatile memory */
Result_t GPS_APP_WriteServiceNumber(const char *pNumber);

/** Read presaved service password from non-volatile memory */
Result_t GPS_APP_ReadServicePassword(char *pPasswd);

/** Write service password to non-volatile memory */
Result_t GPS_APP_WriteServicePassword(const char *pPasswd);

/** Read saved timing number from non-volatile memory */
Result_t GPS_APP_ReadTimingNumber(char *pNumber);

/** Write timing number to non-volatile memory */
Result_t GPS_APP_WriteTimingNumber(const char *pNumber);

/** Read saved tcp/ip server ip and port from non-volatile memory */
Result_t GPS_APP_ReadServerAddr(GPSLocateServerAddr_t *pServerAddr);

/** Write tcp/ip server ip and port to non-volatile memory */
Result_t GPS_APP_WriteServerAddr(const GPSLocateServerAddr_t *pServerAddr);

/** Read saved bearer and sms mode from non-volatile memory */
Result_t GPS_APP_ReadBearerAndMode(unsigned short *pBearerMode);

/** Write bearer and sms mode to non-volatile memory */
Result_t GPS_APP_WriteBearerAndMode(const unsigned short *pBearerMode);

/** Read saved gprs user name from non-volatile memory */
Result_t GPS_APP_ReadGprsUsername(char *pUsername);

/** Write gprs user name to non-volatile memory */
Result_t GPS_APP_WriteGprsUsername(const char *pUsername);

/** Read saved gprs user password from non-volatile memory */
Result_t GPS_APP_ReadGprsPassword(char *pPasswd);

/** Write gprs user password to non-volatile memory */
Result_t GPS_APP_WriteGprsPassword(const char *pPasswd);

/** Read saved bearer and sms mode from non-volatile memory */
Result_t GPS_APP_ReadGprsUploadSettings(unsigned int *pSettings);

/** Write bearer and sms mode to non-volatile memory */
Result_t GPS_APP_WriteGprsUploadSettings(const unsigned int *pSettings);

/** Read saved gps on/off setting from non-volatile memory */
Result_t GPS_APP_ReadGPSOnSettings(unsigned char *pSettings);

/** Write gps on/off setting to non-volatile memory */
Result_t GPS_APP_WriteGPSOnSettings(const unsigned char *pSettings);

/** Read saved handfree setting from non-volatile memory */
Result_t GPS_APP_ReadHFreeSettings(unsigned char *pSettings);

/** Write handfree setting to non-volatile memory */
Result_t GPS_APP_WriteHFreeSettings(const unsigned char *pSettings);

/** Read saved power on times counter from non-volatile memory */
Result_t GPS_APP_ReadPoweronTimes(unsigned long *pCount);

/** Write power on times counter to non-volatile memory */
Result_t GPS_APP_WritePoweronTimes(const unsigned long *pCount);

/** Read defence on/off setting from non-volatile memory */
Result_t GPS_APP_ReadDefenceSetting(unsigned char *pSettings);

/** Write defence on/off setting to non-volatile memory */
Result_t GPS_APP_WriteDefenceSetting(const unsigned char *pSettings);

/** Read mt call profile setting from non-volatile memory */
Result_t GPS_APP_ReadMtcallProfile(unsigned char *pSettings);

/** Write mt call profile setting to non-volatile memory */
Result_t GPS_APP_WriteMtcallProfile(const unsigned char *pSettings);

/** Read saved fixed position from non-volatile memory */
Result_t GPS_APP_ReadFixPosition(GPS_PostionRange_t *pPos);

/** Write fixed position to non-volatile memory */
Result_t GPS_APP_WriteFixPosition(const GPS_PostionRange_t *pPos);

/** Read saved position monitor on/off from non-volatile memory */
Result_t GPS_APP_ReadPosMonitorOnff(unsigned char *pSettings);

/** Write position monitor on/off to non-volatile memory */
Result_t GPS_APP_WritePosMonitorOnff(const unsigned char *pSettings);

/** Read saved gprs apn from non-volatile memory */
Result_t GPS_APP_ReadGprsAPN(char *pAPN, char *pUser, char *pPwd);

/** Write gprs apn to non-volatile memory */
Result_t GPS_APP_WriteGprsAPN(const char *pAPN, const char *pUser, const char *pPwd);

#ifdef GPS_ITRACK_FORMAT
/** Read saved itrack front msg from non-volatile memory */
Result_t GPS_APP_ReadFrontMsg(char *pMsg);

/** Write itrack front msg to non-volatile memory */
Result_t GPS_APP_WriteFrontMsg(const char *pMsg);

/** Read saved itrack rear msg from non-volatile memory */
Result_t GPS_APP_ReadRearMsg(char *pMsg);

/** Write itrack rear msg to non-volatile memory */
Result_t GPS_APP_WriteRearMsg(const char *pMsg);
#endif

/** Read saved bearer and sms mode from non-volatile memory */
Result_t GPS_APP_ReadGprsUploadSettings2(unsigned int *pSettings);

/** Write bearer and sms mode to non-volatile memory */
Result_t GPS_APP_WriteGprsUploadSettings2(const unsigned int *pSettings);

/** Read sos call setting from non-volatile memory */
Result_t GPS_APP_ReadSosCallSetting(unsigned char *pSettings);

/** Write sos call setting to non-volatile memory */
Result_t GPS_APP_WriteSosCallSetting(const unsigned char *pSettings);

/** Read rate limit setting from non-volatile memory */
Result_t GPS_APP_ReadRateLimitSetting(unsigned short *pSettings);

/** Write rate limit setting to non-volatile memory */
Result_t GPS_APP_WriteRateLimitSetting(const unsigned short *pSettings);

/** Read presaved phone number and passowrd from non-volatile memory */
Result_t GPS_APP_ReadNumberRecord(
			kal_uint8 					nId, 	// nId = 0 for service number, 1~3 for user number, others error
			GPS_Saved_Number_t 			*pNumber);

/** Write phone number and password to non-volatile memory */
Result_t GPS_APP_WriteNumberRecord(
			kal_uint8 					nId, 	// nId = 0 for service number, 1~3 for user number, others error
			const GPS_Saved_Number_t 	*pNumber);


#endif /* _GPS_APP_SETTINGS_H */
