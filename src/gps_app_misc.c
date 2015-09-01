/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app.c
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
 * 2007-11-12 11:12 - Initial create.
 ****************************************************************************/

/*************************************************************************
 * Include Statements
 *************************************************************************/
#include <math.h>

#include "kal_release.h"
#include "stack_common.h"
#include "stack_msgs.h"
#include "task_main_func.h"
#include "app_ltlcom.h"
#include "lcd_ip_cqueue.h"
#include "stack_types.h"
#include "task_config.h"
#include "syscomp_config.h"
#include "custom_config.h"
#include "custom_util.h"
#include "stack_init.h"
#include "stack_ltlcom.h"
#include "app_buff_alloc.h"
#include "drv_comm.h"
#include "reg_base.h"
#include "gpio_hw.h"
#include "gpio_sw.h"
#include "em_struct.h"
#include "stack_timer.h"
#include "L4a.h"

#include "gps_app_configurable_const.h"
#include "gps_app_unconfigurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "parser.h"
#include "gps_app.h"
#include "gps_app_nvram_mgr.h"
#include "gps_app_sq_wave.h"
#include "gps_app_timer_mgr.h"
#include "gps_app_pp_buff_mgr.h"
#include "gps_soc.h"
#include "gps_app_ind_mgr.h"
#include "gps_app_mmi_bridge.h"
#include "gps_app_databackup.h"
#include "gps_app_data.h"
#ifdef GPS_DEFENCE_FEATURE
#include "gps_app_defence_moniter.h"
#endif
#ifdef VIBRATION_SENSOR_SUPPORT
#include "gps_app_vibration_sensor.h"
#endif
#include "gps_app_upldmode_moniter.h"

/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Macro defines
 *************************************************************************/
#define GPS_APP_DEBUG
#ifdef GPS_APP_DEBUG
#define gps_app_log		trace_printf
#else
static void gps_app_log(kal_char *fmt, ...) { }
#endif

#define GPS_SIMPLE_LOCATE_TIMEOUT		(KAL_TICKS_5_SEC * 10)	// custom require 80s, we set it to be 50s
#define GPS_MOCALL_DIAL_TIMEOUT			(KAL_TICKS_30_SEC)
#define GPS_MTCALL_RING_TIMEOUT			(KAL_TICKS_5_SEC * 2)
#ifdef GPS_DEFENCE_FEATURE
#define GPS_SETDEFENCEON_TIMER			(KAL_TICKS_5_SEC * 2)
#endif
#ifdef GPS_BACKUPLOAD_DAEMON
#define GPS_BACKUPDAEMON_TIMER			(KAL_TICKS_1_MIN * 1 + KAL_TICKS_30_SEC)
#endif
#ifdef GPS_POSITION_MONITOR
#define GPS_POSMONITOR_TIMER			(KAL_TICKS_1_MIN * 3)
#endif
#ifdef GPS_RATE_MONITOR
#define GPS_RATEMONITOR_TIMER			KAL_TICKS_5_SEC
#endif
#ifdef GPS_MOD_SWAUTOPOWER
#define GPS_MODSWPOWER_TIMER			(KAL_TICKS_1_MIN * 1)
#endif
#define GPS_MOTOPOWERCUT_TIMEOUT		(KAL_TICKS_1_MIN * 10)


/*************************************************************************
 * Local variables
 *************************************************************************/
static kal_bool				gps_trace_on		 	= KAL_TRUE;

static kal_bool				gps_timing_locate_flag 	= KAL_FALSE;
static kal_bool				gps_multi_send_flag		= KAL_FALSE;
static kal_bool				gps_vbat_warn_flag		= KAL_FALSE;

static kal_uint8 			gps_call_type 			= GPSLOCATE_CALLTYPE_UNKNOWN;
static GPS_Locate_State_t	gps_locate_state 		= GPS_LOCATE_STATE_IDLE;

kal_uint8 					gps_uart_sleep_handle	= 0;

static const char *gps_sms_uploadTypeStr[] =
{
	"SMS",
	"TIMER",
	"CALL",
	"ANSWER",
	"SOS",
	"ALARM",
	"OS",
	"RS",
	"OVERSPEED",
	"SAFESPEED",
	"LP",

	"LPSMS",
	"LPTIMER",
	"LPCALL",
	"LPANSWER",
	"LPSOS",
	"LPALARM",
	"LPOS",
	"LPRS",
	"LPOVERSPEED",
	"LPSAFESPEED",
	"LP",

	"CHSMS",
	"CHTIMER",
	"CHCALL",
	"CHANSWER",
	"CHSOS",
	"CHALARM",
	"CHOS",
	"CHRS",
	"CHOVERSPEED",
	"CHSAFESPEED",
	"CHLP",

	""
};

static const char *gps_sms_upldtype_str(kal_uint8 type);

/*************************************************************************
 * Global variables
 *************************************************************************/
extern char gps_locate_info_buff[];
extern GPSLocateRegStateStruct_t gps_reg_state;


/*************************************************************************
 * Function declaration
 *************************************************************************/
extern void gps_locate_send_sms_direct(	const GPSLocatePhoneNumber_t* dest,
										const char* msg,
										unsigned short len);
extern void gps_locate_delete_sms_direct(void);
extern void gps_get_nwinfo_request(kal_uint32 info);
extern void gps_giveup_nwinfo_request(void);
extern void gps_set_gprs_account_direct(unsigned char indx,
									  const char* apn,
									  const char* usr,
									  const char* psw);

extern void MakeGPsCallRejectIncoming(void);
extern void MakeGPSCallReq(kal_uint8 *MsgStruct, void *callBack);
extern kal_bool l4c_nw_get_imei_req (kal_uint8 src_id,
                                     kal_uint8 *imei,
                                     kal_uint8 *svn);

#ifdef GPS_DEFENCE_FEATURE
void GPS_APP_SetDefenceOnTimer(GPSAppTimerID_t Id);
#endif
void GPS_APP_SimpleLocateTimeout(GPSAppTimerID_t Id);
void GPS_APP_MTCallRingTimeout(GPSAppTimerID_t Id);
void GPS_APP_MOCallDialTimeout(GPSAppTimerID_t Id);
#ifdef GPS_POSITION_MONITOR
void GPS_APP_SetPositionTimeout(GPSAppTimerID_t Id);
#endif

extern void aud_speech_set_output_volume(kal_uint8 volume1, kal_int8 digital_gain_index);
extern void L1SP_SetSpeechVolumeLevel( kal_uint8 level );
extern void aud_mic_set_volume(kal_uint8 volume1, kal_uint8 volume2);
extern kal_uint8 aud_get_audio_mode(void);
extern kal_uint8 aud_get_volume_level(kal_uint8 mode, kal_uint8 type);
extern kal_uint8 aud_get_volume_gain(kal_uint8 mode, kal_uint8 type, kal_uint8 level);

//-----------------------------------------------------------------------------
static const char *gps_sms_upldtype_str(kal_uint8 type)
{
	const char *retStr = NULL;

	ASSERT(type < GPS_RMCSMS_TYPE_TOTAL);

	if (gps_app_is_charger_connected)
	{
		retStr = gps_sms_uploadTypeStr[type + GPS_RMCSMS_TYPE_TOTAL * 2];
	}
	else if (gps_lowbattery_warnning)
	{
		retStr = gps_sms_uploadTypeStr[type + GPS_RMCSMS_TYPE_TOTAL];
	}
	else
	{
		retStr = gps_sms_uploadTypeStr[type];
	}

	return retStr;
}

/**
 * Function: GPS_APP_ConstructCellInfoSMS
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_ConstructCellInfoSMS(char *dest, kal_uint8 bs_count, char sep_sign)
{
	char *p_buf;
	int i;

	if (bs_count <= 0) return;

	ASSERT(dest != NULL);

	dest[0] = '\0';
	p_buf = dest;
	sprintf(p_buf, "%c%04x%04x",
			sep_sign, 
			gps_reg_state.lac_value, 
			gps_reg_state.cell_id);

#if (defined GPS_REPORT_NEBR_ARFCN)
	bs_count = (bs_count > GSM_NEIGHBOR_CELL_MAX+1) ? (GSM_NEIGHBOR_CELL_MAX+1) : bs_count;
	for (i = 0; i < bs_count; i++)
	{
		p_buf = dest + strlen(dest);
		sprintf(p_buf, ",%04x%04x", 
				gps_reg_state.nc_arfcn[i], 
				-gps_reg_state.rla_in_quarter_dbm[i]);
	}
#endif
}

/** Construct reply sms from cell info */
void GPS_APP_ConstructCellInfoSMS2(char *dest, kal_uint8 bs_count)
{
	char *p_buf;
	int i;

	if (bs_count <= 0) return;

	ASSERT(dest != NULL);

	dest[0] = '\0';
	p_buf = dest;
	sprintf(p_buf, "BS:%04x%04x",
			gps_reg_state.lac_value, 
			gps_reg_state.cell_id);

#if (defined GPS_REPORT_NEBR_ARFCN)
	bs_count = (bs_count > GSM_NEIGHBOR_CELL_MAX+1) ? (GSM_NEIGHBOR_CELL_MAX+1) : bs_count;
	for (i = 0; i < bs_count; i++)
	{
		p_buf = dest + strlen(dest);
		sprintf(p_buf, ",%04x%04x", 
				gps_reg_state.nc_arfcn[i], 
				-gps_reg_state.rla_in_quarter_dbm[i]);
	}
#endif

	p_buf = dest + strlen(dest);
	sprintf(p_buf, "\r\n");
}

