/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_tcpip_main.c
 *
 * Project:
 * --------
 *   Maui_Software
 *
 * Description:
 * ------------
 *   This file implements gps tcpip component task create function
 *
 * Author:
 * -------
 * Robbie Cao
 * -------
 *
 *============================================================================
 *             HISTORY
 *============================================================================
 * 2007-12-16 22:44 - Initial create.
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
#include "csmcc_defs.h"
#include "soc_api.h"

#include "GPSLocateMsgDefs.h"
#include "gps_app_pp_buff_mgr.h"
#include "gps_app_event_fifo.h"
#include "gps_app_databackup.h"
#include "gps_app_mmi_bridge.h"
#include "gps_soc.h"
#include "gps_app.h"

/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Macro defines
 *************************************************************************/
#define GPS_TCPIP_DEBUG
#ifdef GPS_TCPIP_DEBUG
#define gps_tcpip_log        trace_printf
#else
static void gps_tcpip_log(kal_char *fmt, ...) { }
#endif

/*************************************************************************
 * Local variables
 *************************************************************************/
static kal_uint8 rcvd_buffer[GPS_MAX_RCV_BUFFER_SIZE];
static kal_uint8 snd_buffer[GPS_MAX_SND_BUFFER_SIZE];


/*************************************************************************
 * Golobal variables
 *************************************************************************/
extern GPS_Soc_Transaction_t gps_soc_transaction;

extern kal_uint8 custom_get_csd_profile_num(void);


/*************************************************************************
 * Function declaration
 *************************************************************************/
static void gps_tcpip_main(task_entry_struct * task_entry_ptr);
static kal_bool gps_tcpip_init(task_indx_type task_indx);
static kal_bool gps_tcpip_reset(task_indx_type task_index);

static void gps_tcpip_socket_notify_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_tcpip_send_req_hdlr(ilm_struct *ilm_ptr);
static void gps_tcpip_send_result_cb(int result, char *response, int response_len);

//-----------------------------------------------------------------------------
static void gps_tcpip_socket_notify_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_soc_socket_notify(ilm_ptr->local_para_ptr);
}

//-----------------------------------------------------------------------------
void gps_tcpip_send_req_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    GPSLocateSendThruGPRSReqStruct_t *ind_p;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    ind_p= (GPSLocateSendThruGPRSReqStruct_t*) ilm_ptr->local_para_ptr;
    gps_tcpip_log("gps_tcpip_send_req_hdlr, ip: %d.%d.%d.%d, port: %d", 
    			ind_p->ip_addr[0], 
    			ind_p->ip_addr[1], 
    			ind_p->ip_addr[2], 
    			ind_p->ip_addr[3],
    			ind_p->port);

	memset(&gps_soc_transaction, 0, sizeof(gps_soc_transaction));
	gps_soc_transaction.snd_buffer = (kal_char *)snd_buffer;
	gps_soc_transaction.rcvd_buffer = (kal_int8 *)rcvd_buffer;
	gps_soc_transaction.socket_id = -1;
    gps_soc_transaction.snd_counter = 0;
    gps_soc_transaction.rcvd_counter = 0;

    gps_soc_transaction.cause_type = ind_p->cause_type;
    gps_soc_transaction.src_type = ind_p->src_type;

	gps_soc_transaction.callback = gps_tcpip_send_result_cb;

    gps_soc_transaction.nwt_acount_id = MAX_CSD_PROFILE_NUM + GPS_GPRS_ACCOUNT_IDX - 1;	// APN: cmnet(default)

	gps_soc_transaction.server_ip_addr.port = ind_p->port;
	gps_soc_transaction.server_ip_addr.addr[0] = ind_p->ip_addr[0];
	gps_soc_transaction.server_ip_addr.addr[1] = ind_p->ip_addr[1];
	gps_soc_transaction.server_ip_addr.addr[2] = ind_p->ip_addr[2];
	gps_soc_transaction.server_ip_addr.addr[3] = ind_p->ip_addr[3];
	gps_soc_transaction.server_ip_addr.addr_len = 4;

    memcpy(gps_soc_transaction.rcvd_buffer, ind_p->data, ind_p->data_len*sizeof(GPS_GPRMC_Packed_Struct_t));
    gps_soc_transaction.snd_data_len = ind_p->data_len;
	gps_soc_transaction.snd_counter = 0;

