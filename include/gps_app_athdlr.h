/******************************************************************************
 * File name: gps_app_athdlr.h
 * Author: Robbie Cao
 * Date: 2008-3-11 10:40
 ******************************************************************************/

#ifndef _GPS_APP_ATHDLR_H
#define _GPS_APP_ATHDLR_H

#include "kal_release.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "parser.h"

/*************************************************************************
 * GPS AT change numbers and settings handler macro and function prototype
 *************************************************************************/

/** handler for change timing setting on non-volatile memory */
Result_t GPS_AT_ChangeTimingSetting(const kal_uint8 *pValue);

/** handler for change default timing setting on non-volatile memory */
Result_t GPS_AT_ChangeDefaultTimingSetting(const kal_uint8 *pValue);

/** handler for change user number on non-volatile memory */
Result_t GPS_AT_ChangeSosNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pNumber);

/** handler for change user number on non-volatile memory */
Result_t GPS_AT_ChangeUserNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pNumber);

/** handler for change user password on non-volatile memory */
Result_t GPS_AT_ChangeUserPassword(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pPasswd);

/** handler for change service number on non-volatile memory */
Result_t GPS_AT_ChangeServiceNumber(const char *pNumber);

/** handler for change service password on non-volatile memory */
Result_t GPS_AT_ChangeServicePassword(const char *pPasswd);

/** handler for change shared user password on non-volatile memory */
Result_t GPS_AT_ChangeSharedUserPassword(const char *pPasswd);

/** handler for change server address on non-volatile memory */
Result_t GPS_AT_ChangeServerAddress(const GPSLocateServerAddr_t *pAddr);

/** handler for change working mode setting on non-volatile memory */
Result_t GPS_AT_ChangeWorkingMode(const kal_uint8 *pValue);

/** handler for change number of request base station on non-volatile memory */
Result_t GPS_AT_ChangeBSNumber(const kal_uint8 *pValue);

/** handler for change settings for upload data thru gprs on non-volatile memory */
Result_t GPS_AT_ChangeGprsUploadSetting(const kal_uint32 *pValue);

/** handler for change gprs user on non-volatile memory */
Result_t GPS_AT_ChangeGprsUser(const char *pUser);

/** handler for change gprs password on non-volatile memory */
Result_t GPS_AT_ChangeGprsPassword(const char *pPassword);

/** handler for change gprs apn on non-volatile memory */
Result_t GPS_AT_ChangeGprsAPN(const char *pAPN);

/** handler for change gprs apn, user name & password on non-volatile memory */
Result_t GPS_AT_ChangeGprsAccount(const char *pAPN, const char *pUser, const char *pPwd);

/** handler for turn on/off gps log */
Result_t GPS_AT_SwitchGpsLog(const kal_uint8 *pValue);

/** handler for turn on/off handfree setting */
Result_t GPS_AT_SwitchHandfree(const kal_uint8 *pValue);

/** handler for change gps module working state setting */
Result_t GPS_AT_SwitchGpsProf(const kal_uint8 *pValue);

Result_t GPS_AT_SwitchMtcallProf(const kal_uint8 *pValue);

Result_t GPS_AT_SwitchSosCall(const kal_uint8 *pValue);


#endif /* _GPS_APP_ATHDLR_H */

