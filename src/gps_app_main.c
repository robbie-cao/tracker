/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_main.c
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
 * 2007-11-8 20:34 - Initial create.
 ****************************************************************************/

/*************************************************************************
 * Include Statements
 *************************************************************************/
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
#include "em_struct.h"

#include "reg_base.h"
#include "DrvPdn.h"
#include "bmt.h"
#include "drv_comm.h"
#include "reg_base.h"
#include "gpio_hw.h"
#include "gpio_sw.h"

#include "string.h"
#include "assert.h"
#include "ctype.h"
#include "stdio.h"
#include "stdlib.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app.h"
#include "gps_app_athdlr.h"
#include "gps_app_nvram_mgr.h"
#include "GPSLocateKeyMonitor.h"
#include "gps_app_event_fifo.h"
#include "gps_app_ind_mgr.h"
#include "gps_app_sq_wave.h"
#include "gps_app_pp_buff_mgr.h"
#include "gps_app_mmi_bridge.h"
#ifdef GPS_DEFENCE_FEATURE
#include "gps_app_defence_moniter.h"
#endif
#ifdef CODEPROTECTOR_FS8816_SUPPORT
#include "FS8816_App.h"
#endif //CODEPROTECTOR_FS8816_SUPPORT
#include "gps_app_data.h"
#include "gps_app_databackup.h"

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
 * Local variables
 *************************************************************************/
char gps_locate_info_buff[160];

static kal_bool gps_load_settings_flag = KAL_FALSE;
#ifdef GPS_POWERON_PHONECALL
static kal_bool gps_firstcall_flag = KAL_FALSE;
#endif
GPSLocateRegStateStruct_t gps_reg_state;
//add this flag to indicate the system is shutting down
static kal_bool gps_system_shutting_down = KAL_FALSE;
//add this flag to prevent other modules send messages to this task before this task 
//initialization finished.
static kal_bool gps_app_task_init_finished = KAL_FALSE;
#ifdef GPS_DEFENCE_FEATURE
//add this flag to store where mo call from
static kal_bool gps_defence_call_flag = KAL_FALSE;
#endif

#ifdef CODEPROTECTOR_FS8816_SUPPORT
static const BYTE gps_app_codeprotector_key[] = CODEPROTECTOR_KEY;
#endif //CODEPROTECTOR_FS8816_SUPPORT

/*************************************************************************
 * Global variables
 *************************************************************************/
extern GPS_SMS_Cmd_Handler_Func_t 	gps_sms_cmd_hdlrList[];

//extern void l4c_get_reg_state(GPSLocateRegStateStruct_t *reg_state);

extern void gps_get_nwinfo_request(kal_uint32 info);
extern void gps_giveup_nwinfo_request(void);
extern void MakeGPsCallAcceptIncoming(void);
extern void MakeGPSCallReq(kal_uint8 *MsgStruct, void *callBack);

/*************************************************************************
 * Function declaration
 *************************************************************************/
static void gps_app_mtsms_req_hdlr(ilm_struct *ilm_ptr);
static void gps_app_mtcall_req_hdlr(ilm_struct *ilm_ptr);
static void gps_app_mocall_rsp_hdlr(ilm_struct *ilm_ptr);
static void gps_app_keypad_event_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_app_gps_frame_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_app_at_change_presaved_req_hdlr(ilm_struct *ilm_ptr);
static void gps_app_nw_attach_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_app_del_sms_done_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_app_send_sms_done_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_app_reg_state_info_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_app_battery_status_ind_hdlr(ilm_struct * ilm_ptr);
static void gps_app_simploc_timeout_ind_hdlr(ilm_struct * ilm_ptr);
#ifdef GPS_POSITION_MONITOR
static void gps_app_setpos_timeout_ind_hdlr(ilm_struct * ilm_ptr);
#endif
#ifdef GPS_DEFENCE_FEATURE
static void gps_app_triggerdefence_ind_hdlr(ilm_struct * ilm_ptr);
#endif
static void gps_app_sys_shutting_down_ind_hdlr(ilm_struct * ilm_ptr);
#ifdef GPS_BACKUPLOAD_DAEMON
static void gps_app_backupload_timer_ind_hdlr(ilm_struct * ilm_ptr);
#endif
static void gps_app_ppbuffull4loading_ind_hdlr(ilm_struct * ilm_ptr);
#ifdef VIBRATION_SENSOR_SUPPORT
static void gps_app_vibsensor_ind_hdlr(ilm_struct * ilm_ptr);
#endif //VIBRATION_SENSOR_SUPPORT
static void gps_app_gprmcframe4smsloc_hdlr(const char *rmc_src, int len);
static void gps_app_gprmcframe4gprsupld_hdlr(const char *rmc_src, int len);
#ifdef GPS_POSITION_MONITOR
static void gps_app_gprmcframe4posset_hdlr(const char *rmc_src, int len);
static void gps_app_gprmcframe4posmonitor_hdlr(const char *rmc_src, int len);
#endif
#ifdef GPS_RATE_MONITOR
static void gps_app_gprmcframe4ratemonitor_hdlr(const char *rmc_src, int len);
#endif
#ifdef GPS_MOD_SWAUTOPOWER
static void gps_app_gprmcframe4modswpower_hdlr(const char *rmc_src, int len);
#endif
static void gps_app_gprmcframe4gprsupldimm_hdlr(const char *rmc_src, int len);

#ifdef GPS_NVRAM_TASK
static void gps_app_nvramstore_ind_hdlr(ilm_struct *ilm_ptr);
#endif
static void gps_app_upldmodeswitch_ind_hdlr(ilm_struct *ilm_ptr);

static void gps_app_main(task_entry_struct * task_entry_ptr);
static kal_bool gps_app_init(task_indx_type task_indx);
static kal_bool gps_app_reset(task_indx_type task_index);
#ifdef CODEPROTECTOR_FS8816_SUPPORT
static kal_bool CodeProtectorValidation(void);
#endif //CODEPROTECTOR_FS8816_SUPPORT

//-----------------------------------------------------------------------------
void gps_app_mtsms_req_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateMTSMSReqStruct_t *ind_p;
    GPS_Sms_Instruction_t sms_instr;
	kal_uint8 fetion_header = 0; // for fetion support only
	Result_t result = RESULT_ERROR;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p= (GPSLocateMTSMSReqStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("gps_app_mtsms_req_hdlr, remote number type: %#x, length: %d, number: %s", 
    			ind_p->RemoteNumber.Type, 
    			ind_p->RemoteNumber.Length, 
    			ind_p->RemoteNumber.Number);
    gps_app_log("gps_app_mtsms_req_hdlr, mtsms msg length: %d, data: %s", 
				ind_p->MsgDataLen, 
				ind_p->MsgData);

	memset(&sms_instr, 0, sizeof(sms_instr));
	memcpy(&sms_instr.remote_number, &ind_p->RemoteNumber, sizeof(GPSLocatePhoneNumber_t));


#ifdef GPS_FETION_SUPPORT
	if (ind_p->MsgDataLen > 6 &&
		ind_p->MsgData[0] == 0x05 &&
		ind_p->MsgData[1] == 0x00 &&
		ind_p->MsgData[2] == 0x03)
	{
		fetion_header = 6;
	}
#endif
	result = GPS_SMS_ParseSMS((char *)ind_p->MsgData + fetion_header, &sms_instr);
	if (result != RESULT_OK)
	{
	    gps_app_log("gps_app_mtsms_req_hdlr, parse sms error");
		return;
	}

	result = (gps_sms_cmd_hdlrList[sms_instr.cmd])(&sms_instr);
	if (result != RESULT_OK)
	{
	    gps_app_log("gps_app_mtsms_req_hdlr, sms cmd %d handler error", sms_instr.cmd);
		return;
	}
}

//-----------------------------------------------------------------------------
void gps_app_mtcall_req_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateMTCallReqStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p= (GPSLocateMTCallReqStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("gps_app_mtcall_req_hdlr, remote number type: %#x, length: %d, number: %s", 
    			ind_p->RemoteNumber.Type, 
    			ind_p->RemoteNumber.Length, 
    			ind_p->RemoteNumber.Number);

	if (!GPS_APP_FindNumberInPresavdList(&ind_p->RemoteNumber))
	{
    	gps_app_log("Number not in presaved list");

		return;
	}

	switch(ind_p->ReqCategory)
	{
	case GPSLOCATE_MTCALL_REQ_CATEGORY_SIMPLE_LOCATE:
      	gps_app_log("MT call req category: GPSLOCATE_MTCALL_REQ_CATEGORY_SIMPLE_LOCATE");

		switch (gps_curr_bearer_type)
		{
		case GPS_BEARER_SMS:
			GPS_APP_SingleLocateStart(&ind_p->RemoteNumber, 
							gps_mtcall_answered ? 
							GPS_RMCSMS_TYPE_ANSWER:
							GPS_RMCSMS_TYPE_CALL);

			break;

		case GPS_BEARER_GPRS:
			{
			kal_uint8 *tmpPtr;
			GPSAppEvent_t tmpEvent;
			kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_CALL;
			kal_bool fifo_push = KAL_TRUE;

			if (GPSAppEventFifoIsFull())
			{
				// fifo full, discard req
				gps_app_log("gps_app_mtcall_req_hdlr, fifo full");
				return;
			}

			cause_type = gps_mtcall_answered ? GPS_GPRS_UPLDCAUSETYPE_ANSWER: GPS_GPRS_UPLDCAUSETYPE_CALL;
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
				gps_app_log("gps_app_mtcall_req_hdlr, push req into fifo, OpCode: %d", tmpEvent.OpCode);
			}
			}

			break;

		case GPS_BEARER_CSD:
		case GPS_BEARER_WIFI:
			// not support now, do nothing
			break;

		default:
			// unknown bearer type, do nothing
			break;
		}

		//restore mtcall answered flag
		gps_mtcall_answered = KAL_FALSE;

		//flash the network indicator LED
		GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								GPS_IND_NWSTATUS_INTV * KAL_TICKS_1_SEC);

		break;

	case GPSLOCATE_MTCALL_REQ_CATEGORY_AUTO_ANSWERED:
      	gps_app_log("MT call req category: GPSLOCATE_MTCALL_REQ_CATEGORY_AUTO_ANSWERED");

      	MakeGPsCallAcceptIncoming();
		gps_app_log("MTCALL auto answered");

		//flash the network indicator LED
		GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								GPS_IND_CALLCONNED_INTV * KAL_TICKS_1_SEC);
		if (gps_call_handfree)
		{
			GpsAppMmiBrgHandFree(gps_call_handfree);
		}
		gps_mtcall_answered = KAL_TRUE;

		/*
		* Set ongoing call speech to monitor
		*
		* 1. check monitor or not
		* 2. if monitor, set monitor speech type
		* 3. otherwise, do nothing
		*
		* Set speech type after accepting the MT call. Now, speech should have been turned on.
		*/
		if (gps_mtcall_profile == GPSPROFILE_SILENT /*monitor condition*/)
		{
			GPS_APP_SetOngoingCallSpch(GPSMTCALLTYPE_MONITOR);
		}
		break;

	case GPSLOCATE_MTCALL_REQ_CATEGORY_CALL_CONNECT:
      	gps_app_log("MT call req category: GPSLOCATE_MTCALL_REQ_CATEGORY_CALL_CONNECT");
		break;

	case GPSLOCATE_MTCALL_REQ_CATEGORY_REMOTE_RELEASED:
      	gps_app_log("MT call req category: GPSLOCATE_MTCALL_REQ_CATEGORY_REMOTE_RELEASED");
		break;

	default:
      	gps_app_log("MT call req category: UNKNOWN");
		break;
	}

}

