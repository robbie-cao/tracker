/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_uart_main.c
 *
 * Project:
 * --------
 *   Maui_Software
 *
 * Description:
 * ------------
 *   This file implements gps uart component task create function
 *
 * Author:
 * -------
 * Robbie Cao
 * -------
 *
 *============================================================================
 *             HISTORY
 *============================================================================
 * 2007-11-8 20:46 - Initial create.
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
#include "uart_sw.h"
#include "bmt.h"

#include "GPSLocateMsgDefs.h"
#include "gps_app.h"

/*************************************************************************
 * Macro defines
 *************************************************************************/
//#define GPS_UART_DEBUG
#ifdef GPS_UART_DEBUG
#define gps_uart_log        trace_printf
#else
static void gps_uart_log(kal_char *fmt, ...) { }
#endif

#define MAX_GPS_UART_BUF_LEN     512
#define GPS_UART_PORT            uart_port2
#define GPS_UART_BAUD_RATE       9600     /* defaule UART baud rate  */

/*************************************************************************
 * Local variables
 *************************************************************************/
static kal_uint8 gps_uart_buffer[MAX_GPS_UART_BUF_LEN];
static kal_bool  gps_uart_send_flag = KAL_FALSE;
static GPSLocateGprmcDest_t gps_uart_gprmc_dest = GPSLOCATE_GPRMCDEST_UNKNOWN;
static GPSLocateGprmcObjective_t gps_uart_gprmc_objective = GPSLOCATE_GPRMCOBJECTIVE_UNKNOWN;

/*************************************************************************
 * Golobal variables
 *************************************************************************/
extern kal_uint8 send_Rxilm[MAX_UART_PORT_NUM];

extern GPSLocateServerAddr_t        gps_tcpip_server_addr;
extern kal_uint16 					gps_samp_interval;
extern kal_uint16 					gps_upload_cnt;

extern module_type UART_GetOwnerID(UART_PORT port);
extern void UART_ClrRxBuffer(UART_PORT port, module_type ownerid);

/*************************************************************************
 * Function declaration
 *************************************************************************/
static void gps_uart_configure(unsigned baud);
static void uart_close_tst_if_gps_uart_use(void);
static void gps_uart_register_cb(void);

static void gps_uart_init_internal(void);
static void gps_uart_main(task_entry_struct * task_entry_ptr);
static kal_bool gps_uart_init(task_indx_type task_indx);
static kal_bool gps_uart_reset(task_indx_type task_index);

static void gps_uart_ready2read_ind_hdlr(ilm_struct *ilm_ptr);
static void gps_uart_sampgprmc_ind_hdlr(ilm_struct *ilm_ptr);


//-----------------------------------------------------------------------------
static void gps_uart_configure(unsigned baud)
{
    UARTDCBStruct dcb = {
        UART_BAUD_115200,    /* init */
        len_8,      /* dataBits; */
        sb_1,       /* stopBits; */
        pa_none,    /* parity; */
        fc_none,
        0x11,   /* xonChar; */
        0x13,   /* xoffChar; */
        KAL_FALSE
    };

    dcb.baud = (UART_baudrate) baud;
    UART_SetDCBConfig(GPS_UART_PORT, &dcb, MOD_GPS_UART);
}

//-----------------------------------------------------------------------------
static void gps_uart_rx_cb(UART_PORT port)
{
    ilm_struct *msg;
    ASSERT(port == GPS_UART_PORT);

    if (send_Rxilm[port]) { /* RX indication has been sent */
        msg = allocate_ilm(MOD_UART1_HISR + GPS_UART_PORT - uart_port1);
        msg->src_mod_id = MOD_UART1_HISR + GPS_UART_PORT - uart_port1;
        msg->dest_mod_id = MOD_GPS_UART;
        msg->sap_id = INVALID_SAP;
        msg->msg_id = MSG_ID_UART_READY_TO_READ_IND;
        msg->local_para_ptr = NULL;
        msg->peer_buff_ptr = NULL;
        msg_send_ext_queue(msg);
        send_Rxilm[port] = KAL_FALSE;
    }
}

//-----------------------------------------------------------------------------
static void gps_uart_register_cb(void)
{
#ifdef __MTK_TARGET__
    UART_Register_RX_cb(GPS_UART_PORT, MOD_GPS_UART, gps_uart_rx_cb);
#endif
}

