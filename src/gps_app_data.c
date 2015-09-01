/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_data.c
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
 * 2008-3-10 16:42 - Initial create.
 ****************************************************************************/

/*************************************************************************
 * Include Statements
 *************************************************************************/
#include "kal_release.h"

#include "gps_app_configurable_const.h"
#include "gps_app_unconfigurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_nema.h"
#include "gps_app_settings.h"
#include "gps_app_timer_mgr.h"

/*************************************************************************
 * Macro defines
 *************************************************************************/


/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Local variables
 *************************************************************************/


/*************************************************************************
 * Global variables
 *************************************************************************/
kal_uint8					gps_module_en_setting	= 0;
kal_bool					gps_allow_shutdown_system = KAL_TRUE;
kal_bool					gps_lowbattery_warnning	= KAL_FALSE;
kal_bool					gps_app_is_charger_connected = KAL_FALSE;
kal_bool					gps_mtcall_connecting	= KAL_FALSE;
kal_bool					gps_mtcall_answered		= KAL_FALSE;
#ifdef GPS_DEFENCE_FEATURE
kal_bool					gps_set_defence			= KAL_FALSE;
#endif
kal_uint8					gps_mtcall_profile		= 0;

kal_uint8					gps_mocall_state		= GPSLOCATE_MOCALLSTATE_IDLE;
kal_uint8					gps_multi_send_index	= 0;
kal_uint8					gps_sos_call_index		= 0;

kal_uint8					gps_call_handfree		= 1;
kal_uint8					gps_timing 				= 0;
kal_uint8					gps_timing_default		= 0;
kal_uint32					gps_timing_in_min		= 0;
kal_uint8					gps_bs_num 				= 1;
kal_uint8					gps_warn_vbat			= 0;
GPS_Saved_Number_t			gps_sc_number;
GPS_Saved_Number_t			gps_usr_numbers[GPS_PRESAVED_NUMBER_TOTAL];
#ifndef GPS_SEPARATE_USER_PASSWOR
char						gps_usr_passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
#endif
GPS_Saved_Number_t*			gps_sos_numbers			= gps_usr_numbers;
GPS_Saved_Number_t*			gps_tgt_numgrp_sms		= gps_usr_numbers;
GPS_Saved_Number_t*			gps_tgt_numgrp_mocall	= gps_usr_numbers;

char						gps_gprs_username[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
char						gps_gprs_userpwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];

char						gps_gprs_apn[GPSLOCATE_PRESAVED_APN_BUFFER_LEN];
char						gps_gprs_apnuser[GPSLOCATE_PRESAVED_APNUSER_BUFFER_LEN];
char						gps_gprs_apnpwd[GPSLOCATE_PRESAVED_APNPWD_BUFFER_LEN];

#ifdef GPS_ITRACK_FORMAT
char						gps_itrack_sfmsg[GPSLOCATE_PRESAVED_ITRACKMSG_LEN];
char						gps_itrack_srmsg[GPSLOCATE_PRESAVED_ITRACKMSG_LEN];
#endif

GPSLocatePhoneNumber_t		gps_act_remote_num;
GPSLocatePhoneNumber_t		gps_act_sc_num;
GPSLocatePhoneNumber_t		gps_curr_timing_num;

GPSLocatePhoneNumber_t  	gps_mocall_number;

GPS_Bearer_Type_t			gps_curr_bearer_type 	= GPS_BEARER_SMS;
GPS_SMS_Mode_Type_t			gps_curr_sms_mode	 	= GPS_SMS_MODE_P2P;
GPSLocateServerAddr_t		gps_tcpip_server_addr;

kal_uint16 					gps_samp_interval 		= 0;
kal_uint16 					gps_upload_cnt 			= 0;
kal_uint16 					gps_samp_interval2 		= 0;
kal_uint16 					gps_upload_cnt2			= 0;

#ifdef GPS_POSITION_MONITOR
GPS_PostionRange_t			gps_curr_position;
kal_bool					gps_posmonitor_on		= KAL_FALSE;
#endif
#ifdef GPS_RATE_MONITOR
kal_uint16					gps_rate_limit			= 10;
#endif

GPSAppTimer_t 				gps_simpleloc_timer		= NULL;
GPSAppTimer_t 				gps_timingloc_timer		= NULL;
GPSAppTimer_t 				gps_mtcall_timer		= NULL;
GPSAppTimer_t 				gps_mocall_timer		= NULL;
GPSAppTimer_t 				gps_mocall_timer2		= NULL;
#ifdef GPS_POWERON_PHONECALL
GPSAppTimer_t 				gps_1stcall_timer		= NULL;
#endif
GPSAppTimer_t 				gps_getnwinfo_timer		= NULL;
GPSAppTimer_t 				gps_sampgprmc_timer		= NULL;
#ifdef GPS_BACKUPLOAD_DAEMON
GPSAppTimer_t 				gps_backupdaemon_timer	= NULL;
#endif
#ifdef GPS_DEFENCE_FEATURE
GPSAppTimer_t 				gps_defence_timer		= NULL;
#endif
#ifdef GPS_POSITION_MONITOR
GPSAppTimer_t 				gps_setposition_timer	= NULL;
GPSAppTimer_t 				gps_posmonitor_timer	= NULL;
#endif
#ifdef GPS_RATE_MONITOR
GPSAppTimer_t 				gps_ratemonitor_timer	= NULL;
#endif
#ifdef GPS_MOD_SWAUTOPOWER
GPSAppTimer_t 				gps_modswpower_timer	= NULL;
#endif
#ifdef GPS_MOTOPWRCUT_FEATURE
GPSAppTimer_t 				gps_motopowercut_timer	= NULL;
GPSAppTimer_t 				gps_motopoweron_timer	= NULL;
kal_uint8					gps_motopower_cut		= 0;
kal_uint8					gps_motopower_on		= 0;
#endif

char						gps_imei_str[GPSLOCATE_IMEI_BUFFER_LEN];

kal_uint16					gps_upldimm_cause_type	= 0;
kal_uint8					gps_smsupld_cause_type	= 0;

kal_uint8					gps_sos_phonecall		= 1;


