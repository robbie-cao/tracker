/******************************************************************************
 * File name: gps_app_util.h
 * Author: Robbie Cao
 * Date: 2008-3-11 10:45
 ******************************************************************************/

#ifndef _GPS_APP_UTIL_H
#define _GPS_APP_UTIL_H

#include "kal_release.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "parser.h"
#include "gps_app_nema.h"

typedef struct {
	kal_uint32	val ;
	const char*	pattern ;
} Keyword_t ;

kal_uint8 GPS_APP_FindNumberInPresavdList(const GPSLocatePhoneNumber_t *pNumber);
Result_t GPS_APP_CheckNumberPassword(const GPSLocatePhoneNumber_t *pNumber, const char *passwd);
Result_t GPS_APP_CheckServicePassword(const GPSLocatePhoneNumber_t *pNumber, const char *service_passwd);
Result_t GPS_APP_CheckGprsPassword(const GPSLocatePhoneNumber_t *pNumber, const char *passwd);

kal_uint8 GPS_APP_GetNumberType(const char *pNumber);

double GPS_APP_RmcDegree2Rad(kal_uint8 d, kal_uint8 c, kal_uint16 cf);

/** Caculate 8-bit checksum */
kal_uint8 GPS_APP_Checksum8(kal_uint8 *buff, kal_uint32 size);

char* GPS_APP_StrReplace(char* A, char* B, char* C);

#endif /* _GPS_APP_UTIL_H */