//-----------------------------------------------------------------------------
void gps_app_mocall_rsp_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateMOCallRsptIndStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p= (GPSLocateMOCallRsptIndStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("gps_app_mocall_rsp_hdlr, remote number type: %#x, length: %d, number: %s", 
    			ind_p->RemoteNumber.Type, 
    			ind_p->RemoteNumber.Length, 
    			ind_p->RemoteNumber.Number);

	switch(ind_p->RspCategory)
	{
	case GPSLOCATE_MOCALL_RSP_CATEGORY_NOACCEPT:
      	gps_app_log("MO call rsp category: GPSLOCATE_MOCALL_RSP_CATEGORY_NOACCEPT");

		if (gps_sos_call_index <= 0 ||
			gps_sos_call_index >= GPSLOCATE_USER_NUMBER_MAX)
		{
			gps_app_log("made call to all numbers in saved list done.");
			gps_app_log("no number in saved list answer.");
#ifdef GPS_DEFENCE_FEATURE
			gps_defence_call_flag = KAL_FALSE;
#endif
			gps_sos_call_index = 0;
			gps_mocall_state = GPSLOCATE_MOCALLSTATE_IDLE;
			GPS_APP_SetCallType(GPSLOCATE_CALLTYPE_UNKNOWN);
		}
		else
		{
			gps_app_log("cannot reach or no answer from number %d: %s", 
						gps_sos_call_index, gps_tgt_numgrp_mocall[gps_sos_call_index-1].number);
			for (; gps_sos_call_index < GPSLOCATE_USER_NUMBER_MAX; gps_sos_call_index++)
			{
				if (strlen(gps_tgt_numgrp_mocall[gps_sos_call_index].number))
					break;
			}
			if (gps_sos_call_index >= GPSLOCATE_USER_NUMBER_MAX)
			{
				gps_app_log("made call to all numbers in saved list done.");
				gps_app_log("no number in saved list answer.");
				gps_mocall_state = GPSLOCATE_MOCALLSTATE_IDLE;
				break;
			}
			gps_mocall_state = GPSLOCATE_MOCALLSTATE_WAITINGNEXT;
			if (gps_mocall_timer2 != NULL)
			{
				GPSAppTimer_Reset(gps_mocall_timer2, 
						GPS_APP_MOCallDialNextNumber, 
						KAL_TICKS_5_SEC * 2,
						0, 
						KAL_TRUE);
			}
			else
			{
				gps_mocall_timer2 = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
						GPS_APP_MOCallDialNextNumber, 
						KAL_TICKS_5_SEC * 2,
						0, 
						KAL_TRUE);
			}
		}

      	break;

	case GPSLOCATE_MOCALL_RSP_CATEGORY_CALLNEXT:
      	gps_app_log("MO call rsp category: GPSLOCATE_MOCALL_RSP_CATEGORY_CALLNEXT");
		gps_app_log("make phone call to number %d: %s", 
					gps_sos_call_index + 1, 
					gps_tgt_numgrp_mocall[gps_sos_call_index].number);
		MakeGPSCallReq((kal_uint8 *)gps_tgt_numgrp_mocall[gps_sos_call_index].number, 
					   (void *)GPS_APP_MOCallDialTimerStart);
		gps_sos_call_index++;

#if (defined GPS_VIB_4SOSCALLSTARTING)
		// play vibrator once to let user know that SOS call start
		GpsAppMmiBrgRunVibratorOnce();
#endif

		break;

	case GPSLOCATE_MOCALL_RSP_CATEGORY_ANSWER:
      	gps_app_log("MO call rsp category: GPSLOCATE_MOCALL_RSP_CATEGORY_ANSWER");
		gps_app_log("number %d answered. sos call end.", gps_sos_call_index);
		// stop mo call timer, GPS_APP_MOCallDialTimeout() handler will not be invoked
		GPS_APP_MOCallDialTimerStop();
		GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								GPS_IND_CALLCONNED_INTV * KAL_TICKS_1_SEC);
#if (defined GPS_VIB_4SOSCALLCONNECT)
		// play vibrator once to let user know SOS call connected
		GpsAppMmiBrgRunVibratorOnce();
#endif

#if 0
		if (gps_call_handfree)
		{
			GpsAppMmiBrgHandFree(gps_call_handfree);
		}
#endif
#ifdef GPS_DEFENCE_FEATURE
		if (GPS_APP_DefenceOn() && gps_defence_call_flag)
		{
			// defence alarm on
			GPS_APP_EnDefenceAlarm();	
		}
		gps_defence_call_flag = KAL_FALSE;
#endif
		gps_sos_call_index = 0;
		gps_mocall_state = GPSLOCATE_MOCALLSTATE_CONNECTED;
      	break;

	case GPSLOCATE_MOCALL_RSP_CATEGORY_DISCONNECT:
      	gps_app_log("MO call rsp category: GPSLOCATE_MOCALL_RSP_CATEGORY_DISCONNECT");
      	break;

	case GPSLOCATE_MOCALL_RSP_CATEGORY_RELEASED:
      	gps_app_log("MO call rsp category: GPSLOCATE_MOCALL_RSP_CATEGORY_RELEASED");
		GPS_APP_SetCallType(GPSLOCATE_CALLTYPE_UNKNOWN);
		GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								GPS_IND_NWSTATUS_INTV * KAL_TICKS_1_SEC);
		gps_mocall_state = GPSLOCATE_MOCALLSTATE_IDLE;
      	break;

	default:
      	gps_app_log("MO call rsp category: UNKNOWN");
      	break;
	}
}

//-----------------------------------------------------------------------------
void gps_app_keypad_event_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateKeypadEventIndStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p= (GPSLocateKeypadEventIndStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("gps_app_keypad_event_ind_hdlr, KeyCode: %d, KeyType: %d", 
    			ind_p->KeyCode, 
    			ind_p->KeyType);

	switch(ind_p->KeyType)
	{
	case GPSLOCATE_KEYTYPE_PRESS:
      	gps_app_log("Keypad event, KeyType: GPSLOCATE_KEYTYPE_PRESS");
		switch (ind_p->KeyCode)
		{
		case GPSLOCATE_KEYCODE_SOS:
			if (gps_mtcall_connecting == KAL_TRUE)
			{
				GPSLocateMTCallReqStruct_t* pReqData;

				pReqData = (GPSLocateMTCallReqStruct_t*)construct_local_para(sizeof(GPSLocateMTCallReqStruct_t), TD_UL);
				pReqData->ReqCategory = GPSLOCATE_MTCALL_REQ_CATEGORY_AUTO_ANSWERED;
				memcpy(&pReqData->RemoteNumber, &gps_act_remote_num, sizeof(gps_act_remote_num));
				
				SendMsg2GPSApp(MOD_GPS_APP_TASK, MSG_ID_GPSLOCATE_MTCALL_REQ, (void*)pReqData);

				if (gps_mtcall_timer != NULL)
				{
					GPSAppTimer_Stop(gps_mtcall_timer, KAL_TRUE);
					gps_mtcall_timer = NULL;
				}
				gps_mtcall_connecting = KAL_FALSE;
			}
			break;
			
		case GPSLOCATE_KEYCODE_END:
			/****End key pressed****/
			if (gps_mocall_state == GPSLOCATE_MOCALLSTATE_CONNECTED)
			{
				//hangup call
				//gps_app_log("Hangup the active MO call");
				GPS_APP_HangupActiveMOCall();
				gps_mocall_state = GPSLOCATE_MOCALLSTATE_IDLE;
			}
			if (gps_mtcall_connecting == KAL_TRUE)
			{
				if (gps_mtcall_timer != NULL)
				{
					GPSAppTimer_Stop(gps_mtcall_timer, KAL_TRUE);
					gps_mtcall_timer = NULL;
				}
				gps_mtcall_connecting = KAL_FALSE;
				gps_mtcall_answered = KAL_FALSE;
			}
			break;
		}
      	break;

	case GPSLOCATE_KEYTYPE_LONGPRESS:
      	gps_app_log("Keypad event, KeyType: GPSLOCATE_KEYTYPE_LONGPRESS");
		switch(ind_p->KeyCode)
		{
		case GPSLOCATE_KEYCODE_SOS:
			if (gps_mtcall_connecting == KAL_TRUE || 
				gps_mtcall_answered == KAL_TRUE ||
				gps_mocall_state != GPSLOCATE_MOCALLSTATE_IDLE)
			{
				// reject sos key long press if 
				//   1, mt call connecting/connected
				//   2, there's any mo call connecting/connected or waiting to make a mo call
				break;
			}
#if (defined GPS_VIB_4SOSCALLSTARTING)
			// play vibrator once to let user know that SOS call start
			GpsAppMmiBrgRunVibratorOnce();
#endif
			switch (gps_curr_bearer_type)
			{
			case GPS_BEARER_SMS:
		      	GPS_APP_SosSendSmsStart(GPS_SAVEDNUMBERTYPE_SOS);

				// start to make phone call to the numbers in saved list at the same time
				GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_SOS);

				break;

			case GPS_BEARER_GPRS:
				{
				kal_uint8 *tmpPtr;
				GPSAppEvent_t tmpEvent;
				kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_SOS;
				kal_bool fifo_push = KAL_TRUE;

				if (GPSAppEventFifoIsFull())
				{
					// fifo full, discard req
					gps_app_log("gps_app_keypad_event_ind_hdlr, fifo full");
					return;
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
					gps_app_log("gps_app_keypad_event_ind_hdlr, sos gprs upload, push req into fifo, OpCode: %d", tmpEvent.OpCode);
				}
				}

				// should start to make phone call to the numbers in saved list 
				// at the same time, but mtk 6225 only support gprs class b, 
				// so it cannot send data through gprs and make phone call at the 
				// same time. gprs class a can support send data through gprs and
				// make phone call at the same time.
				// now start to make phone call after send data through gprs done.
				// refer gps_tcpip_send_result_cb()
				// GPS_APP_SosMOCallStart();

				break;

			case GPS_BEARER_CSD:
			case GPS_BEARER_WIFI:
				// not support now, do nothing
				break;

			default:
				// unknown bearer type, do nothing
				break;
			}
			break;
			
		case GPSLOCATE_KEYCODE_END:
			/*
			* once implementing End key long pressing process here, please remove the current 
			* implementation in Keybrd.c
			*/
			break;
		}

      	break;

	default:
      	gps_app_log("Keypad event, KeyType: UNKOWN");
		break;
	}
}