#ifndef GPS_SOC_BLOCKING_MODE
    if (gps_soc_create_socket() == KAL_FALSE)
    {
        gps_soc_output_result(GPS_SOC_NO_MEMORY, NULL, 0);
        return ;
    }

    gps_soc_transaction.state = GPS_SOC_TCP_CON_CREATING;
    gps_tcpip_log("gps_tcpip_send_req_hdlr, transaction state: %d", gps_soc_transaction.state);

	gps_soc_tcp_send_request();

#else /* GPS_SOC_BLOCKING_MODE */

	gps_soc_tcp_send_request2();

#endif /* GPS_SOC_BLOCKING_MODE */
}

//-----------------------------------------------------------------------------
void gps_tcpip_send_result_cb(int result, char *response, int response_len)
{
	switch (gps_soc_transaction.src_type)
	{
	case GPS_GPRS_UPLDSRCTYPE_STBUFF:
		break;

	case GPS_GPRS_UPLDSRCTYPE_LDBUFF:
		GPSPPBufferFinishLoading();
		break;

#ifdef GPS_BACKUPLOAD_DAEMON
	case GPS_GPRS_UPLDSRCTYPE_BACKUP:
		if (result == GPS_SOC_SUCCESS)
		{
			// finish loading missing data from backup file
			SendMsg2GPSApp(
					stack_int_get_active_module_id(), 
					MSG_ID_GPSLOCATE_DELCURRBAKF_IND, 
					NULL);
		}
		break;
#endif

	default:
		break;
	}

	switch (gps_soc_transaction.cause_type)
	{
	case GPS_GPRS_UPLDCAUSETYPE_SMS:
	case GPS_GPRS_UPLDCAUSETYPE_CALL:
	case GPS_GPRS_UPLDCAUSETYPE_ANSWER:
		break;

	case GPS_GPRS_UPLDCAUSETYPE_AUTO:
	case GPS_GPRS_UPLDCAUSETYPE_AUTOLOW:
#ifdef GPS_BACKUPLOAD_DAEMON
		if (result != GPS_SOC_SUCCESS &&
			gps_soc_transaction.snd_data_len > 0)
		{
			// store missing data into backup file
			GPSAppDataBackupStore(
						(kal_uint8 *)gps_soc_transaction.rcvd_buffer, 
						gps_soc_transaction.snd_data_len * sizeof(GPS_GPRMC_Packed_Struct_t)
						);
		}
#endif
		break;

	case GPS_GPRS_UPLDCAUSETYPE_SOS:
#ifdef GPS_VIB_4SOSGPRSUPLDSUC
		// play vibrator once to let user know SOS require has been 
		// sent to server through gprs
		if (result == GPS_SOC_SUCCESS)
		{
			GpsAppMmiBrgRunVibratorOnce();
		}
#endif
		GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_SOS);
		break;

	case GPS_GPRS_UPLDCAUSETYPE_GEOOS:
	case GPS_GPRS_UPLDCAUSETYPE_GEORS:
	case GPS_GPRS_UPLDCAUSETYPE_DEF:
#ifdef GPS_VIB_4SOSGPRSUPLDSUC
		// play vibrator once to let user know SOS require has been 
		// sent to server through gprs
		if (result == GPS_SOC_SUCCESS)
		{
			GpsAppMmiBrgRunVibratorOnce();
		}
#endif
		GPS_APP_SosMOCallStart(GPS_SAVEDNUMBERTYPE_USER);
		break;

	case GPS_GPRS_UPLDCAUSETYPE_LP:
#ifdef GPS_BACKUPLOAD_DAEMON
	case GPS_GPRS_UPLDCAUSETYPE_BACKUP:
#endif
	default:
		break;
	}

	gps_tcpip_log("gps_tcpip_send_result_cb, Pop req from fifo, OpCode: %d", 
				GPSAppEventFifoCurrItem()->OpCode);
	GPSAppEventFifoPop();
	if (!GPSAppEventFifoIsEmpty())
	{
		GPS_APP_EventFifoHandler(GPSAppEventFifoCurrItem());
	}
}

