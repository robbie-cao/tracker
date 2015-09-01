/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_sms.c
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
 * 2008-3-10 14:02 - Initial create.
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
#include "parser.h"
#include "gps_app_sms.h"
#include "gps_app_util.h"
#include "gps_app.h"
#include "gps_app_nvram_mgr.h"
#include "gps_app_sq_wave.h"
#include "gps_app_timer_mgr.h"
#include "gps_app_pp_buff_mgr.h"
#include "gps_soc.h"
#include "gps_app_ind_mgr.h"
#include "gps_app_mmi_bridge.h"
#include "gps_app_data.h"
#ifdef GPS_DEFENCE_FEATURE
#include "gps_app_defence_moniter.h"
#endif
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

#ifdef GPS_FETION_SUPPORT
#define CMD_STR(str)					"~s*~a*~c:?"##str##"~s*"
#else
#define CMD_STR(str)					"~s*"##str##"~s*"
#endif


/*************************************************************************
 * Type defines
 *************************************************************************/


/**
 * Cmd in SMS content which is used to set/get/control GPS module
 */
typedef enum
{
	GPS_SMS_CMD_CHANGE_PRESAVED_NUMBER,
	GPS_SMS_CMD_CHANGE_PRESAVED_SOSNUM,
	GPS_SMS_CMD_CHANGE_SERVICE_NUMBER,
	GPS_SMS_CMD_ENABLE_GPS_MODULE,
	GPS_SMS_CMD_DISABLE_GPS_MODULE,
	GPS_SMS_CMD_CANCEL_SETTINGS,
	GPS_SMS_CMD_SET_TIMING_LOCATE,
	GPS_SMS_CMD_SET_BASE_STATSION,
	GPS_SMS_CMD_SIMPLE_LOCATE_REQ,
	GPS_SMS_CMD_SET_PASSWORDD,
#ifdef GPS_ITRACK_FORMAT
	GPS_SMS_CMD_ITRACK_LOCATE_REQ,
	GPS_SMS_CMD_ITRACK_SFMSG,
	GPS_SMS_CMD_ITRACK_SRMSG,
	GPS_SMS_CMD_ITRACK_GTAM,
#endif

	GPS_SMS_CMD_SET_BEARERANDMODE,

#ifdef GPS_GSG_SUPPORT
	GPS_SMS_CMD_SWITCH_SQUAREWAVE,
	GPS_SMS_CMD_ENABLE_GPS_SQWAVE,
#endif
	GPS_SMS_CMD_GET_CELLINFO,
	GPS_SMS_CMD_SET_MONITOR_PROF,

	GPS_SMS_CMD_SET_GPRS_USRNAME,
	GPS_SMS_CMD_SET_GPRS_PASSWORD,
	GPS_SMS_CMD_SET_GPRS_APN,
	GPS_SMS_CMD_SET_GPRS_ACCOUNT,
	GPS_SMS_CMD_SET_TCPIPSERVER,
	GPS_SMS_CMD_SET_GPRS_UPLOAD,
	GPS_SMS_CMD_SET_GPRS_UPLOAD2,
	GPS_SMS_CMD_GPRS_UPLOADIMM,

#ifdef GPS_POSITION_MONITOR
	GPS_SMS_CMD_SETPOSITION,
	GPS_SMS_CMD_SETPOSITION2,
	GPS_SMS_CMD_SETPOSITION3,
	GPS_SMS_CMD_POSMON2ONFF,
#endif
#ifdef GPS_RATE_MONITOR
	GPS_SMS_CMD_SETRATELIMIT,
#endif
	GPS_SMS_CMD_OPENDOORSQWAVE,
#ifdef GPS_DEFENCE_FEATURE
	GPS_SMS_CMD_SETDEFENCE,
#endif
#ifdef VIBRATION_SENSOR_SUPPORT
	GPS_SMS_CMD_VIBSENSEOR,
#endif
#ifdef GPS_MOD_SWAUTOPOWER
	GPS_SMS_CMD_SWAUTOPOWER,
#endif
	GPS_SMS_CMD_SOSPHONECALL,
#ifdef GPS_MOTOPWRCUT_FEATURE
	GPS_SMS_CMD_POWEROFFREQ,
	GPS_SMS_CMD_POWEROFFCONFIRM,
	GPS_SMS_CMD_POWERONREQ,
	GPS_SMS_CMD_POWERONCONFIRM,
	GPS_SMS_CMD_POWEROFFREQ2,
	GPS_SMS_CMD_POWERONREQ2,
#endif

	/* hide super sms cmd */
	GPS_SMS_CMD_SP_RESET_POWERTIMES,
	GPS_SMS_CMD_SP_GET_ALLNUMBER,
	GPS_SMS_CMD_SP_GET_ALLSETTINGS,
	GPS_SMS_CMD_SP_GET_ALLCONFIG,
	GPS_SMS_CMD_SP_ENABLE_GPS,
	GPS_SMS_CMD_SP_DISABLE_GPS,
	GPS_SMS_CMD_SP_SIMP_LOCATE,
	GPS_SMS_CMD_SP_SWITCHTRACE,
	GPS_SMS_CMD_SP_SET_VOLUME,
	GPS_SMS_CMD_SP_SET_ALERTTYPE,
	GPS_SMS_CMD_SP_SET_HANDFREE,
	GPS_SMS_CMD_SP_GET_SWVERSION,

	GPS_SMS_CMD_TOTAL
} GPS_Sms_Cmd_t;

/*************************************************************************
 * Local variables
 *************************************************************************/
static char gps_reply_sms [64];

const Keyword_t gps_sms_kwdList [] = 
{
	{	GPS_SMS_CMD_CHANGE_PRESAVED_NUMBER,	CMD_STR("~c*(~c+?~d{4,20})~c*(~w{4,4})~c*(~l/1-3/)~c*~c*")			},
	{	GPS_SMS_CMD_CHANGE_PRESAVED_SOSNUM,	CMD_STR("~c*SOS~c*(~c+?~d{3,20})~c*(~w{4,4})~c*(~l/1-3/)~c*~c*")	},
	{	GPS_SMS_CMD_CHANGE_SERVICE_NUMBER,	CMD_STR("#(~c+?~d{4,20})#(~w{4,4})#(~w{4,4})##")					},
	{	GPS_SMS_CMD_ENABLE_GPS_MODULE,		CMD_STR("222(~w{4,4})")												},
	{	GPS_SMS_CMD_DISABLE_GPS_MODULE,		CMD_STR("333(~w{4,4})")												},
	{	GPS_SMS_CMD_CANCEL_SETTINGS,		CMD_STR("zyzyz(~w{4,4})")											},
	{	GPS_SMS_CMD_SET_TIMING_LOCATE,		CMD_STR("4(~d{2,2})(~w{4,4})")										},
	{	GPS_SMS_CMD_SET_BASE_STATSION,		CMD_STR("55(~l/0-7/)(~w{4,4})")										},
	{	GPS_SMS_CMD_SIMPLE_LOCATE_REQ,		CMD_STR("666(~w{4,4})")												},
	{	GPS_SMS_CMD_SET_PASSWORDD,			CMD_STR("777(~w{4,4})(~w{4,4})")									},
#ifdef GPS_ITRACK_FORMAT
	{	GPS_SMS_CMD_ITRACK_LOCATE_REQ,		CMD_STR("(~w{4,4})~s*,~s*locate")									},
	{	GPS_SMS_CMD_ITRACK_SFMSG,			CMD_STR("(~w{4,4})~s*,~s*sfmsg~s*,")								},
	{	GPS_SMS_CMD_ITRACK_SRMSG,			CMD_STR("(~w{4,4})~s*,~s*srmsg~s*,")								},
	{	GPS_SMS_CMD_ITRACK_GTAM,			CMD_STR("~c*GTAM#(~w{4,4})#")													},
#endif

	{	GPS_SMS_CMD_SET_BEARERANDMODE,		CMD_STR("7(~l/01/)(~l/01/)(~w{4,4})")								},

#ifdef GPS_GSG_SUPPORT
	{	GPS_SMS_CMD_SWITCH_SQUAREWAVE,		CMD_STR("20(~l/01/)(~w{4,4})")										},
	{	GPS_SMS_CMD_ENABLE_GPS_SQWAVE,		CMD_STR("202(~w{4,4})")												},
#endif
	{	GPS_SMS_CMD_GET_CELLINFO,			CMD_STR("111(~w{4,4})")												},
	{	GPS_SMS_CMD_SET_MONITOR_PROF,		CMD_STR("00(~l/01/)(~w{4,4})")										},

	{	GPS_SMS_CMD_SET_GPRS_USRNAME,		CMD_STR("#801#(~w{4,4})#(~l/a-zA-Z0-9._-/{2,20})##")				},
	{	GPS_SMS_CMD_SET_GPRS_PASSWORD,		CMD_STR("#802#(~w{4,4})#(~w{4,4})#(~w{4,4})##")						},
	{	GPS_SMS_CMD_SET_GPRS_APN,			CMD_STR("#803#(~w{4,4})#(~l/a-zA-Z0-9._-/{2,36})##")				},
	{	GPS_SMS_CMD_SET_GPRS_ACCOUNT,		CMD_STR("#803#(~w{4,4})#(~l/a-zA-Z0-9._-/{2,36})#(~l/a-zA-Z0-9._-/{2,20})#(~l/a-zA-Z0-9._-/{2,20})##")		},
	{	GPS_SMS_CMD_SET_TCPIPSERVER,		CMD_STR("#804#(~w{4,4})#(~d{1,3})~c.(~d{1,3})~c.(~d{1,3})~c.(~d{1,3})#(~d+)##")	},
	{	GPS_SMS_CMD_SET_GPRS_UPLOAD,		CMD_STR("#805#(~w{4,4})#(~d+)#(~d{1,3})##")							},
	{	GPS_SMS_CMD_SET_GPRS_UPLOAD2,		CMD_STR("#809#(~w{4,4})#(~d+)#(~d{1,3})##")							},
	{	GPS_SMS_CMD_GPRS_UPLOADIMM, 		CMD_STR("#806#(~w{4,4})##")											},

#ifdef GPS_POSITION_MONITOR
	{	GPS_SMS_CMD_SETPOSITION,			CMD_STR("#901#(~w{4,4})#(~d{1,3})~c.(~d)##")						},
	{	GPS_SMS_CMD_SETPOSITION2,			CMD_STR("003(~w{4,4})(~l/EW/)(~d{1,5})~c.(~d{4,4})(~l/NS/)(~d{1,4})~c.(~d{4,4})R(~d{1,3})~c.(~d)")	},
	{	GPS_SMS_CMD_SETPOSITION3,			CMD_STR("004(~w{4,4})(~l/EW/)(~d{1,3})~c.(~d{5,5})(~l/NS/)(~d{1,2})~c.(~d{5,5})R(~d{1,3})~c.(~d)")	},
	{	GPS_SMS_CMD_POSMON2ONFF,			CMD_STR("21(~l/01/)(~w{4,4})")										},
#endif
#ifdef GPS_RATE_MONITOR
	{	GPS_SMS_CMD_SETRATELIMIT,			CMD_STR("#122#(~w{4,4})#(~d{1,3})##")								},
#endif
	{	GPS_SMS_CMD_OPENDOORSQWAVE,			CMD_STR("999(~w{4,4})")												},
#ifdef GPS_DEFENCE_FEATURE
	{	GPS_SMS_CMD_SETDEFENCE,				CMD_STR("01(~l/01/)(~w{4,4})")										},
#endif
#ifdef VIBRATION_SENSOR_SUPPORT
	{	GPS_SMS_CMD_VIBSENSEOR,				CMD_STR("100(~w{4,4})")												},
#endif
#ifdef GPS_MOD_SWAUTOPOWER
	{	GPS_SMS_CMD_SWAUTOPOWER,			CMD_STR("101(~w{4,4})")												},
#endif
	{	GPS_SMS_CMD_SOSPHONECALL,			CMD_STR("15(~l/01/)(~w{4,4})")										},
#ifdef GPS_MOTOPWRCUT_FEATURE
	{	GPS_SMS_CMD_POWEROFFREQ,			CMD_STR("900(~w{4,4})")												},
	{	GPS_SMS_CMD_POWEROFFCONFIRM,		CMD_STR("901(~w{4,4})")												},
	{	GPS_SMS_CMD_POWERONREQ,				CMD_STR("902(~w{4,4})")												},
	{	GPS_SMS_CMD_POWERONCONFIRM,			CMD_STR("903(~w{4,4})")												},
	{	GPS_SMS_CMD_POWEROFFREQ2,			CMD_STR("940(~w{4,4})")												},
	{	GPS_SMS_CMD_POWERONREQ2,			CMD_STR("941(~w{4,4})")												},
#endif

	{	GPS_SMS_CMD_SP_RESET_POWERTIMES,	CMD_STR("~c*RSTP#")													},
	{	GPS_SMS_CMD_SP_GET_ALLNUMBER,		CMD_STR("~c*GTAN#(~w{4,4})#")										},
	{	GPS_SMS_CMD_SP_GET_ALLSETTINGS,		CMD_STR("~c*GTAS#(~w{4,4})#")										},
	{	GPS_SMS_CMD_SP_GET_ALLCONFIG,		CMD_STR("~c*RCONF#")												},
	{	GPS_SMS_CMD_SP_ENABLE_GPS,			CMD_STR("~c*SENG#")													},
	{	GPS_SMS_CMD_SP_DISABLE_GPS,			CMD_STR("~c*SDSG#")													},
	{	GPS_SMS_CMD_SP_SIMP_LOCATE,			CMD_STR("~c*SLOC#")													},
	{	GPS_SMS_CMD_SP_SWITCHTRACE,			CMD_STR("~c*564(~l/01/)#")											},
	{	GPS_SMS_CMD_SP_SET_VOLUME,			CMD_STR("~c*VOL(~l/0-2/)(~l/0-6/)#")								},
	{	GPS_SMS_CMD_SP_SET_ALERTTYPE,		CMD_STR("~c*ALERT(~l/0-5/)#")										},
	{	GPS_SMS_CMD_SP_SET_HANDFREE,		CMD_STR("~c*HFREE(~l/01/)#")										},
	{	GPS_SMS_CMD_SP_GET_SWVERSION,		CMD_STR("~c*SWVER#")												},

	{   0xFF,								0																	} 
} ;


