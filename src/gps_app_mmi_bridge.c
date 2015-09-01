/******************************************************************************
* File name: 
*	gps_app_mmi_bridge.c
*
* Description:
*	The bridge between GPS app and PlutoMMI.
*	In the current stage, GPS application has to implement some features depending on
*	MMI task. So we create this bridge to connect GPS app and MMI. At the same time, 
*	isolate them as well.
*	In the next stage, we will isolate GPS app and MMI completely. At that time, this bridge
*	will be removed.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.02.19
*
******************************************************************************/
#include "kal_release.h"
#include "stack_common.h"
#include "stack_msgs.h"
#include "task_main_func.h"
#include "app_ltlcom.h"
#include "custom_config.h"

#include "gps_app_mmi_bridge.h"

typedef void (*PsFuncPtr) (void *);

static void GpsAppMmiBrgHandFreeHandler(void* msg);
static void GpsAppMmiBrgSetMtCallAlertHandler(void* msg);
static void GpsAppMmiBrgSetVolumeHandler(void* msg);
static void GpsAppMmiBrgStopMtCallIndicationHandler(void* msg);
static void GpsAppMmiBrgSetProfileHandler(void* msg);
static void GpsAppMmiBrgResetToneProfileHandler(void* msg);

extern void LoudSpeakerFunction(void);
extern void GpsAppMmiSetVolume(GpsAppMmiBrgVolType_t type, unsigned char level);
extern void SetProtocolEventHandler(PsFuncPtr funcPtr, kal_uint16 eventID);
extern void ClearProtocolEventHandler(kal_uint16 eventID);
extern kal_int16 SetMtCallAlertTypeEnum(GpsAppMmiBrgMtCallAlertType_t callalerttype);
extern void StartRingtoneOrVibrator(void);
extern void StopRingtoneOrVibrator(void);
extern void StartVibrator(void);
extern void StopVibrator(void);
extern void RunVibratorOnce(void);
extern void RunSmsVibratorOnce(void);

extern void KbCBackCallIncomingRejected(void);
extern void gpsappmmibrgsetprofile(kal_uint8 profile);
extern int gpsappmmibrgResetTone(void);

extern kal_uint8 gactivatedprofile;

void GpsAppMmiBrgHandFree(kal_bool On)
{
	ilm_struct *ilm_ptr = NULL;
	module_type src_mod;
	GpsAppMmiBrgHandFreeStruct_t* req;
	
	SetProtocolEventHandler(GpsAppMmiBrgHandFreeHandler, 
		GPSAPPMMIBRG_MSGID_HANDFREE);

	req = (GpsAppMmiBrgHandFreeStruct_t*)construct_local_para(sizeof(GpsAppMmiBrgHandFreeStruct_t), TD_UL);
	req->On = On;
	
	src_mod = stack_int_get_active_module_id();
	ilm_ptr = allocate_ilm(src_mod);
	ilm_ptr->src_mod_id = src_mod;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->msg_id = GPSAPPMMIBRG_MSGID_HANDFREE;
	ilm_ptr->local_para_ptr = (local_para_struct*) req;
	ilm_ptr->peer_buff_ptr = NULL;

	msg_send_ext_queue(ilm_ptr);
}

static void GpsAppMmiBrgHandFreeHandler(void* msg)
{
	GpsAppMmiBrgHandFreeStruct_t* msgReq = (GpsAppMmiBrgHandFreeStruct_t*)msg;
	
	ClearProtocolEventHandler(GPSAPPMMIBRG_MSGID_HANDFREE);
	LoudSpeakerFunction();
}

void GpsAppMmiBrgSetVolume(GpsAppMmiBrgVolType_t type, unsigned char level)
{
	ilm_struct *ilm_ptr = NULL;
	module_type src_mod;
	GpsAppMmiBrgSetVolStruct_t* req;
	
	SetProtocolEventHandler(GpsAppMmiBrgSetVolumeHandler, 
		GPSAPPMMIBRG_MSGID_SET_VOLUME);

	req = (GpsAppMmiBrgSetVolStruct_t*)construct_local_para(sizeof(GpsAppMmiBrgSetVolStruct_t), TD_UL);
	req->volume_type = type;
	req->volume_level = level;

	src_mod = stack_int_get_active_module_id();
	ilm_ptr = allocate_ilm(src_mod);
	ilm_ptr->src_mod_id = src_mod;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->msg_id = GPSAPPMMIBRG_MSGID_SET_VOLUME;
	ilm_ptr->local_para_ptr = (local_para_struct*) req;
	ilm_ptr->peer_buff_ptr = NULL;

	msg_send_ext_queue(ilm_ptr);
}

static void GpsAppMmiBrgSetVolumeHandler(void* msg)
{
	GpsAppMmiBrgSetVolStruct_t *msgReq = (GpsAppMmiBrgSetVolStruct_t*) msg;
	
	ClearProtocolEventHandler(GPSAPPMMIBRG_MSGID_SET_VOLUME);
	GpsAppMmiSetVolume(msgReq->volume_type, msgReq->volume_level);
}

