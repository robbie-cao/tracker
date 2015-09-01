/******************************************************************************
File name: GPSLocateMsgDefs.h
Author: Hongji Zhou
Date: 2007-11-03
******************************************************************************/

#ifndef _GPSLOCATEMSGDEFS_H
#define _GPSLOCATEMSGDEFS_H

#include "kal_release.h"
#include "stack_common.h"
#include "stack_msgs.h"	// for reference of MSG_ID_GPSLOCATE_CODE_BEGIN
#include "app_ltlcom.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"

/*
Major modules:
1. GPS App module/task
2. MT SMS module
3. MT voice call module
4. MO voice call module
5. Key monitor module
6. UART (GPS) monitor module
7. Customer AT command module
8. NVRAM manager module
*/

/*
Message ID definition.

Actually, the following IDs are inserted in the enumeration 'msg_type' in
stack_msgs.h by custom_sap.h

!!!!!! Note !!!!!!
MSG_ID_GPSLOCATE_CODE_BEGIN must be defined in custom_sap.h, together with
the paired ID MSG_ID_GPSLOCATE_CODE_END. The two IDs reserved a range for GPS
locate in stack message ID space.
MSG_ID_GPSLOCATE_CODE_END must not be less than (MSG_ID_GPSLOCATE_Dummy - 1).

On MTK platform, it's recommended that add the following two lines in 
custom_sap.h

MSG_ID_GPSLOCATE_CODE_BEGIN,
MSG_ID_GPSLOCATE_CODE_END = MSG_ID_GPSLOCATE_CODE_BEGIN + 20,

The two lines reserves 6 IDs in the enumeration 'msg_type'.
Once you add new message ID, don't forget to adjust the value of
MSG_ID_GPSLOCATE_CODE_END.
*/
enum
{
	/*
	MSG_ID_GPSLOCATE_MTSMS_REQ

	Usage: 
	MT SMS module sends this message to GPS locate App module, with raw SMS 
	content and sender number. After receiving this message, GPS locate App
	module parse the raw content to find the corresponding instruction and
	take the related action.
	
	Message data structure:
	GPSLocateMTSMSReqStruct_t
	*/
	MSG_ID_GPSLOCATE_MTSMS_REQ = MSG_ID_GPSLOCATE_CODE_BEGIN,
	
	/*
	MSG_ID_GPSLOCATE_MTCALL_REQ

	Usage:
	Once MT call module finds one of the three pre-saved controller numbers 
	calling in, it will send this message to GPS locate App module with the
	caller number. After receiving this message, GPS locate App module do one
	time locating.

	Message data structure:
	GPSLocateMTCallReqStruct_t
	*/
	MSG_ID_GPSLOCATE_MTCALL_REQ,

	/*
	MSG_ID_GPSLOCATE_MOCALL_RSP

	Usage:
	If seeking help phone call isn't accepted, MO call module will send this
	message to GPS locate App module.

	Usage:
	GPSLocateMOCallRsptIndStruct_t
	*/
	MSG_ID_GPSLOCATE_MOCALL_RSP,

	/*
	MSG_ID_GPSLOCATE_KEYPAD_EVENT_IND

	Usage:
	Key monitor sends this message with the key code and pressing type to GPS
	locate App module, to indicate one key event occurrence.

	Message data structure:
	GPSLocateKeypadEventIndStruct_t
	*/
	MSG_ID_GPSLOCATE_KEYPAD_EVENT_IND,

	/*
	MSG_ID_GPSLOCATE_GPS_FRAME_DATA_IND

	Usage:
	GPS module send this message associated with GPS module frame data to GPS
	locate App module. GPS locate App module will parse and process the raw
	data and send the info to requester through sms.

	Message data structure:
	GPSLocateGPSFrameDataIndStruct_t
	*/
	MSG_ID_GPSLOCATE_GPS_FRAME_DATA_IND,