/*************************************************************************
 * Global variables
 *************************************************************************/


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
extern kal_char* release_verno(void);


/*
 * The following functions handle sms cmd in requirement spec
 */

/** Change presave number */
static Result_t GPS_SMS_ChangePresavedNumber(const GPS_Sms_Instruction_t *instruction);
/** Change presave number in sms service center mode*/
Result_t GPS_SMS_ChangePresavedSosNum(const GPS_Sms_Instruction_t *instruction);
/** Change service number in sms service center mode*/
static Result_t GPS_SMS_ChangeServiceNumber(const GPS_Sms_Instruction_t *instruction);
/** Enable GPS module */
static Result_t GPS_SMS_EnableGPSModule(const GPS_Sms_Instruction_t *instruction);
/** Disable GPS module */
static Result_t GPS_SMS_DisableGPSModule(const GPS_Sms_Instruction_t *instruction);
/** Cancel all settings for GPS module and restore to default */
static Result_t GPS_SMS_CancelGPSSettings(const GPS_Sms_Instruction_t *instruction);
/** Set timing locate interval */
static Result_t GPS_SMS_SetTimingLocate(const GPS_Sms_Instruction_t *instruction);
/** Set base station number to get */
static Result_t GPS_SMS_SetBaseStation(const GPS_Sms_Instruction_t *instruction);
/** Start a single locate requirement */
static Result_t GPS_SMS_SimpleLocate(const GPS_Sms_Instruction_t *instruction);
/** Set new user password */
static Result_t GPS_SMS_SetPassword(const GPS_Sms_Instruction_t *instruction);
#ifdef GPS_ITRACK_FORMAT
/** Set iTrack front message */
static Result_t GPS_SMS_SetFrontMsg(const GPS_Sms_Instruction_t *instruction);
/** Set iTrack rear message */
static Result_t GPS_SMS_SetRearMsg(const GPS_Sms_Instruction_t *instruction);
/** Get iTrack front/rear message */
static Result_t GPS_SMS_GetiTrackMsg(const GPS_Sms_Instruction_t *instruction);
#endif
/** Set bearer type and sms mode */
static Result_t GPS_SMS_SetBearerAndMode(const GPS_Sms_Instruction_t *instruction);
#ifdef GPS_GSG_SUPPORT
/** Set square wave on/off */
static Result_t GPS_SMS_SetSquareWave(const GPS_Sms_Instruction_t *instruction);
/** Enable GPS module and set square wave on */
static Result_t GPS_SMS_EnableGPSAndSQWave(const GPS_Sms_Instruction_t *instruction);
#endif
/** Request cell info */
static Result_t GPS_SMS_GetCellInfo(const GPS_Sms_Instruction_t *instruction);
/** Set monitoring profile */
static Result_t GPS_SMS_SetMonitorProf(const GPS_Sms_Instruction_t *instruction);
/** Set GPRS user name */
static Result_t GPS_SMS_SetGPRSUsername(const GPS_Sms_Instruction_t *instruction);
/** Set GPRS user password */
static Result_t GPS_SMS_SetGPRSPasswd(const GPS_Sms_Instruction_t *instruction);
/** Set GPRS APN */
static Result_t GPS_SMS_SetGPRSAPN(const GPS_Sms_Instruction_t *instruction);
/** Set GPRS account(include apn, user name & password) */
static Result_t GPS_SMS_SetGPRSAccount(const GPS_Sms_Instruction_t *instruction);
/** Set tcp/ip server ip and port */
static Result_t GPS_SMS_SetTCPIPServer(const GPS_Sms_Instruction_t *instruction);
/** Set upload through GPRS settings */
static Result_t GPS_SMS_SetGPRSUpload(const GPS_Sms_Instruction_t *instruction);
/** Set upload through GPRS settings */
static Result_t GPS_SMS_SetGPRSUpload2(const GPS_Sms_Instruction_t *instruction);
/** Start upload gprmc frame in ping-pong buffer to tcp/ip server immediately requirement */
static Result_t GPS_SMS_GPRSUploadImm(const GPS_Sms_Instruction_t *instruction);
#ifdef GPS_POSITION_MONITOR
/** Start position monitoring or stop position monitoring */
static Result_t GPS_SMS_SetPosition(const GPS_Sms_Instruction_t *instruction);
/** Set position for monitoring */
static Result_t GPS_SMS_SetPosition2(const GPS_Sms_Instruction_t *instruction);
/** Set position monitor on/off */
static Result_t GPS_SMS_SetPosMonOnff(const GPS_Sms_Instruction_t *instruction);
#endif
#ifdef GPS_RATE_MONITOR
/** Start rate monitoring or stop rate monitoring */
static Result_t GPS_SMS_SetRateLimit(const GPS_Sms_Instruction_t *instruction);
#endif
/** Start to generate a squre wave */
static Result_t GPS_SMS_SetOpenDoorWave(const GPS_Sms_Instruction_t *instruction);
#ifdef GPS_DEFENCE_FEATURE
/** Set defence on/off */
static Result_t GPS_SMS_SetDefence(const GPS_Sms_Instruction_t *instruction);
#endif
#ifdef VIBRATION_SENSOR_SUPPORT
/** Set vibrator sensor on/off */
static Result_t GPS_SMS_SetVibSensor(const GPS_Sms_Instruction_t *instruction);
#endif
#ifdef GPS_MOD_SWAUTOPOWER
/** Set gps module auto on/off by sw */
static Result_t GPS_SMS_SetSWAutoPower(const GPS_Sms_Instruction_t *instruction);
#endif
/** Set sos phone call on/off */
static Result_t GPS_SMS_SetSosCall(const GPS_Sms_Instruction_t *instruction);
#ifdef GPS_MOTOPWRCUT_FEATURE
/** Power off request */
static Result_t GPS_SMS_PowerOffReq(const GPS_Sms_Instruction_t *instruction);
/** Power off confirm */
static Result_t GPS_SMS_PowerOffConfirm(const GPS_Sms_Instruction_t *instruction);
/** Power on request */
static Result_t GPS_SMS_PowerOnReq(const GPS_Sms_Instruction_t *instruction);
/** Power on confirm */
static Result_t GPS_SMS_PowerOnConfirm(const GPS_Sms_Instruction_t *instruction);
/** Power off */
static Result_t GPS_SMS_PowerOffReq2(const GPS_Sms_Instruction_t *instruction);
/** Power on */
static Result_t GPS_SMS_PowerOnReq2(const GPS_Sms_Instruction_t *instruction);
#endif

/*
 * The following functions handle user defined sms cmd(super cmd for test)
 */