//-----------------------------------------------------------------------------
void gps_app_gps_frame_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateGPSFrameDataIndStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p = (GPSLocateGPSFrameDataIndStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("gps_app_gps_frame_ind_hdlr, objective: %#04x, frame len: %d, data: %s", 
				ind_p->Objective, 
				ind_p->FrameDataLen, 
				ind_p->FrameData);

	if (ind_p->Objective == GPSLOCATE_GPRMCOBJECTIVE_UNKNOWN)
	{
		ASSERT(0);
	}

	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_SMSLOC)
	{
		gps_app_gprmcframe4smsloc_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}

	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_GPRSUPLD)
	{
		gps_app_gprmcframe4gprsupld_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}

#ifdef GPS_POSITION_MONITOR
	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_POSSET)
	{
		gps_app_gprmcframe4posset_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}

	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_POSMONITOR)
	{
		gps_app_gprmcframe4posmonitor_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}
#endif

#ifdef GPS_RATE_MONITOR
	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_RATEMONITOR)
	{
		gps_app_gprmcframe4ratemonitor_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER)
	{
		gps_app_gprmcframe4modswpower_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}
#endif

	if (ind_p->Objective & GPSLOCATE_GPRMCOBJECTIVE_GPRSUPLDIMM)
	{
		gps_app_gprmcframe4gprsupldimm_hdlr(ind_p->FrameData, ind_p->FrameDataLen);
	}
}

//-----------------------------------------------------------------------------
void gps_app_at_change_presaved_req_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateATChangePreSavedDataReqStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p = (GPSLocateATChangePreSavedDataReqStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("gps_app_at_change_presaved_req_hdlr, req category: %d", ind_p->ReqCategory) ;

	switch(ind_p->ReqCategory)
	{
	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVICENUMBER:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVICENUMBER");
      	GPS_AT_ChangeServiceNumber(ind_p->Data.ServiceNumber);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVICEPASSWORD:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVICEPASSWORD");
      	GPS_AT_ChangeServicePassword(ind_p->Data.ServicePassword);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERNUMBER:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERNUMBER");
      	GPS_AT_ChangeUserNumber(ind_p->Index, ind_p->Data.UserNumber);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERPASSWORD:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERPASSWORD");
      	GPS_AT_ChangeUserPassword(ind_p->Index, ind_p->Data.UserPassword);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SOSNUMBER:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_USERNUMBER");
      	GPS_AT_ChangeSosNumber(ind_p->Index, ind_p->Data.SosNumber);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SETTINGS:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SETTINGS");
      	GPS_AT_ChangeTimingSetting(&ind_p->Data.Settings);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SETTINGSDEFAULT:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SETTINGSDEFAULT");
      	GPS_AT_ChangeDefaultTimingSetting(&ind_p->Data.SettingsBackup);
		break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SHAREUSRPWD:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SHAREUSRPWD");
      	GPS_AT_ChangeSharedUserPassword(ind_p->Data.SharedUsrPwd);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVERADDR:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SERVERADDR");
      	GPS_AT_ChangeServerAddress(&ind_p->Data.ServerAddr);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_WORKINGMODE:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_WORKINGMODE");
      	GPS_AT_ChangeWorkingMode(&ind_p->Data.WorkingMode);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_BSNUMBER:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_BSNUMBER");
      	GPS_AT_ChangeBSNumber(&ind_p->Data.BSNumber);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSUPLOAD:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSUPLOAD");
      	GPS_AT_ChangeGprsUploadSetting(&ind_p->Data.GprsUploadSetting);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSUSER:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSUSER");
      	GPS_AT_ChangeGprsUser(ind_p->Data.ServiceNumber);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSPWD:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSPWD");
      	GPS_AT_ChangeGprsPassword(ind_p->Data.ServicePassword);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSAPN:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSAPN");
      	GPS_AT_ChangeGprsAPN(ind_p->Data.APN);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSACC:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPRSACC");
      	GPS_AT_ChangeGprsAccount(ind_p->Data.APN, ind_p->GAccountUser, ind_p->GAccountPwd);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPSLOG:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPSLOG");
      	GPS_AT_SwitchGpsLog(&ind_p->Data.Settings);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_HANDFREE:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_HANDFREE");
      	GPS_AT_SwitchHandfree(&ind_p->Data.Settings);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPSPROF:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_GPSPROF");
      	GPS_AT_SwitchGpsProf(&ind_p->Data.Settings);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_MTCALLPROF:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_MTCALLPROF");
      	GPS_AT_SwitchMtcallProf(&ind_p->Data.Settings);
      	break;

	case GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SOSCALL:
      	gps_app_log("AT change req category: GPSLOCATE_ATCHG_PRESAVEDDATA_REQ_CATEGORY_SOSCALL");
      	GPS_AT_SwitchSosCall(&ind_p->Data.Settings);
      	break;

	default:
      	gps_app_log("AT change req category: UNKNOWN");
		break;
	}
}

//-----------------------------------------------------------------------------
static void gps_app_nw_attach_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateNetworkAttachIndStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p = (GPSLocateNetworkAttachIndStruct_t*) ilm_ptr->local_para_ptr;
    gps_app_log("Network attach ind, stats: %d, PLMN: %s, GSM state: %d, GPRS state: %d, GPRS status: %d",
				ind_p->Status,
				ind_p->PLMN,
				ind_p->GSMState,
				ind_p->GPRSState,
				ind_p->GPRSStatus
    			);
    catcher_sys_trace("Network attach ind, stats: %d, PLMN: %s, GSM state: %d, GPRS state: %d, GPRS status: %d",
				ind_p->Status,
				ind_p->PLMN,
				ind_p->GSMState,
				ind_p->GPRSState,
				ind_p->GPRSStatus
    			);

	// Ensure call GPS_APP_Init_All() only once
	if (gps_load_settings_flag == KAL_FALSE)
	{
		GPS_APP_Init_All();
		gps_load_settings_flag = KAL_TRUE;
	}

	if (ind_p->Status == GPSLOCATE_NWSTATUS_OK)
	{
		if (gps_system_shutting_down == KAL_FALSE)
		{
			//flash the network indicator LED
			GPSLocateLEDIndicatorFlash(GPS_LOC_IND_NETWORK,
									KAL_TRUE,
									2 * KAL_TICKS_100_MSEC,
									GPS_IND_NWSTATUS_INTV * KAL_TICKS_1_SEC);
		}
#ifdef GPS_POWERON_PHONECALL
		if (BMT.PWRon == PWRKEYPWRON && 
			GPIO_ReadIO(GPS_SMS_MODE_GPIO) && /* only make phone call and send sms when PIN19 is high */
			gps_firstcall_flag == KAL_FALSE)
		{
			// Only get here when system normal power on
			//GPS_APP_MakeCallAndSndSmsStart();
			gps_firstcall_flag = KAL_TRUE;
    		gps_app_log("Start first call to number %d: %s",
						1, gps_usr_numbers[0].number);
			MakeGPSCallReq((kal_uint8 *)gps_usr_numbers[0].number, NULL);
			gps_get_nwinfo_request(RR_EM_LAI_INFO | RR_EM_MEASUREMENT_REPORT_INFO);
			gps_1stcall_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GPS_APP_MakeCallAndSendSmsHandler,
									KAL_TICKS_1_SEC * 2,
									0,
									KAL_TRUE);
		}
#endif
	}
	else
	{
		if (gps_system_shutting_down == KAL_FALSE)
		{
			//drive the network indicator LED always on
			GPSLocateLEDIndicatorAlwaysOn(GPS_LOC_IND_NETWORK, KAL_TRUE);
		}
	}
}

//-----------------------------------------------------------------------------
static void gps_app_del_sms_done_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/

}

//-----------------------------------------------------------------------------
static void gps_app_send_sms_done_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_app_log("gps_app_send_sms_done_ind_hdlr");

	if (GPS_APP_GetTimingLocateFlag() != KAL_TRUE &&
		GPS_APP_GetMultiSendFlag() != KAL_TRUE)
	{
		gps_app_log("gps_app_send_sms_done_ind_hdlr, delete all sms.");
		GPS_SMS_DeleteAllSms();
	}

	if (GPS_APP_GetMultiSendFlag() == KAL_TRUE)
	{
		if (gps_multi_send_index <= 0 ||
			gps_multi_send_index >= GPSLOCATE_USER_NUMBER_MAX)
		{
			gps_app_log("gps_app_send_sms_done_ind_hdlr, reply sms to all numbers in saved list done.");
			// reset multi send flag and index
			GPS_APP_SetMultiSendFlag(KAL_FALSE);

			gps_multi_send_index = 0;
		}
		else
		{
			for (; gps_multi_send_index < GPSLOCATE_USER_NUMBER_MAX; gps_multi_send_index++)
			{
				if (strlen(gps_tgt_numgrp_sms[gps_multi_send_index].number))
					break;
			}
			if (gps_multi_send_index >= GPSLOCATE_USER_NUMBER_MAX)
			{
				gps_app_log("gps_app_send_sms_done_ind_hdlr, reply sms to all numbers in saved list done.");
				// reset multi send flag and index
				GPS_APP_SetMultiSendFlag(KAL_FALSE);
				return ;
			}

			if (gps_curr_sms_mode != GPS_SMS_MODE_SC)
			{
				strcpy((char *)gps_act_remote_num.Number, gps_tgt_numgrp_sms[gps_multi_send_index].number);
				gps_act_remote_num.Type = GPS_APP_GetNumberType((char *)gps_act_remote_num.Number);
				gps_act_remote_num.Length = strlen((char *)gps_act_remote_num.Number);

				GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
				gps_app_log("gps_app_send_sms_done_ind_hdlr, reply sms \"%s\" to number %d: %s", 
							gps_locate_info_buff, 
							gps_multi_send_index+1, 
							gps_act_remote_num.Number);
			}
			else
			{
				GPS_APP_StrReplace(gps_locate_info_buff, (char *)gps_act_remote_num.Number, gps_tgt_numgrp_sms[gps_multi_send_index].number);
				GPS_SMS_ConstructAndSendSms(&gps_act_sc_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
				gps_app_log("gps_app_send_sms_done_ind_hdlr, reply sms \"%s\" to number: %s", 
							gps_locate_info_buff, 
							gps_act_sc_num.Number);

				strcpy((char *)gps_act_remote_num.Number, gps_tgt_numgrp_sms[gps_multi_send_index].number);
				gps_act_remote_num.Type = GPS_APP_GetNumberType((char *)gps_act_remote_num.Number);
				gps_act_remote_num.Length = strlen((char *)gps_act_remote_num.Number);
			}

			gps_multi_send_index++;
		}
	}
}