	/*
	MSG_ID_GPSLOCATE_AT_CHANGE_PRESAVED_DATA_REQ

	Usage:
	When customer AT command module receives the customized AT commands, it will
	parse these commands. Then, the module will send this message with the command
	parameter data (new service number, user number or backup settings) to GPS 
	locate App module. According to the received message and its associated data, 
	GPS locate App module changes service number/password, user number/password,
	settings or the default value of settings.

	Message data structure:
	GPSLocateATChangePreSavedDataReqStruct_t
	*/
	MSG_ID_GPSLOCATE_AT_CHANGE_PRESAVED_DATA_REQ,

	/*
	MSG_ID_GPSLOCATE_NW_ATTACH_IND

	Usage:
	When network attached, L4 send this message to GPS locate App task.

	Message data structure:
	GPSLocateNetworkAttachIndStruct_t
	*/
	MSG_ID_GPSLOCATE_NW_ATTACH_IND,

	/*
	MSG_ID_GPSLOCATE_DELETE_SMS_DONE_IND

	Usage:
	Once L4 layer finish deleting SMS, it will send this message to GPS App task to
	indicate it is not busy now.

	Message data structure:
	GPSLocateDeleteSmsDoneIndStruct_t
	*/
	MSG_ID_GPSLOCATE_DELETE_SMS_DONE_IND,

	/*
	MSG_ID_GPSLOCATE_SEND_SMS_DONE_IND

	Usage:
	Once L4 layer finish sending SMS, it will send this message to GPS App task to
	report the result of the sending.

	Message data structure:
	GPSLocateSendSmsDoneIndStruct_t
	*/
	MSG_ID_GPSLOCATE_SEND_SMS_DONE_IND,
	

	/*
	MSG_ID_GPSLOCATE_SEND_THRU_GPRS_REQ

	Usage:
	Once GPS APP task get gprmc frame or GPS UART task get gprmc data timeout, 
	they will send this message to GPS TCPIP task to
	send the result to tcp/ip server through GPRS.

	Message data structure:
	GPSLocateSendThruGPRSReqStruct_t
	*/
	MSG_ID_GPSLOCATE_SEND_THRU_GPRS_REQ,

	/*
	MSG_ID_GPSLOCATE_REGSTATEINFO_IND

	Usage:
	Once L4 layer request LAI info, it will send this message to GPS App task to
	report the current REGSTATE info.

	Message data structure:
	GPSLocateRegStateStruct_t
	*/
	MSG_ID_GPSLOCATE_REGSTATEINFO_IND,

	/*
	MSG_ID_GPSLOCATE_BATTERY_STATUS_IND

	Usage:
	L4 layer report the battery status through this message.

	Message data structure:
	GPSLocateBatteryStatusIndStruct_t
	*/
	MSG_ID_GPSLOCATE_BATTERY_STATUS_IND,

	/*
	MSG_ID_GPSLOCATE_SIMPLOC_TIMEOUT_IND

	Usage:
	ostimer hisr report single locate req timout through this message.

	Message data structure:
	NULL
	*/
	MSG_ID_GPSLOCATE_SIMPLOC_TIMEOUT_IND,

#ifdef GPS_POSITION_MONITOR
	/*
	MSG_ID_GPSLOCATE_SETPOS_TIMEOUT_IND

	Usage:
	ostimer hisr report set position req timout through this message.

	Message data structure:
	NULL
	*/
	MSG_ID_GPSLOCATE_SETPOS_TIMEOUT_IND,
#endif

	/*
	MSG_ID_GPSLOCATE_SAMPLINGGPRMC_IND

	Usage:
	GPS App task send this message to UART2 task to indicate UART2 task 
	to send GPRMC data to GPS App task.

	Message data structure:
	GPSLocateSamplingGprmcIndStruct_t
	*/
	MSG_ID_GPSLOCATE_SAMPLINGGPRMC_IND,

#ifdef GPS_DEFENCE_FEATURE
	/*
	MSG_ID_GPSLOCATE_TRIGGER_DEFENCE

	Usage:
	Defence monitor HISR send this message to GPS App task to indicate that
	defence is triggered.

	Message data structure:
	None
	*/
	MSG_ID_GPSLOCATE_TRIGGER_DEFENCE,
#endif