//-----------------------------------------------------------------------------
static void uart_close_tst_if_gps_uart_use(void)
{
    extern kal_bool tst_is_uart_open;
    extern kal_bool tst_is_L1Trc_open;
    extern kal_bool tst_is_PsTrc_open;
    extern UART_PORT TST_PORT;
    extern UART_PORT tst_L1Trc_port;
    extern UART_PORT tst_PsTrc_port;

    tst_is_uart_open = KAL_FALSE;
    if (tst_L1Trc_port == TST_PORT) {
        tst_is_L1Trc_open = KAL_FALSE;
    }

    if (tst_PsTrc_port == TST_PORT) {
        tst_is_PsTrc_open = KAL_FALSE;
    }
}

//-----------------------------------------------------------------------------
static void gps_uart_init_internal(void)
{
    kal_bool bSuccess = KAL_FALSE;

    if (UART_GetOwnerID(GPS_UART_PORT) == MOD_TST ||
        UART_GetOwnerID(GPS_UART_PORT) == MOD_TST_READER) {
        uart_close_tst_if_gps_uart_use();
    }

    UART_Close(GPS_UART_PORT, UART_GetOwnerID(GPS_UART_PORT));

    // bind UART port to TOUCHPAD task
    bSuccess = UART_Open(GPS_UART_PORT, MOD_GPS_UART);
    ASSERT(bSuccess == KAL_TRUE);

    gps_uart_configure(GPS_UART_BAUD_RATE);

    catcher_sys_trace("gps_uart_init_internal");

    gps_uart_register_cb();    // should not use the default callback funtion, or it will cause the ctrl buffer-----construct local para, then cause fatal error

	/*
	If the cause of power on is charger plug in, don't power on GPS module
	*/
	if (BMT.PWRon == CHRPWRON)
	{
		//power off GPS
		GPS_APP_GPSModulePwrCtrl(KAL_FALSE, GPSMODULEPWRID_GPS);
	}
	else
	{
    		//power on GPS
		GPS_APP_GPSModulePwrCtrl(KAL_TRUE, GPSMODULEPWRID_GPS);
	}
}

//-----------------------------------------------------------------------------
static void gps_uart_ready2read_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    int len = 0, count = 0;
    kal_uint8 status;
    char *p_buf, *p_gprmc_start, *p_gprmc_end;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	memset(gps_uart_buffer, 0, sizeof(gps_uart_buffer));

    count = UART_GetBytesAvail(GPS_UART_PORT);
    gps_uart_log("Avaliable data count: %d", count);
    if (count <= 0) return;
    // discard data from uart vfifo if data count 
    // is larger than size of gps_uart_buffer,
    // to avoid overflow
    if (count > sizeof(gps_uart_buffer))
    {
       // read all data from uart vfifo
       do {
          len = UART_GetBytes(GPS_UART_PORT, 
                        gps_uart_buffer, 
                        (count > sizeof(gps_uart_buffer)) ? sizeof(gps_uart_buffer) : count, 
                        &status, 
                        MOD_GPS_UART);
          gps_uart_log("UART_GetBytes count: %d, len: %d", count, len);
          count -= len;
       } while (count > 0);
       UART_ClrRxBuffer(GPS_UART_PORT, MOD_GPS_UART);
       // discard the data from uart vfifo
       return;
    }

    len = UART_GetBytes(GPS_UART_PORT, gps_uart_buffer, count, &status, MOD_GPS_UART);
    UART_ClrRxBuffer(GPS_UART_PORT, MOD_GPS_UART);
    gps_uart_log("UART_GetBytes count: %d, len: %d, gps_uart_buffer:", count, len);
    gps_uart_log("%s", (char *)gps_uart_buffer);

    if (len != count)
        return;

    if (gps_uart_send_flag != KAL_TRUE)
        return;

    if (gps_uart_gprmc_objective == GPSLOCATE_GPRMCOBJECTIVE_UNKNOWN)
        return;

    p_buf = (char *)gps_uart_buffer;
    p_gprmc_start = strstr(p_buf, "$GPRMC");
    if (p_gprmc_start == NULL) return;
    p_gprmc_end = strstr(p_gprmc_start, "\r\n");
    if (p_gprmc_end == NULL) return;
	len = p_gprmc_end - p_gprmc_start + 2;
    
	switch (gps_uart_gprmc_dest)
	{
	case GPSLOCATE_GPRMCDEST_APPTASK:
		{
		GPSLocateGPSFrameDataIndStruct_t* ReqData;

	    ReqData = (GPSLocateGPSFrameDataIndStruct_t*)construct_local_para(
	                      sizeof(GPSLocateGPSFrameDataIndStruct_t),
	                      TD_UL);
	    memset(ReqData->FrameData, 0, sizeof(ReqData->FrameData));
	    memcpy(ReqData->FrameData, p_gprmc_start, len);
	    ReqData->FrameDataLen = len;
	    ReqData->Objective = gps_uart_gprmc_objective;

		SendMsg2GPSApp(MOD_GPS_UART, MSG_ID_GPSLOCATE_GPS_FRAME_DATA_IND, (void *)ReqData);
		}

		break;

	case GPSLOCATE_GPRMCDEST_TCPIP:
		break;

	default:
		break;
	}