/**
 * Function: GPS_APP_ConstructGprmcSMS
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ConstructGprmcSMS(char *dest, GPS_GPRMC_RawData_t *gprmc_data, char sep_sign)
{
	char *p_buf;

	ASSERT(dest != NULL);

	dest[0] = '\0';
	p_buf = dest;
	GPS_APP_ConstructCellInfoSMS(p_buf, gps_bs_num, sep_sign);

	p_buf = dest + strlen(dest);
	if (gprmc_data == NULL)
	{
		sprintf(p_buf, "%cError GPS GPRMC FRAME DATA", 
				sep_sign);
	}
	else
	{
		sprintf(p_buf, "%c%s,%c,%s,%c,%s,%s%c%s,%s", 
				sep_sign,
				gprmc_data->longitude,
				gprmc_data->ew,
				gprmc_data->latitude,
				gprmc_data->ns,
				gprmc_data->rate,
				gprmc_data->direction,
				sep_sign,
				gprmc_data->utc_date,
				gprmc_data->utc_time
				);
	}

	if (gps_curr_sms_mode == GPS_SMS_MODE_SC)
	{
		p_buf = dest + strlen(dest);
		sprintf(p_buf, "%c%s%c%s", 
				sep_sign, 
				gps_sc_number.passwd, 
				sep_sign, 
				(char *)gps_act_remote_num.Number);
	}
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "%c%s", sep_sign, gps_imei_str);

	p_buf = dest + strlen(dest);
	sprintf(p_buf, "%c%s", sep_sign, gps_sms_upldtype_str(gps_smsupld_cause_type));

	p_buf = dest + strlen(dest);
	sprintf(p_buf, "%c%c", 
			sep_sign, 
			sep_sign);

	return RESULT_OK;
}

/**
 * Function: GPS_APP_ConstructReadableGprmcSMS
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ConstructReadableGprmcSMS(char *dest, GPS_GPRMC_RawData_t *gprmc_data, kal_uint8 type)
{
	char *p_buf;
	Result_t result = RESULT_ERROR;
	GPS_GPRMC_Packed_Struct_t rmc_pack;
	double speed_km = 0.0;
	double long_d, lat_d;

	ASSERT(dest != NULL);

	dest[0] = '\0';
	p_buf = dest;

	p_buf = dest + strlen(dest);
	if (gprmc_data == NULL)
	{
		sprintf(p_buf, "Error GPS GPRMC FRAME DATA\r\n");
		p_buf = dest + strlen(dest);
		GPS_APP_ConstructCellInfoSMS2(p_buf, gps_bs_num);
		goto DONE;
	}

	result = GPS_APP_GPRMC_Raw2Packed(&rmc_pack, gprmc_data);
	if (result != RESULT_OK)
	{
		sprintf(p_buf, "Error GPS GPRMC FRAME DATA\r\n");
		p_buf = dest + strlen(dest);
		GPS_APP_ConstructCellInfoSMS2(p_buf, gps_bs_num);
		goto DONE;
	}

	// latitude
	lat_d = rmc_pack.latitude_d + rmc_pack.latitude_c / 60.0 + rmc_pack.latitude_cf / 600000.0;
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Lat:%c%.5f\r\n", 
			rmc_pack.latitude_ns ? '-' : '+', 
			lat_d);
	// longitude
	long_d = rmc_pack.longitude_d + rmc_pack.longitude_c / 60.0 + rmc_pack.longitude_cf / 600000.0;
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Long:%c%.5f\r\n", 
			rmc_pack.longitude_ew ? '-' : '+', 
			long_d);
	// speed
	speed_km = 1.852 * ((double)rmc_pack.rate + (double)rmc_pack.rate_f / 100.0);
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Speed:%.2fKM/H\r\n", speed_km);
	// direction
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Direction:%s\r\n", gprmc_data->direction);
	// date
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Date:20%02d-%02d-%02d\r\n", 
			rmc_pack.date_y, 
			rmc_pack.date_m, 
			rmc_pack.date_d);
	// time
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Time:%02d:%02d:%02d\r\n", 
			rmc_pack.time_h, 
			rmc_pack.time_m, 
			rmc_pack.time_s);
	// base station
	p_buf = dest + strlen(dest);
	GPS_APP_ConstructCellInfoSMS2(p_buf, gps_bs_num);
	// fix status
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "Fix:%c\r\n", gprmc_data->status);

DONE:
	p_buf = dest + strlen(dest);
	sprintf(p_buf, "ID:%s\r\n", gps_imei_str);

	p_buf = dest + strlen(dest);
	sprintf(p_buf, "STATE:%s\r\n", gps_sms_upldtype_str(type));

	return RESULT_OK;
}

/**
 * Function: GPS_APP_GetNWInfoHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_GetNWInfoHandler(GPSAppTimerID_t Id)
{
    char *p_buf;

	// stop the timer
	if (gps_getnwinfo_timer != NULL)
	{
		GPSAppTimer_Stop(gps_getnwinfo_timer, KAL_TRUE);
		gps_getnwinfo_timer = NULL;
	}
	// stop request nwinfo
	gps_giveup_nwinfo_request();

	gps_locate_info_buff[0] = '\0';
	p_buf = gps_locate_info_buff;

	if (gps_curr_bearer_type == GPS_BEARER_SMS && 
		gps_curr_sms_mode == GPS_SMS_MODE_SC)
	{
		GPS_APP_ConstructCellInfoSMS(p_buf, 1+GSM_NEIGHBOR_CELL_MAX, GPS_VALID_SEPARATOR_SIGN);

		p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
		sprintf(p_buf, "%c%s%c%s", 
				GPS_VALID_SEPARATOR_SIGN, 
				gps_sc_number.passwd, 
				GPS_VALID_SEPARATOR_SIGN, 
				(char *)gps_act_remote_num.Number);

		p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
		sprintf(p_buf, "%c%c", 
				GPS_VALID_SEPARATOR_SIGN, 
				GPS_VALID_SEPARATOR_SIGN);
	}
	else
	{
		GPS_APP_ConstructCellInfoSMS2(p_buf, 1+GSM_NEIGHBOR_CELL_MAX);
	}

	if (gps_curr_sms_mode == GPS_SMS_MODE_SC)
	{
		GPS_SMS_ConstructAndSendSms(&gps_act_sc_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
		gps_app_log("GPS_SMS_GetCellInfo, reply sms \"%s\" to number: %s", 
					gps_locate_info_buff, 
					gps_act_sc_num.Number);
	}
	else
	{
		GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
		gps_app_log("GPS_SMS_GetCellInfo, reply sms \"%s\" to number: %s", 
					gps_locate_info_buff, 
					gps_act_remote_num.Number);
	}
}

#ifdef GPS_DEFENCE_FEATURE
/**
 * Function: GPS_APP_DefenceTimerStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_DefenceTimerStop()
{
	gps_app_log("GPS_APP_DefenceTimerStop");
	if (gps_defence_timer != NULL)
	{
		GPSAppTimer_Stop(gps_defence_timer, KAL_TRUE);
		gps_defence_timer = NULL;
	}
}

/**
 * Function: GPS_APP_SetDefenceOnTimer
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetDefenceOnTimer(GPSAppTimerID_t Id)
{
	gps_app_log("GPS_APP_SetDefenceOnTimer");

	GPS_APP_DefenceTimerStop();
	GPS_APP_EnableDefence();
}

/**
 * Function: GPS_APP_SetDefenceON
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetDefenceON(void)
{
	gps_app_log("GPS_APP_SetDefenceON");

	if (gps_defence_timer != NULL)
	{
		GPSAppTimer_Reset(gps_defence_timer, 
				GPS_APP_SetDefenceOnTimer, 
				GPS_SETDEFENCEON_TIMER,
				0, 
				KAL_TRUE);
	}
	else
	{
		gps_defence_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_SetDefenceOnTimer,
									GPS_SETDEFENCEON_TIMER,
									0,
									KAL_TRUE);
	}
}
#endif

/**
 * Function: GPS_APP_MTCallRingTimeout
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_MTCallRingTimeout(GPSAppTimerID_t Id)
{
	GPSLocateMTCallReqStruct_t* pReqData;

	gps_app_log("GPS_APP_MTCallRingTimeout, current active number: %s, type: %#x, len: %d", 
				gps_act_remote_num.Number, 
				gps_act_remote_num.Type, 
				gps_act_remote_num.Length);

	gps_mtcall_connecting = KAL_FALSE;

	pReqData = (GPSLocateMTCallReqStruct_t*)construct_local_para(sizeof(GPSLocateMTCallReqStruct_t), TD_UL);
	pReqData->ReqCategory = GPSLOCATE_MTCALL_REQ_CATEGORY_AUTO_ANSWERED;
	memcpy(&pReqData->RemoteNumber, &gps_act_remote_num, sizeof(gps_act_remote_num));
	
	SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_MTCALL_REQ, (void*)pReqData);
}


/**
 * Function: GPS_APP_MTCallRingTimerStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *	KAL_TRUE - the timer is started successfully
 *	KAL_FALSE - the timer is not started because of the unaccepted remote number
 *
 */
#if GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
void 
#else //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
kal_bool 
#endif //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
GPS_APP_MTCallRingTimerStart(GPSLocatePhoneNumber_t *pNumber)
{
	kal_uint8 found = 0;

    gps_app_log("GPS_APP_MTCallRingTimerStart, remote number type: %#x, length: %d, number: %s", 
    			pNumber->Type, 
    			pNumber->Length, 
    			pNumber->Number);

	found = GPS_APP_FindNumberInPresavdList(pNumber);
	if (!found)	
	{
		gps_app_log("Number %s not in saved list, reject it", (char *)pNumber->Number);
	#if GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
		GpsAppMmiBrgHangupAllCalls();
		return;
	#else //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
		GPS_APP_RejectUnindicatedMTCall();
		return KAL_FALSE;
	#endif //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
	}

	//flash the network indicator LED
	GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								GPS_IND_CALLCONNING_INTV * KAL_TICKS_1_SEC);

    memcpy(&gps_act_remote_num, pNumber, sizeof(gps_act_remote_num));

	if (gps_mtcall_timer != NULL)
	{
		GPSAppTimer_Reset(gps_mtcall_timer, 
				GPS_APP_MTCallRingTimeout, 
				GPS_MTCALL_RING_TIMEOUT,
				0, 
				KAL_TRUE);
	}
	else
	{
		gps_mtcall_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_MTCallRingTimeout,
									GPS_MTCALL_RING_TIMEOUT,
									0,
									KAL_TRUE);
	}

	GPS_APP_SetCallType(GPSLOCATE_CALLTYPE_MT);
	gps_mtcall_connecting = KAL_TRUE;
	gps_mtcall_answered = KAL_FALSE;
#if !(GPSLOCATE_INDICATE_UNACCEPTED_MTCALL)
	return KAL_TRUE;
#endif //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
}