/** Reset power times count */
static Result_t GPS_SMS_ResetPowerTimes(const GPS_Sms_Instruction_t *instruction);
/** Get all number and password (both users and service center) */
static Result_t GPS_SMS_GetAllNumber(const GPS_Sms_Instruction_t *instruction);
/** Get all saved settings */
static Result_t GPS_SMS_GetAllSettings(const GPS_Sms_Instruction_t *instruction);
/** Get all saved config(number, passwd, settings, etc) */
static Result_t GPS_SMS_GetAllConfig(const GPS_Sms_Instruction_t *instruction);
/** Enable GPS module */
static Result_t GPS_SMS_EnableGPS(const GPS_Sms_Instruction_t *instruction);
/** Disable GPS module */
static Result_t GPS_SMS_DisableGPS(const GPS_Sms_Instruction_t *instruction);
/** Start a simple locate */
static Result_t GPS_SMS_SimpLocate(const GPS_Sms_Instruction_t *instruction);
/** Turn on/off log */
static Result_t GPS_SMS_SwitchTrace(const GPS_Sms_Instruction_t *instruction);
/** Set volume */
static Result_t GPS_SMS_SetVolume(const GPS_Sms_Instruction_t *instruction);
/** Set MT call alert type */
static Result_t GPS_SMS_SetMTAlertType(const GPS_Sms_Instruction_t *instruction);
/** Turn on/off handfree when accept incoming call */
static Result_t GPS_SMS_SetHandfree(const GPS_Sms_Instruction_t *instruction);
/** Get sw version */
static Result_t GPS_SMS_GetSwVersion(const GPS_Sms_Instruction_t *instruction);


GPS_SMS_Cmd_Handler_Func_t gps_sms_cmd_hdlrList[] =
{
	GPS_SMS_ChangePresavedNumber,
	GPS_SMS_ChangePresavedSosNum,
	GPS_SMS_ChangeServiceNumber,
	GPS_SMS_EnableGPSModule,
	GPS_SMS_DisableGPSModule,
	GPS_SMS_CancelGPSSettings,
	GPS_SMS_SetTimingLocate,
	GPS_SMS_SetBaseStation,
	GPS_SMS_SimpleLocate,
	GPS_SMS_SetPassword,
#ifdef GPS_ITRACK_FORMAT
	GPS_SMS_SimpleLocate,
	GPS_SMS_SetFrontMsg,
	GPS_SMS_SetRearMsg,
	GPS_SMS_GetiTrackMsg,
#endif

	GPS_SMS_SetBearerAndMode,

#ifdef GPS_GSG_SUPPORT
	GPS_SMS_SetSquareWave,
	GPS_SMS_EnableGPSAndSQWave,
#endif
	GPS_SMS_GetCellInfo,
	GPS_SMS_SetMonitorProf,

	GPS_SMS_SetGPRSUsername,
	GPS_SMS_SetGPRSPasswd,
	GPS_SMS_SetGPRSAPN,
	GPS_SMS_SetGPRSAccount,
	GPS_SMS_SetTCPIPServer,
	GPS_SMS_SetGPRSUpload,
	GPS_SMS_SetGPRSUpload2,
	GPS_SMS_GPRSUploadImm,

#ifdef GPS_POSITION_MONITOR
	GPS_SMS_SetPosition,
	GPS_SMS_SetPosition2,
	GPS_SMS_SetPosition2,
	GPS_SMS_SetPosMonOnff,
#endif
#ifdef GPS_RATE_MONITOR
	GPS_SMS_SetRateLimit,
#endif
	GPS_SMS_SetOpenDoorWave,
#ifdef GPS_DEFENCE_FEATURE
	GPS_SMS_SetDefence,
#endif
#ifdef VIBRATION_SENSOR_SUPPORT
	GPS_SMS_SetVibSensor,
#endif
#ifdef GPS_MOD_SWAUTOPOWER
	GPS_SMS_SetSWAutoPower,
#endif
	GPS_SMS_SetSosCall,
#ifdef GPS_MOTOPWRCUT_FEATURE
	GPS_SMS_PowerOffReq,
	GPS_SMS_PowerOffConfirm,
	GPS_SMS_PowerOnReq,
	GPS_SMS_PowerOnConfirm,
	GPS_SMS_PowerOffReq2,
	GPS_SMS_PowerOnReq2,
#endif

	GPS_SMS_ResetPowerTimes,
	GPS_SMS_GetAllNumber,
	GPS_SMS_GetAllSettings,
	GPS_SMS_GetAllConfig,
	GPS_SMS_EnableGPS,
	GPS_SMS_DisableGPS,
	GPS_SMS_SimpLocate,
	GPS_SMS_SwitchTrace,
	GPS_SMS_SetVolume,
	GPS_SMS_SetMTAlertType,
	GPS_SMS_SetHandfree,
	GPS_SMS_GetSwVersion,

	NULL
};


