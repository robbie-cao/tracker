/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_soc.c
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
 * 2008-1-16 1:13 - Initial create.
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
#include "soc_api.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_pp_buff_mgr.h"
#include "gps_app_nema.h"
#include "gps_soc.h"
#include "gps_app.h"
#include "gps_app_data.h"

/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Macro defines
 *************************************************************************/
#define GPS_SOC_DEBUG
#ifdef GPS_SOC_DEBUG
#define gps_soc_log        trace_printf
#else
static void gps_soc_log(kal_char *fmt, ...) { }
#endif


/*************************************************************************
 * Local variables
 *************************************************************************/
GPS_Soc_Transaction_t gps_soc_transaction;

const char *gps_soc_uploadTypeStr[] =
{
	"SMS",
	"CALL",
	"ANSWER",
	"AUTO",
	"AUTOLOW",
	"SOS",
	"OS",
	"RS",
	"OVERSPEED",
	"SAFESPEED",
	"DEF",
#ifdef GPS_BACKUPLOAD_DAEMON
	"BACKUP",
#endif
	"LP",

	"LPSMS",
	"LPCALL",
	"LPANSWER",
	"LPAUTO",
	"LPAUTOLOW",
	"LPSOS",
	"LPOS",
	"LPRS",
	"LPOVERSPEED",
	"LPSAFESPEED",
	"LPDEF",
#ifdef GPS_BACKUPLOAD_DAEMON
	"LPBACKUP",
#endif
	"LP",

	"CHSMS",
	"CHCALL",
	"CHANSWER",
	"CHAUTO",
	"CHAUTOLOW",
	"CHSOS",
	"CHOS",
	"CHRS",
	"CHOVERSPEED",
	"CHSAFESPEED",
	"CHDEF",
#ifdef GPS_BACKUPLOAD_DAEMON
	"CHBACKUP",
#endif
	"CHLP",

	""
};

/*************************************************************************
 * Golobal variables
 *************************************************************************/
extern char	gps_gprs_username[];
extern char	gps_gprs_userpwd[];
extern char gps_imei_str[];


/*************************************************************************
 * Function declaration
 *************************************************************************/
static const char *gps_soc_upldtype_str(kal_uint8 type);


//-----------------------------------------------------------------------------
static const char *gps_soc_upldtype_str(kal_uint8 type)
{
	const char *retStr = NULL;

	ASSERT(type < GPS_GPRS_UPLDCAUSETYPE_TOTAL);

	if (gps_app_is_charger_connected)
	{
		retStr = gps_soc_uploadTypeStr[type + GPS_GPRS_UPLDCAUSETYPE_TOTAL * 2];
	}
	else if (gps_lowbattery_warnning)
	{
		retStr = gps_soc_uploadTypeStr[type + GPS_GPRS_UPLDCAUSETYPE_TOTAL];
	}
	else
	{
		retStr = gps_soc_uploadTypeStr[type];
	}

	return retStr;
}

//-----------------------------------------------------------------------------
void gps_soc_init(void)
{
}

//-----------------------------------------------------------------------------
int gps_soc_request_abort(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (gps_soc_transaction.socket_id >= 0)
    {
        ilm_struct *ilm_send = NULL;
		int ret;

        ret = soc_close(gps_soc_transaction.socket_id);
    	gps_soc_log("soc_close result: %d", ret);
		kal_sleep_task(250);
        soc_close_nwk_account_by_id(MOD_GPS_TCPIP, gps_soc_transaction.nwt_acount_id);

        /* close socket and disconnect bearer here */
        ilm_send = allocate_ilm(MOD_GPS_TCPIP);
        ilm_send->msg_id = MSG_ID_APP_SOC_DEACTIVATE_REQ;
        ilm_send->peer_buff_ptr = NULL;
        ilm_send->local_para_ptr = NULL;
        SEND_ILM(MOD_GPS_TCPIP, MOD_SOC, SOC_APP_SAP, ilm_send);
    }
    gps_soc_transaction.post_retry_counter = 0;

    gps_soc_stop_timer();

    return 0;
}