void GpsAppMmiBrgSetMTCallAlert(GpsAppMmiBrgMtCallAlertType_t type)
{
	ilm_struct *ilm_ptr = NULL;
	module_type src_mod;
	GpsAppMmiBrgSetMtCallAlertTypeStruct_t* req;

	//prevent to modify other profiles than the normal one
	if (gactivatedprofile != GPSPROFILE_NORMAL)
	{
		return;
	}
	SetProtocolEventHandler(GpsAppMmiBrgSetMtCallAlertHandler, 
		GPSAPPMMIBRG_MSGID_SET_MTCALLALERT);
	req = (GpsAppMmiBrgSetMtCallAlertTypeStruct_t*)construct_local_para(sizeof(GpsAppMmiBrgSetMtCallAlertTypeStruct_t), TD_UL);
	req->AlertType = type;

	src_mod = stack_int_get_active_module_id();
	ilm_ptr = allocate_ilm(src_mod);
	ilm_ptr->src_mod_id = src_mod;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->msg_id = GPSAPPMMIBRG_MSGID_SET_MTCALLALERT;
	ilm_ptr->local_para_ptr = (local_para_struct*) req;
	ilm_ptr->peer_buff_ptr = NULL;

	msg_send_ext_queue(ilm_ptr);
}

static void GpsAppMmiBrgSetMtCallAlertHandler(void* msg)
{
	GpsAppMmiBrgSetMtCallAlertTypeStruct_t* msgReq = (GpsAppMmiBrgSetMtCallAlertTypeStruct_t*)msg;
	
	ClearProtocolEventHandler(GPSAPPMMIBRG_MSGID_SET_MTCALLALERT);
	SetMtCallAlertTypeEnum(msgReq->AlertType);
}

void GpsAppMmiBrgStopMtCallIndication(void)
{
	ilm_struct *ilm_ptr = NULL;
	module_type src_mod;

	SetProtocolEventHandler(GpsAppMmiBrgStopMtCallIndicationHandler, 
		GPSAPPMMIBRG_MSGID_STOPMTCALLINDICATION);

	src_mod = stack_int_get_active_module_id();
	ilm_ptr = allocate_ilm(src_mod);
	ilm_ptr->src_mod_id = src_mod;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->msg_id = GPSAPPMMIBRG_MSGID_STOPMTCALLINDICATION;
	ilm_ptr->local_para_ptr = NULL;
	ilm_ptr->peer_buff_ptr = NULL;

	msg_send_ext_queue(ilm_ptr);
}

static void GpsAppMmiBrgStopMtCallIndicationHandler(void* msg)
{
	ClearProtocolEventHandler(GPSAPPMMIBRG_MSGID_STOPMTCALLINDICATION);
	StopRingtoneOrVibrator();
}

void GpsAppMmiBrgStartVibrator(void)
{
	StartVibrator();
}

void GpsAppMmiBrgStopVibrator(void)
{
	StopVibrator();
}

void GpsAppMmiBrgRunVibratorOnce(void)
{
	RunVibratorOnce();
}

void GpsAppMmiBrgRunSmsVibratorOnce(void)
{
	RunSmsVibratorOnce();
}

void GpsAppMmiBrgHangupAllCalls(void)
{
	KbCBackCallIncomingRejected();
}

void GpsAppMmiBrgSetProfile(GpsAppMmiBrgProfile_t Profile)
{
	GpsAppMmiBrgSetProfileStruct_t* req;
	ilm_struct *ilm_ptr = NULL;
	module_type src_mod;

	if (Profile >= GPSPROFILE_Total || Profile == gactivatedprofile)
	{
		return;
	}
	SetProtocolEventHandler(GpsAppMmiBrgSetProfileHandler, 
		GPSAPPMMIBRG_MSGID_SETPROFILE);
	req = (GpsAppMmiBrgSetProfileStruct_t*)construct_local_para(sizeof(GpsAppMmiBrgSetProfileStruct_t), TD_UL);
	req->Profile = Profile;

	src_mod = stack_int_get_active_module_id();
	ilm_ptr = allocate_ilm(src_mod);
	ilm_ptr->src_mod_id = src_mod;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->msg_id = GPSAPPMMIBRG_MSGID_SETPROFILE;
	ilm_ptr->local_para_ptr = (local_para_struct*) req;
	ilm_ptr->peer_buff_ptr = NULL;

	msg_send_ext_queue(ilm_ptr);
}

static void GpsAppMmiBrgSetProfileHandler(void* msg)
{
	GpsAppMmiBrgSetProfileStruct_t* msgReq = (GpsAppMmiBrgSetProfileStruct_t*)msg;

	ClearProtocolEventHandler(GPSAPPMMIBRG_MSGID_SETPROFILE);
	gpsappmmibrgsetprofile(msgReq->Profile);
}

GpsAppMmiBrgProfile_t GpsAppMmIBrgGetCurrProfile(void)
{
	return gactivatedprofile;
}

void GpsAppMmiBrgResetToneProfile(void)
{
	ilm_struct *ilm_ptr = NULL;
	module_type src_mod;

	SetProtocolEventHandler(GpsAppMmiBrgResetToneProfileHandler, 
		GPSAPPMMIBRG_MSGID_RESETTONEPROFILE);

	src_mod = stack_int_get_active_module_id();
	ilm_ptr = allocate_ilm(src_mod);
	ilm_ptr->src_mod_id = src_mod;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->msg_id = GPSAPPMMIBRG_MSGID_RESETTONEPROFILE;
	ilm_ptr->local_para_ptr = NULL;
	ilm_ptr->peer_buff_ptr = NULL;

	msg_send_ext_queue(ilm_ptr);
}

static void GpsAppMmiBrgResetToneProfileHandler(void* msg)
{
	ClearProtocolEventHandler(GPSAPPMMIBRG_MSGID_RESETTONEPROFILE);
	gpsappmmibrgResetTone();
}

