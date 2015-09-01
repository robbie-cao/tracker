/******************************************************************************
* File name: 
*	gps_app_pp_buff_mgr.h
*
* Description: 
*	The header file for the interface of pingpong buffer for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.7
*
******************************************************************************/
#ifndef _GPS_APP_PP_BUFF_MGR_H_
#define _GPS_APP_PP_BUFF_MGR_H_

#include "gps_app_nema.h"

typedef void (*BufferFullCallback_t)(const GPS_GPRMC_Packed_Struct_t* buff, kal_uint16 buff_len, void *info);
typedef enum
{
	GPS_PP_BUFF_STATUS_IDLE,			//idle
	GPS_PP_BUFF_STATUS_STORING,		//storing
	GPS_PP_BUFF_STATUS_LOADING,		//loading
	GPS_PP_BUFF_STATUS_FULL,			//full
	
	GPS_PP_BUFF_STATUS_Invalid
} GPSPPBufferStatus_t;

typedef enum
{
	GPS_PP_BUFF_RESULT_NO_ERROR = 0,		//no error
	GPS_PP_BUFF_RESULT_INVALID_PARAM,	//invalid parameters
	GPS_PP_BUFF_RESULT_BUFFER_FULL		//buffer full
	
} GPSPPBufferResult_t;

typedef enum
{
	GPS_PP_BUFF_PURPOSE_REFERNCE,		//reference
	GPS_PP_BUFF_PURPOSE_LOADING,		//loading
	
	GPS_PP_BUFF_PURPOSE_Invalid
} GPSPPBufferPurpose_t;

void GPSPPBufferInit(BufferFullCallback_t callback);
void GPSPPBufferReset(BufferFullCallback_t callback);
GPSPPBufferResult_t GPSPPBufferStore(const GPS_GPRMC_Packed_Struct_t* pData, kal_uint16 size);
GPSPPBufferResult_t GPSPPBufferSetThreshold(kal_uint16 threshold);
void GPSPPBufferFinishLoading(void);
GPSPPBufferStatus_t GPSPPBufferGetStoringBuffer(GPS_GPRMC_Packed_Struct_t** pBuff, 
													kal_uint16* pDepth);
GPSPPBufferStatus_t GPSPPBufferGetLoadingBuffer(
	GPSPPBufferPurpose_t Purpose,
	GPS_GPRMC_Packed_Struct_t** pBuff,
	kal_uint16* pDepth
	);

#endif //_GPS_APP_PP_BUFF_MGR_H_