/**
 * Function: GPS_APP_MTCallRingTimerStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_MTCallRingTimerStop()
{
	gps_app_log("GPS_APP_MTCallRingTimerStop");
	gps_mtcall_connecting = KAL_FALSE;
	GPS_APP_SetCallType(GPSLOCATE_CALLTYPE_UNKNOWN);
	if (gps_mtcall_timer != NULL)
	{
		GPSAppTimer_Stop(gps_mtcall_timer, KAL_TRUE);
		gps_mtcall_timer = NULL;
	}
}


void GPS_APP_MOCallDialTimeout(GPSAppTimerID_t Id)
{
	GPSLocateMOCallRsptIndStruct_t* pReqData;

	gps_app_log("GPS_APP_MOCallDialTimeout");
	GPS_APP_MOCallDialTimerStop();
	GPS_APP_CancelDialing();

	pReqData = (GPSLocateMOCallRsptIndStruct_t*)construct_local_para(sizeof(GPSLocateMOCallRsptIndStruct_t), TD_UL);
	pReqData->RspCategory = GPSLOCATE_MOCALL_RSP_CATEGORY_NOACCEPT;
	//should copy remotenumber to here,then phone know dial which number next.//Ellen
	kal_mem_cpy(pReqData->RemoteNumber.Number, gps_mocall_number.Number, gps_mocall_number.Length);
	pReqData->RemoteNumber.Number[gps_mocall_number.Length] = 0x00;
	pReqData->RemoteNumber.Length = gps_mocall_number.Length;

	SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_MOCALL_RSP, (void*)pReqData);
}

void GPS_APP_MOCallDialNextNumber(GPSAppTimerID_t Id)
{
	GPSLocateMOCallRsptIndStruct_t* pReqData;

	gps_app_log("GPS_APP_MOCallDialNextNumber");

	if (gps_mocall_timer2 != NULL)
	{
		GPSAppTimer_Stop(gps_mocall_timer2, KAL_TRUE);
		gps_mocall_timer2 = NULL;
	}

	gps_mocall_state = GPSLOCATE_MOCALLSTATE_CONNECTING;
	pReqData = (GPSLocateMOCallRsptIndStruct_t*)construct_local_para(sizeof(GPSLocateMOCallRsptIndStruct_t), TD_UL);
	pReqData->RspCategory = GPSLOCATE_MOCALL_RSP_CATEGORY_CALLNEXT;
	//copy remote number to here, then phone know dial which number next
	kal_mem_cpy(pReqData->RemoteNumber.Number, gps_mocall_number.Number, gps_mocall_number.Length);
	pReqData->RemoteNumber.Number[gps_mocall_number.Length] = 0x00;
	pReqData->RemoteNumber.Length = gps_mocall_number.Length;

	SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_MOCALL_RSP, (void*)pReqData);
}

/**
 * Function: GPS_APP_MOCallDialTimerStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_MOCallDialTimerStart(char *pNumber)
{
	int length = 0;

	if (pNumber == NULL) return;

	gps_app_log("GPS_APP_MOCallDialTimerStart");

	GPS_APP_SetCallType(GPSLOCATE_CALLTYPE_MO);
	if (gps_mocall_timer != NULL)
	{
		GPSAppTimer_Reset(gps_mocall_timer, 
				GPS_APP_MOCallDialTimeout, 
				GPS_MOCALL_DIAL_TIMEOUT,
				0, 
				KAL_TRUE);
	}
	else
	{
		gps_mocall_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_MOCallDialTimeout,
									GPS_MOCALL_DIAL_TIMEOUT,
									0,
									KAL_TRUE);
	}

	length = strlen(pNumber);
	// Here copy mocall dialed number into golbal
	if(length > 0)
	{
		kal_mem_set(&gps_mocall_number, 0, sizeof(gps_mocall_number));
		strcpy((char *)gps_mocall_number.Number, pNumber);
		gps_mocall_number.Length = length;
		gps_mocall_number.Type= GPS_APP_GetNumberType((char *)gps_mocall_number.Number);
		
	}

	//flash the network indicator LED
	GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								GPS_IND_CALLCONNING_INTV * KAL_TICKS_1_SEC);
}

/**
 * Function: GPS_APP_MOCallDialTimerStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_MOCallDialTimerStop()
{
	gps_app_log("GPS_APP_MOCallDialTimerStop");
	if (gps_mocall_timer != NULL)
	{
		GPSAppTimer_Stop(gps_mocall_timer, KAL_TRUE);
		gps_mocall_timer = NULL;
	}
}

#ifdef GPS_MOTOPWRCUT_FEATURE
void GPS_APP_MotoPowerCutTimeout(GPSAppTimerID_t Id)
{
	gps_motopower_cut = 0;
	GPS_APP_MotoPowerCutTimerStop();
}

void GPS_APP_MotoPowerCutTimerStart(void)
{
	if (gps_motopowercut_timer != NULL)
	{
		GPSAppTimer_Reset(gps_motopowercut_timer, 
				GPS_APP_MotoPowerCutTimeout, 
				GPS_MOTOPOWERCUT_TIMEOUT,
				0, 
				KAL_TRUE);
	}
	else
	{
		gps_motopowercut_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_MotoPowerCutTimeout,
									GPS_MOTOPOWERCUT_TIMEOUT,
									0,
									KAL_TRUE);
	}
}

void GPS_APP_MotoPowerCutTimerStop(void)
{
	gps_app_log("GPS_APP_MotoPowerCutTimerStop");
	if (gps_motopowercut_timer != NULL)
	{
		GPSAppTimer_Stop(gps_motopowercut_timer, KAL_TRUE);
		gps_motopowercut_timer = NULL;
	}
}

void GPS_APP_MotoPowerOnTimeout(GPSAppTimerID_t Id)
{
	gps_motopower_on = 0;
	GPS_APP_MotoPowerOnTimerStop();
}

void GPS_APP_MotoPowerOnTimerStart(void)
{
	if (gps_motopoweron_timer != NULL)
	{
		GPSAppTimer_Reset(gps_motopoweron_timer, 
				GPS_APP_MotoPowerOnTimeout, 
				GPS_MOTOPOWERCUT_TIMEOUT,
				0, 
				KAL_TRUE);
	}
	else
	{
		gps_motopoweron_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_MotoPowerOnTimeout,
									GPS_MOTOPOWERCUT_TIMEOUT,
									0,
									KAL_TRUE);
	}
}

void GPS_APP_MotoPowerOnTimerStop(void)
{
	gps_app_log("GPS_APP_MotoPowerOnTimerStop");
	if (gps_motopoweron_timer != NULL)
	{
		GPSAppTimer_Stop(gps_motopoweron_timer, KAL_TRUE);
		gps_motopoweron_timer = NULL;
	}
}
#endif

/**
 * Function: GPS_APP_SimpleLocateTimeout
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SimpleLocateTimeout(GPSAppTimerID_t Id)
{
	gps_app_log("GPS_APP_SimpleLocateTimeout");
	SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_SIMPLOC_TIMEOUT_IND, (void*)NULL);
}

/**
 * Function: GPS_APP_SingleLocateStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SingleLocateStart(const GPSLocatePhoneNumber_t *pNumber, kal_uint8 cause_type)
{
	gps_app_log("GPS_APP_SingleLocateStart, start a single locate req");

	gps_smsupld_cause_type = cause_type;
	gps_get_nwinfo_request(RR_EM_LAI_INFO | RR_EM_MEASUREMENT_REPORT_INFO);

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_SMSLOC, 
					KAL_TRUE);
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_START);
	memset(&gps_act_remote_num, 0, sizeof(gps_act_remote_num));
	memcpy(&gps_act_remote_num, pNumber, sizeof(gps_act_remote_num));

	//start the timer
	if (gps_simpleloc_timer == NULL)
	{
		gps_simpleloc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_SimpleLocateTimeout,
									GPS_SIMPLE_LOCATE_TIMEOUT,
									0,
									KAL_TRUE);
	}
	else
	{
		GPSAppTimer_Reset(gps_simpleloc_timer, 
				GPS_APP_SimpleLocateTimeout, 
				GPS_SIMPLE_LOCATE_TIMEOUT,
				0, 
				KAL_TRUE);
	}
}

/**
 * Function: GPS_APP_SingleLocateStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SingleLocateStop(void)
{
	if (gps_simpleloc_timer != NULL)
	{
		GPSAppTimer_Stop(gps_simpleloc_timer, KAL_TRUE);
		gps_simpleloc_timer = NULL;
	}
}

#ifdef GPS_POSITION_MONITOR
/**
 * Function: GPS_APP_SetPositionTimeout
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetPositionTimeout(GPSAppTimerID_t Id)
{
	gps_app_log("GPS_APP_SetPositionTimeout");
	SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_SETPOS_TIMEOUT_IND, (void*)NULL);
}

/**
 * Function: GPS_APP_SetPositionStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetPositionStart(const GPSLocatePhoneNumber_t *pNumber)
{
	gps_app_log("GPS_APP_SetPositionStart, start to require current location");

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_POSSET, 
					KAL_TRUE);
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_START);
    memcpy(&gps_act_remote_num, pNumber, sizeof(gps_act_remote_num));

	//start the timer
	if (gps_setposition_timer == NULL)
	{
		gps_setposition_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_SetPositionTimeout,
									GPS_SIMPLE_LOCATE_TIMEOUT,
									0,
									KAL_TRUE);
	}
	else
	{
		GPSAppTimer_Reset(gps_setposition_timer, 
				GPS_APP_SetPositionTimeout, 
				GPS_SIMPLE_LOCATE_TIMEOUT,
				0, 
				KAL_TRUE);
	}
}

/**
 * Function: GPS_APP_SetPositionStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetPositionStop(void)
{
	if (gps_setposition_timer != NULL)
	{
		GPSAppTimer_Stop(gps_setposition_timer, KAL_TRUE);
		gps_setposition_timer = NULL;
	}
}

/**
 * Function: GPS_APP_PosMonitorStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_PosMonitorStart(void)
{
	if (gps_posmonitor_timer != NULL)
	{
		GPSAppTimer_Reset(gps_posmonitor_timer, 
				GPS_APP_PosMonitorRepeatHandler, 
				GPS_POSMONITOR_TIMER,
				GPS_POSMONITOR_TIMER, 
				KAL_TRUE);
	}
	else
	{
		gps_posmonitor_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_PosMonitorRepeatHandler,
									GPS_POSMONITOR_TIMER,
									GPS_POSMONITOR_TIMER,
									KAL_TRUE);
	}
}

/**
 * Function: GPS_APP_PosMonitorStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_PosMonitorStop(void)
{
	if (gps_posmonitor_timer != NULL)
	{
		GPSAppTimer_Stop(gps_posmonitor_timer, KAL_TRUE);
		gps_posmonitor_timer = NULL;
	}
}

/**
 * Function: GPS_APP_SampGprmc4PosMonitorStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SampGprmc4PosMonitorStart(void)
{
	gps_app_log("GPS_APP_SampGprmc4PosMonitorStart");

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_POSMONITOR, 
					KAL_TRUE);
}

/**
 * Function: GPS_APP_PosMonitorRepeatHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_PosMonitorRepeatHandler(GPSAppTimerID_t Id)
{
	GPS_APP_SampGprmc4PosMonitorStart();
}
#endif

#ifdef GPS_RATE_MONITOR
/**
 * Function: GPS_APP_RateMonitorStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_RateMonitorStart(void)
{
	if (gps_ratemonitor_timer != NULL)
	{
		GPSAppTimer_Reset(gps_ratemonitor_timer, 
				GPS_APP_RateMonitorRepeatHandler, 
				GPS_RATEMONITOR_TIMER,
				GPS_RATEMONITOR_TIMER, 
				KAL_TRUE);
	}
	else
	{
		gps_ratemonitor_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_RateMonitorRepeatHandler,
									GPS_RATEMONITOR_TIMER,
									GPS_RATEMONITOR_TIMER,
									KAL_TRUE);
	}
}

/**
 * Function: GPS_APP_RateMonitorStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_RateMonitorStop(void)
{
	if (gps_ratemonitor_timer != NULL)
	{
		GPSAppTimer_Stop(gps_ratemonitor_timer, KAL_TRUE);
		gps_ratemonitor_timer = NULL;
	}
}

/**
 * Function: GPS_APP_SampGprmc4RateMonitorStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SampGprmc4RateMonitorStart(void)
{
	gps_app_log("GPS_APP_SampGprmc4RateMonitorStart");

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_RATEMONITOR, 
					KAL_TRUE);
}

/**
 * Function: GPS_APP_RateMonitorRepeatHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_RateMonitorRepeatHandler(GPSAppTimerID_t Id)
{
	GPS_APP_SampGprmc4RateMonitorStart();
}
#endif

/**
 * Function: GPS_APP_TimingLocateRepeatHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_TimingLocateRepeatHandler(GPSAppTimerID_t Id)
{
	if (gps_curr_bearer_type != GPS_BEARER_SMS)
	{
		return;
	}

	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_AUTO && 
		!GPS_APP_CheckGPSModuleOn())
	{
		return;
	}

	gps_app_log("GPS_APP_TimingLocateRepeatHandler, current timing number: %s, type: %#x, len: %d", 
				gps_curr_timing_num.Number, gps_curr_timing_num.Type, gps_curr_timing_num.Length);

	GPS_APP_SetTimingLocateFlag(KAL_TRUE);
	GPS_APP_SingleLocateStart(&gps_curr_timing_num, GPS_RMCSMS_TYPE_TIMER);
}

/**
 * Function: GPS_APP_SampGprmcStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SampGprmcStart()
{
	gps_app_log("GPS_APP_SampGprmcStart");

	gps_get_nwinfo_request(RR_EM_LAI_INFO);

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_GPRSUPLD, 
					KAL_TRUE);
}

/**
 * Function: GPS_APP_SampGprmcRepeatHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SampGprmcRepeatHandler(GPSAppTimerID_t Id)
{
	if (gps_curr_bearer_type != GPS_BEARER_GPRS)
	{
		return;
	}

	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_AUTO && 
		!GPS_APP_CheckGPSModuleOn())
	{
		return;
	}

	GPS_APP_SampGprmcStart();
}

/**
 * Function: GPS_APP_SampGprmcAndSendStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SampGprmcAndSendStart(void)
{
	gps_app_log("GPS_APP_SampGprmcAndSendStart");

	gps_get_nwinfo_request(RR_EM_LAI_INFO);

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_GPRSUPLDIMM, 
					KAL_TRUE);
}

#ifdef GPS_BACKUPLOAD_DAEMON
/**
 * Function: GPS_APP_UploadBackupDataRepeatHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_UploadBackupDataRepeatHandler(GPSAppTimerID_t Id)
{
	SendMsg2GPSApp(
				stack_int_get_active_module_id(),
				MSG_ID_GPSLOCATE_BACKUPLOADTIMER_IND,
				NULL);
}
#endif

#ifdef GPS_POWERON_PHONECALL
/**
 * Function: GPS_APP_MakeCallAndSendSmsHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_MakeCallAndSendSmsHandler(GPSAppTimerID_t Id)
{
	gps_app_log("GPS_APP_MakeCallAndSendSmsHandler");

	GPS_APP_MakeCallAndSndSmsStart();
	if (gps_1stcall_timer != NULL)
	{
		GPSAppTimer_Stop(gps_1stcall_timer, KAL_TRUE);
		gps_1stcall_timer = NULL;
	}
}
#endif

/**
 * Function: GPS_APP_DefSendSmsStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_DefSendSmsStart(GPS_SavedNumberType_t type)
{
	GPSLocatePhoneNumber_t tmpNum;
	int i;

	gps_app_log("GPS_APP_DefSendSmsStart");

	switch (type)
	{
	case GPS_SAVEDNUMBERTYPE_USER:
		gps_tgt_numgrp_sms = gps_usr_numbers;
		break;
	case GPS_SAVEDNUMBERTYPE_SOS:
		gps_tgt_numgrp_sms = gps_sos_numbers;
		break;
	default:
		ASSERT(0);
		break;
	}

	GPS_APP_SetMultiSendFlag(KAL_TRUE);

	for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		if (strlen(gps_tgt_numgrp_sms[i].number))
			break;
	}
	if (i >= GPSLOCATE_USER_NUMBER_MAX)
	{
		gps_app_log("GPS_APP_DefSendSmsStart, all numbers empty, do nothing");
		return;
	}

	memset(&tmpNum, 0, sizeof(tmpNum));
	strcpy((char *)tmpNum.Number, gps_tgt_numgrp_sms[i].number);
	tmpNum.Type = GPS_APP_GetNumberType((char *)tmpNum.Number);
	tmpNum.Length = strlen((char *)tmpNum.Number);

	GPS_APP_SingleLocateStart(&tmpNum, GPS_RMCSMS_TYPE_ALARM);
	gps_multi_send_index = i + 1;
}

/**
 * Function: GPS_APP_GeoSendSmsStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_GeoSendSmsStart(GPS_SavedNumberType_t type, kal_bool out)
{
	GPSLocatePhoneNumber_t tmpNum;
	int i;

	gps_app_log("GPS_APP_GeoSendSmsStart");

	switch (type)
	{
	case GPS_SAVEDNUMBERTYPE_USER:
		gps_tgt_numgrp_sms = gps_usr_numbers;
		break;
	case GPS_SAVEDNUMBERTYPE_SOS:
		gps_tgt_numgrp_sms = gps_sos_numbers;
		break;
	default:
		ASSERT(0);
		break;
	}

	GPS_APP_SetMultiSendFlag(KAL_TRUE);

	for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		if (strlen(gps_tgt_numgrp_sms[i].number))
			break;
	}
	if (i >= GPSLOCATE_USER_NUMBER_MAX)
	{
		gps_app_log("GPS_APP_GeoSendSmsStart, all numbers empty, do nothing");
		return;
	}

	memset(&tmpNum, 0, sizeof(tmpNum));
	strcpy((char *)tmpNum.Number, gps_tgt_numgrp_sms[i].number);
	tmpNum.Type = GPS_APP_GetNumberType((char *)tmpNum.Number);
	tmpNum.Length = strlen((char *)tmpNum.Number);

	GPS_APP_SingleLocateStart(&tmpNum, out ? GPS_RMCSMS_TYPE_GEOOS : GPS_RMCSMS_TYPE_GEORS);
	gps_multi_send_index = i + 1;
}

/**
 * Function: GPS_APP_SosSendSmsStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SosSendSmsStart(GPS_SavedNumberType_t type)
{
	GPSLocatePhoneNumber_t tmpNum;
	int i;

	gps_app_log("GPS_APP_SosSendSmsStart");

	switch (type)
	{
	case GPS_SAVEDNUMBERTYPE_USER:
		gps_tgt_numgrp_sms = gps_usr_numbers;
		break;
	case GPS_SAVEDNUMBERTYPE_SOS:
		gps_tgt_numgrp_sms = gps_sos_numbers;
		break;
	default:
		ASSERT(0);
		break;
	}

	GPS_APP_SetMultiSendFlag(KAL_TRUE);

	for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		if (strlen(gps_tgt_numgrp_sms[i].number))
			break;
	}
	if (i >= GPSLOCATE_USER_NUMBER_MAX)
	{
		gps_app_log("GPS_APP_SosSendSmsStart, all numbers empty, do nothing");
		return;
	}

	memset(&tmpNum, 0, sizeof(tmpNum));
	strcpy((char *)tmpNum.Number, gps_tgt_numgrp_sms[i].number);
	tmpNum.Type = GPS_APP_GetNumberType((char *)tmpNum.Number);
	tmpNum.Length = strlen((char *)tmpNum.Number);

	GPS_APP_SingleLocateStart(&tmpNum, GPS_RMCSMS_TYPE_SOS);
	gps_multi_send_index = i + 1;
}

/**
 * Function: GPS_APP_LowBattWarnSendSmsStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_LowBattWarnSendSmsStart(GPS_SavedNumberType_t type)
{
	GPSLocatePhoneNumber_t tmpNum;
	int i;

	gps_app_log("GPS_APP_LowBattWarnSendSmsStart");

	switch (type)
	{
	case GPS_SAVEDNUMBERTYPE_USER:
		gps_tgt_numgrp_sms = gps_usr_numbers;
		break;
	case GPS_SAVEDNUMBERTYPE_SOS:
		gps_tgt_numgrp_sms = gps_sos_numbers;
		break;
	default:
		ASSERT(0);
		break;
	}

	GPS_APP_SetMultiSendFlag(KAL_TRUE);
	GPS_APP_SetVbatWarnFlag(KAL_TRUE);

	gps_multi_send_index = 1;
	for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		if (strlen(gps_tgt_numgrp_sms[i].number))
			break;
	}
	if (i >= GPSLOCATE_USER_NUMBER_MAX)
	{
		gps_app_log("GPS_APP_LowBattWarnSendSmsStart, all numbers empty, do nothing");
		return;
	}

	memset(&tmpNum, 0, sizeof(tmpNum));
	strcpy((char *)tmpNum.Number, gps_tgt_numgrp_sms[i].number);
	tmpNum.Type = GPS_APP_GetNumberType((char *)tmpNum.Number);
	tmpNum.Length = strlen((char *)tmpNum.Number);

	GPS_APP_SingleLocateStart(&tmpNum, GPS_RMCSMS_TYPE_LP);
	gps_multi_send_index = i + 1;
}

/**
 * Function: GPS_APP_SosMOCallStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SosMOCallStart(GPS_SavedNumberType_t type)
{
	int i;

	if (!gps_sos_phonecall) return;
	gps_app_log("GPS_APP_SosMOCallStart");

	switch (type)
	{
	case GPS_SAVEDNUMBERTYPE_USER:
		gps_tgt_numgrp_mocall = gps_usr_numbers;
		break;
	case GPS_SAVEDNUMBERTYPE_SOS:
		gps_tgt_numgrp_mocall = gps_sos_numbers;
		break;
	default:
		ASSERT(0);
		break;
	}

	for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		if (strlen(gps_tgt_numgrp_mocall[i].number))
			break;
	}
	if (i >= GPSLOCATE_USER_NUMBER_MAX)
	{
		gps_app_log("GPS_APP_SosMOCallStart, all numbers empty, do nothing");
		gps_mocall_state = GPSLOCATE_MOCALLSTATE_IDLE;
		return;
	}
	MakeGPSCallReq((kal_uint8 *)gps_tgt_numgrp_mocall[i].number, 
				   (void *)GPS_APP_MOCallDialTimerStart);
	gps_app_log("GPS_APP_SosMOCallStart, make phone call to number %d: %s", 
				i + 1, 
				gps_tgt_numgrp_mocall[i].number);
	gps_sos_call_index = i + 1;
	gps_mocall_state = GPSLOCATE_MOCALLSTATE_CONNECTING;
}

#ifdef GPS_POWERON_PHONECALL
/**
 * Function: GPS_APP_MakeCallAndSndSmsStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_MakeCallAndSndSmsStart(void)
{
	// This function will be called when system boot up and network status is OK.
	// It will be called only once.
	// Declare flag as static to ensure this function is invoked only once.
	static kal_bool flag = KAL_TRUE;
	GPSLocatePhoneNumber_t tmpNum;
    char *p_buf;

	if (!flag)
		return;

	gps_app_log("GPS_APP_MakeCallAndSndSmsStart");

	flag = KAL_FALSE;

	memset(&tmpNum, 0, sizeof(tmpNum));
	strcpy((char *)tmpNum.Number, gps_usr_numbers[0].number);
	tmpNum.Type = GPS_APP_GetNumberType((char *)tmpNum.Number);
	tmpNum.Length = strlen((char *)tmpNum.Number);

#if 1	// Only reply base station info
	// stop request nwinfo
	gps_giveup_nwinfo_request();

	gps_locate_info_buff[0] = '\0';
	p_buf = gps_locate_info_buff;
	GPS_APP_ConstructCellInfoSMS(p_buf, 1+GSM_NEIGHBOR_CELL_MAX, GPS_VALID_SEPARATOR_SIGN);

	p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
	sprintf(p_buf, "%c%c", 
			GPS_VALID_SEPARATOR_SIGN, 
			GPS_VALID_SEPARATOR_SIGN);

	p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
	sprintf(p_buf, "GSM OK");

	GPS_SMS_ConstructAndSendSms(&tmpNum, gps_locate_info_buff, strlen(gps_locate_info_buff));
	gps_app_log("GPS_APP_MakeCallAndSndSmsStart, reply sms \"%s\" to number: %s", 
				gps_locate_info_buff, 
				tmpNum.Number);

#else

    GPS_APP_SingleLocateStart(&tmpNum, GPS_RMCSMS_TYPE_SMS);
#endif
}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
/**
 * Function: GPS_APP_ModSWPowerTimer
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_ModSWPowerTimer(GPSAppTimerID_t Id)
{
	gps_app_log("GPS_APP_ModSWPowerTimer");

	GPS_APP_ModSWPowerTimerStop();

	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_TRUE);
}

/**
 * Function: GPS_APP_ModSWPowerTimerStart
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_ModSWPowerTimerStart(void)
{
	//start the timer
	if (gps_modswpower_timer == NULL)
	{
		gps_modswpower_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_ModSWPowerTimer,
									GPS_MODSWPOWER_TIMER,
									0,
									KAL_TRUE);
	}
	else
	{
		GPSAppTimer_Reset(gps_modswpower_timer, 
				GPS_APP_ModSWPowerTimer, 
				GPS_MODSWPOWER_TIMER,
				0, 
				KAL_TRUE);
	}
}

/**
 * Function: GPS_APP_ModSWPowerTimerStop
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_ModSWPowerTimerStop(void)
{
	if (gps_modswpower_timer != NULL)
	{
		GPSAppTimer_Stop(gps_modswpower_timer, KAL_TRUE);
		gps_modswpower_timer = NULL;
	}
}
#endif

/**
 * Function: GPS_APP_Init_All
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_Init_All(void)
{
	Result_t result = RESULT_ERROR;
	char tmpStr[3];
	kal_uint16 tmpBM = 0;
	kal_uint32 tmpSetting = 0;
	kal_uint8 i;

	gps_uart_sleep_handle = L1SM_GetHandle();

	gps_app_log("GPS_APP_Init_All, read all settings from fs");

#ifdef GPS_CHECK_POWERONTIMES
	result = GPS_APP_ReadPoweronTimes((unsigned long *)&tmpSetting);
//	if (result != RESULT_OK) ASSERT(0);
//	gps_app_log("Power on times count: %d", tmpSetting);
	if (tmpSetting > GPS_POWERON_TIMES_MAX) ASSERT(0);
	tmpSetting += 1;
	result = GPS_APP_WritePoweronTimes((unsigned long *)&tmpSetting);
//	if (result != RESULT_OK) ASSERT(0);
#endif

	for (i = 1; i <= GPS_PRESAVED_NUMBER_TOTAL; i++)
	{
		result = GPS_APP_ReadNumberRecord(i, &gps_usr_numbers[i-1]);
		if (result != RESULT_OK)
		{
			gps_app_log("error when reading number %d", i);
		}
		else
		{
			gps_app_log("Number %d, %s, password: %s", i, 
						gps_usr_numbers[i-1].number, 
						gps_usr_numbers[i-1].passwd);
		}
	}
#ifndef GPS_SEPARATE_USER_PASSWOR
	// All user user the same password, copy the first user password as the public password
	strcpy(gps_usr_passwd, gps_usr_numbers[0].passwd);
	gps_app_log("User password: %s", gps_usr_passwd);
#endif
//	memcpy(gps_sos_numbers, gps_usr_numbers, sizeof(GPS_Saved_Number_t)*GPS_PRESAVED_NUMBER_TOTAL);

	memset(&gps_act_sc_num, 0, sizeof(gps_act_sc_num));
	result = GPS_APP_ReadNumberRecord(0, &gps_sc_number);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading service number");
	}
	else
	{
		gps_app_log("Service number: %s, password: %s",
					gps_sc_number.number, 
					gps_sc_number.passwd);

		strcpy((char *)gps_act_sc_num.Number, gps_sc_number.number);
		gps_act_sc_num.Type = GPS_APP_GetNumberType((char *)gps_act_sc_num.Number);
		gps_act_sc_num.Length = strlen((char *)gps_act_sc_num.Number);
	}

	memset(&gps_curr_timing_num, 0, sizeof(gps_curr_timing_num));
	result = GPS_APP_ReadTimingNumber((char *)gps_curr_timing_num.Number);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading timing number");
	}
	else
	{
		gps_app_log("Timing number: %s", (char *)gps_curr_timing_num.Number);
		gps_curr_timing_num.Type = GPS_APP_GetNumberType((char *)gps_curr_timing_num.Number);
		gps_curr_timing_num.Length = strlen((char *)gps_curr_timing_num.Number);
	}

	result = GPS_APP_ReadBSNumber(&gps_bs_num);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading request base station number");
	}
	else
	{
		gps_app_log("Request base station number: %d", gps_bs_num);
	}

	result = GPS_APP_ReadTimingValue(&gps_timing);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading timing value");
	}
	else
	{
		gps_app_log("Timing setting: %d", gps_timing);
	}

	result = GPS_APP_ReadHFreeSettings(&gps_call_handfree);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading handfree setting");
	}
	else
	{
		gps_app_log("Handfree setting: %d", gps_call_handfree);
	}

	result = GPS_APP_ReadMtcallProfile(&gps_mtcall_profile);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading mtcall profile setting");
	}
	else
	{
		gps_app_log("MT call profile setting: %d", gps_mtcall_profile);
		GpsAppMmiBrgSetProfile(gps_mtcall_profile);
	}

	result = GPS_APP_ReadGprsAPN(gps_gprs_apn, gps_gprs_apnuser, gps_gprs_apnpwd);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading gprs apn");
	}
	else
	{
		gps_app_log("GPRS APN: %s, %s, %s", 
					gps_gprs_apn, 
					gps_gprs_apnuser, 
					gps_gprs_apnpwd);
		gps_set_gprs_account_direct(GPS_GPRS_ACCOUNT_IDX,
					gps_gprs_apn, 
					gps_gprs_apnuser, 
					gps_gprs_apnpwd);
	}

	result = GPS_APP_ReadServerAddr(&gps_tcpip_server_addr);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading tcp/ip server address");
	}
	else
	{
		gps_app_log("TCP/IP Server: %d.%d.%d.%d:%d", 
					gps_tcpip_server_addr.addr[0],
					gps_tcpip_server_addr.addr[1],
					gps_tcpip_server_addr.addr[2],
					gps_tcpip_server_addr.addr[3],
					gps_tcpip_server_addr.port);
	}

	result = GPS_APP_ReadBearerAndMode(&tmpBM);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading bearer and sms mode settings");
	}
	else
	{
		gps_curr_bearer_type = (tmpBM >> 8) & 0xff;
		gps_curr_sms_mode = tmpBM & 0xff;
		gps_app_log("Bearer: %d, sms mode: %d", gps_curr_bearer_type, gps_curr_sms_mode);
	}

	memset(&gps_gprs_username, 0, GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN);
	result = GPS_APP_ReadGprsUsername(gps_gprs_username);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading gprs username");
	}
	else
	{
		gps_app_log("GPRS user name: %s", gps_gprs_username);
	}

	memset(&gps_gprs_userpwd, 0, GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN);
	result = GPS_APP_ReadGprsPassword(gps_gprs_userpwd);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading gprs password");
	}
	else
	{
		gps_app_log("GPRS user password: %s", gps_gprs_userpwd);
	}

	memset(gps_imei_str, 0, GPSLOCATE_IMEI_BUFFER_LEN);
	for (i = 0; i < 3; i++)
	{
		result = l4c_nw_get_imei_req(stack_int_get_active_module_id(), 
					(kal_uint8 *)gps_imei_str, 
					(kal_uint8 *)tmpStr);
		if (result == 1) break;
		// sleep 50 ms and try again
		kal_sleep_task(50);
	}
	gps_app_log("IMEI: %s", gps_imei_str);

	GPSPPBufferInit(GPS_APP_PPBufferFullCB);
	result = GPS_APP_ReadGprsUploadSettings(&tmpSetting);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading gprs upload settings");
	}
	else
	{
		gps_samp_interval = tmpSetting & 0xffff;
		gps_upload_cnt = (tmpSetting >> 16) & 0xffff;
		gps_app_log("Sampling interval: %d, upload count: %d", gps_samp_interval, gps_upload_cnt);
	}
	result = GPS_APP_ReadGprsUploadSettings2(&tmpSetting);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading gprs upload settings 2");
	}
	else
	{
		gps_samp_interval2 = tmpSetting & 0xffff;
		gps_upload_cnt2 = (tmpSetting >> 16) & 0xffff;
		gps_app_log("Sampling 2 interval: %d, upload count: %d", gps_samp_interval2, gps_upload_cnt2);
	}
	if (gps_curr_bearer_type == GPS_BEARER_GPRS)
	{
		GPS_UpldModeMonitorStart();
		if (GPS_APP_GPRSUpldMode())
		{
			if (gps_upload_cnt > 0 && gps_samp_interval > 0)
			{
				GPSPPBufferSetThreshold(gps_upload_cnt);
				gps_sampgprmc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
											GPS_APP_SampGprmcRepeatHandler,
											gps_samp_interval * KAL_TICKS_1_SEC,
											gps_samp_interval * KAL_TICKS_1_SEC,
											KAL_TRUE);
				gps_app_log("start GPS_APP_SampGprmcRepeatHandler");
			}
		}
		else
		{
			if (gps_upload_cnt2 > 0 && gps_samp_interval2 > 0)
			{
				GPSPPBufferSetThreshold(gps_upload_cnt2);
				gps_sampgprmc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
											GPS_APP_SampGprmcRepeatHandler,
											gps_samp_interval2 * KAL_TICKS_1_SEC,
											gps_samp_interval2 * KAL_TICKS_1_SEC,
											KAL_TRUE);
				gps_app_log("start GPS_APP_SampGprmcRepeatHandler");
			}
		}
	}

	GPS_APP_SetTimingLocateFlag(KAL_FALSE);
	if (gps_curr_bearer_type == GPS_BEARER_SMS && 	/* gps_curr_bearer_type must be init before here */
		gps_timing != 0)
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
		if (strlen((char *)gps_curr_timing_num.Number) == 0)
		{
			memset(&gps_curr_timing_num, 0, sizeof(gps_curr_timing_num));
			strcpy((char *)gps_curr_timing_num.Number, gps_usr_numbers[0].number);
			gps_curr_timing_num.Type = GPS_APP_GetNumberType((char *)gps_curr_timing_num.Number);
			gps_curr_timing_num.Length = strlen((char *)gps_curr_timing_num.Number);
		}
		gps_timingloc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_TimingLocateRepeatHandler,
									gps_timing_in_min * KAL_TICKS_1_MIN,
									gps_timing_in_min * KAL_TICKS_1_MIN,
									KAL_TRUE);
		gps_app_log("start GPS_APP_TimingLocateRepeatHandler");
	}