//-----------------------------------------------------------------------------
void gps_soc_output_result(int ret, char *out_str, int len)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    gps_soc_log("soc result: %d", ret);
    gps_soc_transaction.state = GPS_SOC_CLOSING;
    gps_soc_request_abort();
	if (gps_soc_transaction.callback != NULL)
	{
    	gps_soc_transaction.callback(ret, out_str, len);
	}
}

//-----------------------------------------------------------------------------
int gps_soc_tcp_send_request(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    gps_soc_log("gps_soc_tcp_send_request, transaction state: %d", gps_soc_transaction.state);

    if (gps_soc_transaction.state == GPS_SOC_TCP_CON_CREATING)
    {
        kal_int8 ret;

        gps_soc_log("connect to %d.%d,%d,%d and port: %d",
            gps_soc_transaction.server_ip_addr.addr[0],
            gps_soc_transaction.server_ip_addr.addr[1],
            gps_soc_transaction.server_ip_addr.addr[2],
            gps_soc_transaction.server_ip_addr.addr[3],
            gps_soc_transaction.server_ip_addr.port);

        ret = soc_connect(gps_soc_transaction.socket_id, &gps_soc_transaction.server_ip_addr);
        gps_soc_log("connect result is %d", ret);

        if (ret == SOC_SUCCESS)
        {
            gps_soc_tcp_send_request();
            return GPS_SOC_SUCCESS;
        }
        else if (ret == SOC_WOULDBLOCK)
        {
            /* waits for socket notify */
            // msgId: MSG_ID_APP_SOC_NOTIFY_IND, 
            // it will be handled by gps_soc_socket_notify
            return GPS_SOC_SUCCESS;
        }
        else
        {
            if (ret == SOC_ERROR)
            {
                gps_soc_output_result(GPS_SOC_PEER_NOT_REACHABLE, NULL, 0);
                return GPS_SOC_PEER_NOT_REACHABLE;
            }
            else
            {
                gps_soc_output_result(GPS_SOC_ERROR, NULL, 0);
                return GPS_SOC_ERROR;
            }
        }
    }
    else if(gps_soc_transaction.state == GPS_SOC_TCP_CON_CREATED || 
			gps_soc_transaction.state == GPS_SOC_REQ_SEND_RETRY ||
            gps_soc_transaction.state == GPS_SOC_REQ_SENDING)
    {
        kal_int32 ret;
		const GPS_GPRMC_Packed_Struct_t *pPack;
		char *pBuff;
		int i, count;

		pBuff = gps_soc_transaction.snd_buffer;
		pBuff[0] = '\0';
        if (gps_soc_transaction.state != GPS_SOC_REQ_SENDING)
        {
            gps_soc_transaction.snd_counter = 0;
			sprintf(pBuff, "#%s#%s#%s#%s#%d\r\n", 
					gps_imei_str,
					gps_gprs_username, 
					gps_gprs_userpwd, 
					gps_soc_upldtype_str(gps_soc_transaction.cause_type),
					gps_soc_transaction.snd_data_len);
        }
        gps_soc_transaction.state = GPS_SOC_REQ_SENDING;
        gps_soc_log("send request to %d.%d,%d,%d and port: %d",
            gps_soc_transaction.server_ip_addr.addr[0],
            gps_soc_transaction.server_ip_addr.addr[1],
            gps_soc_transaction.server_ip_addr.addr[2],
            gps_soc_transaction.server_ip_addr.addr[3],
            gps_soc_transaction.server_ip_addr.port);

		pPack = (GPS_GPRMC_Packed_Struct_t *)gps_soc_transaction.rcvd_buffer;
		pPack += gps_soc_transaction.snd_counter;
		count = (gps_soc_transaction.snd_data_len - gps_soc_transaction.snd_counter) <= GPS_SEND_ITEMS_ONETIME ?
				(gps_soc_transaction.snd_data_len - gps_soc_transaction.snd_counter) :
				GPS_SEND_ITEMS_ONETIME;
		for (i = 0; i < count; i++)
		{
		    Result_t result = RESULT_ERROR;

			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			sprintf(pBuff, "#") ;
			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			sprintf(pBuff, "%04x%04x", pPack->lac, pPack->cid) ;
			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			result = GPS_APP_GPRMC_Packed2Str(pBuff, pPack);
			pPack++;
		}

		if (gps_soc_transaction.snd_counter + count >= (kal_int32) gps_soc_transaction.snd_data_len)
		{
			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			sprintf(pBuff, "##\r\n");
		}

    	gps_soc_log("send data len: %d, data: %s", 
	    			strlen(gps_soc_transaction.snd_buffer),
	    			gps_soc_transaction.snd_buffer);
        ret = soc_send(
                gps_soc_transaction.socket_id,
                (kal_uint8*)gps_soc_transaction.snd_buffer,
                strlen(gps_soc_transaction.snd_buffer),
                0);
        gps_soc_log("send request result, sent_bytes: %d", ret);

        if (ret > 0)
        {
            gps_soc_transaction.snd_counter += count;
            if (gps_soc_transaction.snd_counter >= (kal_int32) gps_soc_transaction.snd_data_len)
            {
                gps_soc_transaction.state = GPS_SOC_REQ_SENT;
            }
			kal_sleep_task(250);
			gps_soc_tcp_send_request();
            return GPS_SOC_SUCCESS;
        }
        else
        {
            if (ret == SOC_WOULDBLOCK)
            {
                /* waits for socket notify */
            	// msgId: MSG_ID_APP_SOC_NOTIFY_IND, 
            	// it will be handled by gps_soc_socket_notify
                return GPS_SOC_SUCCESS;
            }
            else
            {
                if (ret == SOC_ERROR)
                {
                    gps_soc_output_result(GPS_SOC_PEER_NOT_REACHABLE, NULL, 0);
                    return GPS_SOC_PEER_NOT_REACHABLE;
                }
                else
                {
                    gps_soc_output_result(GPS_SOC_ERROR, NULL, 0);
                    return GPS_SOC_ERROR;
                }
            }
        }
    }
    else if(gps_soc_transaction.state == GPS_SOC_REQ_SENT)
    {
        gps_soc_output_result(GPS_SOC_SUCCESS, NULL, 0);
        return GPS_SOC_SUCCESS;
    }
    else
    {
        gps_soc_output_result(GPS_SOC_ERROR, NULL, 0);
        return GPS_SOC_ERROR;
    }

    return GPS_SOC_ERROR;
}