	/*
	MSG_ID_GPSLOCATE_SYSTEM_SHUTTING_DOWN_IND

	Usage:
	System module sends this message to GPS App task to indicate that the
	system is shutting down.

	Message data structure:
	None
	*/
	MSG_ID_GPSLOCATE_SYSTEM_SHUTTING_DOWN_IND,

	/*
	MSG_ID_GPSLOCATE_BACKUPLOADTIMER_IND

	Usage:
	Timer HISR sends this message to GPS App task to indicate that
	back upload timer arrive.

	Message data structure:
	None
	*/
	MSG_ID_GPSLOCATE_BACKUPLOADTIMER_IND,

	/*
	MSG_ID_GPSLOCATE_PPBUFFULL4LOADING_IND

	Usage:
	Ping-pong buffer callback sends this message to GPS App task to indicate that
	ping/pong buffer full for loading.

	Message data structure:
	GPSLocatePPBufFull4LoadingIndStruct_t
	*/
	MSG_ID_GPSLOCATE_PPBUFFULL4LOADING_IND,

#ifdef VIBRATION_SENSOR_SUPPORT
	/*
	MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND

	Usage:
	Report vibration sensor status

	Message data structure:
	GPSLocateVibSensorStatusIndStruct_t
	*/
	MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND,

#endif //VIBRATION_SENSOR_SUPPORT

	MSG_ID_GPSLOCATE_NVRAMSTORE_IND,

	MSG_ID_GPSLOCATE_SWITCHGPRSUPLD,

#ifdef GPS_BACKUPLOAD_DAEMON
	MSG_ID_GPSLOCATE_DELCURRBAKF_IND,
#endif

	/*
	MSG_ID_GPSLOCATE_Dummy

	!!!!!! Note !!!!!!
	This ID is just a indicator for the definition end. Don't reference it!
	Add your new message ID before this dummy ID.
	*/
	MSG_ID_GPSLOCATE_Dummy
};

/*
GPS locate MT call request category enumeration
*/
typedef enum
{
	/*
	Simple locate
	*/
	GPSLOCATE_MTCALL_REQ_CATEGORY_SIMPLE_LOCATE = 0,
	/*
	Auto answer
	*/
	GPSLOCATE_MTCALL_REQ_CATEGORY_AUTO_ANSWERED,
	/*
	Call connect
	*/
	GPSLOCATE_MTCALL_REQ_CATEGORY_CALL_CONNECT,
	/*
	Remote released
	*/
	GPSLOCATE_MTCALL_REQ_CATEGORY_REMOTE_RELEASED,

	GPSLOCATE_MTCALL_REQ_CATEGORY_Total
} GPSLocateMTCallReqCategory_t;

/*
GPS locate MO call response category enumeration
*/
typedef enum
{
	/*
	No accept
	*/
	GPSLOCATE_MOCALL_RSP_CATEGORY_NOACCEPT = 0,
	/*
	Make call to next number
	*/
	GPSLOCATE_MOCALL_RSP_CATEGORY_CALLNEXT,
	/*
	Answer
	*/
	GPSLOCATE_MOCALL_RSP_CATEGORY_ANSWER,
	/*
	Disconnect
	*/
	GPSLOCATE_MOCALL_RSP_CATEGORY_DISCONNECT,
	/*
	Remote released
	*/
	GPSLOCATE_MOCALL_RSP_CATEGORY_RELEASED,

	GPSLOCATE_MOCALL_RSP_CATEGORY_Total
} GPSLocateMOCallRspCategory_t;

