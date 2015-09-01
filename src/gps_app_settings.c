/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_settings.c
 *
 * Project:
 * --------
 *   Maui_Software
 *
 * Description:
 * ------------
 *   This file implements gps locate module component task create function
 *
 * Author:
 * -------
 * Robbie Cao
 * -------
 *
 *============================================================================
 *             HISTORY
 *============================================================================
 * 2008-3-10 15:10 - Initial create.
 ****************************************************************************/

/*************************************************************************
 * Include Statements
 *************************************************************************/
#include "kal_release.h"
#include "app_buff_alloc.h"
#include "custom_util.h"
#include "gpio_sw.h"
#include "em_struct.h"

#include "gps_app_configurable_const.h"
#include "gps_app_unconfigurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_settings.h"
#include "gps_app_nvram_mgr.h"

/*************************************************************************
 * Macro defines
 *************************************************************************/
#define GPS_APP_DEBUG
#ifdef GPS_APP_DEBUG
#define gps_app_log		trace_printf
#else
static void gps_app_log(kal_char *fmt, ...) { }
#endif


/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Local variables
 *************************************************************************/


/*************************************************************************
 * Global variables
 *************************************************************************/


/*************************************************************************
 * Function declaration
 *************************************************************************/

/**
 * Function: GPS_APP_ReadBSNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadBSNumber(kal_uint8 *pValue)
{
	kal_bool res;

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_BSNUMBER, 
				pValue, 
				GPS_NVRAM_RECSIZE_BSNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteBSNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteBSNumber(const kal_uint8 *pValue)
{
	kal_bool res;

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_BSNUMBER, 
				pValue, 
				GPS_NVRAM_RECSIZE_BSNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadTimingValue
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadTimingValue(kal_uint8 *pValue)
{
	kal_bool res;

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_SETTINGS, 
				pValue, 
				GPS_NVRAM_RECSIZE_SETTINGS);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteTimingValue
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteTimingValue(const kal_uint8 *pValue)
{
	kal_bool res;

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_SETTINGS, 
				pValue, 
				GPS_NVRAM_RECSIZE_SETTINGS);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadDefaultTimingValue
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadDefaultTimingValue(kal_uint8 *pValue)
{
	kal_bool res;

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_SETTINGSBACKUP, 
				pValue, 
				GPS_NVRAM_RECSIZE_SETTINGSBACKUP);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteDefaultTimingValue
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteDefaultTimingValue(const kal_uint8 *pValue)
{
	kal_bool res;

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_SETTINGSBACKUP, 
				pValue, 
				GPS_NVRAM_RECSIZE_SETTINGSBACKUP);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadUserNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadUserNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number, 0 for service number
			char 		*pNumber)
{
	kal_bool res;
	int i;
	char tmpBuff[GPS_NVRAM_RECSIZE_USERNUMBER];
	char *p = tmpBuff;

	ASSERT(pNumber != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_USERNUMBER, 
				tmpBuff, 
				GPS_NVRAM_RECSIZE_USERNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}
	for (i = 1; i < nId; i++)
	{
		p += GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN;
	}
	memcpy(pNumber, p, GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN);

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteUserNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteUserNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number, 0 for service number
			const char 	*pNumber)
{
	kal_bool res;
	int i;
	char tmpBuff[GPS_NVRAM_RECSIZE_USERNUMBER];
	char *p = tmpBuff;

	ASSERT(pNumber != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_USERNUMBER, 
				tmpBuff, 
				GPS_NVRAM_RECSIZE_USERNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}
	for (i = 1; i < nId; i++)
	{
		p += GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN;
	}
	memcpy(p, pNumber, GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN);
	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_USERNUMBER, 
				tmpBuff, 
				GPS_NVRAM_RECSIZE_USERNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadUserPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadUserPassword(
			kal_uint8 	nId, 		// nId = 1~3 for user number, 0 for service number
			char 		*pPasswd)
{
	kal_bool res;
	int i;
	char tmpBuff[GPS_NVRAM_RECSIZE_USERPASSWORD];
	char *p = tmpBuff;

	ASSERT(pPasswd!= NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_USERPASSWORD, 
				tmpBuff, 
				GPS_NVRAM_RECSIZE_USERPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}
	for (i = 1; i < nId; i++)
	{
		p += GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN;
	}
	memcpy(pPasswd, p, GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN);

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteUserPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteUserPassword(
			kal_uint8 	nId, 		// nId = 1~3 for user number, 0 for service number
			const char 	*pPasswd)
{
	kal_bool res;
	int i;
	char tmpBuff[GPS_NVRAM_RECSIZE_USERPASSWORD];
	char *p = tmpBuff;

	ASSERT(pPasswd!= NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_USERPASSWORD, 
				tmpBuff, 
				GPS_NVRAM_RECSIZE_USERPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}
	for (i = 1; i < nId; i++)
	{
		p += GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN;
	}
	memcpy(p, pPasswd, GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN);
	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_USERPASSWORD, 
				tmpBuff, 
				GPS_NVRAM_RECSIZE_USERPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadServiceNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadServiceNumber(char *pNumber)
{
	kal_bool res;

	ASSERT(pNumber != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_SERVICENUMBER, 
				pNumber, 
				GPS_NVRAM_RECSIZE_SERVICENUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteServiceNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteServiceNumber(const char *pNumber)
{
	kal_bool res;

	ASSERT(pNumber != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_SERVICENUMBER, 
				pNumber, 
				GPS_NVRAM_RECSIZE_SERVICENUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadServicePassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadServicePassword(char *pPasswd)
{
	kal_bool res;

	ASSERT(pPasswd != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_SERVICEPASSWORD, 
				pPasswd, 
				GPS_NVRAM_RECSIZE_SERVICEPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteServicePassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteServicePassword(const char *pPasswd)
{
	kal_bool res;

	ASSERT(pPasswd != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_SERVICEPASSWORD, 
				pPasswd, 
				GPS_NVRAM_RECSIZE_SERVICEPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadTimingNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadTimingNumber(char *pNumber)
{
	kal_bool res;

	ASSERT(pNumber != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_TIMINGNUMBER, 
				pNumber, 
				GPS_NVRAM_RECSIZE_TIMINGNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteTimingNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteTimingNumber(const char *pNumber)
{
	kal_bool res;

	ASSERT(pNumber != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_TIMINGNUMBER, 
				pNumber, 
				GPS_NVRAM_RECSIZE_TIMINGNUMBER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadServerAddr
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadServerAddr(GPSLocateServerAddr_t *pServerAddr)
{
	kal_bool res;

	ASSERT(pServerAddr != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_SERVERADDR, 
				pServerAddr, 
				GPS_NVRAM_RECSIZE_SERVERADDR);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteServerAddr
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteServerAddr(const GPSLocateServerAddr_t *pServerAddr)
{
	kal_bool res;

	ASSERT(pServerAddr != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_SERVERADDR, 
				pServerAddr, 
				GPS_NVRAM_RECSIZE_SERVERADDR);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadBearerAndMode
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadBearerAndMode(unsigned short *pBearerMode)
{
	kal_bool res;

	ASSERT(pBearerMode != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_BEARERMODE, 
				pBearerMode, 
				GPS_NVRAM_RECSIZE_BEARERMODE);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteBearerAndMode
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteBearerAndMode(const unsigned short *pBearerMode)
{
	kal_bool res;

	ASSERT(pBearerMode != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_BEARERMODE, 
				pBearerMode, 
				GPS_NVRAM_RECSIZE_BEARERMODE);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadGprsUsername
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadGprsUsername(char *pUsername)
{
	kal_bool res;

	ASSERT(pUsername != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSUSER, 
				pUsername, 
				GPS_NVRAM_RECSIZE_GPRSUSER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteGprsUsername
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteGprsUsername(const char *pUsername)
{
	kal_bool res;

	ASSERT(pUsername != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSUSER, 
				pUsername, 
				GPS_NVRAM_RECSIZE_GPRSUSER);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadGprsPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadGprsPassword(char *pPasswd)
{
	kal_bool res;

	ASSERT(pPasswd != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSPASSWORD, 
				pPasswd, 
				GPS_NVRAM_RECSIZE_GPRSPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteGprsPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteGprsPassword(const char *pPasswd)
{
	kal_bool res;

	ASSERT(pPasswd != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSPASSWORD, 
				pPasswd, 
				GPS_NVRAM_RECSIZE_GPRSPASSWORD);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadGprsUploadSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadGprsUploadSettings(unsigned int *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSUPLOADSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteGprsUploadSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteGprsUploadSettings(const unsigned int *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSUPLOADSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadGPSOnSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadGPSOnSettings(unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPSONSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_GPSONSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteGPSOnSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteGPSOnSettings(const unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPSONSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_GPSONSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadHFreeSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadHFreeSettings(unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_HFREEONSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_HFREEONSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteHFreeSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteHFreeSettings(const unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_HFREEONSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_HFREEONSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadPoweronTimes
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadPoweronTimes(unsigned long *pCount)
{
	kal_bool res;

	ASSERT(pCount != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_POWERONCOUNT, 
				pCount, 
				GPS_NVRAM_RECSIZE_POWERONCOUNT);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WritePoweronTimes
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WritePoweronTimes(const unsigned long *pCount)
{
	kal_bool res;

	ASSERT(pCount != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_POWERONCOUNT, 
				pCount, 
				GPS_NVRAM_RECSIZE_POWERONCOUNT);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadDefenceSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadDefenceSetting(unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_DEFENCEONSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_DEFENCEONSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteDefenceSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteDefenceSetting(const unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_DEFENCEONSETTING, 
				pSettings, 
				GPS_NVRAM_RECSIZE_DEFENCEONSETTING);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Read mt call profile setting from non-volatile memory */