#ifdef GPS_SOC_BLOCKING_MODE
//-----------------------------------------------------------------------------
int gps_soc_tcp_send_request2(void)
{
	kal_uint8 val = 1; // non-blocking mode, default
	kal_int32 res;
	kal_int8 error; 
	kal_int32 detail_cause;
	kal_bool set_header = KAL_FALSE;

	gps_soc_transaction.socket_id = soc_create(PF_INET, SOCK_STREAM, 0, MOD_GPS_TCPIP, gps_soc_transaction.nwt_acount_id);
	gps_soc_log("soc_create, socket_id: %d", gps_soc_transaction.socket_id);
	if (gps_soc_transaction.socket_id < 0)
	{
		if (gps_soc_transaction.callback != NULL)
		{
	    	gps_soc_transaction.callback(0, NULL, 0);
		}
		return GPS_SOC_ERROR;
	}

	val = 0;	// blocking mode
	res = soc_setsockopt(gps_soc_transaction.socket_id, SOC_NBIO, &val, sizeof(val));
	gps_soc_log("soc_setsockopt, val: %d, result: %d", val, res);
	if (res < 0)
	{
		goto ERROR_HANDLE;
	}
	val = SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT;
	res = soc_setsockopt(gps_soc_transaction.socket_id, SOC_ASYNC, &val, sizeof(val));
	gps_soc_log("soc_setsockopt, val: %d, result: %d", val, res);
	if (res < 0)
	{
		goto ERROR_HANDLE;
	}
	
	gps_soc_log("Connect to %d.%d.%d.%d and port: %d",
				    gps_soc_transaction.server_ip_addr.addr[0],
				    gps_soc_transaction.server_ip_addr.addr[1],
				    gps_soc_transaction.server_ip_addr.addr[2],
				    gps_soc_transaction.server_ip_addr.addr[3],
				    gps_soc_transaction.server_ip_addr.port);
	
	res = soc_connect(gps_soc_transaction.socket_id, &gps_soc_transaction.server_ip_addr);
	gps_soc_log("soc_connect res: %d", res);
	if (res < 0)
	{
		res = soc_get_last_error(gps_soc_transaction.socket_id, &error, &detail_cause);
		gps_soc_log("res: %d soc_connect error: %d, detail_cause: %d", 
					res, error, detail_cause);

		goto ERROR_HANDLE;
	}
	
	while (gps_soc_transaction.snd_counter < gps_soc_transaction.snd_data_len)
	{
		const GPS_GPRMC_Packed_Struct_t *pPack;
		char *pBuff;
		int i, count;

		pPack = (GPS_GPRMC_Packed_Struct_t *)gps_soc_transaction.rcvd_buffer;
		pPack += gps_soc_transaction.snd_counter;
		pBuff = gps_soc_transaction.snd_buffer;
		pBuff[0] = '\0';
		if (!set_header)
		{
			sprintf(pBuff, "#%s#%s#%s#%s#%d\r\n", 
					gps_imei_str,
					gps_gprs_username, 
					gps_gprs_userpwd, 
					gps_soc_upldtype_str(gps_soc_transaction.cause_type),
					gps_soc_transaction.snd_data_len);
			set_header = KAL_TRUE;
		}
		count = (gps_soc_transaction.snd_data_len - gps_soc_transaction.snd_counter) <= GPS_SEND_ITEMS_ONETIME ?
				(gps_soc_transaction.snd_data_len - gps_soc_transaction.snd_counter) :
				GPS_SEND_ITEMS_ONETIME;
		for (i = 0; i < count; i++)
		{
		    Result_t result = RESULT_ERROR;

			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			sprintf(pBuff, "#");
			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			sprintf(pBuff, "%04x%04x", pPack->lac, pPack->cid) ;
			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			result = GPS_APP_GPRMC_Packed2Str(pBuff, pPack);
			pPack++;
		}

		if (gps_soc_transaction.snd_counter + count >= (kal_int32) gps_soc_transaction.snd_data_len)
		{
			pBuff = gps_soc_transaction.snd_buffer + strlen(gps_soc_transaction.snd_buffer);
			sprintf(pBuff, "##\r\n");
		}

	    gps_soc_log("send data len: %d, data: %s", 
		   			strlen(gps_soc_transaction.snd_buffer),
		   			gps_soc_transaction.snd_buffer);
		res = soc_send(gps_soc_transaction.socket_id, 
					   (kal_uint8*)gps_soc_transaction.snd_buffer, 
					   strlen(gps_soc_transaction.snd_buffer), 
					   0);
		gps_soc_log("Http send request result, sent_bytes: %d", res);
		if (res < 0)
		{
			res = soc_get_last_error(gps_soc_transaction.socket_id, &error, &detail_cause);
			gps_soc_log("res: %d soc_send error: %d, detail_cause: %d", 
						res, error, detail_cause);
			break;
		}
		kal_sleep_task(250);

		gps_soc_transaction.snd_counter += count;
	}
  
ERROR_HANDLE:
	if (gps_soc_transaction.callback != NULL)
	{
	   	gps_soc_transaction.callback(0, NULL, 0);
	}

	kal_sleep_task(250);
	res = soc_close(gps_soc_transaction.socket_id);
	gps_soc_log("soc_close res: %d", res);
	if (res < 0)
	{
		res = soc_get_last_error(gps_soc_transaction.socket_id, &error, &detail_cause);
		gps_soc_log("res: %d soc_close error: %d, detail_cause: %d", 
					res, error, detail_cause);
	}
	kal_sleep_task(250);

	soc_close_nwk_account_by_id(MOD_GPS_TCPIP, gps_soc_transaction.nwt_acount_id);

	return GPS_SOC_SUCCESS;
}
#endif /* #define GPS_SOC_BLOCKING_MODE */