#if 0	// temp comment this line
	GPS_APP_SetUart2Sleep(KAL_TRUE);
#endif
	gps_uart_send_flag = KAL_FALSE;
	gps_uart_gprmc_objective = GPSLOCATE_GPRMCOBJECTIVE_UNKNOWN;
}

//-----------------------------------------------------------------------------
static void gps_uart_sampgprmc_ind_hdlr(ilm_struct *ilm_ptr)
{
	GPSLocateSamplingGprmcIndStruct_t* ind_p;

	ind_p = (GPSLocateSamplingGprmcIndStruct_t*)ilm_ptr->local_para_ptr;
	gps_uart_log("gps_uart_sampgprmc_ind_hdlr, dest: %d, flag: %d", 
				ind_p->Dest, 
				ind_p->Flag);
	gps_uart_gprmc_dest = ind_p->Dest;

	gps_uart_gprmc_objective  = (ind_p->Flag == KAL_TRUE) ? 
								(gps_uart_gprmc_objective | ind_p->Objective) :
								(gps_uart_gprmc_objective & ~ind_p->Objective);

	gps_uart_send_flag = (gps_uart_gprmc_objective == GPSLOCATE_GPRMCOBJECTIVE_UNKNOWN) ?
						 KAL_FALSE :
						 KAL_TRUE;
}

/*************************************************************************
* FUNCTION
*  gps_uart_create
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
kal_bool gps_uart_create(comptask_handler_struct **handle)
{
   static const comptask_handler_struct gps_uart_handler_info =
   {
      gps_uart_main,  /* task entry function */
      gps_uart_init,  /* task initialization function */
      NULL,  /* task configuration function */
      gps_uart_reset,  /* task reset handler */
      NULL,  /* task termination handler */
   };

   *handle = (comptask_handler_struct *)&gps_uart_handler_info;
   return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_uart_main
* DESCRIPTION
*   This function is the main function of GPS UART task
* PARAMETERS
*   task_entry_ptr  IN   taks entry of GPS UART
* RETURNS
*   None.
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static void gps_uart_main(task_entry_struct * task_entry_ptr)
{
   ilm_struct current_ilm;

   gps_uart_init_internal();

   while ( 1 ) {

      receive_msg_ext_q(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);

      switch (current_ilm.msg_id) {
      case MSG_ID_UART_READY_TO_READ_IND:
          gps_uart_log("GPS UART receive message MSG_ID_UART_READY_TO_READ_IND.");
          gps_uart_ready2read_ind_hdlr(&current_ilm);

          break;

      case MSG_ID_GPSLOCATE_SAMPLINGGPRMC_IND:
          gps_uart_log("GPS UART receive message MSG_ID_GPSLOCATE_SAMPLINGGPRMC_IND.");
		  gps_uart_sampgprmc_ind_hdlr(&current_ilm);

		  break;

      default:
         break;
      }
      free_ilm(&current_ilm);
   }
}

/*****************************************************************************
* FUNCTION
*   gps_uart_init
* DESCRIPTION
*   Init function if GPS UART task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static kal_bool gps_uart_init(task_indx_type task_indx)
{
    /* Do task's initialization here.
     * Notice that: shouldn't execute modules reset handler since 
     * stack_task_reset() will do. */

    return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_uart_reset
* DESCRIPTION
*   Reset function if GPS UART task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static kal_bool gps_uart_reset(task_indx_type task_index)
{
   return KAL_TRUE;
}