Result_t GPS_APP_ReadMtcallProfile(unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_MTCALLPROFILE, 
				pSettings, 
				GPS_NVRAM_RECSIZE_MTCALLPROFILE);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Write mt call profile setting to non-volatile memory */
Result_t GPS_APP_WriteMtcallProfile(const unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_MTCALLPROFILE, 
				pSettings, 
				GPS_NVRAM_RECSIZE_MTCALLPROFILE);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadFixPosition
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadFixPosition(GPS_PostionRange_t *pPos)
{
	kal_bool res;

	ASSERT(pPos != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_FIXEDPOSITION, 
				pPos, 
				GPS_NVRAM_RECSIZE_FIXEDPOSITION);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteFixPosition
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteFixPosition(const GPS_PostionRange_t *pPos)
{
	kal_bool res;

	ASSERT(pPos != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_FIXEDPOSITION, 
				pPos, 
				GPS_NVRAM_RECSIZE_FIXEDPOSITION);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadPosMonitorOnff
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadPosMonitorOnff(unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_POSMONITORONFF, 
				pSettings, 
				GPS_NVRAM_RECSIZE_POSMONITORONFF);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WritePosMonitorOnff
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WritePosMonitorOnff(const unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_POSMONITORONFF, 
				pSettings, 
				GPS_NVRAM_RECSIZE_POSMONITORONFF);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Read saved gprs apn from non-volatile memory */
Result_t GPS_APP_ReadGprsAPN(char *pAPN, char *pUser, char *pPwd)
{
	kal_bool res;

	ASSERT(pAPN != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSAPN, 
				pAPN, 
				GPS_NVRAM_RECSIZE_GPRSAPN);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	if (pUser != NULL)
	{
		res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSAPNUSER, 
					pUser, 
					GPS_NVRAM_RECSIZE_GPRSAPNUSER);
		if (res != KAL_TRUE)
		{
			return RESULT_ERROR;
		}
	}

	if (pPwd != NULL)
	{
		res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSAPNPWD, 
					pPwd, 
					GPS_NVRAM_RECSIZE_GPRSAPNPWD);
		if (res != KAL_TRUE)
		{
			return RESULT_ERROR;
		}
	}

	return RESULT_OK;
}