//-----------------------------------------------------------------------------
void gps_soc_tcp_recv_response(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    if (gps_soc_transaction.state < GPS_SOC_RSP_RCVD)
    {
        int ret_val = 0;

        gps_soc_transaction.state = GPS_SOC_RSP_RCVING;
        ret_val = soc_recv(
                    gps_soc_transaction.socket_id,
                    (kal_uint8*) (gps_soc_transaction.rcvd_buffer + gps_soc_transaction.rcvd_counter),
                    (GPS_MAX_RCV_BUFFER_SIZE - gps_soc_transaction.rcvd_counter),
                    0);
        if (ret_val > 0)
        {
            gps_soc_transaction.rcvd_counter += ret_val;

            gps_soc_output_result(
                GPS_SOC_SUCCESS,
                (char*)gps_soc_transaction.rcvd_buffer,
                gps_soc_transaction.rcvd_counter);
        }
        else
        {
            if (ret_val == SOC_WOULDBLOCK)
            {
                /* waits for socket notify */
            	// msgId: MSG_ID_APP_SOC_NOTIFY_IND, 
            	// it will be handled by gps_soc_socket_notify
                return;
            }
            else
            {
                gps_soc_output_result(GPS_SOC_ERROR, NULL, 0);
            }
        }
    }
}