typedef enum
{
	/*
	MT Call
	*/
	GPSLOCATE_CALLTYPE_MT = 0,
	/*
	MO Call
	*/
	GPSLOCATE_CALLTYPE_MO,
	/*
	UNKOWN Call Type
	*/
	GPSLOCATE_CALLTYPE_UNKNOWN,

	GPSLOCATE_CALLTYPE_Total
} GPSLocateCallType_t;

/*
GPS locate key code enumeration for key event
*/
typedef enum
{
	/*
	SOS, seek help
	*/
	GPSLOCATE_KEYCODE_SOS = 0,

	/*
	End key
	*/
	GPSLOCATE_KEYCODE_END,
	
	GPSLOCATE_KEYCODE_Total
} GPSLocateKeyCode_t;

/*
GPS locate key type enumeration for key event
*/
typedef enum
{
	/*
	Normal press
	*/
	GPSLOCATE_KEYTYPE_PRESS = 0,
	/*
	Long press
	*/
	GPSLOCATE_KEYTYPE_LONGPRESS,

	GPSLOCATE_KEYTYPE_Total
} GPSLocateKeyType_t;

typedef enum
{
	/*
	Change service number
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVICENUMBER = 0,
	/*
	Change service password
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVICEPASSWORD,
	/*
	Change user number
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERNUMBER,
	/*
	Change user password
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERPASSWORD,
	/*
	Change sos number
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SOSNUMBER,
	/*
	Change settings
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SETTINGS,
	/*
	Change default settings
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SETTINGSDEFAULT,
	/*
	Change shared user password
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SHAREUSRPWD,
	/*
	Change server address
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVERADDR,
	/*
	Change current working mode: sms p2p, sms service center or gprs mode
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_WORKINGMODE,
	/*
	Change number of request base station info
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_BSNUMBER,
	/*
	Change settings for upload GPRMC through GPRS
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSUPLOAD,
	/*
	Change GPRS user
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSUSER,
	/*
	Change GPRS password
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSPWD,
	/*
	Change APN
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSAPN,
	/*
	Change GPRS Account
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSACC,
	/*
	Turn on/off log
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPSLOG,
	/*
	Turn on/off handfree
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_HANDFREE,
	/*
	Set gps module working state
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPSPROF,
	/*
	Set mt call profile: normal/silent
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_MTCALLPROF,
	/*
	Set sos call on/off
	*/
	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SOSCALL,

	GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_Total
} GPSLocateATChangePreSavedDataReqCategory_t;

/*
EM status ind category enumeration
*/
typedef enum
{
	GPSLOCATE_RR_EM_LAI_INFO,
	GPSLOCATE_RR_EM_MEASUREMENT_REPORT_INFO,

	GPSLOCATE_RR_EM_CATEGORY_Total
} GPSLocateEMStatusIndCategory_t;

/*
GPS locate network status enumeration
Actually, this enumeration is an aliased copy of MMIRACRESPONSEENUM 
which is defined in SimDetectionDef.h
*/
typedef enum
{
	GPSLOCATE_NWSTATUS_OK = 0,		//MMI_RAC_OK
	GPSLOCATE_NWSTATUS_NO_CELL,		//MMI_RAC_NO_CELL
	GPSLOCATE_NWSTATUS_LIMITED_SERVICE,	//MMI_RAC_LIMITED_SERVICE
	GPSLOCATE_NWSTATUS_ERROR,		//MMI_RAC_ERROR
	GPSLOCATE_NWSTATUS_INVALID_SIM,	//MMI_RAC_INVALID_SIM
	GPSLOCATE_NWSTATUS_ATTEMPT_TO_UPDATE,	//MMI_RAC_ATTEMPT_TO_UPDATE
	GPSLOCATE_NWSTATUS_SEARCHING,		//MMI_RAC_SEARCHING
	
	GPSLOCATE_NWSTATUS_Invalid
} GPSLocateNetworkStatus_t;

