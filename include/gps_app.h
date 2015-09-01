/******************************************************************************
 * File name: gps_app.h
 * Author: Robbie Cao
 * Date: 2007-11-12 10:08
 ******************************************************************************/

#ifndef _GPS_APP_H
#define _GPS_APP_H

#include "kal_release.h"
#include "stack_common.h"
#include "stack_msgs.h"	// for reference of MSG_ID_GPSLOCATE_CODE_BEGIN
#include "app_ltlcom.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "parser.h"
#include "gps_app_sms.h"
#include "gps_app_nema.h"
#include "gps_app_settings.h"
#include "gps_app_util.h"
#include "gps_app_event_fifo.h"
#include "gps_app_timer_mgr.h"

enum
{
	GPS_RMCSMS_TYPE_SMS,
	GPS_RMCSMS_TYPE_TIMER,
	GPS_RMCSMS_TYPE_CALL,
	GPS_RMCSMS_TYPE_ANSWER,
	GPS_RMCSMS_TYPE_SOS,
	GPS_RMCSMS_TYPE_ALARM,
	GPS_RMCSMS_TYPE_GEOOS,
	GPS_RMCSMS_TYPE_GEORS,
	GPS_RMCSMS_TYPE_OVERSPEED,
	GPS_RMCSMS_TYPE_SAFESPEED,
	GPS_RMCSMS_TYPE_LP,	/* lp is the last one */

	GPS_RMCSMS_TYPE_TOTAL
};

enum
{
	GPS_MODULE_POWERSTATE_ON,
	GPS_MODULE_POWERSTATE_OFF,
	GPS_MODULE_POWERSTATE_AUTO,
	GPS_MODULE_POWERSTATE_SWAUTO,

	GPS_MODULE_POWERSTATE_TOTAL
};


/** Construct reply sms from cell info */
void GPS_APP_ConstructCellInfoSMS(char *dest, kal_uint8 bs_count, char sep_sign);

/** Construct reply sms from cell info */
void GPS_APP_ConstructCellInfoSMS2(char *dest, kal_uint8 bs_count);

/** Construct reply sms from GPRMC data */
Result_t GPS_APP_ConstructGprmcSMS(char *dest, GPS_GPRMC_RawData_t *gprmc_data, char sep_sign);

/** Construct reply sms from GPRMC data */
Result_t GPS_APP_ConstructReadableGprmcSMS(char *dest, GPS_GPRMC_RawData_t *gprmc_data, kal_uint8 type);


/** Get timing locate flag */
kal_bool GPS_APP_GetTimingLocateFlag(void);

/** Set timing locate flag */
void GPS_APP_SetTimingLocateFlag(kal_bool flag);

/** Get sending multiple sms flag */
kal_bool GPS_APP_GetMultiSendFlag(void);

/** Set sending multiple sms flag */
void GPS_APP_SetMultiSendFlag(kal_bool flag);

/** Get vbat low warning flag */
kal_bool GPS_APP_GetVbatWarnFlag(void);

/** Set vbat low warning flag */
void GPS_APP_SetVbatWarnFlag(kal_bool flag);

/*
 * GPS locate state
 * 
 * state machine:
 *
 * IDLE<----------------------+
 *  |                         |
 *  |             -----+      |
 *  |             |    |      |
 *  V             V    |      |
 * START------>REPEAT--+      |
 *  | |          | |          |
 *  | |          | |          |
 *  | +--------+ | |          |
 *  |          | | +---+      |
 *  V          V V     |      |
 * TIMEOUT---->DONE----|------+
 *  ^                  |
 *  |                  |
 *  +------------------+
 *
 */
typedef enum
{
	GPS_LOCATE_STATE_IDLE,
	GPS_LOCATE_STATE_START,
	GPS_LOCATE_STATE_REPEAT,
	GPS_LOCATE_STATE_TIMEOUT,
	GPS_LOCATE_STATE_DONE
} GPS_Locate_State_t;

typedef enum
{
	GPSMODULEPWRID_GPS = 0,

#if (defined(CODEPROTECTOR_FS8816_SUPPORT) && FS8816_PWR_CTRL && (FS8816_PWR_CTRL_GPIO == GPS_MODULE_ENALBE_GPIO))
	GPSMODULEPWRID_FS8816,
#endif // defined(CODEPROTECTOR_FS8816_SUPPORT) && FS8816_PWR_CTRL && (FS8816_PWR_CTRL_GPIO == GPS_MODULE_ENALBE_GPIO)

	GPSMODULEPWRID_Total
} GPSModulePwrID_t;

/** Get locate state */
GPS_Locate_State_t GPS_APP_GetLocateState(void);