/**
 * Function: GPS_SMS_ParseSMS
 *
 * Usage: Parse SMS content and get GPS SMS instruction cmd, data
 *
 * Parameters:
 *  sms			- pointer to sms content
 *  instruction	- gps sms instruction get from sms content
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_ParseSMS(const char *sms, GPS_Sms_Instruction_t *instruction)
{
	ParserMatch_t	match ;				//	pattern-matching results
	ParserToken_t	token [9] ;			//	pattern-matching tokens
	char			tknBuf [64] ;
	kal_uint32		tmpVal ;
	const Keyword_t* kwd;
	kal_bool found = KAL_FALSE ;
	Result_t result = RESULT_ERROR;

	if (sms == NULL || instruction == NULL)
		return RESULT_ERROR;
	
	gps_app_log("GPS_SMS_ParseSMS, sms: %s", sms);
	for ( kwd = gps_sms_kwdList; kwd->pattern != NULL; kwd++) {
		memset(token, 0, sizeof(token));
		memset(tknBuf, 0, sizeof(tknBuf));
		ParserInitMatch( sms, &match ) ;

		result = ParserMatchPattern( kwd->pattern, sms, &match, token );

		if ( result == RESULT_OK ) {
			found = KAL_TRUE ;
			instruction->cmd = kwd->val;

			switch ( kwd->val ) {
			case GPS_SMS_CMD_CHANGE_PRESAVED_NUMBER:
				gps_app_log("cmdID: GPS_SMS_CMD_CHANGE_PRESAVED_NUMBER");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_number, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->index = tmpVal;

				break;

			case GPS_SMS_CMD_CHANGE_PRESAVED_SOSNUM:
				gps_app_log("cmdID: GPS_SMS_CMD_CHANGE_PRESAVED_SOSNUM");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_number, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->index = tmpVal;

				break;

			case GPS_SMS_CMD_CHANGE_SERVICE_NUMBER:
				gps_app_log("cmdID: GPS_SMS_CMD_CHANGE_SERVICE_NUMBER");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_number, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				ParserTknToStr ( &token[2], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_passwd, tknBuf, token[2].tknLen);

				break;

			case GPS_SMS_CMD_ENABLE_GPS_MODULE:
				gps_app_log("cmdID: GPS_SMS_CMD_ENABLE_GPS_MODULE");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_DISABLE_GPS_MODULE:
				gps_app_log("cmdID: GPS_SMS_CMD_DISABLE_GPS_MODULE");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_CANCEL_SETTINGS:
				gps_app_log("cmdID: GPS_SMS_CMD_CANCEL_SETTINGS");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_SET_TIMING_LOCATE:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_TIMING_LOCATE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->timing = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;

			case GPS_SMS_CMD_SET_BASE_STATSION:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_BASE_STATSION");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->bs_num = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;

			case GPS_SMS_CMD_SIMPLE_LOCATE_REQ:
				gps_app_log("cmdID: GPS_SMS_CMD_SIMPLE_LOCATE_REQ");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_SET_PASSWORDD:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_PASSWORDD");

				strcpy(instruction->new_number, (char *)instruction->remote_number.Number);

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_passwd, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;

#ifdef GPS_ITRACK_FORMAT
			case GPS_SMS_CMD_ITRACK_LOCATE_REQ:
				gps_app_log("cmdID: GPS_SMS_CMD_ITRACK_LOCATE_REQ");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_ITRACK_SFMSG:
				gps_app_log("cmdID: GPS_SMS_CMD_ITRACK_SFMSG");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);
				gps_app_log("sfmsg: %s", match.pLine);
				memset(instruction->apn, 0, sizeof(instruction->apn));
				strncpy(instruction->apn, match.pLine, GPSLOCATE_PRESAVED_ITRACKMSG_LEN-1);

				break;

			case GPS_SMS_CMD_ITRACK_SRMSG:
				gps_app_log("cmdID: GPS_SMS_CMD_ITRACK_SRMSG");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);
				gps_app_log("srmsg: %s", match.pLine);
				memset(instruction->apn, 0, sizeof(instruction->apn));
				strncpy(instruction->apn, match.pLine, GPSLOCATE_PRESAVED_ITRACKMSG_LEN-1);

				break;

			case GPS_SMS_CMD_ITRACK_GTAM:
				gps_app_log("cmdID: GPS_SMS_CMD_ITRACK_GTAM");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;
#endif

			case GPS_SMS_CMD_SET_BEARERANDMODE:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_BEARERANDMODE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->bearer = tmpVal;

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->sms_mode = tmpVal;

				ParserTknToStr ( &token[2], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[2].tknLen);

				break;

			case GPS_SMS_CMD_SET_TCPIPSERVER:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_TCPIPSERVER");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->server_ip.addr[0] = tmpVal;

				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->server_ip.addr[1] = tmpVal;

				ParserTknToUInt ( &token[3], &tmpVal ) ;
				instruction->server_ip.addr[2] = tmpVal;

				ParserTknToUInt ( &token[4], &tmpVal ) ;
				instruction->server_ip.addr[3] = tmpVal;

				ParserTknToUInt ( &token[5], &tmpVal ) ;
				instruction->server_ip.port = tmpVal;

				break;

#ifdef GPS_GSG_SUPPORT
			case GPS_SMS_CMD_SWITCH_SQUAREWAVE:
				gps_app_log("cmdID: GPS_SMS_CMD_SWITCH_SQUAREWAVE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->sq_wave = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;

			case GPS_SMS_CMD_ENABLE_GPS_SQWAVE:
				gps_app_log("cmdID: GPS_SMS_CMD_ENABLE_GPS_SQWAVE");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;
#endif

			case GPS_SMS_CMD_GET_CELLINFO:
				gps_app_log("cmdID: GPS_SMS_CMD_GET_CELLINFO");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_SET_MONITOR_PROF:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_MONITOR_PROF");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;

			case GPS_SMS_CMD_SET_GPRS_USRNAME:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_GPRS_USRNAME");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_number, tknBuf, token[1].tknLen);

				break;

			case GPS_SMS_CMD_SET_GPRS_PASSWORD:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_GPRS_PASSWORD");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_passwd, tknBuf, token[1].tknLen);

				ParserTknToStr ( &token[2], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->srv_passwd, tknBuf, token[2].tknLen);

				break;

			case GPS_SMS_CMD_SET_GPRS_APN:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_GPRS_APN");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->apn, tknBuf, token[1].tknLen);

				break;

			case GPS_SMS_CMD_SET_GPRS_ACCOUNT:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_GPRS_ACCOUNT");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->apn, tknBuf, token[1].tknLen);

				ParserTknToStr ( &token[2], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->new_number, tknBuf, token[2].tknLen);

				ParserTknToStr ( &token[3], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->gpwd, tknBuf, token[3].tknLen);

				break;

			case GPS_SMS_CMD_SET_GPRS_UPLOAD:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_GPRS_UPLOAD");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->interval = tmpVal;

				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->upload_cnt = tmpVal;

				break;

			case GPS_SMS_CMD_SET_GPRS_UPLOAD2:
				gps_app_log("cmdID: GPS_SMS_CMD_SET_GPRS_UPLOAD2");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->interval = tmpVal;

				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->upload_cnt = tmpVal;

				break;

			case GPS_SMS_CMD_GPRS_UPLOADIMM:
				gps_app_log("cmdID: GPS_SMS_CMD_GPRS_UPLOADIMM");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

#ifdef GPS_POSITION_MONITOR
			case GPS_SMS_CMD_SETPOSITION:
				gps_app_log("cmdID: GPS_SMS_CMD_SETPOSITION");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->pos_range.radius = tmpVal;
				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->pos_range.radius_f = tmpVal;

				break;

			case GPS_SMS_CMD_SETPOSITION2:
				gps_app_log("cmdID: GPS_SMS_CMD_SETPOSITION2");

				// password
				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				// longitude
				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				if (tknBuf[0] == 'E')
					instruction->pos_range.longitude_ew = 0;
				else
					instruction->pos_range.longitude_ew = 1;
				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->pos_range.longitude_d = tmpVal / 100;
				instruction->pos_range.longitude_c = tmpVal % 100;
				ParserTknToUInt ( &token[3], &tmpVal ) ;
				instruction->pos_range.longitude_cf = tmpVal;

				// latitude
				ParserTknToStr ( &token[4], tknBuf, sizeof(tknBuf) ) ;
				if (tknBuf[0] == 'N')
					instruction->pos_range.latitude_ns = 0;
				else
					instruction->pos_range.latitude_ns = 1;
				ParserTknToUInt ( &token[5], &tmpVal ) ;
				instruction->pos_range.latitude_d = tmpVal / 100;
				instruction->pos_range.latitude_c = tmpVal % 100;
				ParserTknToUInt ( &token[6], &tmpVal ) ;
				instruction->pos_range.latitude_cf = tmpVal;

				// radius
				ParserTknToUInt ( &token[7], &tmpVal ) ;
				instruction->pos_range.radius = tmpVal;
				ParserTknToUInt ( &token[8], &tmpVal ) ;
				instruction->pos_range.radius_f = tmpVal;

				break;

			case GPS_SMS_CMD_SETPOSITION3:
				gps_app_log("cmdID: GPS_SMS_CMD_SETPOSITION3");

				// password
				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				// longitude
				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				if (tknBuf[0] == 'E')
					instruction->pos_range.longitude_ew = 0;
				else
					instruction->pos_range.longitude_ew = 1;
				ParserTknToUInt ( &token[2], &tmpVal ) ;
				instruction->pos_range.longitude_d = tmpVal;
				ParserTknToUInt ( &token[3], &tmpVal ) ;
				instruction->pos_range.longitude_c = tmpVal * 60 / 100000;
				instruction->pos_range.longitude_cf = (tmpVal * 60 / 10) % 10000;

				// latitude
				ParserTknToStr ( &token[4], tknBuf, sizeof(tknBuf) ) ;
				if (tknBuf[0] == 'N')
					instruction->pos_range.latitude_ns = 0;
				else
					instruction->pos_range.latitude_ns = 1;
				ParserTknToUInt ( &token[5], &tmpVal ) ;
				instruction->pos_range.latitude_d = tmpVal;
				ParserTknToUInt ( &token[6], &tmpVal ) ;
				instruction->pos_range.latitude_c = tmpVal * 60 / 100000;
				instruction->pos_range.latitude_cf = (tmpVal * 60 / 10) % 10000;

				// radius
				ParserTknToUInt ( &token[7], &tmpVal ) ;
				instruction->pos_range.radius = tmpVal;
				ParserTknToUInt ( &token[8], &tmpVal ) ;
				instruction->pos_range.radius_f = tmpVal;

				break;

			case GPS_SMS_CMD_POSMON2ONFF:
				gps_app_log("cmdID: GPS_SMS_CMD_POSMON2ONFF");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;
#endif

#ifdef GPS_RATE_MONITOR
			case GPS_SMS_CMD_SETRATELIMIT:
				gps_app_log("cmdID: GPS_SMS_CMD_SETRATELIMIT");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->setting = tmpVal;

				break;
#endif

			case GPS_SMS_CMD_OPENDOORSQWAVE:
				gps_app_log("cmdID: GPS_SMS_CMD_OPENDOORSQWAVE");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

#ifdef GPS_DEFENCE_FEATURE
			case GPS_SMS_CMD_SETDEFENCE:
				gps_app_log("cmdID: GPS_SMS_CMD_SETDEFENCE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;
#endif

#ifdef VIBRATION_SENSOR_SUPPORT
			case GPS_SMS_CMD_VIBSENSEOR:
				gps_app_log("cmdID: GPS_SMS_CMD_VIBSENSEOR");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;
#endif

#ifdef GPS_MOD_SWAUTOPOWER
			case GPS_SMS_CMD_SWAUTOPOWER:
				gps_app_log("cmdID: GPS_SMS_CMD_SWAUTOPOWER");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;
#endif

			case GPS_SMS_CMD_SOSPHONECALL:
				gps_app_log("cmdID: GPS_SMS_CMD_SOSPHONECALL");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				ParserTknToStr ( &token[1], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[1].tknLen);

				break;

#ifdef GPS_MOTOPWRCUT_FEATURE
			case GPS_SMS_CMD_POWEROFFREQ:
				gps_app_log("cmdID: GPS_SMS_CMD_POWEROFFREQ");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_POWEROFFCONFIRM:
				gps_app_log("cmdID: GPS_SMS_CMD_POWEROFFCONFIRM");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_POWERONREQ:
				gps_app_log("cmdID: GPS_SMS_CMD_POWERONREQ");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_POWERONCONFIRM:
				gps_app_log("cmdID: GPS_SMS_CMD_POWERONCONFIRM");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_POWEROFFREQ2:
				gps_app_log("cmdID: GPS_SMS_CMD_POWEROFFREQ2");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_POWERONREQ2:
				gps_app_log("cmdID: GPS_SMS_CMD_POWERONREQ2");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;
#endif

			case GPS_SMS_CMD_SP_RESET_POWERTIMES:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_RESET_POWERTIMES");
				break;

			case GPS_SMS_CMD_SP_GET_ALLNUMBER:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_GET_ALLNUMBER");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_SP_GET_ALLSETTINGS:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_GET_ALLSETTINGS");

				ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
				strncpy(instruction->passwd, tknBuf, token[0].tknLen);

				break;

			case GPS_SMS_CMD_SP_ENABLE_GPS:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_ENABLE_GPS");
				break;

			case GPS_SMS_CMD_SP_DISABLE_GPS:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_DISABLE_GPS");
				break;

			case GPS_SMS_CMD_SP_SIMP_LOCATE:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_SIMP_LOCATE");
				break;

			case GPS_SMS_CMD_SP_SWITCHTRACE:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_SWITCHTRACE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				break;

			case GPS_SMS_CMD_SP_SET_VOLUME:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_SET_VOLUME");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;
				instruction->setting <<= 16;

				ParserTknToUInt ( &token[1], &tmpVal ) ;
				instruction->setting |= tmpVal;

				break;

			case GPS_SMS_CMD_SP_SET_ALERTTYPE:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_SET_ALERTTYPE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				break;

			case GPS_SMS_CMD_SP_SET_HANDFREE:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_SET_HANDFREE");

				ParserTknToUInt ( &token[0], &tmpVal ) ;
				instruction->setting = tmpVal;

				break;

			case GPS_SMS_CMD_SP_GET_SWVERSION:
				gps_app_log("cmdID: GPS_SMS_CMD_SP_GET_SWVERSION");
				break;

			default:
				gps_app_log("cmdID: none");
				break;
			}
			break;
		}
	}

	return result;
}

/**
 * Function: GPS_SMS_ConstructAndSendSms
 *
 * Usage: Construct a SMS with msg_buf and send it to the given number
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_ConstructAndSendSms(const GPSLocatePhoneNumber_t *p_number, 
									 const char *msg_buf,
									 unsigned short msg_len)
{
	if (msg_len > GPSLOCATE_SMS_MSGDATA_BUFFER_LEN)
	{
		return RESULT_ERROR;
	}
	gps_locate_send_sms_direct(p_number, msg_buf, msg_len);
	return RESULT_OK;
}

/**
 * Function: GPS_SMS_DeleteAllSms
 *
 * Usage: Delete all SMS in storage
 *
 * Parameters:
 *  None
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_DeleteAllSms(void)
{
	gps_locate_delete_sms_direct();
	return RESULT_OK;
}

/**
 * Function: GPS_APP_CheckPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_CheckPassword(const GPSLocatePhoneNumber_t *pNumber, const char *passwd)
{
	Result_t result = RESULT_ERROR;

	result = GPS_APP_CheckNumberPassword(pNumber, passwd);

	if (result != RESULT_OK)
	{
		gps_app_log("GPS_APP_CheckPassword, password not match");
		sprintf(gps_reply_sms, "PASSWORD ERROR");
		GPS_SMS_ConstructAndSendSms(pNumber,
									gps_reply_sms, 
									strlen(gps_reply_sms));
		gps_app_log("GPS_APP_CheckPassword, reply sms: %s", gps_reply_sms);
	}

	return result;
}

/**
 * Function: GPS_SMS_ChangePresavedNumber
 *
 * Usage: Change presave number
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_ChangePresavedNumber(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_ChangePresavedNumber, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteUserNumber(instruction->index, 
									 instruction->new_number);
	if (result == RESULT_OK)
	{
		strcpy(gps_usr_numbers[instruction->index-1].number, instruction->new_number);
	}
	sprintf(gps_reply_sms, "SET USER NUMBER %d %s", 
			instruction->index, 
			(result == RESULT_OK) ? "OK" : "FAIL");
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_ChangePresavedNumber, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_ChangePresavedSosNum
 *
 * Usage: Change presave number in sms service center mode
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_ChangePresavedSosNum(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_ChangePresavedSosNum, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteUserNumber(instruction->index, 
									 instruction->new_number);
	if (result == RESULT_OK)
	{
		strcpy(gps_usr_numbers[instruction->index-1].number, instruction->new_number);
	}
	sprintf(gps_reply_sms, "SET SOS NUMBER %d %s", 
			instruction->index, 
			(result == RESULT_OK) ? "OK" : "FAIL");
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_ChangePresavedSosNum, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_ChangeServiceNumber
 *
 * Usage: Change service number
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_ChangeServiceNumber(const GPS_Sms_Instruction_t *instruction)
{
	GPS_Saved_Number_t tmp_num;
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_ChangeServiceNumber, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	memset(&tmp_num, 0, sizeof(tmp_num));
	strcpy(tmp_num.number, instruction->new_number);
	strcpy(tmp_num.passwd, instruction->new_passwd);
	tmp_num.index = 0;

	result = GPS_APP_WriteNumberRecord(0, &tmp_num);
	if (result == RESULT_OK)
	{
		memcpy(&gps_sc_number,
			   &tmp_num,
			   sizeof(GPS_Saved_Number_t));
		strcpy((char *)gps_act_sc_num.Number, gps_sc_number.number);
		gps_act_sc_num.Type = GPS_APP_GetNumberType((char *)gps_act_sc_num.Number);
		gps_act_sc_num.Length = strlen((char *)gps_act_sc_num.Number);
	}
	sprintf(gps_reply_sms, "SET SMS SERVER NUMBER AND PASSWORD %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_ChangeServiceNumber, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_EnableGPSModule
 *
 * Usage: Enable GPS module
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_EnableGPSModule(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint8 tmpSetting = GPS_MODULE_POWERSTATE_ON;

	gps_app_log("GPS_SMS_EnableGPSModule, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteGPSOnSettings(&tmpSetting);
	if (result == RESULT_OK)
	{
#ifdef VIBRATION_SENSOR_SUPPORT
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_AUTO)
		{
		GPSLocateVibSensorStop();
		}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_SWAUTO)
		{
		GPS_APP_ModSWPowerTimerStop();
		SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_FALSE);
		}
#endif

		// Enable GPS module here
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
		gps_module_en_setting = GPS_MODULE_POWERSTATE_ON;
	}
	sprintf(gps_reply_sms, "GPS ON %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");


	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_EnableGPSModule, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_DisableGPSModule
 *
 * Usage: Disable GPS module
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_DisableGPSModule(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint8 tmpSetting = GPS_MODULE_POWERSTATE_OFF;

	gps_app_log("GPS_SMS_DisableGPSModule, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteGPSOnSettings(&tmpSetting);
	if (result == RESULT_OK)
	{
#ifdef VIBRATION_SENSOR_SUPPORT
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_AUTO)
		{
		GPSLocateVibSensorStop();
		}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_SWAUTO)
		{
		GPS_APP_ModSWPowerTimerStop();
		SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_FALSE);
		}
#endif

		// Disable GPS module here
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
		gps_module_en_setting = GPS_MODULE_POWERSTATE_OFF;
	}
	sprintf(gps_reply_sms, "GPS OFF %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_DisableGPSModule, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_CancelGPSSettings
 *
 * Usage: Cancel all settings for GPS module and restore to default
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_CancelGPSSettings(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	Result_t res2 = RESULT_ERROR;
	kal_uint8 tmpSetting = GPS_MODULE_POWERSTATE_OFF;
	kal_uint32 tmpSetting2 = 0;

	gps_app_log("GPS_SMS_CancelGPSSettings, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Restore default settings here
	switch (gps_curr_bearer_type)
	{
	case GPS_BEARER_SMS:
		result = GPS_APP_WriteTimingValue(&tmpSetting);
		if (result == RESULT_OK)
		{
			gps_timing = 0;
			GPS_APP_SetTimingLocateFlag(KAL_FALSE);
			if (gps_timingloc_timer != NULL)
			{
				GPSAppTimer_Stop(gps_timingloc_timer, KAL_TRUE);
				gps_timingloc_timer = NULL;
			}
		}

		break;

	case GPS_BEARER_GPRS:
		result = GPS_APP_WriteGprsUploadSettings(&tmpSetting2);
		if (result == RESULT_OK)
		{
			kal_uint8 *tmpPtr;
			GPSAppEvent_t tmpEvent;
			kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_SMS;
			kal_bool fifo_push = KAL_TRUE;

			GPS_UpldModeMonitorStop();
			gps_samp_interval = 0;
			gps_upload_cnt = 0;
		    GPSPPBufferSetThreshold(gps_upload_cnt);
			if (gps_sampgprmc_timer != NULL)
			{
				GPSAppTimer_Stop(gps_sampgprmc_timer, KAL_TRUE);
				gps_sampgprmc_timer = NULL;
			}

			// Upload saved gprmc data
			if (GPSAppEventFifoIsFull())
			{
				// fifo full, discard req
				gps_app_log("GPS_SMS_CancelGPSSettings, fifo full");
				return RESULT_ERROR;
			}

			if (GPSAppEventFifoIsEmpty())
			{
				fifo_push = GPS_APP_StartGprsUpload(cause_type);
			}
			if (fifo_push)
			{
				tmpPtr = (kal_uint8 *)get_ctrl_buffer(sizeof(kal_uint8));
				*tmpPtr = cause_type;
				tmpEvent.OpCode = GPS_APP_EVENT_OP_GPRSUPLOAD;
				tmpEvent.LocalPara = tmpPtr;
				GPSAppEventFifoPush(&tmpEvent, KAL_TRUE);
				gps_app_log("GPS_SMS_CancelGPSSettings, push req into fifo, OpCode: %d", tmpEvent.OpCode);
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

	res2 = GPS_APP_WriteGPSOnSettings(&tmpSetting);
	if (res2 == RESULT_OK)
	{
#ifdef VIBRATION_SENSOR_SUPPORT
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_AUTO)
		{
		GPSLocateVibSensorStop();
		}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_SWAUTO)
		{
		GPS_APP_ModSWPowerTimerStop();
		SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_FALSE);
		}
#endif

		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
		gps_module_en_setting = GPS_MODULE_POWERSTATE_OFF;
	}
	if (result == RESULT_OK && res2 == RESULT_OK)
	{
		sprintf(gps_reply_sms, "GPS OFF OK");
	}
	else
	{
		sprintf(gps_reply_sms, "GPS OFF FAIL");
	}
#ifdef GPS_GSG_SUPPORT
	// Turn off square wave
	GSG_Stop();
#endif

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_CancelGPSSettings, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetTimingLocate
 *
 * Usage: Set timing locate interval
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetTimingLocate(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	Result_t res2 = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetTimingLocate, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Set timing value
	result = GPS_APP_WriteTimingValue(&instruction->timing);
	res2 = GPS_APP_WriteTimingNumber((char *)instruction->remote_number.Number);
	if (result == RESULT_OK && res2 == RESULT_OK)
	{
		// store new timing setting and number
		gps_timing = instruction->timing;
		memcpy(&gps_curr_timing_num, &instruction->remote_number, sizeof(gps_curr_timing_num));

		GPS_APP_SetTimingLocateFlag(KAL_FALSE);

		// stop or restart timing report according to current mode setting
		if (gps_curr_bearer_type == GPS_BEARER_SMS)
		{
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
				sprintf(gps_reply_sms, "TIMER START, REPEAT INTERVAL: %d MINUTES", gps_timing_in_min);
				gps_app_log("GPS_SMS_SetTimingLocate, restart GPS_APP_TimingLocateRepeatHandler timer");
			}
			else
			{
				if (gps_timingloc_timer != NULL)
				{
					GPSAppTimer_Stop(gps_timingloc_timer, KAL_TRUE);
					gps_timingloc_timer = NULL;
				}
				sprintf(gps_reply_sms, "TIMER STOP");
			}
		}
		else
		{
			sprintf(gps_reply_sms, "SET TIMER OK, CURRENT TIMER: %d", gps_timing);
		}
	}
	else
	{
		sprintf(gps_reply_sms, "SET TIMER FAIL");
	}
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetTimingLocate, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetBaseStation
 *
 * Usage: Set base station number to get
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetBaseStation(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetBaseStation, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Set base station number
	result = GPS_APP_WriteBSNumber(&instruction->bs_num);
	if (result == RESULT_OK)
	{
		gps_bs_num = instruction->bs_num;
	}
	sprintf(gps_reply_sms, "SET BASESTATION %s",
			(result == RESULT_OK) ? "OK" : "FAIL");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetBaseStation, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SimpleLocate
 *
 * Usage: Start a single locate requirement
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SimpleLocate(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SimpleLocate, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

    GPS_APP_SingleLocateStart(&instruction->remote_number, GPS_RMCSMS_TYPE_SMS);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetPassword
 *
 * Usage: Set new user password
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetPassword(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint8 found = 0;
	int i;

	gps_app_log("GPS_SMS_SetPassword, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Change password
#ifdef GPS_SEPARATE_USER_PASSWOR
	found = GPS_APP_FindNumberInPresavdList(&instruction->remote_number);
	if (!found)
	{
		gps_app_log("GPS_SMS_SetPassword, not found phone number in list");
		sprintf(gps_reply_sms, "NUMBER NOT AUTHORIZED");
		GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
									gps_reply_sms, 
									strlen(gps_reply_sms));
		gps_app_log("GPS_SMS_SetPassword, reply sms: %s", gps_reply_sms);

		return RESULT_ERROR;
	}
	result = GPS_APP_WriteUserPassword(found, instruction->new_passwd);
#else
	for (i = 1; i <= GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		result = GPS_APP_WriteUserPassword(i, instruction->new_passwd);
		strcpy(gps_usr_numbers[i-1].passwd, instruction->new_passwd);
	}
#endif

	if (result == RESULT_OK)
	{
#ifndef GPS_SEPARATE_USER_PASSWOR
		strcpy(gps_usr_passwd, instruction->new_passwd);
#endif
	}
	sprintf(gps_reply_sms, "SET USER PASSWORD %s",
			(result == RESULT_OK) ? "OK" : "FAIL");
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetPassword, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

#ifdef GPS_ITRACK_FORMAT
/**
 * Function: GPS_SMS_SetFrontMsg
 *
 * Usage: Set iTrack front message
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetFrontMsg(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetFrontMsg, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteFrontMsg(instruction->apn);
	if (result == RESULT_OK)
	{
		memset(gps_itrack_sfmsg, 0, GPSLOCATE_PRESAVED_ITRACKMSG_LEN);
		strncpy(gps_itrack_sfmsg, instruction->apn, GPSLOCATE_PRESAVED_ITRACKMSG_LEN-1);
		sprintf(gps_reply_sms, "SFMSG OK=%s", gps_itrack_sfmsg);
	}
	else
	{
		sprintf(gps_reply_sms, "SFMSG FAIL");
	}
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetFrontMsg, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetRearMsg
 *
 * Usage: Set iTrack rear message
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetRearMsg(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetRearMsg, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteRearMsg(instruction->apn);
	if (result == RESULT_OK)
	{
		memset(gps_itrack_srmsg, 0, GPSLOCATE_PRESAVED_ITRACKMSG_LEN);
		strncpy(gps_itrack_srmsg, instruction->apn, GPSLOCATE_PRESAVED_ITRACKMSG_LEN-1);
		sprintf(gps_reply_sms, "SRMSG OK=%s", gps_itrack_srmsg);
	}
	else
	{
		sprintf(gps_reply_sms, "SRMSG FAIL");
	}
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetRearMsg, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_GetiTrackMsg
 *
 * Usage: Get iTrack front/rear message
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_GetiTrackMsg(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	sprintf(gps_reply_sms, "SFMSG=%s\nSRMSG=%s\nID=%s\nUP=%s", 
			gps_itrack_sfmsg,
			gps_itrack_srmsg,
			gps_imei_str,
			gps_usr_passwd);
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_GetiTrackMsg, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

/**
 * Function: GPS_SMS_SetBearerAndMode
 *
 * Usage: Set bearer type and sms mode
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetBearerAndMode(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint16 tmpBM = 0;

	gps_app_log("GPS_SMS_SetBearerAndMode, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);

	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	if (gps_curr_bearer_type == instruction->bearer && gps_curr_sms_mode == instruction->sms_mode)
	{
		sprintf(gps_reply_sms, "MODE NOT CHANGE, CURRENT MODE: %s", 
				gps_curr_bearer_type == GPS_BEARER_GPRS ? "GPRS" :
				gps_curr_sms_mode == GPS_SMS_MODE_P2P ? "SMS P2P" : "SMS SC");

		GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
									gps_reply_sms, 
									strlen(gps_reply_sms));
		gps_app_log("GPS_SMS_SetBearerAndMode, reply sms: %s", gps_reply_sms);

		return RESULT_OK;
	}

	tmpBM = (instruction->bearer << 8) | instruction->sms_mode;
	result = GPS_APP_WriteBearerAndMode(&tmpBM);
	if (result == RESULT_OK)
	{
		Result_t res2 = RESULT_ERROR;
		kal_uint8 tmpValue = 0;

		gps_app_log("bearer: %d, mode: %d", instruction->bearer, instruction->sms_mode);
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
		res2 = GPS_APP_WriteTimingValue(&tmpValue);
		gps_timing = tmpValue;

		// start repeat handler for the setting mode
		switch (instruction->bearer)
		{
		case GPS_BEARER_SMS:
			GPS_UpldModeMonitorStop();
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
			GPS_UpldModeMonitorStart();
			if (GPS_APP_GPRSUpldMode())
			{
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
			}
			else
			{
				if (gps_upload_cnt2 > 0 && gps_samp_interval2 > 0)
				{
					if (gps_sampgprmc_timer != NULL)
					{
						GPSAppTimer_Reset(gps_sampgprmc_timer,
										GPS_APP_SampGprmcRepeatHandler,
										gps_samp_interval2 * KAL_TICKS_1_SEC,
										gps_samp_interval2 * KAL_TICKS_1_SEC,
										KAL_TRUE);
					}
					else
					{
						gps_sampgprmc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
													GPS_APP_SampGprmcRepeatHandler,
													gps_samp_interval2 * KAL_TICKS_1_SEC,
													gps_samp_interval2 * KAL_TICKS_1_SEC,
													KAL_TRUE);
					}
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
		gps_curr_bearer_type = instruction->bearer;
		gps_curr_sms_mode = instruction->sms_mode;

		sprintf(gps_reply_sms, "SET MODE OK, CURRENT MODE: %s", 
				gps_curr_bearer_type == GPS_BEARER_GPRS ? "GPRS" :
				gps_curr_sms_mode == GPS_SMS_MODE_P2P ? "SMS P2P" : "SMS SC");
	}
	else
	{
		sprintf(gps_reply_sms, "SET MODE FAIL");
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetBearerAndMode, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

#ifdef GPS_GSG_SUPPORT
/**
 * Function: GPS_SMS_SetSquareWave
 *
 * Usage: Set square wave on/off
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetSquareWave(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetSquareWave, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_app_log("GPS_SMS_SetSquareWave, square wave: %s", instruction->sq_wave ? "on" : "off");
	if (instruction->sq_wave)
	{
		// Turn on square wave
		GSG_Start();
	}
	else
	{
		// Turn off square wave
		GSG_Stop();
	}
	sprintf(gps_reply_sms, "SQUARE WAVE %s", 
			instruction->sq_wave ? "ON" : "OFF");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_EnableGPSAndSQWave, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_EnableGPSAndSQWave
 *
 * Usage: Enable GPS module and set square wave on
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_EnableGPSAndSQWave(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_EnableGPSAndSQWave, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Enable GPS module here
	GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	gps_module_en_setting = GPS_MODULE_POWERSTATE_ON;
	// Turn on square wave
	GSG_Start();
	// Reply sms
	sprintf(gps_reply_sms, "GPS ON OK AND SQUARE WAVE ON");
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_EnableGPSAndSQWave, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

/**
 * Function: GPS_SMS_GetCellInfo
 *
 * Usage: Request cell info
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_GetCellInfo(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_GetCellInfo, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

    memcpy(&gps_act_remote_num, &instruction->remote_number, sizeof(gps_act_remote_num));
	gps_get_nwinfo_request(RR_EM_LAI_INFO | RR_EM_MEASUREMENT_REPORT_INFO);

	if (gps_getnwinfo_timer != NULL)
	{
		GPSAppTimer_Reset(gps_getnwinfo_timer, 
				GPS_APP_GetNWInfoHandler, 
				KAL_TICKS_1_SEC * 2,
				0, 
				KAL_TRUE);
	}
	else
	{
		gps_getnwinfo_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_GetNWInfoHandler,
									KAL_TICKS_1_SEC * 2,
									0,
									KAL_TRUE);
	}

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetMonitorProf
 *
 * Usage: Set warning vbat
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetMonitorProf(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint8 tmpSetting = (kal_uint8)instruction->setting;

	gps_app_log("GPS_SMS_SetMonitorProf, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteMtcallProfile(&tmpSetting);
	if (result == RESULT_OK)
	{
		// set profile
		GpsAppMmiBrgSetProfile(instruction->setting);
		gps_mtcall_profile = instruction->setting;
		sprintf(gps_reply_sms, "SET PROFILE OK, CURRENT PROFILE: %s", 
				instruction->setting ? "SILENT" : "NORMAL");
	}
	else
	{
		sprintf(gps_reply_sms, "SET PROFILE FAIL");
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetMonitorProf, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetGPRSUsername
 *
 * Usage: Set GPRS user name
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetGPRSUsername(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetGPRSUsername, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_app_log("GPS_SMS_SetGPRSUsername, new gprs username: %s", 
				instruction->new_number);

	result = GPS_APP_WriteGprsUsername(instruction->new_number);
	if (result == RESULT_OK)
	{
		strcpy(gps_gprs_username, instruction->new_number);
	}
	sprintf(gps_reply_sms, "SET SERVER USER NAME %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetGPRSUsername, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetGPRSPasswd
 *
 * Usage: Set GPRS user password
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetGPRSPasswd(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetGPRSPasswd, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_CheckGprsPassword(&instruction->remote_number, instruction->srv_passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_app_log("GPS_SMS_SetGPRSPasswd, new gprs password: %s", 
				instruction->new_passwd);

	result = GPS_APP_WriteGprsPassword(instruction->new_passwd);
	if (result == RESULT_OK)
	{
		strcpy(gps_gprs_userpwd, instruction->new_passwd);
	}
	sprintf(gps_reply_sms, "SET SERVER PASSWORD %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetGPRSPasswd, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetGPRSAPN
 *
 * Usage: Set GPRS APN
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetGPRSAPN(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	char tmpStr[2] = {'\0'};

	gps_app_log("GPS_SMS_SetGPRSAPN, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Set GPRS APN
	gps_set_gprs_account_direct(GPS_GPRS_ACCOUNT_IDX,
								instruction->apn,
								tmpStr,
								tmpStr);
	strcpy(gps_gprs_apn, instruction->apn);
	strcpy(gps_gprs_apnuser, tmpStr);
	strcpy(gps_gprs_apnpwd, tmpStr);
	GPS_APP_WriteGprsAPN(gps_gprs_apn, gps_gprs_apnuser, gps_gprs_apnpwd);

	sprintf(gps_reply_sms, "SET GPRS APN OK");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetGPRSAPN, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetTCPIPServer
 *
 * Usage: Set GPRS account(include apn, user name & password)
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetGPRSAccount(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetGPRSAccount, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Set GPRS APN
	gps_set_gprs_account_direct(GPS_GPRS_ACCOUNT_IDX,
								instruction->apn,
								instruction->new_number,
								instruction->gpwd);
	strcpy(gps_gprs_apn, instruction->apn);
	strcpy(gps_gprs_apnuser, instruction->new_number);
	strcpy(gps_gprs_apnpwd, instruction->gpwd);
	GPS_APP_WriteGprsAPN(gps_gprs_apn, gps_gprs_apnuser, gps_gprs_apnpwd);

	sprintf(gps_reply_sms, "SET GPRS ACCOUNT OK");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetGPRSAccount, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetTCPIPServer
 *
 * Usage: Set tcp/ip server ip and port
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetTCPIPServer(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;

	gps_app_log("GPS_SMS_SetTCPIPServer, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);

	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_app_log("GPS_SMS_SetTCPIPServer, ip: %d.%d.%d.%d, port: %d", 
				instruction->server_ip.addr[0], 
				instruction->server_ip.addr[1], 
				instruction->server_ip.addr[2], 
				instruction->server_ip.addr[3], 
				instruction->server_ip.port);

	result = GPS_APP_WriteServerAddr(&instruction->server_ip);
	if (result == RESULT_OK)
	{
		memcpy(&gps_tcpip_server_addr, &instruction->server_ip, sizeof(gps_tcpip_server_addr));
	}
	sprintf(gps_reply_sms, "SET SERVER IP AND PORT %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetTCPIPServer, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetGPRSUpload
 *
 * Usage: Set upload through GPRS settings
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetGPRSUpload(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint32 tmpSetting;
	kal_uint16 tmpIntv, tmpCnt;

	gps_app_log("GPS_SMS_SetGPRSUpload, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Set gprs upload settings
	// temp limit interval to be 5s at least and upload_cnt to be 10 at least
	tmpIntv = (instruction->interval == 0) ? 
			  0 :
			  (instruction->interval > GPS_SEND_INTERVAL_MIN) ? 
			  instruction->interval : 
			  GPS_SEND_INTERVAL_MIN;
#if 1
	tmpCnt = instruction->upload_cnt;
	if (tmpCnt) 
	{
		if (tmpIntv * tmpCnt < 60)
		{
			tmpIntv = 60 / tmpCnt;
		}
		if (tmpIntv * tmpCnt < 60)
		{
			tmpIntv += 1;
		}
	}
#else
	tmpCnt = (instruction->upload_cnt == 0) ? 
			  0 :
			  (instruction->upload_cnt > GPS_SEND_ITEMS_ONETIME) ? 
			 instruction->upload_cnt :
			 GPS_SEND_ITEMS_ONETIME;
#endif
	tmpSetting = (tmpCnt << 16) | tmpIntv;
	result = GPS_APP_WriteGprsUploadSettings(&tmpSetting);
	if (result == RESULT_OK)
	{
		gps_app_log("interval: %d, upload count: %d", instruction->interval, instruction->upload_cnt);
		gps_samp_interval = tmpIntv;
		gps_upload_cnt = tmpCnt;

		if (!GPS_APP_GPRSUpldMode())
			goto reply_sms;

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
					gps_app_log("GPS_SMS_SetGPRSUpload, push req into fifo, OpCode: %d", tmpEvent.OpCode);
				}
			}
			else
			{
				// fifo full, discard req
				gps_app_log("GPS_SMS_SetGPRSUpload, fifo full");
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

		// reset sampling timer
		if (gps_curr_bearer_type == GPS_BEARER_GPRS)
		{
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
		}
	}

reply_sms:
	if (tmpIntv == 0 || tmpCnt == 0)
	{
		sprintf(gps_reply_sms, "GPRS TIMER STOP %s", 
				(result == RESULT_OK) ? "OK" : "FAIL");
	}
	else
	{
		sprintf(gps_reply_sms, "SET GPS SAMPLING TIME AND QUANTITY %s", 
				(result == RESULT_OK) ? "OK" : "FAIL");
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetGPRSUpload, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetGPRSUpload2
 *
 * Usage: Set upload through GPRS settings
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetGPRSUpload2(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint32 tmpSetting;
	kal_uint16 tmpIntv, tmpCnt;

	gps_app_log("GPS_SMS_SetGPRSUpload2, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	// Set gprs upload settings
	// temp limit interval to be 5s at least and upload_cnt to be 10 at least
	tmpIntv = (instruction->interval == 0) ? 
			  0 :
			  (instruction->interval > GPS_SEND_INTERVAL_MIN) ? 
			  instruction->interval : 
			  GPS_SEND_INTERVAL_MIN;
#if 1
	tmpCnt = instruction->upload_cnt;
	if (tmpCnt) 
	{
		if (tmpIntv * tmpCnt < 60)
		{
			tmpIntv = 60 / tmpCnt;
		}
		if (tmpIntv * tmpCnt < 60)
		{
			tmpIntv += 1;
		}
	}
#else
	tmpCnt = (instruction->upload_cnt == 0) ? 
			  0 :
			  (instruction->upload_cnt > GPS_SEND_ITEMS_ONETIME) ? 
			 instruction->upload_cnt :
			 GPS_SEND_ITEMS_ONETIME;
#endif
	tmpSetting = (tmpCnt << 16) | tmpIntv;
	result = GPS_APP_WriteGprsUploadSettings2(&tmpSetting);
	if (result == RESULT_OK)
	{
		gps_app_log("interval: %d, upload count: %d", instruction->interval, instruction->upload_cnt);
		gps_samp_interval2 = tmpIntv;
		gps_upload_cnt2 = tmpCnt;

		if (GPS_APP_GPRSUpldMode())
			goto reply_sms;

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
					gps_app_log("GPS_SMS_SetGPRSUpload2, push req into fifo, OpCode: %d", tmpEvent.OpCode);
				}
			}
			else
			{
				// fifo full, discard req
				gps_app_log("GPS_SMS_SetGPRSUpload2, fifo full");
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
	    GPSPPBufferSetThreshold(gps_upload_cnt2);

		// reset sampling timer
		if (gps_curr_bearer_type == GPS_BEARER_GPRS)
		{
			if (gps_upload_cnt2 > 0 && gps_samp_interval2 > 0)
			{
				if (gps_sampgprmc_timer != NULL)
				{
					GPSAppTimer_Reset(gps_sampgprmc_timer,
								GPS_APP_SampGprmcRepeatHandler,
								gps_samp_interval2 * KAL_TICKS_1_SEC,
								gps_samp_interval2 * KAL_TICKS_1_SEC,
								KAL_TRUE);
				}
				else
				{
					gps_sampgprmc_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
											GPS_APP_SampGprmcRepeatHandler,
											gps_samp_interval2 * KAL_TICKS_1_SEC,
											gps_samp_interval2 * KAL_TICKS_1_SEC,
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
		}
	}

reply_sms:
	if (tmpIntv == 0 || tmpCnt == 0)
	{
		sprintf(gps_reply_sms, "GPRS REPORT SAMPLING 2 STOP %s", 
				(result == RESULT_OK) ? "OK" : "FAIL");
	}
	else
	{
		sprintf(gps_reply_sms, "GPRS REPORT SAMPLING 2 %s", 
				(result == RESULT_OK) ? "OK" : "FAIL");
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetGPRSUpload2, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_GPRSUploadImm
 *
 * Usage: Start upload gprmc frame in ping-pong buffer to tcp/ip server immediately requirement
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_GPRSUploadImm(const GPS_Sms_Instruction_t *instruction)
{
	Result_t result = RESULT_ERROR;
	kal_uint8 *tmpPtr;
	GPSAppEvent_t tmpEvent;
	kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_SMS;
	kal_bool fifo_push = KAL_TRUE;

	gps_app_log("GPS_SMS_GPRSUploadImm, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	sprintf(gps_reply_sms, "START GPRS UPLOAD");
	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_GPRSUploadImm, reply sms: %s", gps_reply_sms);

	if (GPSAppEventFifoIsFull())
	{
		// fifo full, discard req
		gps_app_log("GPS_SMS_GPRSUploadImm, fifo full");
		return RESULT_ERROR;
	}

	if (GPSAppEventFifoIsEmpty())
	{
		fifo_push = GPS_APP_StartGprsUpload(cause_type);
	}
	if (fifo_push)
	{
		tmpPtr = (kal_uint8 *)get_ctrl_buffer(sizeof(kal_uint8));
		*tmpPtr = cause_type;
		tmpEvent.OpCode = GPS_APP_EVENT_OP_GPRSUPLOAD;
		tmpEvent.LocalPara = tmpPtr;
		GPSAppEventFifoPush(&tmpEvent, KAL_TRUE);
		gps_app_log("GPS_SMS_GPRSUploadImm, push req into fifo, OpCode: %d", tmpEvent.OpCode);
	}

	return RESULT_OK;
}

#ifdef GPS_POSITION_MONITOR
/**
 * Function: GPS_SMS_SetPosition
 *
 * Usage: Start position monitoring or stop position monitoring
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetPosition(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetPosition, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_curr_position.radius = instruction->pos_range.radius;
	gps_curr_position.radius_f = instruction->pos_range.radius_f;
    GPS_APP_SetPositionStart(&instruction->remote_number);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetPosition2
 *
 * Usage: Set position for monitoring
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetPosition2(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetPosition2, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteFixPosition(&instruction->pos_range);
	if (result == RESULT_OK)
	{
		memcpy(&gps_curr_position, &instruction->pos_range, sizeof(gps_curr_position));
	}
	sprintf(gps_reply_sms, "SET GEO-FENCE %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetPosition2, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetRateLimit
 *
 * Usage: Set position monitor on/off
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetPosMonOnff(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetPosMonOnff, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WritePosMonitorOnff((unsigned char *)&instruction->setting);
	if (result == RESULT_OK)
	{
		gps_posmonitor_on = instruction->setting;
		if (gps_posmonitor_on)
		{
			GPS_APP_PosMonitorStart();
		}
		else
		{
			GPS_APP_PosMonitorStop();
		}
		sprintf(gps_reply_sms, "GEO-FENCE %s", 
				gps_posmonitor_on ? "ON" : "OFF");
	}
	else
	{
		sprintf(gps_reply_sms, "SET GEO-FENCE FAIL");
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetPosMonOnff, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

#ifdef GPS_RATE_MONITOR
/**
 * Function: GPS_SMS_SetRateLimit
 *
 * Usage: Start rate monitoring or stop rate monitoring
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetRateLimit(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetRateLimit, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_rate_limit = instruction->setting;
	GPS_APP_WriteRateLimitSetting(&gps_rate_limit);

	if (gps_rate_limit == 0)
	{
		GPS_APP_RateMonitorStop();
	}
	else
	{
		GPS_APP_RateMonitorStart();
	}

	sprintf(gps_reply_sms, "SET RATE LIMIT: %d", gps_rate_limit);

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetRateLimit, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

/**
 * Function: GPS_SMS_SetOpenDoorWave
 *
 * Usage: Start to generate a squre wave
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetOpenDoorWave(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetOpenDoorWave, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);

	if (!GPS_APP_FindNumberInPresavdList(&instruction->remote_number))
	{
		return RESULT_ERROR;
	}
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	GSG_WidthSquareWave(1);

	sprintf(gps_reply_sms, "OPEN DOOR OK");

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetOpenDoorWave, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

#ifdef GPS_DEFENCE_FEATURE
/**
 * Function: GPS_SMS_SetDefence
 *
 * Usage: Set defence on/off
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetDefence(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetDefence, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	if (!GPS_APP_FindNumberInPresavdList(&instruction->remote_number))
	{
		return RESULT_ERROR;
	}
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}


	result = GPS_APP_WriteDefenceSetting((unsigned char *)&instruction->setting);
	if (result == RESULT_OK)
	{
		gps_set_defence = instruction->setting;
		if (gps_set_defence)
		{
			GPS_APP_SetDefenceON();
		}
		else
		{
			GPS_APP_DefenceReset();
		}
		sprintf(gps_reply_sms, "DEFENCE %s", 
				gps_set_defence ? "ON" : "OFF");
	}
	else
	{
		sprintf(gps_reply_sms, "SET DEFENCE FAIL"); 
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetDefence, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

#ifdef VIBRATION_SENSOR_SUPPORT
/**
 * Function: GPS_SMS_SetVibSensor
 *
 * Usage: Set vibrator sensor on/off
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetVibSensor(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;
	kal_uint8 tmpSetting = GPS_MODULE_POWERSTATE_AUTO;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetVibSensor, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteGPSOnSettings(&tmpSetting);
	if (result == RESULT_OK)
	{
#ifdef GPS_MOD_SWAUTOPOWER
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_SWAUTO)
		{
		GPS_APP_ModSWPowerTimerStop();
		SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_FALSE);
		}
#endif
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
		GPSLocateVibSensorStart();
		gps_module_en_setting = GPS_MODULE_POWERSTATE_AUTO;
	}
	sprintf(gps_reply_sms, "VIBRATION SENSOR ON %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");
	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetVibSensor, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
/**
 * Function: GPS_SMS_SetSWAutoPower
 *
 * Usage: Set gps module auto on/off by sw
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetSWAutoPower(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;
	kal_uint8 tmpSetting = GPS_MODULE_POWERSTATE_SWAUTO;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetSWAutoPower, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteGPSOnSettings(&tmpSetting);
	if (result == RESULT_OK)
	{
#ifdef VIBRATION_SENSOR_SUPPORT
		if (gps_module_en_setting == GPS_MODULE_POWERSTATE_AUTO)
		{
		GPSLocateVibSensorStop();
		}
#endif
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
		GPS_APP_ModSWPowerTimerStart();
		gps_module_en_setting = GPS_MODULE_POWERSTATE_SWAUTO;
	}
	sprintf(gps_reply_sms, "SW POWER CONSUMPTION SAVING ON %s", 
			(result == RESULT_OK) ? "OK" : "FAIL");
	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetSWAutoPower, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

/**
 * Function: GPS_SMS_SetSosCall
 *
 * Usage: Set sos phone call on/off
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_SetSosCall(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;
	kal_uint8 tmpSetting = instruction->setting;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_SetSosCall, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	result = GPS_APP_WriteSosCallSetting(&tmpSetting);
	if (result == RESULT_OK)
	{
		// set profile
		gps_sos_phonecall = instruction->setting;
		sprintf(gps_reply_sms, "SET VOICE CALL: %s", 
				instruction->setting ? "ON" : "OFF");
	}
	else
	{
		sprintf(gps_reply_sms, "SET VOICE CALL FAIL");
	}

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetSosCall, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

#ifdef GPS_MOTOPWRCUT_FEATURE
/**
 * Function: GPS_SMS_PowerOffReq
 *
 * Usage: Power off request
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_PowerOffReq(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_PowerOffReq, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_motopower_cut = 1;
	sprintf(gps_reply_sms, "Confirm Power OFF?");
	GPS_APP_MotoPowerCutTimerStart();

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_PowerOffReq, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_PowerOffConfirm
 *
 * Usage: Power off confirm
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_PowerOffConfirm(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_PowerOffConfirm, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	if (!gps_motopower_cut) return RESULT_ERROR;

	GPIO_WriteIO(1, GPS_SQWAVE_GPIO);
	sprintf(gps_reply_sms, "POWER OFF OK"); 
	gps_motopower_cut = 0;
	GPS_APP_MotoPowerCutTimerStop();

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_PowerOffConfirm, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_PowerOnReq
 *
 * Usage: Power on request
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_PowerOnReq(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_PowerOnReq, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	gps_motopower_on = 1;
	sprintf(gps_reply_sms, "Confirm Power ON?");
	GPS_APP_MotoPowerOnTimerStart();

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_PowerOnReq, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_PowerOnConfirm
 *
 * Usage: Power on confirm
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_PowerOnConfirm(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_PowerOnConfirm, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	if (!gps_motopower_on) return RESULT_ERROR;

	GPIO_WriteIO(0, GPS_SQWAVE_GPIO);
	sprintf(gps_reply_sms, "POWER ON OK"); 
	gps_motopower_on = 0;
	GPS_APP_MotoPowerOnTimerStop();

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_PowerOnConfirm, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_PowerOffReq2
 *
 * Usage: Power off request
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_PowerOffReq2(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_PowerOffReq2, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	GPIO_WriteIO(1, GPS_SQWAVE_GPIO);
	sprintf(gps_reply_sms, "POWER OFF OK"); 

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_PowerOffReq2, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_PowerOnReq2
 *
 * Usage: Power on request
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_PowerOnReq2(const GPS_Sms_Instruction_t *instruction)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("GPS_SMS_PowerOnReq2, number: %s, passwd: %s", 
				(char *)instruction->remote_number.Number, 
				instruction->passwd);
	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	GPIO_WriteIO(0, GPS_SQWAVE_GPIO);
	sprintf(gps_reply_sms, "POWER ON OK"); 

	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_PowerOnReq2, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}
#endif

/**
 * Function: GPS_SMS_ResetPowerTimes
 *
 * Usage: Reset power times count
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_ResetPowerTimes(const GPS_Sms_Instruction_t *instruction)
{
	kal_uint32 tmpSetting = 0;

	GPS_APP_WritePoweronTimes((unsigned long *)&tmpSetting);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_GetAllNumber
 *
 * Usage: Get all number and password (both users and service center)
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_GetAllNumber(const GPS_Sms_Instruction_t *instruction)
{
	char tmpBuf[160];
	Result_t result = RESULT_ERROR;

	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	GPS_ListAllSettings2(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllNumber, reply sms: %s", tmpBuf);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_GetAllSettings
 *
 * Usage: Get all settings
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_GetAllSettings(const GPS_Sms_Instruction_t *instruction)
{
	char tmpBuf[160];
	Result_t result = RESULT_ERROR;

	result = GPS_APP_CheckPassword(&instruction->remote_number, instruction->passwd);
	if (result != RESULT_OK)
	{
		return RESULT_ERROR;
	}

	GPS_ListAllSettings1(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllSettings, reply sms: %s", tmpBuf);

	GPS_ListAllSettings3(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllSettings, reply sms: %s", tmpBuf);

	GPS_ListAllSettings4(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllSettings, reply sms: %s", tmpBuf);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_GetAllSettings
 *
 * Usage: Get all saved settings
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_GetAllConfig(const GPS_Sms_Instruction_t *instruction)
{
	char tmpBuf[160];

	GPS_ListAllSettings1(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllConfig, reply sms: %s", tmpBuf);

	GPS_ListAllSettings2(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllConfig, reply sms: %s", tmpBuf);

	GPS_ListAllSettings3(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllConfig, reply sms: %s", tmpBuf);

	GPS_ListAllSettings4(tmpBuf);
	ASSERT(strlen(tmpBuf) <= sizeof(tmpBuf));

	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								tmpBuf, 
								strlen(tmpBuf));
	gps_app_log("GPS_SMS_GetAllConfig, reply sms: %s", tmpBuf);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_EnableGPS
 *
 * Usage: Enable GPS module
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_EnableGPS(const GPS_Sms_Instruction_t *instruction)
{
	// Enable GPS module here
	GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	gps_module_en_setting = GPS_MODULE_POWERSTATE_ON;

	// Reply sms
	sprintf(gps_reply_sms, "GPS ON"); 
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_EnableGPS, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_DisableGPS
 *
 * Usage: Disable GPS module
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_DisableGPS(const GPS_Sms_Instruction_t *instruction)
{
	// Disable GPS module here
	GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	gps_module_en_setting = GPS_MODULE_POWERSTATE_OFF;

	// Reply sms
	sprintf(gps_reply_sms, "GPS OFF"); 
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_DisableGPS, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SwitchTrace
 *
 * Usage: Turn on/off log
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SwitchTrace(const GPS_Sms_Instruction_t *instruction)
{
	GPS_SetTrace(instruction->setting);
	gps_app_log("trace %s", instruction->setting ? "on" : "off");

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetVolume
 *
 * Usage: Set volume
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetVolume(const GPS_Sms_Instruction_t *instruction)
{
	kal_uint16 type, level;
	char *VolumeTypeStr[] = {
		"Receiver",
		"Loud speaker",
		"Microphone",
		NULL
	};

	type = (instruction->setting >> 16) & 0xff;
	level = instruction->setting & 0xff;
	GpsAppMmiBrgSetVolume(type, level);

	memset(gps_reply_sms, 0, sizeof(gps_reply_sms));
	sprintf(gps_reply_sms, "%s volume: %d", VolumeTypeStr[type], level);
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetVolume, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetMTAlertType
 *
 * Usage: Set MT call alert type
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetMTAlertType(const GPS_Sms_Instruction_t *instruction)
{
	char *AlertTypeStr[] = {
		"ALERT NONE",
		"RING",
		"VIBRATION ONLY",
		"VIBRATION AND RING",
		"VIBRATION THEN RING",
		"SILENT",
		NULL
	};

	GpsAppMmiBrgSetMTCallAlert(instruction->setting);

	memset(gps_reply_sms, 0, sizeof(gps_reply_sms));
	sprintf(gps_reply_sms, "Alert type: %s", AlertTypeStr[instruction->setting]);
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetMTAlertType, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SetHandfree
 *
 * Usage: Turn on/off handfree when accept incoming call
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SetHandfree(const GPS_Sms_Instruction_t *instruction)
{
	kal_uint8 tmpSetting = (kal_uint8)instruction->setting;

	// Set handfree setting
	GPS_APP_WriteHFreeSettings(&tmpSetting);
	gps_call_handfree = instruction->setting;

	sprintf(gps_reply_sms, "HANDFREE %s", gps_call_handfree ? "ON" : "OFF");
	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_SetHandfree, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_GetSwVersion
 *
 * Usage: Get sw version
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
static Result_t GPS_SMS_GetSwVersion(const GPS_Sms_Instruction_t *instruction)
{
	sprintf(gps_reply_sms, "%s", release_verno());
	// Reply sms
	GPS_SMS_ConstructAndSendSms(&instruction->remote_number,
								gps_reply_sms, 
								strlen(gps_reply_sms));
	gps_app_log("GPS_SMS_GetSwVersion, reply sms: %s", gps_reply_sms);

	return RESULT_OK;
}

/**
 * Function: GPS_SMS_SimpLocate
 *
 * Usage: Start a single locate require(user defined sms cmd)
 *
 * Parameters:
 *  instruction	- pointer to sms instruction which contain cmd, remote phone number, 
 *                password and setting data
 *
 * Returns:
 *  Result: RESULT_OK or RESULT_ERROR
 */
Result_t GPS_SMS_SimpLocate(const GPS_Sms_Instruction_t *instruction)
{
    GPS_APP_SingleLocateStart(&instruction->remote_number, GPS_RMCSMS_TYPE_SMS);

	return RESULT_OK;
}