//-----------------------------------------------------------------------------
static void gps_app_reg_state_info_ind_hdlr(ilm_struct * ilm_ptr)
{
	GPSLocateRegStateStruct_t* ind_p;
	int i;

	gps_app_log("gps_app_reg_state_info_ind_hdlr");
	ind_p = (GPSLocateRegStateStruct_t*)ilm_ptr->local_para_ptr;
	switch(ind_p->IndCategory)
	{
	case GPSLOCATE_RR_EM_LAI_INFO:
		gps_app_log("mcc=%d%d%d, mnc=%d%d%x, cell_id=%x, lac_value=%x", 
				ind_p->mcc[0], ind_p->mcc[1], ind_p->mcc[2],
				ind_p->mnc[0], ind_p->mnc[1], ind_p->mnc[2],
				ind_p->cell_id, ind_p->lac_value);
		memcpy(gps_reg_state.mcc, ind_p->mcc, 3);
		memcpy(gps_reg_state.mnc, ind_p->mnc, 3);
		gps_reg_state.lac_value = ind_p->lac_value;
		gps_reg_state.cell_id = ind_p->cell_id;

		break;

	case GPSLOCATE_RR_EM_MEASUREMENT_REPORT_INFO:
		gps_app_log("arfcn\t\trssi");
		for (i = 0; i < GSM_NEIGHBOR_CELL_MAX+1; i++)
		{
			gps_app_log("%d\t\t%d", ind_p->nc_arfcn[i], ind_p->rla_in_quarter_dbm[i]);
		}
		memcpy(gps_reg_state.nc_arfcn, ind_p->nc_arfcn, sizeof(gps_reg_state.nc_arfcn));
		memcpy(gps_reg_state.rla_in_quarter_dbm, ind_p->rla_in_quarter_dbm, sizeof(gps_reg_state.rla_in_quarter_dbm));
		memcpy(gps_reg_state.C1, ind_p->C1, sizeof(gps_reg_state.C1));
		memcpy(gps_reg_state.C2, ind_p->C2, sizeof(gps_reg_state.C2));

		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
static void gps_app_battery_status_ind_hdlr(ilm_struct * ilm_ptr)
{
	GPSLocateBatteryStatusIndStruct_t* ind_p;
	static kal_uint8 lp_warn_count = 0;

	ind_p = (GPSLocateBatteryStatusIndStruct_t*)ilm_ptr->local_para_ptr;
	gps_app_log("gps_app_battery_status_ind_hdlr: status=%d, level=%d", 
				ind_p->Status, 
				ind_p->Level);

	switch (ind_p->Status)
	{
	case GPSLOCATE_VBAT_STATUS:
		gps_lowbattery_warnning = (ind_p->Level == GPSLOCATE_BATTERY_LOW_WARNING) ? KAL_TRUE : KAL_FALSE;
		lp_warn_count = (ind_p->Level == GPSLOCATE_BATTERY_LOW_WARNING) ? lp_warn_count + 1 : 0;

		if (ind_p->Level == GPSLOCATE_BATTERY_LOW_WARNING)
		{
			if (gps_system_shutting_down == KAL_FALSE && gps_app_is_charger_connected == KAL_FALSE)
			{
				//flash the battery indicator LED
				GPSLocateLEDIndicatorFlash(GPS_LOC_IND_BATTERY,
										KAL_TRUE,
										2 * KAL_TICKS_100_MSEC,
										KAL_TICKS_1_SEC);
			}

			// low battery upload not more than GPS_LOWBAT_WARN_TIMES times
			if (lp_warn_count > GPS_LOWBAT_WARN_TIMES)
			{
				return;
			}

			// battery low warning, 
			// upload current gps info immediately through current bearer type setting
			switch (gps_curr_bearer_type)
			{
			case GPS_BEARER_SMS:
		      	GPS_APP_LowBattWarnSendSmsStart(GPS_SAVEDNUMBERTYPE_USER);

				break;

			case GPS_BEARER_GPRS:
				{
				kal_uint8 *tmpPtr;
				GPSAppEvent_t tmpEvent;
				kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_LP;
				kal_bool fifo_push = KAL_TRUE;

				if (GPSAppEventFifoIsFull())
				{
					// fifo full, discard req
					gps_app_log("gps_app_battery_status_ind_hdlr, fifo full");
					return;
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
					gps_app_log("gps_app_battery_status_ind_hdlr, vbat low warnning gprs upload, push req into fifo, OpCode: %d", tmpEvent.OpCode);
				}
				}

				break;

			case GPS_BEARER_CSD:
			case GPS_BEARER_WIFI:
				// not support now, do nothing
				break;

			default:
				// unknown bearer type, do nothing
				break;
			}
		}

		break;

	case GPSLOCATE_CHARGER_IN:
	case GPSLOCATE_USB_CHARGER_IN:
		gps_app_is_charger_connected = KAL_TRUE;
		if (gps_system_shutting_down == KAL_FALSE)
		{
			//drive the battery LED always on
			GPSLocateLEDIndicatorAlwaysOn(GPS_LOC_IND_BATTERY, KAL_TRUE);
		}

		break;

	case GPSLOCATE_CHARGER_OUT:
	case GPSLOCATE_USB_CHARGER_OUT:
		gps_app_is_charger_connected = KAL_FALSE;
		if (gps_system_shutting_down == KAL_FALSE)
		{
			//drive the battery LED always off
			GPSLocateLEDIndicatorAlwaysOn(GPS_LOC_IND_BATTERY, KAL_FALSE);
		}

		break;

	case GPSLOCATE_CHARGE_COMPLETE:
		if (gps_system_shutting_down == KAL_FALSE)
		{
			//flash the battery indicator LED
			GPSLocateLEDIndicatorFlash(GPS_LOC_IND_BATTERY,
								KAL_TRUE,
								2 * KAL_TICKS_100_MSEC,
								3 * KAL_TICKS_1_SEC);
		}

		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
static void gps_app_simploc_timeout_ind_hdlr(ilm_struct * ilm_ptr)
{
    char *p_buf;
	int len;

	gps_app_log("gps_app_simploc_timeout_ind_hdlr, current active number: %s, type: %#x, len: %d", 
				gps_act_remote_num.Number, 
				gps_act_remote_num.Type, 
				gps_act_remote_num.Length);

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_SMSLOC, 
					KAL_FALSE);

#ifdef GPS_ITRACK_FORMAT
	memset(gps_locate_info_buff, 0, sizeof(gps_locate_info_buff));
	p_buf = gps_locate_info_buff;

	sprintf(p_buf, "%s%s,%s,", 
			gps_itrack_sfmsg,
			gps_imei_str,
			"TIMEOUT: GPS OUT OF WORK");
	p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
	switch (gps_smsupld_cause_type)
	{
	case GPS_RMCSMS_TYPE_SMS:
		sprintf(p_buf, "SMS,");
		break;
	case GPS_RMCSMS_TYPE_TIMER:
		sprintf(p_buf, "TIMER,");
		break;
	case GPS_RMCSMS_TYPE_CALL:
		sprintf(p_buf, "CALL,");
		break;
	case GPS_RMCSMS_TYPE_ANSWER:
		sprintf(p_buf, "ANSWER,");
		break;
	case GPS_RMCSMS_TYPE_LP:
		sprintf(p_buf, "LP,");
		break;
	case GPS_RMCSMS_TYPE_SOS:
		sprintf(p_buf, "SOS,");
		break;
	case GPS_RMCSMS_TYPE_ALARM:
		sprintf(p_buf, "DEF,");
		break;
	case GPS_RMCSMS_TYPE_GEOOS:
		sprintf(p_buf, "OS,");
		break;
	case GPS_RMCSMS_TYPE_GEORS:
		sprintf(p_buf, "RS,");
		break;
	default:
		sprintf(p_buf, ",");
		break;
	}
	p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
	len = strlen(gps_locate_info_buff);
	if (len + strlen(gps_itrack_srmsg) <= 160)
	{
		sprintf(p_buf, "%s", gps_itrack_srmsg);
	}
	else
	{
		snprintf(p_buf, 160-len, "%s", gps_itrack_srmsg);
	}

#else
	if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_START)
	{
		GPS_APP_SetLocateState(GPS_LOCATE_STATE_TIMEOUT);

		gps_locate_info_buff[0] = '\0';
		p_buf = gps_locate_info_buff;
		GPS_APP_ConstructCellInfoSMS(p_buf, gps_bs_num, GPS_INVALID_SEPARATOR_SIGN);

		p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
		sprintf(p_buf, "%cTIMEOUT: GPS OUT OF WORK", 
				GPS_INVALID_SEPARATOR_SIGN);

		if (gps_curr_bearer_type == GPS_BEARER_SMS && 
			gps_curr_sms_mode == GPS_SMS_MODE_SC)
		{
			p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
			sprintf(p_buf, "%c%s%c%s", 
					GPS_INVALID_SEPARATOR_SIGN, 
					gps_sc_number.passwd, 
					GPS_INVALID_SEPARATOR_SIGN, 
					(char *)gps_act_remote_num.Number);
		}
		p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
		sprintf(p_buf, "%c%c", 
				GPS_INVALID_SEPARATOR_SIGN, 
				GPS_INVALID_SEPARATOR_SIGN);
	}
	else if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_REPEAT)
	{
		GPS_APP_SetLocateState(GPS_LOCATE_STATE_TIMEOUT);
		if (strlen(gps_locate_info_buff) == 0)
		{
			gps_locate_info_buff[0] = '\0';
			p_buf = gps_locate_info_buff;
			GPS_APP_ConstructCellInfoSMS(p_buf, gps_bs_num, GPS_INVALID_SEPARATOR_SIGN);

			p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
			sprintf(p_buf, "%cTIMEOUT: GPS OUT OF WORK", 
					GPS_INVALID_SEPARATOR_SIGN);

			if (gps_curr_bearer_type == GPS_BEARER_SMS && 
				gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
				sprintf(p_buf, "%c%s%c%s", 
						GPS_INVALID_SEPARATOR_SIGN, 
						gps_sc_number.passwd, 
						GPS_INVALID_SEPARATOR_SIGN, 
						(char *)gps_act_remote_num.Number);
			}
			p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
			sprintf(p_buf, "%c%c", 
					GPS_INVALID_SEPARATOR_SIGN, 
					GPS_INVALID_SEPARATOR_SIGN);
		}
	}
	else
	{
		// ASSERT(0);
		return;
	}

	if (GPS_APP_GetVbatWarnFlag() == KAL_TRUE)
	{
		gps_locate_info_buff[0] = '@';
	}
#endif

	if (gps_curr_sms_mode != GPS_SMS_MODE_SC)
	{
		GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
		gps_app_log("gps_app_simploc_timeout_ind_hdlr, reply sms \"%s\" to number: %s", 
					gps_locate_info_buff, 
					gps_act_remote_num.Number);
	}
	else
	{
		GPS_SMS_ConstructAndSendSms(&gps_act_sc_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
		gps_app_log("gps_app_simploc_timeout_ind_hdlr, reply sms \"%s\" to number: %s", 
					gps_locate_info_buff, 
					gps_act_sc_num.Number);
	}

	gps_giveup_nwinfo_request();
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_DONE);
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_IDLE);
	// stop the timer
	GPS_APP_SingleLocateStop();
}