/*
GPS locate network GPRS status
Actually, this enumeration is an aliased copy of l4c_gprs_status_enum
which is defined in ps\l4\include\l4c_common_enum.h
*/
typedef enum
{
	GPSLOCATE_NWGPRS_GPRS_ATTACHED = 0,	//L4C_GPRS_ATTACHED
	GPSLOCATE_NWGPRS_NONE_GPRS,			//L4C_NONE_GPRS
	GPSLOCATE_NWGPRS_GPRS_DETACHED,		//L4C_GPRS_DETACHED
	GPSLOCATE_NWGPRS_INVALID_SIM,			//L4C_INVALID_SIM
	GPSLOCATE_NWGPRS_GPRS_ATTEMPT_ATTACH,//L4C_GPRS_ATTEMPT_ATTACH
	GPSLOCATE_NWGPRS_GPRS_COVERAGE,		//L4C_GPRS_COVERAGE
	GPSLOCATE_NWGPRS_PDP_ATTEMPT_ACT,		//L4C_PDP_ATTEMPT_ACT
	GPSLOCATE_NWGPRS_PDP_DEACTIVED,		//L4C_PDP_DEACTIVED
	GPSLOCATE_NWGPRS_PDP_ACTIVED,			//L4C_PDP_ACTIVED
	
	GPSLOCATE_NWGPRS_Invalid
} GPSLocateNetworkGPRSStatus_t;

/*
GPS locate result
*/
typedef enum
{
	GPSLOCATE_FALSE = 0,
	GPSLOCATE_TRUE
} GPSLocateResult_t;

/*
GPS locate phone number structure
This structure is same as l4c_number_struct, which is defined in
ps\l4\include\l4c_aux_struct.h
*/
typedef struct
{
	unsigned char Type;
	unsigned char Length;
	unsigned char Number[GPSLOCATE_PHONE_NUMBER_BUFFER_LEN];
} GPSLocatePhoneNumber_t;

/*
GPS locate battery status enumeration
This enumeration is same as pmic_status_enum, which is defined
in ps\l4\include\device.h
*/
typedef enum
{
	GPSLOCATE_VBAT_STATUS,    /* Notify the battery voltage, BMT_VBAT_STATUS */
	GPSLOCATE_CHARGER_IN,     /* Charger plug in, BMT_CHARGER_IN */
	GPSLOCATE_CHARGER_OUT,    /* Charger plug out, BMT_CHARGER_OUT */
	GPSLOCATE_OVERVOLPROTECT,    /* The voltage of battery is too high. BMT_OVERVOLPROTECT */
	GPSLOCATE_OVERBATTEMP,    /* The temperature of battery is too high. BMT_OVERBATTEMP */
	GPSLOCATE_LOWBATTEMP,     /* The temperature of battery is too low. BMT_LOWBATTEMP */
	GPSLOCATE_OVERCHARGECURRENT, /* Charge current is too large. BMT_OVERCHARGECURRENT */
	GPSLOCATE_CHARGE_COMPLETE,   /* Charge is completed. BMT_CHARGE_COMPLETE */
	GPSLOCATE_INVALID_BATTERY,    /* invalid battery  BMT_INVALID_BAT*/
	GPSLOCATE_INVALID_CHARGER,   /* invalid charger BMT_INVALID_CHARGER*/
	GPSLOCATE_CHARGING_TIMEOUT, /* Bad battery BMT_CHARGE_TIMEOUT */ 
	GPSLOCATE_LOWCHARGECURRENT, /* Charge current is too Low. BMT_LOWCHARGECURRENT */
	GPSLOCATE_CHARGE_BAD_CONTACT, /* Charger Bad Contact */
	GPSLOCATE_BATTERY_BAD_CONTACT, /* Battery Bad Contact */
	GPSLOCATE_USB_CHARGER_IN,   /* Usb Charger plug in */
	GPSLOCATE_USB_CHARGER_OUT,   /* Usb Charger plug out */
	GPSLOCATE_USB_NO_CHARGER_IN,
	GPSLOCATE_USB_NO_CHARGER_OUT
} GPSLocateBattStatus_t;