/** Set locate state */
void GPS_APP_SetLocateState(GPS_Locate_State_t state);

/** Start a single locate requirement */
void GPS_APP_SingleLocateStart(const GPSLocatePhoneNumber_t *pNumber, kal_uint8 cause_type);

/** Stop single locate requirement timer */
void GPS_APP_SingleLocateStop(void);

#ifdef GPS_POSITION_MONITOR
/** Start require current position and set it to the point to be monitored */
void GPS_APP_SetPositionStart(const GPSLocatePhoneNumber_t *pNumber);

/** Stop require current position timer */
void GPS_APP_SetPositionStop(void);

/** Start position monitor timer */
void GPS_APP_PosMonitorStart(void);

/** Stop position monitor timer */
void GPS_APP_PosMonitorStop(void);

/** Start sampling gprmc frame for position monitor */
void GPS_APP_SampGprmc4PosMonitorStart(void);

/** Position monitor repeat handler */
void GPS_APP_PosMonitorRepeatHandler(GPSAppTimerID_t Id);
#endif

#ifdef GPS_RATE_MONITOR
/** Start rate monitor timer */
void GPS_APP_RateMonitorStart(void);

/** Stop rate monitor timer */
void GPS_APP_RateMonitorStop(void);

/** Start sampling gprmc frame for rate monitor */
void GPS_APP_SampGprmc4RateMonitorStart(void);

/** Rate monitor repeat handler */
void GPS_APP_RateMonitorRepeatHandler(GPSAppTimerID_t Id);
#endif

/** Timing locate require repeat handler */
void GPS_APP_TimingLocateRepeatHandler(GPSAppTimerID_t Id);

/** Start sampling gprmc frame requirement */
void GPS_APP_SampGprmcStart(void);

/** Sample gprmc frame require repeat handler */
void GPS_APP_SampGprmcRepeatHandler(GPSAppTimerID_t Id);

/** Start sampling gprmc frame requirement and send it through gprs immediately */
void GPS_APP_SampGprmcAndSendStart(void);

#ifdef GPS_BACKUPLOAD_DAEMON
/** Upload backup missing gprmc data for repeat handler */
void GPS_APP_UploadBackupDataRepeatHandler(GPSAppTimerID_t Id);
#endif

typedef enum
{
	GPS_SAVEDNUMBERTYPE_USER,
	GPS_SAVEDNUMBERTYPE_SOS,

	GPS_SAVED_NUMBER_TYPE_TOTAL
} GPS_SavedNumberType_t;

/** Make call and send sms to first number after system power on handler */
void GPS_APP_MakeCallAndSendSmsHandler(GPSAppTimerID_t Id);

/** Start defense send SMS */
void GPS_APP_DefSendSmsStart(GPS_SavedNumberType_t type);

/** Start geo defense send SMS */
void GPS_APP_GeoSendSmsStart(GPS_SavedNumberType_t type, kal_bool out);

/** Start SOS help send SMS */
void GPS_APP_SosSendSmsStart(GPS_SavedNumberType_t type);

/** Start low battery warning send SMS */
void GPS_APP_LowBattWarnSendSmsStart(GPS_SavedNumberType_t type);

/** Start SOS help MO Call */
void GPS_APP_SosMOCallStart(GPS_SavedNumberType_t type);

#ifdef GPS_POWERON_PHONECALL
/** Start make MO call and send GPS info to the first user number when system boot up */
void GPS_APP_MakeCallAndSndSmsStart(void);
#endif

#if GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
void 
#else //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
kal_bool 
#endif //GPSLOCATE_INDICATE_UNACCEPTED_MTCALL
GPS_APP_MTCallRingTimerStart(GPSLocatePhoneNumber_t *pNumber);
void GPS_APP_MTCallRingTimerStop(void);
void GPS_APP_MOCallDialTimerStart(char *pNumber);
void GPS_APP_MOCallDialTimerStop(void);
void GPS_APP_MOCallDialNextNumber(GPSAppTimerID_t Id);
#ifdef GPS_DEFENCE_FEATURE
void GPS_APP_DefenceTimerStop(void);
void GPS_APP_SetDefenceON(void);
#endif

#ifdef GPS_MOD_SWAUTOPOWER
void GPS_APP_ModSWPowerTimerStart(void);
void GPS_APP_ModSWPowerTimerStop(void);
#endif

void GPS_APP_GetNWInfoHandler(GPSAppTimerID_t Id);

#ifdef GPS_MOTOPWRCUT_FEATURE
void GPS_APP_MotoPowerCutTimerStart(void);
void GPS_APP_MotoPowerCutTimerStop(void);
void GPS_APP_MotoPowerOnTimerStart(void);
void GPS_APP_MotoPowerOnTimerStop(void);
#endif