//-----------------------------------------------------------------------------
kal_bool gps_soc_create_socket(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    kal_uint8 val = 1;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    gps_soc_transaction.state = GPS_SOC_CREATING;
    gps_soc_log("gps_soc_create_socket, transaction state: %d", gps_soc_transaction.state);
    gps_soc_transaction.socket_id = soc_create(PF_INET, SOCK_STREAM, 0, MOD_GPS_TCPIP, gps_soc_transaction.nwt_acount_id);
    gps_soc_log("Create socket: %d", gps_soc_transaction.socket_id);

    if (gps_soc_transaction.socket_id < 0)
    {
        gps_soc_log("Create socket fail !!");

        return KAL_FALSE;
    }

    if (soc_setsockopt(gps_soc_transaction.socket_id, SOC_NBIO, &val, sizeof(val)) < 0)
    {
        gps_soc_log("Set socket to nonblock mode error !!");

        return KAL_FALSE;
    }

    val = SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT;
    if (soc_setsockopt(gps_soc_transaction.socket_id, SOC_ASYNC, &val, sizeof(val)) < 0)
    {
        gps_soc_log("Set socket to nonblock mode error !!");

        return KAL_FALSE;
    }
    gps_soc_transaction.state = GPS_SOC_CREATED;
    gps_soc_log("gps_soc_create_socket, transaction state: %d", gps_soc_transaction.state);

    return KAL_TRUE;
}

//-----------------------------------------------------------------------------
void gps_soc_socket_notify(void *inMsg)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    app_soc_notify_ind_struct *soc_notify = (app_soc_notify_ind_struct*)inMsg;

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    gps_soc_log("Got socket notify, event_type: %d, result: %d, error_cause: %d, detail_cause: %d", 
    			soc_notify->event_type, 
    			soc_notify->result, 
    			soc_notify->error_cause, 
    			soc_notify->detail_cause);

    switch (soc_notify->event_type)
    {
    case SOC_WRITE:
        gps_soc_tcp_send_request();
		break;

    case SOC_READ:
        gps_soc_tcp_recv_response();
        break;

    case SOC_CONNECT:
        if (soc_notify->result == KAL_TRUE)
        {
            gps_soc_transaction.state = GPS_SOC_TCP_CON_CREATED;
            gps_soc_tcp_send_request();
        }
        else
        {
            gps_soc_output_result(GPS_SOC_PEER_NOT_REACHABLE, NULL, 0);
        }
        break;

    case SOC_CLOSE:
        gps_soc_output_result(GPS_SOC_CNT_RESET_BY_PEER, NULL, 0);
		break;

    default:
        gps_soc_output_result(GPS_SOC_ERROR, NULL, 0);
        break;
    }
}


//-----------------------------------------------------------------------------
void gps_soc_start_timer(void)
{
	gps_soc_log("gps_soc_start_timer");
}

//-----------------------------------------------------------------------------
void gps_soc_stop_timer(void)
{
	gps_soc_log("gps_soc_stop_timer");
}

//-----------------------------------------------------------------------------
void gps_soc_timer_handler(void)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	gps_soc_log("gps_soc_timer_handler");

    if (gps_soc_transaction.post_retry_counter < GPS_MAX_POST_RETRY_NUM)
    {
        gps_soc_transaction.state = GPS_SOC_REQ_SEND_RETRY;
        gps_soc_tcp_send_request();
    }
    else
    {
        gps_soc_output_result(GPS_SOC_PEER_NO_RESPONSE, NULL, 0);
    }
}