/*
GPS locate battery level enumeration
This enumeration is same as battery_level_enum, which is defined
in ps\l4\include\device.h
*/
typedef enum 
{
	GPSLOCATE_BATTERY_LOW_POWEROFF = 0,
	GPSLOCATE_BATTERY_LOW_TX_PROHIBIT,
	GPSLOCATE_BATTERY_LOW_WARNING,
	GPSLOCATE_BATTERY_LEVEL_0,
	GPSLOCATE_BATTERY_LEVEL_1,
	GPSLOCATE_BATTERY_LEVEL_2,
	GPSLOCATE_BATTERY_LEVEL_3, 
	GPSLOCATE_BATTERY_LEVEL_4 = GPSLOCATE_BATTERY_LEVEL_3, /* BATTERY_LEVEL_4 */
	GPSLOCATE_BATTERY_LEVEL_5 = GPSLOCATE_BATTERY_LEVEL_3, /* BATTERY_LEVEL_5 */
	GPSLOCATE_BATTERY_LEVEL_6 = GPSLOCATE_BATTERY_LEVEL_3, /* BATTERY_LEVEL_6 */
	GPSLOCATE_BATTERY_LEVEL_LAST = 9
} GPSLocateBattLevel_t;

#ifdef VIBRATION_SENSOR_SUPPORT
typedef enum
{
	GPSLOCATE_VIBSENSORSTATUS_VIBRATING,
	GPSLOCATE_VIBSENSORSTATUS_SILENCE
}GPSLocateVibSensorStatus_t;
#endif //VIBRATION_SENSOR_SUPPORT

#ifndef LOCAL_PARA_HDR
#define LOCAL_PARA_HDR \
	kal_uint8		ref_count; \
	kal_uint16	msg_len;
#endif //LOCAL_PARA_HDR

/*
Message data structure for MSG_ID_GPSLOCATE_MTSMS_REQ
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocatePhoneNumber_t RemoteNumber;
	unsigned short MsgDataLen;
	unsigned char MsgData[GPSLOCATE_SMS_MSGDATA_BUFFER_LEN];	
} GPSLocateMTSMSReqStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_MTCALL_REQ
*/
typedef struct 
{
	LOCAL_PARA_HDR
	GPSLocateMTCallReqCategory_t ReqCategory;
	GPSLocatePhoneNumber_t RemoteNumber;
} GPSLocateMTCallReqStruct_t;

typedef enum
{
	GPSLOCATE_MOCALLSTATE_IDLE = 0,
	GPSLOCATE_MOCALLSTATE_CONNECTING,
	GPSLOCATE_MOCALLSTATE_CONNECTED,
	GPSLOCATE_MOCALLSTATE_WAITINGNEXT,

	GPSLOCATE_MOCALLSTATE_TOTAL
} GPSLocateMOCallState_t;

/*
Message data structure for MSG_ID_GPSLOCATE_MOCALL_RSP
*/
typedef struct 
{
	LOCAL_PARA_HDR
	GPSLocateMOCallRspCategory_t RspCategory;
	GPSLocatePhoneNumber_t RemoteNumber;
} GPSLocateMOCallRsptIndStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_KEYPAD_EVENT_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateKeyCode_t KeyCode;
	GPSLocateKeyType_t KeyType;
} GPSLocateKeypadEventIndStruct_t;