/*
 * The following functions enable/disable GPS module
 */

/** Check GPS module on/off */
kal_bool GPS_APP_CheckGPSModuleOn(void);

/** GPS module power ctrl */
void GPS_APP_GPSModulePwrCtrl(kal_bool On, GPSModulePwrID_t Id);

/*************************************************************************
 * GPS APP MISC
 *************************************************************************/
void GPS_APP_SetUart2Sleep(kal_bool sleep); 	// sleep = 0, wakeup uart2, 1, sleep uart2
void GPS_APP_Init_All(void);
void GPS_APP_Restore_All(void);
void GPS_APP_EventFifoHandler(GPSAppEvent_t *pEvent);

kal_bool GPS_APP_AllowShutdownSystem(void);

kal_uint8 GPS_APP_GetCallType(void);
void GPS_APP_SetCallType(kal_uint8 call_type);

typedef enum
{
	GPS_GPRS_UPLDCAUSETYPE_SMS,
	GPS_GPRS_UPLDCAUSETYPE_CALL,
	GPS_GPRS_UPLDCAUSETYPE_ANSWER,
	GPS_GPRS_UPLDCAUSETYPE_AUTO,
	GPS_GPRS_UPLDCAUSETYPE_AUTOLOW,
	GPS_GPRS_UPLDCAUSETYPE_SOS,
	GPS_GPRS_UPLDCAUSETYPE_GEOOS,
	GPS_GPRS_UPLDCAUSETYPE_GEORS,
	GPS_GPRS_UPLDCAUSETYPE_OVERSPEED,
	GPS_GPRS_UPLDCAUSETYPE_SAFESPEED,
	GPS_GPRS_UPLDCAUSETYPE_DEF,
#ifdef GPS_BACKUPLOAD_DAEMON
	GPS_GPRS_UPLDCAUSETYPE_BACKUP,
#endif
	GPS_GPRS_UPLDCAUSETYPE_LP,

	GPS_GPRS_UPLDCAUSETYPE_TOTAL
} GPS_GPRS_UploadCauseType_t;

typedef enum
{
	GPS_GPRS_UPLDSRCTYPE_STBUFF,
	GPS_GPRS_UPLDSRCTYPE_LDBUFF,
	GPS_GPRS_UPLDSRCTYPE_GPSUART,
#ifdef GPS_BACKUPLOAD_DAEMON
	GPS_GPRS_UPLDSRCTYPE_BACKUP,
#endif

	GPS_GPRS_UPLDSRCTYPE_TOTAL
} GPS_GPRS_UploadSrcType_t;

typedef enum
{
	//normal
	GPSMTCALLTYPE_NORMAL,
	//monitor
	GPSMTCALLTYPE_MONITOR,

	GPSMTCALLTYPE_Total
}GPSMtcallType_t;

typedef struct
{
	kal_uint8 cause_type;	/* GPS_GPRS_UploadCauseType_t */
	kal_uint8 src_type;		/* GPS_GPRS_UploadSrcType_t */
} GPS_GPRS_UploadInfo_t;

void GPS_APP_SendPackData2TCPIP(
		const GPS_GPRMC_Packed_Struct_t* buff, 
		kal_uint16 buff_len, 
		void *info);		/* GPS_GPRS_UploadInfo_t */
void GPS_APP_PPBufferFullCB(
		const GPS_GPRMC_Packed_Struct_t* buff, 
		kal_uint16 buff_len, 
		void *info);
kal_bool GPS_APP_StartGprsUpload(GPS_GPRS_UploadCauseType_t cause_type);

#ifdef GPS_BACKUPLOAD_DAEMON
void GPS_APP_StoreBackupData(kal_uint8 type);
#endif

kal_bool GPS_TraceOn(void);
void GPS_SetTrace(kal_bool on);

void GPS_ListAllSettings1(char *dest);
void GPS_ListAllSettings2(char *dest);
void GPS_ListAllSettings3(char *dest);
void GPS_ListAllSettings4(char *dest);

void GPS_APP_RejectUnindicatedMTCall(void);
void GPS_APP_CancelDialing(void);
void GPS_APP_HangupActiveMOCall(void);
void GPS_APP_SetOngoingCallSpch(GPSMtcallType_t type);

kal_bool GPS_APP_OutofRange(GPS_PostionRange_t *fix_pos, GPS_GPRMC_Packed_Struct_t *curr_pos);

kal_uint8 GPS_APP_GPRSUpldMode(void);

#endif /* _GPS_APP_H */