#ifdef GPS_BACKUPLOAD_DAEMON
	gps_backupdaemon_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_UploadBackupDataRepeatHandler,
									GPS_BACKUPDAEMON_TIMER,
									GPS_BACKUPDAEMON_TIMER,
									KAL_TRUE);
	gps_app_log("start GPS_APP_UploadBackupDataRepeatHandler");
#endif

#ifdef GPS_DEFENCE_FEATURE
	GPS_DefenceMonitorInit();

	result = GPS_APP_ReadDefenceSetting((unsigned char *)&gps_set_defence);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading defence on/off settings");
	}
	else
	{
		if (gps_set_defence)
		{
			GPS_APP_SetDefenceON();
		}
	}
#endif

#ifdef GPS_POSITION_MONITOR
	result = GPS_APP_ReadFixPosition(&gps_curr_position);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading monitoring position");
	}
	else
	{
		gps_app_log("fix position set to E%d%02d.%04d, N%d%02d.%04d, R%d.%d",
			gps_curr_position.longitude_d,
			gps_curr_position.longitude_c,
			gps_curr_position.longitude_cf,
			gps_curr_position.latitude_d,
			gps_curr_position.latitude_c,
			gps_curr_position.latitude_cf,
			gps_curr_position.radius, 
			gps_curr_position.radius_f);
	}
	
	result = GPS_APP_ReadPosMonitorOnff((unsigned char *)&gps_posmonitor_on);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading position monitor on/off settings");
	}
	else
	{
		gps_app_log("position monitor on/off setting: %d", gps_posmonitor_on);
		if (gps_posmonitor_on)
		{
			GPS_APP_PosMonitorStart();
		}
	}