#ifdef GPS_POSITION_MONITOR
//-----------------------------------------------------------------------------
static void gps_app_setpos_timeout_ind_hdlr(ilm_struct * ilm_ptr)
{
	gps_app_log("gps_app_setpos_timeout_ind_hdlr, current active number: %s, type: %#x, len: %d", 
				gps_act_remote_num.Number, 
				gps_act_remote_num.Type, 
				gps_act_remote_num.Length);

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_POSSET, 
					KAL_FALSE);
	if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_START)
	{
		GPS_APP_SetLocateState(GPS_LOCATE_STATE_TIMEOUT);

		gps_locate_info_buff[0] = '\0';
		sprintf(gps_locate_info_buff, "TIMEOUT: GPS OUT OF WORK");
	}
	else if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_REPEAT)
	{
		GPS_APP_SetLocateState(GPS_LOCATE_STATE_TIMEOUT);
		if (strlen(gps_locate_info_buff) == 0)
		{
			sprintf(gps_locate_info_buff, "TIMEOUT: GPS OUT OF WORK");
		}
	}
	else
	{
		return;
	}

	GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
	gps_app_log("gps_app_setpos_timeout_ind_hdlr, reply sms \"%s\" to number: %s", 
				gps_locate_info_buff, 
				gps_act_remote_num.Number);

	GPS_APP_SetLocateState(GPS_LOCATE_STATE_DONE);
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_IDLE);
	GPS_APP_SetPositionStop();
}
#endif

#ifdef GPS_DEFENCE_FEATURE
//-----------------------------------------------------------------------------
static void gps_app_triggerdefence_ind_hdlr(ilm_struct * ilm_ptr)
{
	if (gps_mtcall_connecting == KAL_TRUE || 
		gps_mtcall_answered == KAL_TRUE ||
		gps_mocall_state != GPSLOCATE_MOCALLSTATE_IDLE)
	{
		// ignore defense trigger if 
		//   1, mt call connecting/connected
		//   2, there's any mo call connecting/connected or waiting to make a mo call
		return;
	}

	gps_defence_call_flag = KAL_TRUE;

	switch (gps_curr_bearer_type)
	{
	case GPS_BEARER_SMS:
		GPS_APP_DefSendSmsStart(GPS_SAVEDNUMBERTYPE_USER);

		// start to make phone call to the numbers in saved list at the same time
		GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_USER);

		break;

	case GPS_BEARER_GPRS:
		{
		kal_uint8 *tmpPtr;
		GPSAppEvent_t tmpEvent;
		kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_DEF;
		kal_bool fifo_push = KAL_TRUE;

		if (GPSAppEventFifoIsFull())
		{
			// fifo full, discard req
			gps_app_log("gps_app_triggerdefence_ind_hdlr, fifo full");
			return;
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
			gps_app_log("gps_app_triggerdefence_ind_hdlr, sos gprs upload, push req into fifo, OpCode: %d", tmpEvent.OpCode);
		}
		}

		// should start to make phone call to the numbers in saved list 
		// at the same time, but mtk 6225 only support gprs class b, 
		// so it cannot send data through gprs and make phone call at the 
		// same time. gprs class a can support send data through gprs and
		// make phone call at the same time.
		// now start to make phone call after send data through gprs done.
		// refer gps_tcpip_send_result_cb()
		// GPS_APP_SosMOCallStart();

		break;

	case GPS_BEARER_CSD:
	case GPS_BEARER_WIFI:
		// not support now, do nothing
		break;

	default:
		// unknown bearer type, do nothing
		break;
	}
}
#endif

//-----------------------------------------------------------------------------
static void gps_app_sys_shutting_down_ind_hdlr(ilm_struct * ilm_ptr)
{
	//set this flag to indicate the system is shutting down
	gps_system_shutting_down = KAL_TRUE;	
}

#ifdef GPS_BACKUPLOAD_DAEMON
//-----------------------------------------------------------------------------
static void gps_app_backupload_timer_ind_hdlr(ilm_struct * ilm_ptr)
{
	kal_uint8 *tmpPtr;
	GPSAppEvent_t tmpEvent;
	kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_BACKUP;
	kal_uint16 tmpCount = 0;
	kal_bool fifo_push = KAL_TRUE;

	if (gps_curr_bearer_type != GPS_BEARER_GPRS)
	{
		return;
	}

	GPSAppDataBackupLoad(NULL, &tmpCount);
	gps_app_log("gps_app_backupload_timer_ind_hdlr, backup file count: %d", tmpCount);
	if (tmpCount <= 0)
	{
		return;
	}

	if (GPSAppEventFifoIsFull())
	{
		// fifo full, discard req
		gps_app_log("gps_app_backupload_timer_ind_hdlr, fifo full");
		return;
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
		gps_app_log("gps_app_backupload_timer_ind_hdlr, push req into fifo, OpCode: %d", tmpEvent.OpCode);
	}
}
#endif

static void gps_app_ppbuffull4loading_ind_hdlr(ilm_struct * ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocatePPBufFull4LoadingIndStruct_t *ind_p;
	kal_uint8 *tmpPtr;
	GPSAppEvent_t tmpEvent;
	kal_uint8 cause_type = GPS_GPRS_UPLDCAUSETYPE_AUTO;
	kal_bool fifo_push = KAL_TRUE;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p = (GPSLocatePPBufFull4LoadingIndStruct_t*) ilm_ptr->local_para_ptr;

	cause_type = GPS_APP_GPRSUpldMode() ? GPS_GPRS_UPLDCAUSETYPE_AUTO : GPS_GPRS_UPLDCAUSETYPE_AUTOLOW;
	if (GPSAppEventFifoIsFull())
	{
		// fifo full, discard req
		gps_app_log("gps_app_ppbuffull4loading_ind_hdlr, fifo full");
#ifdef GPS_BACKUPLOAD_DAEMON
		// store data into backup file
		GPSAppDataBackupStore(
						(kal_uint8 *)ind_p->buff, 
						ind_p->buff_len * sizeof(GPS_GPRMC_Packed_Struct_t)
						);
#endif
		return;
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
		gps_app_log("gps_app_ppbuffull4loading_ind_hdlr, push req into fifo, OpCode: %d", tmpEvent.OpCode);
	}
}

#ifdef VIBRATION_SENSOR_SUPPORT
static void gps_app_vibsensor_ind_hdlr(ilm_struct * ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateVibSensorStatusIndStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p = (GPSLocateVibSensorStatusIndStruct_t*) ilm_ptr->local_para_ptr;
	gps_app_log("gps_app_vibsensor_ind_hdlr, current status: %d", ind_p->Status);

	if (ind_p->Status == GPSLOCATE_VIBSENSORSTATUS_VIBRATING)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}
	else if (ind_p->Status == GPSLOCATE_VIBSENSORSTATUS_SILENCE)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
}
#endif //VIBRATION_SENSOR_SUPPORT

/*************************************************************************
* Function: SendMsg2GPSApp
* Usage: Send message to GPS App task
* Parameters:
*	SrcMode		-	source module ID
*	MsgID		-	message ID
*	pLocalData	-	pointer to local data memory
*					This block of memory should be allocated by the caller
*					invoking 'construct_local_para'.
*	DataLen		-	length of the local data structure
*************************************************************************/
void SendMsg2GPSApp(
			const unsigned short SrcMod,
			const unsigned short MsgID,
			void* pLocalData
			)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    ilm_struct *send_ilm;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    send_ilm = allocate_ilm(SrcMod);
    send_ilm->src_mod_id = SrcMod;
    send_ilm->dest_mod_id = MOD_GPS_APP_TASK;
    send_ilm->msg_id = MsgID;
    send_ilm->local_para_ptr = (local_para_struct *)pLocalData;

    msg_send_ext_queue(send_ilm);
}

//-----------------------------------------------------------------------------
static void gps_app_gprmcframe4smsloc_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
    Result_t result = RESULT_ERROR;
    kal_bool valid_stat;

#ifdef GPS_ITRACK_FORMAT
	char *p_buf;
	char tmpBuf[128];

	strcpy(tmpBuf, rmc_src);
	GPS_APP_StrReplace(tmpBuf, "$GPRMC", "GPRMC");
	GPS_APP_StrReplace(tmpBuf, "*", ",");
	GPS_APP_StrReplace(tmpBuf, "\r\n", "");

	memset(gps_locate_info_buff, 0, sizeof(gps_locate_info_buff));
	p_buf = gps_locate_info_buff;

	sprintf(p_buf, "%s%s,%s,", 
			gps_itrack_sfmsg,
			gps_imei_str,
			tmpBuf);
	p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
	switch (gps_smsupld_cause_type)
	{
	case GPS_RMCSMS_TYPE_SMS:
		sprintf(p_buf, "SMS,");
		break;
	case GPS_RMCSMS_TYPE_TIMER:
		sprintf(p_buf, "TIMER,");
		break;
	case GPS_RMCSMS_TYPE_CALL:
		sprintf(p_buf, "CALL,");
		break;
	case GPS_RMCSMS_TYPE_ANSWER:
		sprintf(p_buf, "ANSWER,");
		break;
	case GPS_RMCSMS_TYPE_LP:
		sprintf(p_buf, "LP,");
		break;
	case GPS_RMCSMS_TYPE_SOS:
		sprintf(p_buf, "SOS,");
		break;
	case GPS_RMCSMS_TYPE_ALARM:
		sprintf(p_buf, "DEF,");
		break;
	case GPS_RMCSMS_TYPE_GEOOS:
		sprintf(p_buf, "OS,");
		break;
	case GPS_RMCSMS_TYPE_GEORS:
		sprintf(p_buf, "RS,");
		break;
	default:
		sprintf(p_buf, ",");
		break;
	}
	p_buf = gps_locate_info_buff + strlen(gps_locate_info_buff);
	len = strlen(gps_locate_info_buff);
	if (len + strlen(gps_itrack_srmsg) <= 160)
	{
		sprintf(p_buf, "%s", gps_itrack_srmsg);
	}
	else
	{
		snprintf(p_buf, 160-len, "%s", gps_itrack_srmsg);
	}
