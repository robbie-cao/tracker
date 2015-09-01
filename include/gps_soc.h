/******************************************************************************
 * File name: gps_soc.h
 * Author: Robbie Cao
 * Date: 2008-1-16 1:07
 ******************************************************************************/

#ifndef _GPS_SOC_H
#define _GPS_SOC_H

#include "kal_release.h"
#include "soc_api.h"

#include "gps_app_configurable_const.h"
#include "gps_app_nema.h"

//#define GPS_SOC_BLOCKING_MODE

#define GPS_GPRS_ACCOUNT_IDX		2		/* based 1, max 10 */

#define GPS_SEND_INTERVAL_MIN		5		/* 5 seconds */
#define GPS_SEND_ITEMS_ONETIME		10

#define GPS_MAX_RCV_BUFFER_SIZE		(sizeof(GPS_GPRMC_Packed_Struct_t) * GPS_PP_BUFFER_SIZE) 	// 1250
#define GPS_MAX_SND_BUFFER_SIZE		(sizeof(GPS_GPRMC_Packed_Struct_t) * GPS_PP_BUFFER_SIZE) 	// 1250
											/* must be equal or greater than (sizeof(GPS_GPRMC_Packed_Struct_t)*GPS_PP_BUFFER_SIZE) */
#define GPS_MAX_POST_RETRY_NUM		3		/* for UDP application, ex: DAYTIME and Echo */
#define GPS_SOC_POST_TIMEOUT		5000	/* It means that 5 seconds */

typedef enum
{
    GPS_SOC_INVAL_PARA = -9,
    GPS_SOC_ERROR,
    GPS_SOC_PEER_NOT_REACHABLE,
    GPS_SOC_PEER_NO_RESPONSE,
    GPS_SOC_CNT_RESET_BY_PEER,
    GPS_SOC_BEARER_FAIL,
    GPS_SOC_UNKNOWN_APP,
    GPS_SOC_BUSY,
    GPS_SOC_NO_MEMORY,
    GPS_SOC_SUCCESS
} GPS_Soc_Result_t;

typedef enum
{
    GPS_SOC_IDLE,
    GPS_SOC_CREATING,
    GPS_SOC_CREATED,
    GPS_SOC_TCP_CON_CREATING,
    GPS_SOC_TCP_CON_CREATED,
    GPS_SOC_REQ_SEND_RETRY,
    GPS_SOC_REQ_SENDING,
    GPS_SOC_REQ_SENT,
    GPS_SOC_RSP_RCVING,
    GPS_SOC_RSP_RCVD,
    GPS_SOC_CLOSING,
    GPS_SOC_CLOSED
} GPS_Soc_Transaction_State_t;

typedef enum
{
    SOC_ADDR_TYPE_IP,
    SOC_ADDR_TYPE_DNAME,
    SOC_ADDR_TYPE_INVALID,
    SOC_ADDR_TYPE_TOTAL
} GPS_Soc_AddrType_t;

typedef void (*gps_soc_rsp_t)(int result, char *response, int response_len);

typedef struct
{
    kal_uint8 state;
    kal_int8 socket_id;
    kal_uint8 is_socket_opened;
    kal_uint8 post_retry_counter;

    kal_uint32 nwt_acount_id;
    kal_int8 *rcvd_buffer;
    kal_uint32 rcvd_data_len;
    kal_uint32 rcvd_counter;
    kal_uint32 rcvd_time;
    kal_char *snd_buffer;
    kal_uint32 snd_data_len;
    kal_uint32 snd_counter;
    kal_uint32 snd_time;
    sockaddr_struct server_ip_addr;
	kal_uint8 cause_type;
	kal_uint8 src_type;
    gps_soc_rsp_t callback;
} GPS_Soc_Transaction_t;

void gps_soc_init(void);
kal_bool gps_soc_create_socket(void);
int gps_soc_tcp_send_request(void);		// non-blocking mode
#ifdef GPS_SOC_BLOCKING_MODE
int gps_soc_tcp_send_request2(void);	// blocking mode
#endif
void gps_soc_tcp_recv_response(void);
void gps_soc_socket_notify(void *inMsg);
void gps_soc_output_result(int ret, char *out_str, int len);
int gps_soc_request_abort(void);

/* timer event handler */
void gps_soc_start_timer(void);
void gps_soc_stop_timer(void);
void gps_soc_timer_handler(void);


#endif