#endif

	result = GPS_APP_ReadSosCallSetting((unsigned char *)&gps_sos_phonecall);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading sos call on/off settings");
	}
	else
	{
		gps_app_log("sos call on/off setting: %d", gps_sos_phonecall);
	}

#ifdef VIBRATION_SENSOR_SUPPORT
	GPSLocateVibSensorInit();
#endif

	result = GPS_APP_ReadGPSOnSettings((kal_uint8 *)&gps_module_en_setting);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading gps module on/off setting");
	}
	else
	{
		gps_app_log("GPS module on/off setting: %d", gps_module_en_setting);
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
			GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
			break;
		}
	}

#ifdef GPS_ITRACK_FORMAT
	result = GPS_APP_ReadFrontMsg(gps_itrack_sfmsg);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading itrack sfmsg");
	}
	else
	{
		gps_app_log("itrack sfmsg: %s", gps_itrack_sfmsg);
	}

	result = GPS_APP_ReadRearMsg(gps_itrack_srmsg);
	if (result != RESULT_OK)
	{
		gps_app_log("error when reading itrack srmsg");
	}
	else
	{
		gps_app_log("itrack srmsg: %s", gps_itrack_srmsg);
	}
#endif

	// temp solution
	// reset tone profile to disable camp on tone when power on and 
	// enable power on/off tone for meeting profile
	GpsAppMmiBrgResetToneProfile();
}