#else
	memset(gps_locate_info_buff, 0, sizeof(gps_locate_info_buff));

	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result != RESULT_OK)
	{
		if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_START ||
			GPS_APP_GetLocateState() == GPS_LOCATE_STATE_REPEAT)
		{
			gps_app_log("gps_app_gprmcframe4smsloc_hdlr, error gprmc frame data, repeat...");

			if (gps_curr_bearer_type == GPS_BEARER_SMS && gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				GPS_APP_ConstructGprmcSMS(gps_locate_info_buff, NULL, GPS_INVALID_SEPARATOR_SIGN);
			}
			else
			{
				GPS_APP_ConstructReadableGprmcSMS(gps_locate_info_buff, NULL, gps_smsupld_cause_type);
			}

			if (GPS_APP_GetVbatWarnFlag() == KAL_TRUE)
				goto REPLY_IMMEDIATELY;

			SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
								GPSLOCATE_GPRMCDEST_APPTASK, 
								GPSLOCATE_GPRMCOBJECTIVE_SMSLOC, 
								KAL_TRUE);
			GPS_APP_SetLocateState(GPS_LOCATE_STATE_REPEAT);
		}
		else
		{
			// ASSERT(0);
		}
		return;
	}

	valid_stat = GPS_APP_CheckGPRMCData(&gprmc_data);
	if (valid_stat != KAL_TRUE)
	{
		if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_START ||
			GPS_APP_GetLocateState() == GPS_LOCATE_STATE_REPEAT)
		{
			gps_app_log("gps_app_gprmcframe4smsloc_hdlr, invalid gprmc frame data, repeat...");

			if (gps_curr_bearer_type == GPS_BEARER_SMS && gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				GPS_APP_ConstructGprmcSMS(gps_locate_info_buff, &gprmc_data, GPS_INVALID_SEPARATOR_SIGN);
			}
			else
			{
				GPS_APP_ConstructReadableGprmcSMS(gps_locate_info_buff, &gprmc_data, gps_smsupld_cause_type);
			}

			if (GPS_APP_GetVbatWarnFlag() == KAL_TRUE)
				goto REPLY_IMMEDIATELY;

			SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
								GPSLOCATE_GPRMCDEST_APPTASK, 
								GPSLOCATE_GPRMCOBJECTIVE_SMSLOC, 
								KAL_TRUE);
			GPS_APP_SetLocateState(GPS_LOCATE_STATE_REPEAT);
		}
		else
		{
			// ASSERT(0);
		}
		return;
	}

	if (gps_curr_bearer_type == GPS_BEARER_SMS && gps_curr_sms_mode == GPS_SMS_MODE_SC)
	{
		GPS_APP_ConstructGprmcSMS(gps_locate_info_buff, &gprmc_data, GPS_VALID_SEPARATOR_SIGN);
	}
	else
	{
		GPS_APP_ConstructReadableGprmcSMS(gps_locate_info_buff, &gprmc_data, gps_smsupld_cause_type);
	}

REPLY_IMMEDIATELY:
	if (GPS_APP_GetVbatWarnFlag() == KAL_TRUE &&
		gps_curr_bearer_type == GPS_BEARER_SMS && 
		gps_curr_sms_mode == GPS_SMS_MODE_SC)
	{
		gps_locate_info_buff[0] = '@';
	}
#endif

	if (gps_curr_sms_mode != GPS_SMS_MODE_SC)
	{
		GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
		gps_app_log("gps_app_gprmcframe4smsloc_hdlr, reply sms \"%s\" to number: %s", 
					gps_locate_info_buff, 
					gps_act_remote_num.Number);
	}
	else
	{
		GPS_SMS_ConstructAndSendSms(&gps_act_sc_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
		gps_app_log("gps_app_gprmcframe4smsloc_hdlr, reply sms \"%s\" to number: %s", 
					gps_locate_info_buff, 
					gps_act_sc_num.Number);
	}

	gps_giveup_nwinfo_request();
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_DONE);
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_IDLE);
	//stop the timer
	GPS_APP_SingleLocateStop();
}

//-----------------------------------------------------------------------------
static void gps_app_gprmcframe4gprsupld_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
    Result_t result = RESULT_ERROR;
	
	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result == RESULT_OK)
	{
		GPS_GPRMC_Packed_Struct_t gprmc_pack;
		Result_t res2 = RESULT_ERROR;
		
		res2 = GPS_APP_GPRMC_Raw2Packed(&gprmc_pack, &gprmc_data);
		if (res2 == RESULT_OK)
		{
			// add current gsm cell info(lac & cell id) into gprmc packed data
			gprmc_pack.lac = gps_reg_state.lac_value;
			gprmc_pack.cid = gps_reg_state.cell_id;
			
			GPSPPBufferStore(&gprmc_pack, 1);
		}
	}
	gps_giveup_nwinfo_request();
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
}

#ifdef GPS_POSITION_MONITOR
//-----------------------------------------------------------------------------
static void gps_app_gprmcframe4posset_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
    Result_t result = RESULT_ERROR;

	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result == RESULT_OK && GPS_APP_CheckGPRMCData(&gprmc_data) == KAL_TRUE)
	{
	    GPS_GPRMC_Packed_Struct_t pack_data;
		Result_t res2 = RESULT_ERROR;
		
		res2 = GPS_APP_GPRMC_Raw2Packed(&pack_data, &gprmc_data);
		if (res2 == RESULT_OK)
		{
			gps_curr_position.latitude_d = pack_data.latitude_d;
			gps_curr_position.latitude_c = pack_data.latitude_c;
			gps_curr_position.latitude_cf = pack_data.latitude_cf;
			gps_curr_position.latitude_ns = pack_data.latitude_ns;

			gps_curr_position.longitude_d = pack_data.longitude_d;
			gps_curr_position.longitude_c = pack_data.longitude_c;
			gps_curr_position.longitude_cf = pack_data.longitude_cf;
			gps_curr_position.longitude_ew = pack_data.longitude_ew;

			sprintf(gps_locate_info_buff, "Position set to %s,%c,%s,%c,R%d.%d",
					gprmc_data.latitude, 
					gprmc_data.ns, 
					gprmc_data.longitude, 
					gprmc_data.ew, 
					gps_curr_position.radius, 
					gps_curr_position.radius_f);
			GPS_APP_WriteFixPosition(&gps_curr_position);
		}
		else
		{
			sprintf(gps_locate_info_buff, "INVALID GPS DATA");
		}
	}
	else
	{
		if (GPS_APP_GetLocateState() == GPS_LOCATE_STATE_START ||
			GPS_APP_GetLocateState() == GPS_LOCATE_STATE_REPEAT)
		{
			gps_app_log("gps_app_gprmcframe4posset_hdlr, invalid gprmc frame, repeat...");

			sprintf(gps_locate_info_buff, "INVALID GPS DATA");

			SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
								GPSLOCATE_GPRMCDEST_APPTASK, 
								GPSLOCATE_GPRMCOBJECTIVE_POSSET, 
								KAL_TRUE);
			GPS_APP_SetLocateState(GPS_LOCATE_STATE_REPEAT);
		}
		return;
	}

	GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
	gps_app_log("gps_app_gprmcframe4posset_hdlr, reply sms \"%s\" to number: %s", 
				gps_locate_info_buff, 
				gps_act_remote_num.Number);

	GPS_APP_SetLocateState(GPS_LOCATE_STATE_DONE);
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
	GPS_APP_SetLocateState(GPS_LOCATE_STATE_IDLE);
	GPS_APP_SetPositionStop();
}

//-----------------------------------------------------------------------------
static void gps_app_gprmcframe4posmonitor_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
	GPS_GPRMC_Packed_Struct_t gprmc_pack;
    Result_t result = RESULT_ERROR;
	Result_t res2 = RESULT_ERROR;
    kal_bool valid_stat;
	kal_bool out_current;
	static kal_bool out_last = KAL_FALSE;

	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result != RESULT_OK) return;

	valid_stat = GPS_APP_CheckGPRMCData(&gprmc_data);
	if (valid_stat != KAL_TRUE) return;

	res2 = GPS_APP_GPRMC_Raw2Packed(&gprmc_pack, &gprmc_data);
	if (res2 != RESULT_OK) return;

	out_current = GPS_APP_OutofRange(&gps_curr_position, &gprmc_pack);
	if (out_current != out_last)
	{
		out_last = out_current;
		switch (gps_curr_bearer_type)
		{
		case GPS_BEARER_SMS:
	      	//GPS_APP_GeoSendSmsStart(GPS_SAVEDNUMBERTYPE_USER, out_current);
	      	gps_tgt_numgrp_sms = gps_usr_numbers;
			{
			int i;

			for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
			{
				if (strlen(gps_tgt_numgrp_sms[i].number))
					break;
			}
			if (i >= GPSLOCATE_USER_NUMBER_MAX)
			{
				goto out;
			}

			strcpy((char *)gps_act_remote_num.Number, gps_tgt_numgrp_sms[i].number);
			gps_act_remote_num.Type = GPS_APP_GetNumberType((char *)gps_act_remote_num.Number);
			gps_act_remote_num.Length = strlen((char *)gps_act_remote_num.Number);

			gps_smsupld_cause_type = out_current ? GPS_RMCSMS_TYPE_GEOOS : GPS_RMCSMS_TYPE_GEORS;
			if (gps_curr_bearer_type == GPS_BEARER_SMS && gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				GPS_APP_ConstructGprmcSMS(gps_locate_info_buff, &gprmc_data, GPS_VALID_SEPARATOR_SIGN);
			}
			else
			{
				GPS_APP_ConstructReadableGprmcSMS(gps_locate_info_buff, &gprmc_data, gps_smsupld_cause_type);
			}

			if (GPS_APP_GetVbatWarnFlag() == KAL_TRUE &&
				gps_curr_bearer_type == GPS_BEARER_SMS && 
				gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				gps_locate_info_buff[0] = '@';
			}

			if (gps_curr_sms_mode != GPS_SMS_MODE_SC)
			{
				GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
				gps_app_log("gps_app_gprmcframe4posmonitor_hdlr, reply sms \"%s\" to number: %s", 
							gps_locate_info_buff, 
							gps_act_remote_num.Number);
			}
			else
			{
				GPS_SMS_ConstructAndSendSms(&gps_act_sc_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
				gps_app_log("gps_app_gprmcframe4posmonitor_hdlr, reply sms \"%s\" to number: %s", 
							gps_locate_info_buff, 
							gps_act_sc_num.Number);
			}
	      	}

			// start to make phone call to the numbers in saved list at the same time
			GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_USER);

			break;

		case GPS_BEARER_GPRS:
			{
			kal_uint8 *tmpPtr;
			GPSAppEvent_t tmpEvent;
			kal_uint8 cause_type = out_current ? GPS_GPRS_UPLDCAUSETYPE_GEOOS : GPS_GPRS_UPLDCAUSETYPE_GEORS;
			kal_bool fifo_push = KAL_TRUE;

			if (GPSAppEventFifoIsFull())
			{
				// fifo full, discard req
				gps_app_log("gps_app_gprmcframe4posmonitor_hdlr, fifo full");
				return;
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
				gps_app_log("gps_app_gprmcframe4posmonitor_hdlr, sos gprs upload, push req into fifo, OpCode: %d", tmpEvent.OpCode);
			}
			}

			// should start to make phone call to the numbers in saved list 
			// at the same time, but mtk 6225 only support gprs class b, 
			// so it cannot send data through gprs and make phone call at the 
			// same time. gprs class a can support send data through gprs and
			// make phone call at the same time.
			// now start to make phone call after send data through gprs done.
			// refer gps_tcpip_send_result_cb()
			// GPS_APP_SosMOCallStart();

			break;

		case GPS_BEARER_CSD:
		case GPS_BEARER_WIFI:
			// not support now, do nothing
			break;

		default:
			// unknown bearer type, do nothing
			break;
		}
	}

out:
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
}
#endif

