/******************************************************************************
 * File name: gps_app_sms.h
 * Author: Robbie Cao
 * Date: 2008-3-11 10:45
 ******************************************************************************/

#ifndef _GPS_APP_SMS_H
#define _GPS_APP_SMS_H

#include "kal_release.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_settings.h"
#include "gps_app_nema.h"
#include "parser.h"

/*************************************************************************
 * GPS SMS related data type, macro and function prototype
 *************************************************************************/

/**
 *
 */
typedef struct
{
	/**
	 * The following three fields are general for all sms instruction 
	 */
	kal_uint8 				cmd;
	GPSLocatePhoneNumber_t 	remote_number;
										/* remote number which send sms */
	char 					passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];

	/**
	 * The following three fields are for the follwing sms cmd: 
	 *	GPS_SMS_CMD_CHANGE_PRESAVED_NUMBER,
	 *	GPS_SMS_CMD_CHANGE_PRESAVED_SOSNUM,
	 *	GPS_SMS_CMD_CHANGE_SERVICE_NUMBER,
	 *	GPS_SMS_CMD_SET_PASSWORDD,
	 */
	char 					new_number[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
										/* new number which will be changed to */
	char 					new_passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
										/* new passwd which will be changed to */
	kal_uint8 				index;		/* new number index */

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_CHANGE_SERVICE_NUMBER,
	 */
	char 					srv_passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
										/* old service passwd which for check */

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_TIMING_LOCATE,
	 */
	kal_uint8 				timing; 	/* time interval which user want to get back gps locate info */

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_BASE_STATSION,
	 */
	kal_uint8 				bs_num; 	/* base station number which user want to get back */

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_BEARERANDMODE,
	 */
	GPS_Bearer_Type_t		bearer;		/* bearer type */
	GPS_SMS_Mode_Type_t		sms_mode;	/* sms mode */

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_TCPIPSERVER,
	 */
	GPSLocateServerAddr_t	server_ip;	/* server ip address and port */

#ifdef GPS_GSG_SUPPORT
	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SWITCH_SQUAREWAVE,
	 */
	kal_uint8				sq_wave;	/* square wave on/off switch */
#endif

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_GPRS_UPLOAD,
	 */
	kal_uint16				interval;	/* sampling interval */
	kal_uint16				upload_cnt;	/* upload count thru gprs */

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_GPRS_APN,
	 *	GPS_SMS_CMD_SET_GPRS_ACCOUNT,
	 */
	char 					apn[64];
	char 					gpwd[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_GPRS_USRNAME,
	 */
	// temp use new_number[] to store GPRS user name to be set

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SET_GPRS_PASSWORD,
	 */
	// use new_passwd[] to store GPRS user password to be set
	// and srv_passwd[] to store old GPRS user password will be used for checking

#ifdef GPS_POSITION_MONITOR
	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SETPOSITION2,
	 */
	GPS_PostionRange_t		pos_range;
#endif

	/**
	 * The following one field is for the follwing sms cmd: 
	 *	GPS_SMS_CMD_SP_SET_VOLUME,
	 *	GPS_SMS_CMD_SP_SET_ALERTTYPE,
	 *	GPS_SMS_CMD_SP_SET_HANDFREE,
	 *	GPS_SMS_CMD_SETPOSITION,
	 *	GPS_SMS_CMD_SETRATELIMIT,
	 *	GPS_SMS_CMD_SP_SWITCHTRACE,
	 *	GPS_SMS_CMD_SET_MONITOR_PROF,
	 */
	kal_uint32				setting;	/* common setting */
} GPS_Sms_Instruction_t;

typedef Result_t (*GPS_SMS_Cmd_Handler_Func_t)(const GPS_Sms_Instruction_t *instruction);

/** Parse SMS content and get GPS SMS instruction cmd, data */
Result_t GPS_SMS_ParseSMS(const char *sms, GPS_Sms_Instruction_t *instruction);

/** Construct a SMS with msg_buf and send it to the given number */
Result_t GPS_SMS_ConstructAndSendSms(const GPSLocatePhoneNumber_t *p_number, 
									 const char *msg_buf,
									 unsigned short msg_len);

/** Delete all SMS in the storage*/
Result_t GPS_SMS_DeleteAllSms(void);

Result_t GPS_APP_CheckPassword(const GPSLocatePhoneNumber_t *pNumber, const char *passwd);

#endif /* _GPS_APP_SMS_H */