/**
 * Function: GPS_APP_Restore_All
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_Restore_All(void)
{
}

/**
 * Function: GPS_APP_EventFifoHandler
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_EventFifoHandler(GPSAppEvent_t *pEvent)
{
	gps_app_log("GPS_APP_EventFifoHandler");

	if (pEvent == NULL) return;

	switch(pEvent->OpCode)
	{
	case GPS_APP_EVENT_OP_GPRSUPLOAD:
		gps_app_log("OpCode: GPS_APP_EVENT_OP_GPRSUPLOAD");
		if (!GPS_APP_StartGprsUpload(*((kal_uint8 *)pEvent->LocalPara)))
		{
			gps_app_log("GPS_APP_EventFifoHandler, Pop req from fifo, OpCode: %d", 
						GPSAppEventFifoCurrItem()->OpCode);
			GPSAppEventFifoPop();
		}

		break;

	default:
		gps_app_log("OpCode: UNKNOWN");
		break;
	}
}

//-----------------------------------------------------------------------------
void GPS_APP_SendPackData2TCPIP(
		const GPS_GPRMC_Packed_Struct_t* buff, 
		kal_uint16 buff_len, 
		void *info)
{
    ilm_struct *send_ilm;
    GPSLocateSendThruGPRSReqStruct_t* ReqData;
	GPS_GPRS_UploadInfo_t *pInfo;

	ASSERT(buff != NULL);
	ASSERT(info != NULL);

	pInfo = (GPS_GPRS_UploadInfo_t *)info;
	ReqData = (GPSLocateSendThruGPRSReqStruct_t*)construct_local_para(
                sizeof(GPSLocateSendThruGPRSReqStruct_t),
                TD_UL);
    ReqData->ip_addr[0] = gps_tcpip_server_addr.addr[0];
    ReqData->ip_addr[1] = gps_tcpip_server_addr.addr[1];
    ReqData->ip_addr[2] = gps_tcpip_server_addr.addr[2];
    ReqData->ip_addr[3] = gps_tcpip_server_addr.addr[3];
    ReqData->port = gps_tcpip_server_addr.port;
    ReqData->data = (kal_uint8 *)buff;
    ReqData->data_len = buff_len;
	ReqData->cause_type = pInfo->cause_type;
	ReqData->src_type = pInfo->src_type;

    send_ilm = allocate_ilm(MOD_GPS_APP_TASK);
    send_ilm->src_mod_id = MOD_GPS_APP_TASK;
    send_ilm->dest_mod_id = MOD_GPS_TCPIP;
    send_ilm->msg_id = MSG_ID_GPSLOCATE_SEND_THRU_GPRS_REQ;
    send_ilm->local_para_ptr = (local_para_struct *)ReqData;
    msg_send_ext_queue(send_ilm);

    gps_app_log("GPS_APP_SendPackData2TCPIP, send data to %d.%d.%d.%d:%d", 
                gps_tcpip_server_addr.addr[0],
                gps_tcpip_server_addr.addr[1],
                gps_tcpip_server_addr.addr[2],
                gps_tcpip_server_addr.addr[3],
                gps_tcpip_server_addr.port);

}

//-----------------------------------------------------------------------------
void GPS_APP_PPBufferFullCB(
		const GPS_GPRMC_Packed_Struct_t* buff, 
		kal_uint16 buff_len, 
		void *info)
{
	GPSLocatePPBufFull4LoadingIndStruct_t* pReqData;

	pReqData = (GPSLocatePPBufFull4LoadingIndStruct_t*)construct_local_para(sizeof(GPSLocatePPBufFull4LoadingIndStruct_t), TD_UL);
	pReqData->buff = (void *)buff;
	pReqData->buff_len = buff_len;
	
	SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_PPBUFFULL4LOADING_IND, (void*)pReqData);
}

//-----------------------------------------------------------------------------
kal_bool GPS_APP_StartGprsUpload(GPS_GPRS_UploadCauseType_t cause_type)
{
	GPS_GPRMC_Packed_Struct_t *pBuff = NULL;
	kal_uint16 buff_len = 0;
	GPSPPBufferStatus_t status;
	GPS_GPRS_UploadInfo_t info;
#ifdef GPS_BACKUPLOAD_DAEMON
	static kal_uint8 tmpBuff[GPS_MAX_SND_BUFFER_SIZE]; 	// buffer size must be equal or greater than (sizeof(GPS_GPRMC_Packed_Struct_t)*GPS_PP_BUFFER_SIZE)
	GPSAppDataBackupError_t res = GPSAPP_DATABACKUP_ERROR_NONE;
#endif

	ASSERT(cause_type < GPS_GPRS_UPLDCAUSETYPE_TOTAL);

	gps_app_log("GPS_APP_StartGprsUpload, cause_type: %d", cause_type);

	switch (cause_type)
	{
	case GPS_GPRS_UPLDCAUSETYPE_AUTO:
	case GPS_GPRS_UPLDCAUSETYPE_AUTOLOW:
		status = GPSPPBufferGetLoadingBuffer(GPS_PP_BUFF_PURPOSE_LOADING, &pBuff, &buff_len);
		gps_app_log("Get PP Loading Buffer, status: %d, buff_len: %d", status, buff_len);
		info.cause_type = cause_type;
		info.src_type = GPS_GPRS_UPLDSRCTYPE_LDBUFF;

		break;

	case GPS_GPRS_UPLDCAUSETYPE_SMS:
	case GPS_GPRS_UPLDCAUSETYPE_CALL:
	case GPS_GPRS_UPLDCAUSETYPE_ANSWER:
	case GPS_GPRS_UPLDCAUSETYPE_SOS:
	case GPS_GPRS_UPLDCAUSETYPE_GEOOS:
	case GPS_GPRS_UPLDCAUSETYPE_GEORS:
	case GPS_GPRS_UPLDCAUSETYPE_DEF:
	case GPS_GPRS_UPLDCAUSETYPE_LP:
		status = GPSPPBufferGetStoringBuffer(&pBuff, &buff_len);
		gps_app_log("Get PP Storing Buffer, status: %d, buff_len: %d", status, buff_len);
		if (buff_len == 0)
		{
			gps_upldimm_cause_type = cause_type;
			GPS_APP_SampGprmcAndSendStart();

			return KAL_FALSE;
		}
		info.cause_type = cause_type;
		info.src_type = GPS_GPRS_UPLDSRCTYPE_STBUFF;

		break;

#ifdef GPS_BACKUPLOAD_DAEMON
	case GPS_GPRS_UPLDCAUSETYPE_BACKUP:
		// read missing data from backup folder
		buff_len = GPS_MAX_SND_BUFFER_SIZE;
		res = GPSAppDataBackupLoad(tmpBuff, &buff_len);
		gps_app_log("Get backup data, res: %d, buff_len: %d", res, buff_len);
		if (res != GPSAPP_DATABACKUP_ERROR_NONE || 
			buff_len % sizeof(GPS_GPRMC_Packed_Struct_t) != 0 ||
			buff_len == 0)
		{
			GPSAppDataBackupDelCurrFile();
			return KAL_FALSE;
		}
		pBuff = (GPS_GPRMC_Packed_Struct_t *)tmpBuff;
		buff_len /= sizeof(GPS_GPRMC_Packed_Struct_t);
		info.cause_type = cause_type;
		info.src_type = GPS_GPRS_UPLDSRCTYPE_BACKUP;

		break;
#endif

	default:
		ASSERT(0);
		break;
	}

	GPS_APP_SendPackData2TCPIP(pBuff, buff_len, (void *)&info);

	return KAL_TRUE;
}

#ifdef GPS_BACKUPLOAD_DAEMON
void GPS_APP_StoreBackupData(kal_uint8 type)
{
	GPS_GPRMC_Packed_Struct_t* pBuff = NULL;
	kal_uint16 buff_len = 0;
	GPSPPBufferStatus_t status;

	ASSERT(type < GPS_PP_BUFF_PURPOSE_Invalid);

	gps_app_log("GPS_APP_StoreBackupData, type: %d", type);

	switch (type)
	{
	case GPS_PP_BUFF_PURPOSE_REFERNCE:
		status = GPSPPBufferGetStoringBuffer(&pBuff, &buff_len);
		gps_app_log("Get PP Storing Buffer, status: %d, buff_len: %d", status, buff_len);
		break;

	case GPS_PP_BUFF_PURPOSE_LOADING:
		status = GPSPPBufferGetLoadingBuffer(GPS_PP_BUFF_PURPOSE_LOADING, &pBuff, &buff_len);
		gps_app_log("Get PP Loading Buffer, status: %d, buff_len: %d", status, buff_len);
		break;

	default:
		break;
	}

	if (pBuff != NULL && buff_len > 0)
	{
		GPSAppDataBackupStore(
						(kal_uint8 *)pBuff, 
						buff_len * sizeof(GPS_GPRMC_Packed_Struct_t)
						);
	}

	if (type == GPS_PP_BUFF_PURPOSE_LOADING)
	{
		GPSPPBufferFinishLoading();
	}
}
#endif

kal_uint8 GPS_APP_GetCallType()
{
	return gps_call_type;
}

void GPS_APP_SetCallType(kal_uint8 call_type)
{
	gps_call_type = call_type;
}

/**
 * Function: GPS_APP_GetTimingLocateFlag
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_bool GPS_APP_GetTimingLocateFlag(void)
{
	return gps_timing_locate_flag;
}

/**
 * Function: GPS_APP_SetTimingLocateFlag
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetTimingLocateFlag(kal_bool flag)
{
	gps_timing_locate_flag = flag;
}

/**
 * Function: GPS_APP_GetMultiSendFlag
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_bool GPS_APP_GetMultiSendFlag(void)
{
	return gps_multi_send_flag;
}

/**
 * Function: GPS_APP_SetMultiSendFlag
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetMultiSendFlag(kal_bool flag)
{
	gps_multi_send_flag = flag;
}

/**
 * Function: GPS_APP_GetVbatWarnFlag
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_bool GPS_APP_GetVbatWarnFlag(void)
{
	return gps_vbat_warn_flag;
}

/**
 * Function: GPS_APP_SetVbatWarnFlag
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetVbatWarnFlag(kal_bool flag)
{
	gps_vbat_warn_flag = flag;
}

/**
 * Function: GPS_APP_GetLocateState
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
GPS_Locate_State_t GPS_APP_GetLocateState(void)
{
	return gps_locate_state;
}

/**
 * Function: GPS_APP_SetLocateState
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
void GPS_APP_SetLocateState(GPS_Locate_State_t state)
{
	gps_locate_state = state;
}

/**
 * Function: GPS_APP_CheckGPSModuleOn
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_bool GPS_APP_CheckGPSModuleOn()
{
#if 1
	kal_uint16 value;

	value = DRV_Reg(GPIO_DOUT1);
	if (value & (1 << GPS_MODULE_ENALBE_GPIO))
	{
		return KAL_TRUE;
	}
	else
	{
		return KAL_FALSE;
	}
#else
	return GPIO_ReadIO(GPS_MODULE_ENALBE_GPIO);
#endif	
}

/**
 * Function: 
 *	GPS_APP_GPSModulePwrCtrl
 *
 * Usage: 
 *	Control GPS module power on/off
 *	Maybe, more than one device will share this power control. Therefore, using 
 *	power ID to indicate them.
 *
 * Parameters:
 *	On - KAL_TRUE: power on; KAL_FALSE: power off (maybe)
 *	Id - Power ID
 *
 * Returns:
 *
 */
