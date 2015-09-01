/******************************************************************************
 * File name: gps_app.h
 * Author: Robbie Cao
 * Date: 2007-11-12 10:08
 ******************************************************************************/

#ifndef _GPS_APP_DATA_H
#define _GPS_APP_DATA_H

#include "kal_release.h"
#include "stack_common.h"
#include "stack_msgs.h"	// for reference of MSG_ID_GPSLOCATE_CODE_BEGIN
#include "app_ltlcom.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_settings.h"
#include "gps_app_nema.h"
#include "gps_app_timer_mgr.h"

/*************************************************************************
 * Global variables
 *************************************************************************/
extern char gps_locate_info_buff[];
extern GPSLocateRegStateStruct_t gps_reg_state;

extern kal_uint8					gps_module_en_setting;
extern kal_bool						gps_allow_shutdown_system;
extern kal_bool						gps_lowbattery_warnning;
extern kal_bool						gps_app_is_charger_connected;
extern kal_bool						gps_mtcall_connecting;
extern kal_bool						gps_mtcall_answered;
#ifdef GPS_DEFENCE_FEATURE
extern kal_bool						gps_set_defence;
#endif
extern kal_uint8					gps_mtcall_profile;

extern kal_uint8					gps_mocall_state;
extern kal_uint8					gps_multi_send_index;
extern kal_uint8					gps_sos_call_index;

extern kal_uint8					gps_call_handfree;
extern kal_uint8					gps_timing;
extern kal_uint8					gps_timing_default;
extern kal_uint32					gps_timing_in_min;
extern kal_uint8					gps_bs_num;
extern kal_uint8					gps_warn_vbat;
extern GPS_Saved_Number_t			gps_sc_number;
extern GPS_Saved_Number_t			gps_usr_numbers[];
#ifndef GPS_SEPARATE_USER_PASSWOR
extern char							gps_usr_passwd[];
#endif
extern GPS_Saved_Number_t*			gps_sos_numbers;
extern GPS_Saved_Number_t*			gps_tgt_numgrp_sms;
extern GPS_Saved_Number_t*			gps_tgt_numgrp_mocall;

extern char							gps_gprs_username[];
extern char							gps_gprs_userpwd[];

extern char							gps_gprs_apn[];
extern char							gps_gprs_apnuser[];
extern char							gps_gprs_apnpwd[];

#ifdef GPS_ITRACK_FORMAT
extern char							gps_itrack_sfmsg[];
extern char							gps_itrack_srmsg[];
#endif

extern GPSLocatePhoneNumber_t		gps_act_remote_num;
extern GPSLocatePhoneNumber_t		gps_act_sc_num;
extern GPSLocatePhoneNumber_t		gps_curr_timing_num;

extern GPSLocatePhoneNumber_t  		gps_mocall_number;

extern GPS_Bearer_Type_t			gps_curr_bearer_type;
extern GPS_SMS_Mode_Type_t			gps_curr_sms_mode;
extern GPSLocateServerAddr_t		gps_tcpip_server_addr;

extern kal_uint16 					gps_samp_interval;
extern kal_uint16 					gps_upload_cnt;
extern kal_uint16 					gps_samp_interval2;
extern kal_uint16 					gps_upload_cnt2;

#ifdef GPS_POSITION_MONITOR
extern GPS_PostionRange_t			gps_curr_position;
extern kal_bool						gps_posmonitor_on;
#endif
#ifdef GPS_RATE_MONITOR
extern kal_uint16					gps_rate_limit;
#endif

extern GPSAppTimer_t 				gps_simpleloc_timer;
extern GPSAppTimer_t 				gps_timingloc_timer;
extern GPSAppTimer_t 				gps_mtcall_timer;
extern GPSAppTimer_t 				gps_mocall_timer2;
extern GPSAppTimer_t 				gps_mocall_timer;
#ifdef GPS_POWERON_PHONECALL
extern GPSAppTimer_t 				gps_1stcall_timer;
#endif
extern GPSAppTimer_t 				gps_getnwinfo_timer;
extern GPSAppTimer_t 				gps_sampgprmc_timer;
#ifdef GPS_BACKUPLOAD_DAEMON
extern GPSAppTimer_t 				gps_backupdaemon_timer;
#endif
#ifdef GPS_DEFENCE_FEATURE
extern GPSAppTimer_t 				gps_defence_timer;
#endif
#ifdef GPS_POSITION_MONITOR
extern GPSAppTimer_t 				gps_setposition_timer;
extern GPSAppTimer_t 				gps_posmonitor_timer;
#endif
#ifdef GPS_RATE_MONITOR
extern GPSAppTimer_t 				gps_ratemonitor_timer;
#endif
#ifdef GPS_MOD_SWAUTOPOWER
extern GPSAppTimer_t 				gps_modswpower_timer;
#endif
#ifdef GPS_MOTOPWRCUT_FEATURE
extern GPSAppTimer_t 				gps_motopowercut_timer;
extern GPSAppTimer_t 				gps_motopoweron_timer;
extern kal_uint8					gps_motopower_cut;
extern kal_uint8					gps_motopower_on;
#endif

extern char							gps_imei_str[];		// IMEI string length 15

extern kal_uint16					gps_upldimm_cause_type;
extern kal_uint8					gps_smsupld_cause_type;

extern kal_uint8					gps_sos_phonecall;

#endif /* _GPS_APP_DATA_H */