#ifdef GPS_RATE_MONITOR
//-----------------------------------------------------------------------------
static void gps_app_gprmcframe4ratemonitor_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
	GPS_GPRMC_Packed_Struct_t gprmc_pack;
    Result_t result = RESULT_ERROR;
	Result_t res2 = RESULT_ERROR;
    kal_bool valid_stat;
	kal_bool over = KAL_FALSE;
	static kal_bool over_last = KAL_FALSE;

	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result != RESULT_OK) return;

	valid_stat = GPS_APP_CheckGPRMCData(&gprmc_data);
	if (valid_stat != KAL_TRUE) return;

	res2 = GPS_APP_GPRMC_Raw2Packed(&gprmc_pack, &gprmc_data);
	if (res2 != RESULT_OK) return;

	over = (double)gprmc_pack.rate * 1.852 > (double)gps_rate_limit;
	if (over != over_last)
	{
		//GpsAppMmiBrgRunSmsVibratorOnce();
		over_last = over;
		switch (gps_curr_bearer_type)
		{
		case GPS_BEARER_SMS:
	      	//GPS_APP_GeoSendSmsStart(GPS_SAVEDNUMBERTYPE_USER, out_current);
	      	gps_tgt_numgrp_sms = gps_usr_numbers;
			{
			int i;

			for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
			{
				if (strlen(gps_tgt_numgrp_sms[i].number))
					break;
			}
			if (i >= GPSLOCATE_USER_NUMBER_MAX)
			{
				goto out;
			}

			strcpy((char *)gps_act_remote_num.Number, gps_tgt_numgrp_sms[i].number);
			gps_act_remote_num.Type = GPS_APP_GetNumberType((char *)gps_act_remote_num.Number);
			gps_act_remote_num.Length = strlen((char *)gps_act_remote_num.Number);

			gps_smsupld_cause_type = over ? GPS_RMCSMS_TYPE_OVERSPEED : GPS_RMCSMS_TYPE_SAFESPEED;
			if (gps_curr_bearer_type == GPS_BEARER_SMS && gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				GPS_APP_ConstructGprmcSMS(gps_locate_info_buff, &gprmc_data, GPS_VALID_SEPARATOR_SIGN);
			}
			else
			{
				GPS_APP_ConstructReadableGprmcSMS(gps_locate_info_buff, &gprmc_data, gps_smsupld_cause_type);
			}

			if (GPS_APP_GetVbatWarnFlag() == KAL_TRUE &&
				gps_curr_bearer_type == GPS_BEARER_SMS && 
				gps_curr_sms_mode == GPS_SMS_MODE_SC)
			{
				gps_locate_info_buff[0] = '@';
			}

			if (gps_curr_sms_mode != GPS_SMS_MODE_SC)
			{
				GPS_SMS_ConstructAndSendSms(&gps_act_remote_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
				gps_app_log("gps_app_gprmcframe4ratemonitor_hdlr, reply sms \"%s\" to number: %s", 
							gps_locate_info_buff, 
							gps_act_remote_num.Number);
			}
			else
			{
				GPS_SMS_ConstructAndSendSms(&gps_act_sc_num, gps_locate_info_buff, strlen(gps_locate_info_buff));
				gps_app_log("gps_app_gprmcframe4ratemonitor_hdlr, reply sms \"%s\" to number: %s", 
							gps_locate_info_buff, 
							gps_act_sc_num.Number);
			}
	      	}

#ifndef GPS_SENDALL2FIRSTNUMBER
			// start to make phone call to the numbers in saved list at the same time
			GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_USER);
#endif

			break;

		case GPS_BEARER_GPRS:
			{
			kal_uint8 *tmpPtr;
			GPSAppEvent_t tmpEvent;
			kal_uint8 cause_type = over ? GPS_GPRS_UPLDCAUSETYPE_OVERSPEED : GPS_GPRS_UPLDCAUSETYPE_SAFESPEED;
			kal_bool fifo_push = KAL_TRUE;

			if (GPSAppEventFifoIsFull())
			{
				// fifo full, discard req
				gps_app_log("gps_app_gprmcframe4posmonitor_hdlr, fifo full");
				return;
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
				gps_app_log("gps_app_gprmcframe4posmonitor_hdlr, sos gprs upload, push req into fifo, OpCode: %d", tmpEvent.OpCode);
			}
			}

			break;

		case GPS_BEARER_CSD:
		case GPS_BEARER_WIFI:
			// not support now, do nothing
			break;

		default:
			// unknown bearer type, do nothing
			break;
		}
	}

out:
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
}
#endif

#ifdef GPS_MOD_SWAUTOPOWER
static void gps_app_gprmcframe4modswpower_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
    Result_t result = RESULT_ERROR;
    kal_bool valid_stat;

	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result != RESULT_OK) goto NOT_VALID_RMC;

	valid_stat = GPS_APP_CheckGPRMCData(&gprmc_data);
	if (valid_stat != KAL_TRUE) goto NOT_VALID_RMC;

	// valid gprmc data, power down gps module for saving power consumption
	GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	// restart timer for next check cycle
	GPS_APP_ModSWPowerTimerStart();
	return;

NOT_VALID_RMC:
	if (!GPS_APP_CheckGPSModuleOn())
	{
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}

	SetGprmcDestAndObjective(MOD_GPS_APP_TASK, 
					GPSLOCATE_GPRMCDEST_APPTASK, 
					GPSLOCATE_GPRMCOBJECTIVE_MODSWPOWER, 
					KAL_TRUE);

}
#endif

//-----------------------------------------------------------------------------
static void gps_app_gprmcframe4gprsupldimm_hdlr(const char *rmc_src, int len)
{
    GPS_GPRMC_RawData_t gprmc_data;
    Result_t result = RESULT_ERROR;
	kal_bool send2tcp = KAL_FALSE;
	
	result = GPS_APP_ParseGPRMCData(rmc_src, &gprmc_data);
	if (result == RESULT_OK)
	{
		static GPS_GPRMC_Packed_Struct_t gprmc_pack;
		Result_t res2 = RESULT_ERROR;
		
		res2 = GPS_APP_GPRMC_Raw2Packed(&gprmc_pack, &gprmc_data);
		if (res2 == RESULT_OK)
		{
			if (GPSAppEventFifoIsEmpty())
			{
				GPS_GPRS_UploadInfo_t info;
				kal_uint8 *tmpPtr;
				GPSAppEvent_t tmpEvent;

				// add current gsm cell info(lac & cell id) into gprmc packed data
				gprmc_pack.lac = gps_reg_state.lac_value;
				gprmc_pack.cid = gps_reg_state.cell_id;
				
				info.cause_type = gps_upldimm_cause_type;
				info.src_type = GPS_GPRS_UPLDSRCTYPE_GPSUART;

				GPS_APP_SendPackData2TCPIP(&gprmc_pack, 1, (void *)&info);

				tmpPtr = (kal_uint8 *)get_ctrl_buffer(sizeof(kal_uint8));
				*tmpPtr = gps_upldimm_cause_type;
				tmpEvent.OpCode = GPS_APP_EVENT_OP_GPRSUPLOAD;
				tmpEvent.LocalPara = tmpPtr;
				GPSAppEventFifoPush(&tmpEvent, KAL_TRUE);
				gps_app_log("gps_app_gprmcframe4gprsupldimm_hdlr, push req into fifo, OpCode: %d", tmpEvent.OpCode);

				send2tcp = KAL_TRUE;
			}
		}
	}
	gps_giveup_nwinfo_request();
	if (gps_module_en_setting == GPS_MODULE_POWERSTATE_OFF)
	{
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}

	if (send2tcp) return;
	if (gps_upldimm_cause_type == GPS_GPRS_UPLDCAUSETYPE_SOS)
	{
		GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_SOS);
	}
	else if (gps_upldimm_cause_type == GPS_GPRS_UPLDCAUSETYPE_GEOOS ||
		gps_upldimm_cause_type == GPS_GPRS_UPLDCAUSETYPE_GEORS ||
		gps_upldimm_cause_type == GPS_GPRS_UPLDCAUSETYPE_DEF)
	{
		GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_USER);
	}
}

//-----------------------------------------------------------------------------
static void gps_app_nvramstore_ind_hdlr(ilm_struct *ilm_ptr)
{
	GPSLocateNvramWrData2FS();
}