void GPS_APP_GPSModulePwrCtrl(kal_bool On, GPSModulePwrID_t Id)
{
	static kal_uint8 PwrStatus = 0x00;

	if (On == KAL_TRUE)
	{
		PwrStatus |= 1 << Id;
		DRV_Reg(GPIO_DOUT1) |= (1 << GPS_MODULE_ENALBE_GPIO);
	}
	else
	{
		PwrStatus &= ~(1 << Id);
		if (PwrStatus == 0x00)
		{
			DRV_Reg(GPIO_DOUT1) &= ~(1 << GPS_MODULE_ENALBE_GPIO);
		}
	}
}

/**
 * Function: GPS_APP_SetUart2Sleep
 *
 * Usage: Set UART2 sleep or wakeup
 *
 * Parameters:
 * 	sleep = 0, wakeup uart2
 * 			1, sleep uart2
 *
 * Returns:
 *
 */
void GPS_APP_SetUart2Sleep(kal_bool sleep)
{
	if (sleep == KAL_FALSE)
	{
		L1SM_SleepDisable(gps_uart_sleep_handle);
	}
	else
	{
		L1SM_SleepEnable(gps_uart_sleep_handle);
	}
}

kal_bool GPS_APP_AllowShutdownSystem(void)
{
	return gps_allow_shutdown_system;
}

kal_bool GPS_TraceOn()
{
	return gps_trace_on;
}

void GPS_SetTrace(kal_bool on)
{
	gps_trace_on = on;
}

void GPS_ListAllSettings1(char *dest)
{
	char *pBuf = dest;
	kal_uint8 tmpMode = 0;

	sprintf(pBuf, "IMEI:%s\r\n", gps_imei_str);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "MOD:");
	pBuf = dest + strlen(dest);
	switch (gps_curr_bearer_type)
	{
	case GPS_BEARER_SMS:
		tmpMode = (gps_curr_sms_mode == GPS_SMS_MODE_SC) ? 1 : 0;
		sprintf(pBuf, "SMS %s", (gps_curr_sms_mode == GPS_SMS_MODE_SC) ? "SC" : "P2P");
		break;
	case GPS_BEARER_GPRS:
		tmpMode = 2;
		sprintf(pBuf, "GPRS");
		break;
	case GPS_BEARER_CSD:
	case GPS_BEARER_WIFI:
	default:
		tmpMode = 0xff;
		break;
	}
	pBuf = dest + strlen(dest);
	sprintf(pBuf, "\r\n");
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "GPS:");
	pBuf = dest + strlen(dest);
	switch (gps_module_en_setting)
	{
	case GPS_MODULE_POWERSTATE_ON:		sprintf(pBuf, "ON"); break;
	case GPS_MODULE_POWERSTATE_OFF:		sprintf(pBuf, "OFF"); break;
	case GPS_MODULE_POWERSTATE_AUTO:	sprintf(pBuf, "AUTO"); break;
	default:	sprintf(pBuf, "UNKOWN"); break;
	}
	pBuf = dest + strlen(dest);
	sprintf(pBuf, "\r\n");
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "HFR:%s\r\n", gps_call_handfree ? "ON" : "OFF");
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "MTPRF:%s\r\n", gps_mtcall_profile ? "SILENT" : "NORMAL" );
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "BS:%d\r\n", gps_bs_num);
	pBuf = dest + strlen(dest);

}