#define GPSLOCATE_GPRMCOBJECTIVE_UNKNOWN		0x0000
#define GPSLOCATE_GPRMCOBJECTIVE_SMSLOC			0x0001	/* GPRMC frame will be used for sms location require */
#define	GPSLOCATE_GPRMCOBJECTIVE_GPRSUPLD		0x0002	/* GPRMC frame will be used for gprs upload */
#ifdef GPS_POSITION_MONITOR
#define GPSLOCATE_GPRMCOBJECTIVE_POSSET			0x0004	/* GPRMC frame will be used for position set for monitor */
#define GPSLOCATE_GPRMCOBJECTIVE_POSMONITOR		0x0008	/* GPRMC frame will be used for position monitor */
#endif
#ifdef GPS_RATE_MONITOR
#define GPSLOCATE_GPRMCOBJECTIVE_RATEMONITOR	0x0010	/* GPRMC frame will be used for rate monitor */
#endif
#ifdef GPS_MOD_SWAUTOPOWER
#define GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER		0x0020	/* GPRMC frame will be used for check rmc valid or not */
#endif
#define GPSLOCATE_GPRMCOBJECTIVE_GPRSUPLDIMM	0x8000	/* GPRMC frame will be used for upload through gprs immediately */

typedef kal_uint16		GPSLocateGprmcObjective_t;


/*
Message data structure for MSG_ID_GPSLOCATE_GPSMODULE_DATA_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	/*
	According to GPS module specification, frame data is of Ansii string
	format and ended with '\0'. Thus it is no need to define another field
	to indicate the length of frame data.
	*/
	char FrameData[GPSLOCATE_GPS_FRAME_DATA_BUFFER_LEN];
	int FrameDataLen;
	GPSLocateGprmcObjective_t Objective;
} GPSLocateGPSFrameDataIndStruct_t;

/*
 * TCP/IP Server ip and port structure
 */
typedef struct {
	kal_uint8	addr[4];
	kal_uint16	port;
} GPSLocateServerAddr_t;

/*
Message data structure for MSG_ID_GPSLOCATE_AT_CHANGE_PRESAVED_DATA_REQ
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateATChangePreSavedDataReqCategory_t ReqCategory;
	/*
	This index indicates which user number or password to be changed.
	*/
	unsigned char Index;
	union {
		char ServiceNumber[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
		char ServicePassword[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
		char UserNumber[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
		char UserPassword[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
		char SosNumber[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
		char SharedUsrPwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
		char APN[GPSLOCATE_PRESAVED_APN_BUFFER_LEN];
		unsigned char Settings;
		unsigned char SettingsBackup;
		unsigned char WorkingMode;
		unsigned char BSNumber;
		unsigned int GprsUploadSetting;
		GPSLocateServerAddr_t ServerAddr;
		// use ServiceNumber to store GPRS user name
		// use ServicePassword to store GPRS password
		// use Settings to store log on/off switch
		// use Settings to store handfree on/off switch
	} Data;
	char GAccountUser[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
	char GAccountPwd[GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN];
} GPSLocateATChangePreSavedDataReqStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_NW_ATTACH_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	/*
	Status
	*/
	GPSLocateNetworkStatus_t Status;
	/*
	PLMN
	null-terminated string
	*/
	char PLMN[GPSLOCATE_NWPLMN_BUFFER_LEN];
	/*
	GSM state
	*/
	unsigned char GSMState;
	/*
	GPRS state
	*/
	unsigned char GPRSState;
	/*
	GPRS status
	*/
	GPSLocateNetworkGPRSStatus_t GPRSStatus;
} GPSLocateNetworkAttachIndStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_DELETE_SMS_DONE_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateResult_t Result;
} GPSLocateDeleteSmsDoneIndStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_SEND_SMS_DONE_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateResult_t Result;
} GPSLocateSendSmsDoneIndStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_SEND_THRU_GPRS_REQ
*/
typedef struct
{
	LOCAL_PARA_HDR
	kal_uint8 ip_addr[4];
	kal_uint16 port;
	kal_uint8 *data;
	kal_uint16 data_len;
	kal_uint8 cause_type;
	kal_uint8 src_type;
} GPSLocateSendThruGPRSReqStruct_t;

#define GSM_NEIGHBOR_CELL_MAX		6
/*
Message Data structure for MSG_ID_GPSLOCATE_REGSTATEINFO_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
   GPSLocateEMStatusIndCategory_t IndCategory;

   // for RR_EM_LAI_INFO
   kal_uint8				mcc[3];
   kal_uint8				mnc[3];
   kal_uint16				cell_id;  	// for +CREG, +CGREG
   kal_uint16				lac_value;	// for +CREG, +CGREG

   // for RR_EM_MEASUREMENT_REPORT_INFO
   kal_uint16 				nc_arfcn[GSM_NEIGHBOR_CELL_MAX+1];    /* Top 12 */
   kal_int16				rla_in_quarter_dbm[GSM_NEIGHBOR_CELL_MAX+1];	//rssi level for each carrier
   kal_int16				C1[GSM_NEIGHBOR_CELL_MAX+1];		//for neighbor cell1
   kal_int16				C2[GSM_NEIGHBOR_CELL_MAX+1];		//neighbor cell2	
} GPSLocateRegStateStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_BATTERY_STATUS_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateBattStatus_t 	Status;
	GPSLocateBattLevel_t	Level;
} GPSLocateBatteryStatusIndStruct_t;

