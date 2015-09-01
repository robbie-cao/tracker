/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_athdlr.c
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
 * 2008-3-10 15:02 - Initial create.
 ****************************************************************************/

/*************************************************************************
 * Include Statements
 *************************************************************************/
#include "kal_release.h"
#include "app_buff_alloc.h"
#include "custom_util.h"
#include "gpio_sw.h"
#include "em_struct.h"
#include "custom_config.h"

#include "gps_app_configurable_const.h"
#include "gps_app_unconfigurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_athdlr.h"
#include "gps_app.h"
#include "gps_app_nvram_mgr.h"
#include "gps_app_timer_mgr.h"
#include "gps_app_pp_buff_mgr.h"
#include "gps_soc.h"
#include "gps_app_data.h"
#ifdef VIBRATION_SENSOR_SUPPORT
#include "gps_app_vibration_sensor.h"
#endif

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
extern void gps_set_gprs_account_direct(unsigned char indx,
									  const char* apn,
									  const char* usr,
									  const char* psw);


/**
 * Function: GPS_AT_ChangeTimingSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeTimingSetting(const kal_uint8 *pValue)
{
	GPS_APP_WriteTimingValue(pValue);

	gps_timing = *pValue;
	GPS_APP_SetTimingLocateFlag(KAL_FALSE);

	if (gps_curr_bearer_type != GPS_BEARER_SMS)
	{
		return RESULT_OK;
	}

	if (gps_timing != 0)
	{
		GPS_APP_SetTimingLocateFlag(KAL_TRUE);
		if (gps_timing <= 60)
		{
			gps_timing_in_min = gps_timing;
		}
		else
		{
			gps_timing_in_min = (gps_timing - 60) * 60;
		}

		if (gps_timingloc_timer != NULL)
		{
			GPSAppTimer_Reset(gps_timingloc_timer, 
				GPS_APP_TimingLocateRepeatHandler, 
				gps_timing_in_min * KAL_TICKS_1_MIN,
				gps_timing_in_min * KAL_TICKS_1_MIN, 
				KAL_TRUE);
		}
		else
		{
			gps_timingloc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
										GPS_APP_TimingLocateRepeatHandler,
										gps_timing_in_min * KAL_TICKS_1_MIN,
										gps_timing_in_min * KAL_TICKS_1_MIN,
										KAL_TRUE);
		}
		gps_app_log("GPS_AT_ChangeTimingSetting, restart GPS_APP_TimingLocateRepeatHandler timer");
	}
	else
	{
		if (gps_timingloc_timer != NULL)
		{
			GPSAppTimer_Stop(gps_timingloc_timer, KAL_TRUE);
			gps_timingloc_timer = NULL;
		}
	}

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeDefaultTimingSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeDefaultTimingSetting(const kal_uint8 *pValue)
{
	GPS_APP_WriteDefaultTimingValue(pValue);
	gps_timing_default = *pValue;

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeSosNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeSosNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pNumber)
{
	ASSERT(nId >= 1 && nId <= 3);

	GPS_APP_WriteUserNumber(nId, pNumber);
	strcpy(gps_usr_numbers[nId-1].number, pNumber);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeUserNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeUserNumber(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pNumber)
{
	ASSERT(nId >= 1 && nId <= 3);

	GPS_APP_WriteUserNumber(nId, pNumber);
	strcpy(gps_usr_numbers[nId-1].number, pNumber);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeUserPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeUserPassword(
			kal_uint8 	nId, 		// nId = 1~3 for user number
			const char 	*pPasswd)
{
	ASSERT(nId >= 1 && nId <= 3);

	GPS_APP_WriteUserPassword(nId, pPasswd);
	strcpy(gps_usr_numbers[nId-1].passwd, pPasswd);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeServiceNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeServiceNumber(const char *pNumber)
{
	GPS_APP_WriteServiceNumber(pNumber);
	strcpy(gps_sc_number.number, pNumber);
	strcpy((char *)gps_act_sc_num.Number, gps_sc_number.number);
	gps_act_sc_num.Type = GPS_APP_GetNumberType((char *)gps_act_sc_num.Number);
	gps_act_sc_num.Length = strlen((char *)gps_act_sc_num.Number);

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
Result_t GPS_AT_ChangeServicePassword(const char *pPasswd)
{
	GPS_APP_WriteServicePassword(pPasswd);
	strcpy(gps_sc_number.passwd, pPasswd);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeSharedUserPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeSharedUserPassword(const char *pPasswd)
{
	int i;

	for (i = 1; i <= GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		GPS_APP_WriteUserPassword(i, pPasswd);
		strcpy(gps_usr_numbers[i-1].passwd, pPasswd);
	}
	strcpy(gps_usr_passwd, pPasswd);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeServerAddress
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeServerAddress(const GPSLocateServerAddr_t *pAddr)
{
	GPS_APP_WriteServerAddr(pAddr);
	memcpy(&gps_tcpip_server_addr, pAddr, sizeof(gps_tcpip_server_addr));

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeWorkingMode
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeWorkingMode(const kal_uint8 *pValue)
{
	kal_uint16 tmpBM = 0;
	kal_uint8 tmpValue = 0;

	switch (*pValue) {
	case 0: tmpBM = GPS_BEARER_SMS << 8 | GPS_SMS_MODE_P2P; break;
	case 1: tmpBM = GPS_BEARER_SMS << 8 | GPS_SMS_MODE_SC; break;
	case 2: tmpBM = GPS_BEARER_GPRS << 8; break;
	default: tmpBM = 0; break;
	}

	if ((gps_curr_bearer_type << 8 | gps_curr_sms_mode) == tmpBM) return RESULT_OK;

	GPS_APP_WriteBearerAndMode(&tmpBM);

	// stop repeat handler for current mode
	if (gps_timingloc_timer != NULL)
	{
		GPSAppTimer_Stop(gps_timingloc_timer, KAL_TRUE);
		gps_timingloc_timer = NULL;
	}
	if (gps_sampgprmc_timer != NULL)
	{
		GPSAppTimer_Stop(gps_sampgprmc_timer, KAL_TRUE);
		gps_sampgprmc_timer = NULL;
	}
	// reset gps_timing to 0
	GPS_APP_WriteTimingValue(&tmpValue);
	gps_timing = tmpValue;

	// start repeat handler for the setting mode
	switch ((tmpBM >> 8) & 0xff)
	{
	case GPS_BEARER_SMS:
		if (gps_timing != 0)
		{
			GPS_APP_SetTimingLocateFlag(KAL_TRUE);
			if (gps_timing <= 60)
			{
				gps_timing_in_min = gps_timing;
			}
			else
			{
				gps_timing_in_min = (gps_timing - 60) * 60;
			}

			if (gps_timingloc_timer != NULL)
			{
				GPSAppTimer_Reset(gps_timingloc_timer, 
					GPS_APP_TimingLocateRepeatHandler, 
					gps_timing_in_min * KAL_TICKS_1_MIN,
					gps_timing_in_min * KAL_TICKS_1_MIN,
					KAL_TRUE);
			}
			else
			{
				gps_timingloc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
											GPS_APP_TimingLocateRepeatHandler,
											gps_timing_in_min * KAL_TICKS_1_MIN,
											gps_timing_in_min * KAL_TICKS_1_MIN,
											KAL_TRUE);
			}
			gps_app_log("GPS_SMS_SetTimingLocate, restart GPS_APP_TimingLocateRepeatHandler timer");
		}

		break;

	case GPS_BEARER_GPRS:
		if (gps_upload_cnt > 0 && gps_samp_interval > 0)
		{
			if (gps_sampgprmc_timer != NULL)
			{
				GPSAppTimer_Reset(gps_sampgprmc_timer,
							GPS_APP_SampGprmcRepeatHandler,
							gps_samp_interval * KAL_TICKS_1_SEC,
							gps_samp_interval * KAL_TICKS_1_SEC,
							KAL_TRUE);
			}
			else
			{
				gps_sampgprmc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
										GPS_APP_SampGprmcRepeatHandler,
										gps_samp_interval * KAL_TICKS_1_SEC,
										gps_samp_interval * KAL_TICKS_1_SEC,
										KAL_TRUE);
			}
		}

		break;

	case GPS_BEARER_CSD:
		break;

	case GPS_BEARER_WIFI:
		break;

	default:
		break;
	}

	// set current mode to new
	gps_curr_bearer_type = (tmpBM >> 8) & 0xff;
	gps_curr_sms_mode = tmpBM & 0xff;

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeBSNumber
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeBSNumber(const kal_uint8 *pValue)
{
	GPS_APP_WriteBSNumber(pValue);
	gps_bs_num = *pValue;

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeGprsUploadSetting
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeGprsUploadSetting(const kal_uint32 *pValue)
{
	kal_uint32 tmpSetting;
	kal_uint16 tmpIntv, tmpCnt;

	tmpIntv = *pValue & 0xffff;
	tmpCnt = (*pValue >> 16) & 0xffff;
	// temp limit interval to be 5s at least and upload_cnt to be 10 at least
	gps_samp_interval = (tmpIntv == 0) ? 
						0 :
						(tmpIntv > GPS_SEND_INTERVAL_MIN) ? 
						tmpIntv : 
						GPS_SEND_INTERVAL_MIN;
#if 1
	gps_upload_cnt = tmpCnt;
	if (gps_upload_cnt) 
	{
		if (gps_samp_interval * gps_upload_cnt < 60)
		{
			gps_samp_interval = 60 / gps_upload_cnt;
		}
		if (gps_samp_interval * gps_upload_cnt < 60)
		{
			gps_samp_interval += 1;
		}
	}
#else
	gps_upload_cnt = (tmpCnt == 0) ? 
					  0 :
					  (tmpCnt > GPS_SEND_ITEMS_ONETIME) ? 
					  tmpCnt :
					  GPS_SEND_ITEMS_ONETIME;

#endif
	tmpSetting = gps_upload_cnt << 16 | gps_samp_interval;
	GPS_APP_WriteGprsUploadSettings(&tmpSetting);

	// upload data in current storing buffer immediately
	if (gps_curr_bearer_type == GPS_BEARER_GPRS)
	{
		if (GPSAppEventFifoIsEmpty())
		{
			kal_uint8 *tmpPtr;
			GPSAppEvent_t tmpEvent;
			kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_SMS;

			if (GPS_APP_StartGprsUpload(cause_type))
			{
				tmpPtr = (kal_uint8 *)get_ctrl_buffer(sizeof(kal_uint8));
				*tmpPtr = cause_type;
				tmpEvent.OpCode = GPS_APP_EVENT_OP_GPRSUPLOAD;
				tmpEvent.LocalPara = tmpPtr;
				GPSAppEventFifoPush(&tmpEvent, KAL_TRUE);
				gps_app_log("GPS_AT_ChangeGprsUploadSetting, push req into fifo, OpCode: %d", tmpEvent.OpCode);
			}
		}
		else
		{
			// fifo full, discard req
			gps_app_log("GPS_AT_ChangeGprsUploadSetting, fifo full");
#ifdef GPS_BACKUPLOAD_DAEMON
			// store data in current storing buffer into backup file
			GPS_APP_StoreBackupData(GPS_PP_BUFF_PURPOSE_REFERNCE);
#endif
		}
	}

	// reset pp buffer threshold
	// Note: reset pp buffer threshold must be after finish one of the following:
	//   1. upload data in current storing buffer to server through gprs
	//   2. store data in current buffer into backup files
	GPSPPBufferReset(GPS_APP_PPBufferFullCB);
	GPSPPBufferSetThreshold(gps_upload_cnt);

	if (gps_curr_bearer_type != GPS_BEARER_GPRS)
	{
		return RESULT_OK;
	}

	// reset sampling timer
	if (gps_upload_cnt > 0 && gps_samp_interval > 0)
	{
		if (gps_sampgprmc_timer != NULL)
		{
			GPSAppTimer_Reset(gps_sampgprmc_timer,
						GPS_APP_SampGprmcRepeatHandler,
						gps_samp_interval * KAL_TICKS_1_SEC,
						gps_samp_interval * KAL_TICKS_1_SEC,
						KAL_TRUE);
		}
		else
		{
			gps_sampgprmc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_SampGprmcRepeatHandler,
									gps_samp_interval * KAL_TICKS_1_SEC,
									gps_samp_interval * KAL_TICKS_1_SEC,
									KAL_TRUE);
		}
	}
	else
	{
		if (gps_sampgprmc_timer != NULL)
		{
			GPSAppTimer_Stop(gps_sampgprmc_timer, KAL_TRUE);
			gps_sampgprmc_timer = NULL;
		}
	}

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeGprsUser
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeGprsUser(const char *pUser)
{
	GPS_APP_WriteGprsUsername(pUser);
	strcpy(gps_gprs_username, pUser);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeGprsPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeGprsPassword(const char *pPassword)
{
	GPS_APP_WriteGprsPassword(pPassword);
	strcpy(gps_gprs_userpwd, pPassword);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeGprsAPN
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeGprsAPN(const char *pAPN)
{
	char tmpStr[2] = {'\0'};

	// Set GPRS APN
	gps_set_gprs_account_direct(GPS_GPRS_ACCOUNT_IDX,
								pAPN,
								tmpStr,
								tmpStr);
	strcpy(gps_gprs_apn, pAPN);
	strcpy(gps_gprs_apnuser, tmpStr);
	strcpy(gps_gprs_apnpwd, tmpStr);
	GPS_APP_WriteGprsAPN(gps_gprs_apn, gps_gprs_apnuser, gps_gprs_apnpwd);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_ChangeGprsAccount
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_ChangeGprsAccount(const char *pAPN, const char *pUser, const char *pPwd)
{
	// Set GPRS APN
	gps_set_gprs_account_direct(GPS_GPRS_ACCOUNT_IDX,
								pAPN,
								pUser,
								pPwd);
	strcpy(gps_gprs_apn, pAPN);
	strcpy(gps_gprs_apnuser, pUser);
	strcpy(gps_gprs_apnpwd, pPwd);
	GPS_APP_WriteGprsAPN(gps_gprs_apn, gps_gprs_apnuser, gps_gprs_apnpwd);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_SwitchGpsLog
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_SwitchGpsLog(const kal_uint8 *pValue)
{
	GPS_SetTrace(*pValue);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_SwitchHandfree
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_SwitchHandfree(const kal_uint8 *pValue)
{
	GPS_APP_WriteHFreeSettings(pValue);
	gps_call_handfree = *pValue;

	return RESULT_OK;
}

/**
 * Function: GPS_AT_SwitchGpsProf
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_SwitchGpsProf(const kal_uint8 *pValue)
{
	if (gps_module_en_setting == *pValue) return RESULT_OK;

	// clean current prof before change to another
	switch (gps_module_en_setting)
	{
	case GPS_MODULE_POWERSTATE_ON:
	case GPS_MODULE_POWERSTATE_OFF:
		break;
	case GPS_MODULE_POWERSTATE_AUTO:
#ifdef VIBRATION_SENSOR_SUPPORT
		GPSLocateVibSensorStop();
#endif
		break;
	case GPS_MODULE_POWERSTATE_SWAUTO:
#ifdef GPS_MOD_SWAUTOPOWER
		GPS_APP_ModSWPowerTimerStop();
		SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_FALSE);
#endif
		break;
	default:
		break;
	}

	GPS_APP_WriteGPSOnSettings(pValue);
	gps_module_en_setting = *pValue;

	switch (gps_module_en_setting)
	{
	case GPS_MODULE_POWERSTATE_ON:
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
		break;
	case GPS_MODULE_POWERSTATE_OFF:
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
		break;
	case GPS_MODULE_POWERSTATE_AUTO:
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
#ifdef VIBRATION_SENSOR_SUPPORT
		GPSLocateVibSensorStart();
#endif
		break;
	case GPS_MODULE_POWERSTATE_SWAUTO:
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
#ifdef GPS_MOD_SWAUTOPOWER
		GPS_APP_ModSWPowerTimerStart();
#endif
		break;

	default:
		ASSERT(0);
		break;
	}

	return RESULT_OK;
}

/**
 * Function: GPS_AT_SwitchMtcallProf
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_SwitchMtcallProf(const kal_uint8 *pValue)
{
	if (gps_mtcall_profile == *pValue) return RESULT_OK;

	GPS_APP_WriteMtcallProfile(pValue);
	gps_mtcall_profile = *pValue;
	GpsAppMmiBrgSetProfile(gps_mtcall_profile);

	return RESULT_OK;
}

/**
 * Function: GPS_AT_SwitchSosCall
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_AT_SwitchSosCall(const kal_uint8 *pValue)
{
	if (gps_sos_phonecall == *pValue) return RESULT_OK;

	GPS_APP_WriteSosCallSetting(pValue);
	gps_sos_phonecall = *pValue;

	return RESULT_OK;
}