/*************************************************************************
* FUNCTION
*  gps_tcpip_create
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
kal_bool gps_tcpip_create(comptask_handler_struct **handle)
{
   static const comptask_handler_struct gps_tcpip_handler_info =
   {
      gps_tcpip_main,  /* task entry function */
      gps_tcpip_init,  /* task initialization function */
      NULL,  /* task configuration function */
      gps_tcpip_reset,  /* task reset handler */
      NULL,  /* task termination handler */
   };

   *handle = (comptask_handler_struct *)&gps_tcpip_handler_info;

   return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_tcpip_main
* DESCRIPTION
*   This function is the main function of GPS TCPIP task
* PARAMETERS
*   task_entry_ptr  IN   taks entry of GPS TCPIP
* RETURNS
*   None.
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static void gps_tcpip_main(task_entry_struct * task_entry_ptr)
{
   ilm_struct current_ilm;

   while ( 1 ) {
      receive_msg_ext_q(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);

      switch (current_ilm.msg_id) {
      case MSG_ID_GPSLOCATE_SEND_THRU_GPRS_REQ:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_GPSLOCATE_SEND_THRU_GPRS_REQ");
         gps_tcpip_send_req_hdlr(&current_ilm);

         break;

#ifndef GPS_SOC_BLOCKING_MODE
      case MSG_ID_APP_SOC_NOTIFY_IND:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_NOTIFY_IND");
		 gps_tcpip_socket_notify_ind_hdlr(&current_ilm);
		 break;

      case MSG_ID_APP_SOC_BEARER_INFO_IND:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_BEARER_INFO_IND");
		 break;

      case MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND");
		 break;

      case MSG_ID_APP_SOC_GET_HOST_BY_ADDR_IND:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_GET_HOST_BY_ADDR_IND");
		 break;

      case MSG_ID_APP_SOC_GET_HOST_BY_SRV_IND:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_GET_HOST_BY_SRV_IND");
		 break;

      case MSG_ID_APP_SOC_GET_HOST_BY_NAPTR_IND: 
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_GET_HOST_BY_NAPTR_IND");
		 break;

      case MSG_ID_APP_SOC_DEACTIVATE_CNF:
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_DEACTIVATE_CNF");
		 break;

      case MSG_ID_APP_SOC_DEACTIVATE_REQ:   
         gps_tcpip_log("GPS TCPIP receive msg: MSG_ID_APP_SOC_DEACTIVATE_REQ");
		 break;
#endif /* GPS_SOC_BLOCKING_MODE */

      default:
         gps_tcpip_log("GPS TCPIP receive UNKNOWN msg: %d", current_ilm.msg_id);
         break;
      }
      free_ilm(&current_ilm);
   }
}

/*****************************************************************************
* FUNCTION
*   gps_tcpip_init
* DESCRIPTION
*   Init function if GPS TCPIP task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static kal_bool gps_tcpip_init(task_indx_type task_indx)
{
    /* Do task's initialization here.
     * Notice that: shouldn't execute modules reset handler since 
     * stack_task_reset() will do. */

    return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_tcpip_reset
* DESCRIPTION
*   Reset function if GPS TCPIP task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static kal_bool gps_tcpip_reset(task_indx_type task_index)
{
   return KAL_TRUE;
}