/** Write gprs apn to non-volatile memory */
Result_t GPS_APP_WriteGprsAPN(const char *pAPN, const char *pUser, const char *pPwd)
{
	kal_bool res;

	ASSERT(pAPN != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSAPN, 
				pAPN, 
				GPS_NVRAM_RECSIZE_GPRSAPN);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	if (pUser != NULL)
	{
		res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSAPNUSER, 
					pUser, 
					GPS_NVRAM_RECSIZE_GPRSAPNUSER);
		if (res != KAL_TRUE)
		{
			return RESULT_ERROR;
		}
	}

	if (pPwd != NULL)
	{
		res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSAPNPWD, 
					pPwd, 
					GPS_NVRAM_RECSIZE_GPRSAPNPWD);
		if (res != KAL_TRUE)
		{
			return RESULT_ERROR;
		}
	}

	return RESULT_OK;
}

#ifdef GPS_ITRACK_FORMAT
/** Read saved itrack front msg from non-volatile memory */
Result_t GPS_APP_ReadFrontMsg(char *pMsg)
{
	kal_bool res;

	ASSERT(pMsg != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_ITRACKFRONTMSG, 
				pMsg, 
				GPS_NVRAM_RECSIZE_ITRACKFRONTMSG);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Write itrack front msg to non-volatile memory */
Result_t GPS_APP_WriteFrontMsg(const char *pMsg)
{
	kal_bool res;

	ASSERT(pMsg != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_ITRACKFRONTMSG, 
				pMsg, 
				GPS_NVRAM_RECSIZE_ITRACKFRONTMSG);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Read saved itrack rear msg from non-volatile memory */
Result_t GPS_APP_ReadRearMsg(char *pMsg)
{
	kal_bool res;

	ASSERT(pMsg != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_ITRACKREARMSG, 
				pMsg, 
				GPS_NVRAM_RECSIZE_ITRACKREARMSG);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Write itrack rear msg to non-volatile memory */
Result_t GPS_APP_WriteRearMsg(const char *pMsg)
{
	kal_bool res;

	ASSERT(pMsg != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_ITRACKREARMSG, 
				pMsg, 
				GPS_NVRAM_RECSIZE_ITRACKREARMSG);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}
#endif

/**
 * Function: GPS_APP_ReadGprsUploadSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadGprsUploadSettings2(unsigned int *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_GPRSUPLOADSETTING2, 
				pSettings, 
				GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING2);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteGprsUploadSettings
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteGprsUploadSettings2(const unsigned int *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_GPRSUPLOADSETTING2, 
				pSettings, 
				GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING2);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadSosCallSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadSosCallSetting(unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_SOSPHONECALL, 
				pSettings, 
				GPS_NVRAM_RECSIZE_SOSPHONECALL);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteSosCallSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteSosCallSetting(const unsigned char *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_SOSPHONECALL, 
				pSettings, 
				GPS_NVRAM_RECSIZE_SOSPHONECALL);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Read rate limit setting from non-volatile memory */
Result_t GPS_APP_ReadRateLimitSetting(unsigned short *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramReadRecord(GPS_NVRAM_RECID_RATELIMIT, 
				pSettings, 
				GPS_NVRAM_RECSIZE_RATELIMIT);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/** Write rate limit setting to non-volatile memory */
Result_t GPS_APP_WriteRateLimitSetting(const unsigned short *pSettings)
{
	kal_bool res;

	ASSERT(pSettings != NULL);

	res = GPSLocateNvramWriteRecord(GPS_NVRAM_RECID_RATELIMIT, 
				pSettings, 
				GPS_NVRAM_RECSIZE_RATELIMIT);
	if (res != KAL_TRUE)
	{
		return RESULT_ERROR;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ReadNumberRecord
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ReadNumberRecord(
			kal_uint8 				nId, 
			GPS_Saved_Number_t 		*pNumber)
{
	Result_t res = RESULT_ERROR;
	GPS_Presaved_Number_Type_t	number_type;

	ASSERT(pNumber != NULL);

	if (nId == 0)
	{
		number_type = GPS_PRESAVED_NUMBER_TYPE_SERVICE;
	}
	else if (nId >= 1 && nId <= GPS_PRESAVED_NUMBER_TOTAL)
	{
		number_type = GPS_PRESAVED_NUMBER_TYPE_USER;
	}
	else
	{
		return RESULT_ERROR;
	}

	if (number_type == GPS_PRESAVED_NUMBER_TYPE_USER)
	{
		res = GPS_APP_ReadUserNumber(nId, pNumber->number);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
		res = GPS_APP_ReadUserPassword(nId, pNumber->passwd);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
	}
	else if (number_type == GPS_PRESAVED_NUMBER_TYPE_SERVICE)
	{
		res = GPS_APP_ReadServiceNumber(pNumber->number);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
		res = GPS_APP_ReadServicePassword(pNumber->passwd);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
	}
	pNumber->index = nId;

	return RESULT_OK;
}

/**
 * Function: GPS_APP_WriteNumberRecord
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_WriteNumberRecord(
			kal_uint8 					nId, 
			const GPS_Saved_Number_t 	*pNumber)
{
	Result_t res = RESULT_ERROR;
	GPS_Presaved_Number_Type_t	number_type;

	ASSERT(pNumber != NULL);

	if (nId == 0)
	{
		number_type = GPS_PRESAVED_NUMBER_TYPE_SERVICE;
	}
	else if (nId >= 1 && nId <= GPS_PRESAVED_NUMBER_TOTAL)
	{
		number_type = GPS_PRESAVED_NUMBER_TYPE_USER;
	}
	else
	{
		return RESULT_ERROR;
	}

	if (number_type == GPS_PRESAVED_NUMBER_TYPE_USER)
	{
		res = GPS_APP_WriteUserNumber(nId, pNumber->number);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
		res = GPS_APP_WriteUserPassword(nId, pNumber->passwd);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
	}
	else if (number_type == GPS_PRESAVED_NUMBER_TYPE_SERVICE)
	{
		res = GPS_APP_WriteServiceNumber(pNumber->number);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
		res = GPS_APP_WriteServicePassword(pNumber->passwd);
		if (res != RESULT_OK)
		{
			return RESULT_ERROR;
		}
	}

	return RESULT_OK;
}