static void gps_app_upldmodeswitch_ind_hdlr(ilm_struct *ilm_ptr)
{
	if (gps_curr_bearer_type != GPS_BEARER_GPRS)
		return;

	if (GPS_APP_GPRSUpldMode())
	{
		if (gps_upload_cnt > 0 && gps_samp_interval > 0)
		{
			GPSPPBufferReset(GPS_APP_PPBufferFullCB);
			GPSPPBufferSetThreshold(gps_upload_cnt);
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
	else
	{
		if (gps_upload_cnt2 > 0 && gps_samp_interval2 > 0)
		{
			GPSPPBufferReset(GPS_APP_PPBufferFullCB);
			GPSPPBufferSetThreshold(gps_upload_cnt2);
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


/*************************************************************************
* Function: SendUrgentMsg2GPSApp
* Usage: Send urgent message to GPS App task
* Parameters:
*	SrcMode		-	source module ID
*	MsgID		-	message ID
*	pLocalData	-	pointer to local data memory
*					This block of memory should be allocated by the caller
*					invoking 'construct_local_para'.
*************************************************************************/
void SendUrgentMsg2GPSApp(
			const unsigned short SrcMod,
			const unsigned short MsgID,
			void* pLocalData
			)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    ilm_struct *send_ilm;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    send_ilm = allocate_ilm(SrcMod);
    send_ilm->src_mod_id = SrcMod;
    send_ilm->dest_mod_id = MOD_GPS_APP_TASK;
    send_ilm->msg_id = MsgID;
    send_ilm->local_para_ptr = (local_para_struct *)pLocalData;

    msg_send_ext_queue_to_head(send_ilm);
}

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
			)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    ilm_struct *send_ilm;
    GPSLocateSamplingGprmcIndStruct_t* IndData = NULL;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	GPS_APP_SetUart2Sleep(!Flag);

    send_ilm = allocate_ilm(SrcMod);
    send_ilm->src_mod_id = SrcMod;
    send_ilm->dest_mod_id = MOD_GPS_UART;
    send_ilm->msg_id = MSG_ID_GPSLOCATE_SAMPLINGGPRMC_IND;

    IndData = (GPSLocateSamplingGprmcIndStruct_t*)construct_local_para(
                            sizeof(GPSLocateSamplingGprmcIndStruct_t),
                            TD_UL);
    IndData->Dest = Dest;
    IndData->Objective = Objective;
	IndData->Flag = Flag;

    send_ilm->local_para_ptr = (local_para_struct *)IndData;

    msg_send_ext_queue(send_ilm);
}

/*************************************************************************
* FUNCTION
*  gps_app_create
*
* DESCRIPTION
*
* PARAMETERS
*
* RETURNS
*  None
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_bool gps_app_create(comptask_handler_struct **handle)
{
   static const comptask_handler_struct gps_app_handler_info =
   {
      gps_app_main,  /* task entry function */
      gps_app_init,  /* task initialization function */
      NULL,  /* task configuration function */
      gps_app_reset,  /* task reset handler */
      NULL,  /* task termination handler */
   };

   *handle = (comptask_handler_struct *)&gps_app_handler_info;
   return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_app_main
* DESCRIPTION
*   This function is the main function of GPS APP task
* PARAMETERS
*   task_entry_ptr  IN   taks entry of GPS APP
* RETURNS
*   None.
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
void gps_app_main(task_entry_struct * task_entry_ptr)
{
   ilm_struct current_ilm;

#ifdef CODEPROTECTOR_FS8816_SUPPORT
	kal_bool bCodeProtectorResult;

	bCodeProtectorResult = CodeProtectorValidation();
	if (bCodeProtectorResult == KAL_TRUE)
	{
		/****** Process for validate successful ******/
	}
	else
	{
		/****** Process for validation fail ******/
	}
#endif //CODEPROTECTOR_FS8816_SUPPORT	

	/*
	Don't initialize key monitor and send LAI request, if power on cause is charger plug in
	*/
	if (BMT.PWRon != CHRPWRON)
	{
		GPSLocateKeyMonitorInit();
	}
	memset(&gps_reg_state, 0, sizeof(GPSLocateRegStateStruct_t));
	//turn on network indicator LED after normal power up, otherwise turn off
	GPSLocateLEDIndicatorAlwaysOn(GPS_LOC_IND_NETWORK, 
					(BMT.PWRon == PWRKEYPWRON) ? KAL_TRUE : KAL_FALSE);
	//turn on battery indicator LED after charger power up, otherwise turn off
	GPSLocateLEDIndicatorAlwaysOn(GPS_LOC_IND_BATTERY, 
					(BMT.PWRon == CHRPWRON) ? KAL_TRUE : KAL_FALSE);
#ifdef GPS_BACKUPLOAD_DAEMON
	//initialize data backup
	GPSAppDataBackupInit();
#endif

	GPS_UpldModeMonitorInit();
	gps_app_task_init_finished = KAL_TRUE;

   while ( 1 ) {
      receive_msg_ext_q(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);

#ifdef CODEPROTECTOR_FS8816_SUPPORT
	if (bCodeProtectorResult == KAL_FALSE)
	{
		//code protector validation fail, do not process any message
		GPS_SetTrace(KAL_TRUE);
		gps_app_log("VALIDATE FAIL!!!");
		free_ilm(&current_ilm);
		continue;
	}
#endif //CODEPROTECTOR_FS8816_SUPPORT	

	/*
	Don't process any messages rather than battery status, if the power on cause is
	charger plug in.
	*/
	if ((BMT.PWRon == CHRPWRON)
		&& (current_ilm.msg_id != MSG_ID_GPSLOCATE_BATTERY_STATUS_IND))
	{
		free_ilm(&current_ilm);
		continue;
	}
	
      switch (current_ilm.msg_id) {
      case MSG_ID_GPSLOCATE_MTSMS_REQ:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_MTSMS_REQ");
      	gps_app_mtsms_req_hdlr(&current_ilm);
      	break;
	
      case MSG_ID_GPSLOCATE_MTCALL_REQ:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_MTCALL_REQ");
      	gps_app_mtcall_req_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_MOCALL_RSP:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_MOCALL_RSP");
      	gps_app_mocall_rsp_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_KEYPAD_EVENT_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_KEYPAD_EVENT_IND");
      	gps_app_keypad_event_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_GPS_FRAME_DATA_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_GPS_FRAME_DATA_IND");
        gps_app_gps_frame_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_AT_CHANGE_PRESAVED_DATA_REQ:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_AT_CHANGE_PRESAVED_DATA_REQ");
      	gps_app_at_change_presaved_req_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_NW_ATTACH_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_NW_ATTACH_IND");
      	gps_app_nw_attach_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_DELETE_SMS_DONE_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_DELETE_SMS_DONE_IND");
      	gps_app_del_sms_done_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_SEND_SMS_DONE_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_SEND_SMS_DONE_IND");
      	gps_app_send_sms_done_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_REGSTATEINFO_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_REGSTATEINFO_IND");
      	gps_app_reg_state_info_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_BATTERY_STATUS_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_BATTERY_STATUS_IND");
      	gps_app_battery_status_ind_hdlr(&current_ilm);
      	break;

      case MSG_ID_GPSLOCATE_SIMPLOC_TIMEOUT_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_SIMPLOC_TIMEOUT_IND");
      	gps_app_simploc_timeout_ind_hdlr(&current_ilm);
      	break;

#ifdef GPS_POSITION_MONITOR
      case MSG_ID_GPSLOCATE_SETPOS_TIMEOUT_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_SETPOS_TIMEOUT_IND");
      	gps_app_setpos_timeout_ind_hdlr(&current_ilm);
      	break;
#endif

#ifdef GPS_DEFENCE_FEATURE
      case MSG_ID_GPSLOCATE_TRIGGER_DEFENCE:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_TRIGGER_DEFENCE");
      	gps_app_triggerdefence_ind_hdlr(&current_ilm);
      	break;
#endif

      case MSG_ID_GPSLOCATE_SYSTEM_SHUTTING_DOWN_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_SYSTEM_SHUTTING_DOWN_IND");
      	gps_app_sys_shutting_down_ind_hdlr(&current_ilm);
      	break;

#ifdef GPS_BACKUPLOAD_DAEMON
      case MSG_ID_GPSLOCATE_BACKUPLOADTIMER_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_BACKUPTIMER_IND");
      	gps_app_backupload_timer_ind_hdlr(&current_ilm);
      	break;
#endif

      case MSG_ID_GPSLOCATE_PPBUFFULL4LOADING_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_PPBUFFULL4LOADING_IND");
      	gps_app_ppbuffull4loading_ind_hdlr(&current_ilm);
      	break;

#ifdef VIBRATION_SENSOR_SUPPORT
	case MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_VIBSENSORSTATUS_IND");
      	gps_app_vibsensor_ind_hdlr(&current_ilm);
      	break;	
#endif //VIBRATION_SENSOR_SUPPORT

#ifndef GPS_NVRAM_TASK
	  case MSG_ID_GPSLOCATE_NVRAMSTORE_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_NVRAMSTORE_IND");
		gps_app_nvramstore_ind_hdlr(&current_ilm);
		break;
#endif

	  case MSG_ID_GPSLOCATE_SWITCHGPRSUPLD:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_SWITCHGPRSUPLD");
		gps_app_upldmodeswitch_ind_hdlr(&current_ilm);
		break;

#ifdef GPS_BACKUPLOAD_DAEMON
	  case MSG_ID_GPSLOCATE_DELCURRBAKF_IND:
      	gps_app_log("GPS APP receive msg: MSG_ID_GPSLOCATE_DELCURRBAKF_IND");
		GPSAppDataBackupDelCurrFile();
		break;
#endif

      default:
      	gps_app_log("GPS APP receive msg: UNKNOWN, id: %d, default do nothing", current_ilm.msg_id);
        break;
      }

      free_ilm(&current_ilm);
   }
}

/*****************************************************************************
* FUNCTION
*   gps_app_init
* DESCRIPTION
*   Init function if GPS APP task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
kal_bool gps_app_init(task_indx_type task_index)
{
	catcher_sys_trace("gps_app_init");

	//initialize pre-saved data items
	GPSLocateNvramInitDataFile();
	//intialize event FIFO
	GPSAppEventFifoInit();
	// disable/enable UART1 power down
#if 0
	DRVPDN_Disable(DRVPDN_CON1, DRVPDN_CON1_UART1, PDN_UART1);
#else
//	DRVPDN_Enable(DRVPDN_CON1,DRVPDN_CON1_UART1,PDN_UART1);
#endif

#if (defined GPS_GSG_SUPPORT) || (defined GPS_MOTOPWRCUT_FEATURE)
	//intialize GSG(for square wave generating)
	GSG_Init();
#endif
	//initialize LED indicator
	GPSLocateLEDIndicatorInit();

	return KAL_TRUE;
}


/*****************************************************************************
* FUNCTION
*   gps_app_reset
* DESCRIPTION
*   Reset function if GPS APP task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
kal_bool gps_app_reset(task_indx_type task_index)
{
	return KAL_TRUE;
}

kal_bool GPSAppTaskIsReady(void)
{
	return gps_app_task_init_finished;
}

#ifdef CODEPROTECTOR_FS8816_SUPPORT

#if (FS8816_PWR_CTRL)
static void CodeProtectorFS8816PwrCtrl(kal_bool On)
{
#if (FS8816_PWR_CTRL_GPIO == GPS_MODULE_ENALBE_GPIO)
	GPS_APP_GPSModulePwrCtrl(On, GPSMODULEPWRID_FS8816);
#else
	#if (FS8816_PWR_CTRL_GPIO < 16)
	DRV_Reg(GPIO_DOUT1) |= (1 << FS8816_PWR_CTRL_GPIO);
	#else
		#error "ERROR: Invalid definition for FS8816 pwr ctrl GPIO!"
	#endif
#endif //(FS8816_PWR_CTRL_GPIO == GPS_MODULE_ENALBE_GPIO)
}
#endif // FS8816_PWR_CTRL

static kal_bool CodeProtectorValidation(void)
{
	UCHAR Result;

	if (BMT.PWRon == CHRPWRON)
	{
		return KAL_TRUE;
	}

#if FS8816_PWR_CTRL
	//power on
	CodeProtectorFS8816PwrCtrl(KAL_TRUE);
#endif //FS8816_PWR_CTRL

#if FS8816_RST_CTRL
	//reset FS8816
	#if (FS8816_RST_CTRL_GPIO < 16)
	#error "ERROR: Not implement FS8816 reset!"
	#elif (FS8816_RST_CTRL_GPIO < 32)
	//drive high
	//DRV_Reg(GPIO_DOUT2) |= (1 << (FS8816_RST_CTRL_GPIO%16));
	//drive low
	DRV_Reg(GPIO_DOUT2) &= ~(1 << (FS8816_RST_CTRL_GPIO%16));
	kal_sleep_task(1);
	//drive high
	DRV_Reg(GPIO_DOUT2) |= (1 << (FS8816_RST_CTRL_GPIO%16));
	#else
	#error "ERROR: Invalid definition for FS8816 rst ctrl GPIO!"
	#endif
#endif //FS8816_RST_CTRL

	//initialize FS8816 core library
	Result = InitFS8806Lib(FS8816_SLVADDR<<1, (BYTE*)gps_app_codeprotector_key, 0x77);
	if (Result != FS8816API_SUCCESS)
	{
		//fail
	}
#if !(FS8816_PWR_CTRL)
	//wake up FS8816
	WakeFromPowerDownOpr();
#endif //!(FS8816_PWR_CTRL)
	//validate
	Result = Validation();
	//software power down FS8816
	PowerDownChipOpr();
	
#if FS8816_PWR_CTRL
	//shut down power supply
	CodeProtectorFS8816PwrCtrl(KAL_FALSE);
#endif //FS8816_PWR_CTRL

	return (Result == FS8816API_SUCCESS ? KAL_TRUE : KAL_FALSE);
}

#endif // CODEPROTECTOR_FS8816_SUPPORT