/*
GPS locate gprmc info destination
*/
typedef enum
{
	GPSLOCATE_GPRMCDEST_UNKNOWN,
	GPSLOCATE_GPRMCDEST_APPTASK,   /* GPRMC frame will send to APP task */
	GPSLOCATE_GPRMCDEST_TCPIP,     /* GPRMC frame will send to TCPIP task */

	GPSLOCATE_GPRMCDEST_TOTAL
} GPSLocateGprmcDest_t;

/*
Message data structure for MSG_ID_GPSLOCATE_SAMPLINGGPRMC_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateGprmcDest_t Dest;
	GPSLocateGprmcObjective_t Objective;
	kal_bool	Flag;
} GPSLocateSamplingGprmcIndStruct_t;

/*
Message data structure for MSG_ID_GPSLOCATE_PPBUFFULL4LOADING_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	void* buff;
	kal_uint16 buff_len;
	void *info;
} GPSLocatePPBufFull4LoadingIndStruct_t;

#ifdef VIBRATION_SENSOR_SUPPORT
/*
Message data structure for MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND
*/
typedef struct
{
	LOCAL_PARA_HDR
	GPSLocateVibSensorStatus_t Status;
} GPSLocateVibSensorStatusIndStruct_t;
#endif

/*
Function: SendMsg2GPSApp
Usage: Send message to GPS App task
Parameters:
	SrcMode		-	source module ID
	MsgID		-	message ID
	pLocalData	-	pointer to local data memory
					This block of memory should be allocated by the caller
					invoking 'construct_local_para'.
	DataLen		-	length of the local data structure
*/
void SendMsg2GPSApp(
			const unsigned short SrcMod,
			const unsigned short MsgID,
			void* pLocalData
			);

/*
Function: SendUrgentMsg2GPSApp
Usage: Send urgent message to GPS App task
Parameters:
	SrcMode		-	source module ID
	MsgID		-	message ID
	pLocalData	-	pointer to local data memory
					This block of memory should be allocated by the caller
					invoking 'construct_local_para'.
*/
void SendUrgentMsg2GPSApp(
			const unsigned short SrcMod,
			const unsigned short MsgID,
			void* pLocalData
			);

/*************************************************************************
* Function: SetGprmcDestAndObjective
* Usage: Send message to GPS UART task to set GPRMC frame dest and flag
* Parameters:
*	SrcMode		-	source module ID
*	Dest		-	destination which will receive GPRMC frame
*	Objective	-	objective which GPRMC will be used for
*	Flag		-	TRUE/FALSE, send GPRMC frame to destination or not
*************************************************************************/
void SetGprmcDestAndObjective(
			const unsigned short SrcMod,
			GPSLocateGprmcDest_t Dest,
			GPSLocateGprmcObjective_t Objective,
			kal_bool Flag
			);
kal_bool GPSAppTaskIsReady(void);


#endif /* _GPSLOCATEMSGDEFS_H */