void GPS_ListAllSettings2(char *dest)
{
	char *pBuf = dest;
	int i;

	sprintf(pBuf, "SC:%s,%s\r\n", gps_sc_number.number, gps_sc_number.passwd);
	pBuf = dest + strlen(dest);

	for (i = 0; i < GPS_PRESAVED_NUMBER_TOTAL; i++)
	{
		sprintf(pBuf, "U%d:%s\r\n", 
				gps_usr_numbers[i].index, 
				gps_usr_numbers[i].number);
		pBuf = dest + strlen(dest);
	}
	sprintf(pBuf, "UP:%s\r\n", gps_usr_passwd);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "VOICE:%s\r\n", gps_sos_phonecall ? "ON" : "OFF");
	pBuf = dest + strlen(dest);
}

void GPS_ListAllSettings3(char *dest)
{
	char *pBuf = dest;

#ifdef GPS_DEFENCE_FEATURE
	sprintf(pBuf, "DEFENCE:%s\r\n", gps_set_defence ? "ON" : "OFF");
	pBuf = dest + strlen(dest);
#endif

#ifdef GPS_POSITION_MONITOR
	sprintf(pBuf, "GeoFence=%s", gps_posmonitor_on ? "ON" : "OFF");
	pBuf = dest + strlen(dest);
	sprintf(pBuf, ",E(W)%d%02d.%04d,N(S)%d%02d.%04d,R%d.%d",
			gps_curr_position.longitude_d,
			gps_curr_position.longitude_c,
			gps_curr_position.longitude_cf,
			gps_curr_position.latitude_d,
			gps_curr_position.latitude_c,
			gps_curr_position.latitude_cf,
			gps_curr_position.radius, 
			gps_curr_position.radius_f);
	pBuf = dest + strlen(dest);
	sprintf(pBuf, "\r\n");
	pBuf = dest + strlen(dest);
#endif

	sprintf(pBuf, "ST:%d\r\n", gps_timing);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "TN:%s\r\n", gps_curr_timing_num.Number);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "GU:%s,%s\r\n", gps_gprs_username, gps_gprs_userpwd);
	pBuf = dest + strlen(dest);
}

void GPS_ListAllSettings4(char *dest)
{
	char *pBuf = dest;

	sprintf(pBuf, "SRV:%d.%d.%d.%d,%d\r\n", 
			gps_tcpip_server_addr.addr[0], 
			gps_tcpip_server_addr.addr[1], 
			gps_tcpip_server_addr.addr[2], 
			gps_tcpip_server_addr.addr[3], 
			gps_tcpip_server_addr.port);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "APN:%s,%s,%s\r\n", gps_gprs_apn, gps_gprs_apnuser, gps_gprs_apnpwd);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "SAMP:%d,%d\r\n", gps_samp_interval, gps_upload_cnt);
	pBuf = dest + strlen(dest);

	sprintf(pBuf, "SAMP2:%d,%d\r\n", gps_samp_interval2, gps_upload_cnt2);
	pBuf = dest + strlen(dest);
}
void GPS_APP_RejectUnindicatedMTCall(void)
{
	ilm_struct* send_ilm;
	module_type src_mod;
	mmi_cc_ath_req_struct *athReq;

	athReq = (mmi_cc_ath_req_struct*)construct_local_para(sizeof(mmi_cc_ath_req_struct), TD_UL);
	athReq->op_code = L4C_DISCONNECT_MT;

	src_mod = stack_int_get_active_module_id();
	send_ilm = allocate_ilm(src_mod);
	send_ilm->src_mod_id = src_mod;
	send_ilm->dest_mod_id = MOD_L4C;
	send_ilm->msg_id = MSG_ID_MMI_CC_ATH_REQ;
	send_ilm->local_para_ptr = (local_para_struct *)athReq;
	msg_send_ext_queue(send_ilm);
}

void GPS_APP_CancelDialing(void)
{
	ilm_struct* send_ilm;
	module_type src_mod;
	mmi_cc_ath_req_struct *athReq;

	athReq = (mmi_cc_ath_req_struct*)construct_local_para(sizeof(mmi_cc_ath_req_struct), TD_UL);
	athReq->op_code = L4C_DISCONNECT_MO;

	src_mod = stack_int_get_active_module_id();
	send_ilm = allocate_ilm(src_mod);
	send_ilm->src_mod_id = src_mod;
	send_ilm->dest_mod_id = MOD_L4C;
	send_ilm->msg_id = MSG_ID_MMI_CC_ATH_REQ;
	send_ilm->local_para_ptr = (local_para_struct *)athReq;
	msg_send_ext_queue(send_ilm);
}

void GPS_APP_HangupActiveMOCall(void)
{
	ilm_struct* send_ilm;
	module_type src_mod;
	mmi_cc_chld_req_struct *Req;

	Req = (mmi_cc_chld_req_struct*)construct_local_para(sizeof(mmi_cc_chld_req_struct), TD_UL);
	Req->opcode = CSMCC_REL_ACTIVE;
	src_mod = stack_int_get_active_module_id();
	send_ilm = allocate_ilm(src_mod);
	send_ilm->src_mod_id = src_mod;
	send_ilm->dest_mod_id = MOD_L4C;
	send_ilm->msg_id = MSG_ID_MMI_CC_CHLD_REQ;
	send_ilm->local_para_ptr = (local_para_struct *)Req;
	msg_send_ext_queue(send_ilm);
}

/**
 * Function: 
 *	GPS_APP_SetOngoingCallSpch
 *
 * Usage: 
 *	Set ongoing call speech type
 *	It is recommended that using a pair of invoking this function when call accepting and
 *	releasing.
 *	For example: 
 *	//set monitor type when accepting MT call
 *	GPS_APP_SetOngoingCallSpch(GPSMTCALLTYPE_MONITOR);
 *	//set normal type when releasing MT call
 *	GPS_APP_SetOngoingCallSpch(GPSMTCALLTYPE_NORMAL);
 *
 * Parameters:
 *	type - speech type
 *
 * Returns:
 *	None
 */
void GPS_APP_SetOngoingCallSpch(GPSMtcallType_t type)
{
	static GPSMtcallType_t PreviousType = GPSMTCALLTYPE_NORMAL;
	
	if (type == GPSMTCALLTYPE_MONITOR)
	{
		//silence receiver
		//aud_tone_set_output_volume(0, 0);
		aud_speech_set_output_volume(0, 0);
		L1SP_SetSpeechVolumeLevel(0);
		//set mic gain with the max value
		aud_mic_set_volume(GPS_MTCALL_MONITOR_MIC_GAIN, GPS_MTCALL_MONITOR_MIC_GAIN);
		//save this type as "previous type"
		PreviousType = GPSMTCALLTYPE_MONITOR;
	}
	else if (type == GPSMTCALLTYPE_NORMAL)
	{
		kal_uint8 level, volume, aud_mode;

		//if the previous speech type is normal, it is no need to do the following setting again
		if (PreviousType != GPSMTCALLTYPE_NORMAL)
		{
			aud_mode = aud_get_audio_mode();
			//set receiver volume with normal value
			level = aud_get_volume_level(aud_mode, 4 /*AUD_VOLUME_SPH*/);
			volume = aud_get_volume_gain(aud_mode, 4 /*AUD_VOLUME_SPH*/, level);
			aud_speech_set_output_volume(volume, 0);
			L1SP_SetSpeechVolumeLevel(level);
			//set mic gain with normal value
			level = aud_get_volume_level(aud_mode, 2 /*AUD_VOLUME_MIC*/);
			volume = aud_get_volume_gain(aud_mode, 2 /*AUD_VOLUME_MIC*/, level);
			aud_mic_set_volume(volume, volume);
		}
		//save this type as "previous type"
		PreviousType = GPSMTCALLTYPE_NORMAL;
	}
}

kal_bool GPS_APP_OutofRange(GPS_PostionRange_t *fix_pos, GPS_GPRMC_Packed_Struct_t *curr_pos)
{
#if 1
	double d = 0.0;
	double r = 0.0;
	double longA, latA, longB, latB;
	const double EARTH_RADIUS = 6371.004; // km

	// D = arc cos((sin(latA)*sin(latB)) + (cos(latA)*cos(latB)*cos(longA-longB))) * EARTH_RADIUS
	// Point A longitude and latitude: longA, latA
	// Point B longitude and latitude: longB, latB
	// EARTH_RADIUS = 6371.004 km
	// D is the distance between A and B, unit km

	longA = GPS_APP_RmcDegree2Rad(fix_pos->longitude_d, fix_pos->longitude_c, fix_pos->longitude_cf);
	latA  = GPS_APP_RmcDegree2Rad(fix_pos->latitude_d, fix_pos->latitude_c, fix_pos->latitude_cf);
	longB = GPS_APP_RmcDegree2Rad(curr_pos->longitude_d, curr_pos->longitude_c, curr_pos->longitude_cf);
	latB  = GPS_APP_RmcDegree2Rad(curr_pos->latitude_d, curr_pos->latitude_c, curr_pos->latitude_cf);

	d = EARTH_RADIUS * acos(sin(latA)*sin(latB) + cos(latA)*cos(latB)*cos(longB-longA));
	r = (double)fix_pos->radius + (double)fix_pos->radius_f / (double)10;

	gps_app_log("A <--> B: %f", d);

	if (d > r)
	{
		return KAL_TRUE;
	}

	return KAL_FALSE;
#else
	if ((fix_pos->latitude_d != curr_pos->latitude_d) ||
		(fix_pos->latitude_c != curr_pos->latitude_c) ||
		(fix_pos->longitude_d != curr_pos->longitude_d) ||
		(fix_pos->longitude_c != curr_pos->longitude_c))
	{
		return KAL_TRUE;
	}

	return KAL_FALSE;
#endif
}

kal_uint8 GPS_APP_GPRSUpldMode(void)
{
	return GPIO_ReadIO(GPS_GPRS_UPLDMODE_GPIO);
}

