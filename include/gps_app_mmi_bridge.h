/******************************************************************************
* File name: 
*	gps_app_mmi_bridge.h
*
* Description:
*	The header file of the bridge between GPS app and PlutoMMI.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.02.19
*
******************************************************************************/
#ifndef _GPS_APP_MMI_BRIDGE_H_
#define _GPS_APP_MMI_BRIDGE_H_

#include "kal_release.h"
#include "stack_msgs.h"

enum
{
	GPSAPPMMIBRG_MSGID_HANDFREE = MSG_ID_MMI_GPSAPP_BRG_BEGIN,
	GPSAPPMMIBRG_MSGID_SET_VOLUME,
	GPSAPPMMIBRG_MSGID_SET_MTCALLALERT,
	GPSAPPMMIBRG_MSGID_STOPMTCALLINDICATION,
	GPSAPPMMIBRG_MSGID_SETPROFILE,
	GPSAPPMMIBRG_MSGID_RESETTONEPROFILE,
	
	//add new message ID before this line
	GPSAPPMMIBRG_MSGID_Invalid = MSG_ID_MMI_GPSAPP_BRG_END + 1
};

//same as MMI_ALERT_TYPE defined in custom_mmi_default_value.h
typedef enum
{
	GPSMTCALLALERT_ALERT_NONE,	//MMI_ALERT_NONE,
	GPSMTCALLALERT_ALERT_RING,	//MMI_RING,
	GPSMTCALLALERT_ALERT_VIBRATION_ONLY,	//MMI_VIBRATION_ONLY,
	GPSMTCALLALERT_ALERT_VIBRATION_AND_RING,	//MMI_VIBRATION_AND_RING,
	GPSMTCALLALERT_ALERT_VIBRATION_THEN_RING,	//MMI_VIBRATION_THEN_RING,
	GPSMTCALLALERT_ALERT_SILENT	//MMI_SILENT
} GpsAppMmiBrgMtCallAlertType_t;

typedef enum
{
	GPSVOLTYPE_RECEIVER = 0,	//receiver
	GPSVOLTYPE_LOUDSPK,		//loud speaker
	GPSVOLTYPE_MICROPHONE,	//microphone

	//add new type before this line
	GPSVOLTYPE_Invalid
} GpsAppMmiBrgVolType_t;

/*
* same as MMI_PROFILE_ENUM_TYPE defined in SettingProfile.h
*/
typedef enum
{
	GPSPROFILE_NORMAL,	//same as MMI_PROFILE_GENERAL
	GPSPROFILE_SILENT,	//STR_PROFILES_MEETING
	
	GPSPROFILE_Total
} GpsAppMmiBrgProfile_t;

#ifndef LOCAL_PARA_HDR
#define LOCAL_PARA_HDR \
	kal_uint8		ref_count; \
	kal_uint16	msg_len;
#endif //LOCAL_PARA_HDR

typedef struct
{
	LOCAL_PARA_HDR
	GpsAppMmiBrgVolType_t	volume_type;
	kal_uint8	volume_level;
} GpsAppMmiBrgSetVolStruct_t;

typedef struct
{
	LOCAL_PARA_HDR
	kal_bool On;
} GpsAppMmiBrgHandFreeStruct_t;

typedef struct
{
	LOCAL_PARA_HDR
	GpsAppMmiBrgMtCallAlertType_t AlertType;
} GpsAppMmiBrgSetMtCallAlertTypeStruct_t;

typedef struct
{
	LOCAL_PARA_HDR
	GpsAppMmiBrgProfile_t Profile;
} GpsAppMmiBrgSetProfileStruct_t;

void GpsAppMmiBrgHandFree(kal_bool On);
void GpsAppMmiBrgSetVolume(GpsAppMmiBrgVolType_t type, unsigned char level);
void GpsAppMmiBrgSetMTCallAlert(GpsAppMmiBrgMtCallAlertType_t type);
void GpsAppMmiBrgStopMtCallIndication(void);
void GpsAppMmiBrgStartVibrator(void);
void GpsAppMmiBrgStopVibrator(void);
void GpsAppMmiBrgRunVibratorOnce(void);
void GpsAppMmiBrgRunSmsVibratorOnce(void);
void GpsAppMmiBrgHangupAllCalls(void);
void GpsAppMmiBrgSetProfile(GpsAppMmiBrgProfile_t Profile);
GpsAppMmiBrgProfile_t GpsAppMmIBrgGetCurrProfile(void);
void GpsAppMmiBrgResetToneProfile(void);

#endif //_GPS_APP_MMI_BRIDGE_H_

