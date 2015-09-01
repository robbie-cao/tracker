/******************************************************************************
* File name: 
*	gps_app_pp_buff_mgr.c
*
* Description: 
*	This module provide the interface of pingpong buffer for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.7
*
******************************************************************************/

#include "gps_app_configurable_const.h"
#include "gps_app_pp_buff_mgr.h"

/******************************************************************************
* Local constants definition
******************************************************************************/
enum
{
	GPS_PP_BUFF_PING = 0,
	GPS_PP_BUFF_PONG,
	GPS_PP_BUFF_Total
};

/******************************************************************************
* Local data structures definition
******************************************************************************/
typedef struct
{
	kal_uint16 Threshold;
	BufferFullCallback_t BufferFullCallbackFunc;
	kal_uint8 LoadingBufferRefCount;
} GPSPPBufferCtrlBlk_t;

typedef struct
{
	struct
	{
		//char Usage;	//'S' for store, 'L' for Load
		GPSPPBufferStatus_t Status;
		kal_uint16 Index;
	} Header;
	GPS_GPRMC_Packed_Struct_t* Body;
} GPSGPRMCPackedBuffer_t;

/******************************************************************************
* Global variables
******************************************************************************/
static GPSGPRMCPackedBuffer_t gGPSPPBuffer[GPS_PP_BUFF_Total];
static GPS_GPRMC_Packed_Struct_t gGPSPackedPureBuffer[GPS_PP_BUFF_Total][GPS_PP_BUFFER_SIZE];
static GPSPPBufferCtrlBlk_t gGPSPPBufferCtrlBlk;
static kal_semid gGPSPPBufferSema = NULL;

/******************************************************************************
* Static functions
******************************************************************************/

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSPPBufferInit
*
* Usage:
*	Initialize buffer control block
*
* Parameters:
*	callback - call back function for the storing buffer full handler
*
* Return:
*	None
******************************************************************************/
void GPSPPBufferInit(BufferFullCallback_t callback)
{
	if (gGPSPPBufferSema != NULL)
	{
		return;
	}
	//create semaphore
	gGPSPPBufferSema = kal_create_sem("GPSPP SEM", 1);
	ASSERT((gGPSPPBufferSema != NULL));
	gGPSPPBufferCtrlBlk.Threshold = GPS_PP_BUFFER_SIZE;
	gGPSPPBufferCtrlBlk.BufferFullCallbackFunc = callback;
	gGPSPPBufferCtrlBlk.LoadingBufferRefCount = 0;
	//gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Usage = GPS_PP_BUFF_USAGE_STORE;
	gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
	gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index = 0;
	gGPSPPBuffer[GPS_PP_BUFF_PING].Body = (GPS_GPRMC_Packed_Struct_t*)&(gGPSPackedPureBuffer[GPS_PP_BUFF_PING]);
	//gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Usage = GPS_PP_BUFF_USAGE_LOAD;
	gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
	gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index = 0;
	gGPSPPBuffer[GPS_PP_BUFF_PONG].Body = (GPS_GPRMC_Packed_Struct_t*)&(gGPSPackedPureBuffer[GPS_PP_BUFF_PONG]);
}

/******************************************************************************
* Function:
*	GPSPPBufferReset
*
* Usage:
*	Reset buffer control block
*
* Parameters:
*	callback - call back function for the storing buffer full handler
*
* Return:
*	None
******************************************************************************/
void GPSPPBufferReset(BufferFullCallback_t callback)
{
	ASSERT((gGPSPPBufferSema != NULL));
	gGPSPPBufferCtrlBlk.Threshold = GPS_PP_BUFFER_SIZE;
	gGPSPPBufferCtrlBlk.BufferFullCallbackFunc = callback;
	gGPSPPBufferCtrlBlk.LoadingBufferRefCount = 0;
	//gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Usage = GPS_PP_BUFF_USAGE_STORE;
	gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
	gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index = 0;
	gGPSPPBuffer[GPS_PP_BUFF_PING].Body = (GPS_GPRMC_Packed_Struct_t*)&(gGPSPackedPureBuffer[GPS_PP_BUFF_PING]);
	//gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Usage = GPS_PP_BUFF_USAGE_LOAD;
	gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
	gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index = 0;
	gGPSPPBuffer[GPS_PP_BUFF_PONG].Body = (GPS_GPRMC_Packed_Struct_t*)&(gGPSPackedPureBuffer[GPS_PP_BUFF_PONG]);
}

/******************************************************************************
* Function:
*	GPSPPBufferStore
*
* Usage:
*	Store a block of data into the storing buffer
*
* Parameters:
*	pData - pointer to the caller supplied buffer where storing caller's data
*	size - size of caller's data to be stored, the unit is sizeof(GPS_GPRMC_Packed_Struct_t)
*
* Return:
*	GPSPPBufferResult_t
******************************************************************************/
GPSPPBufferResult_t GPSPPBufferStore(const GPS_GPRMC_Packed_Struct_t* pData, kal_uint16 size)
{
	GPSPPBufferResult_t result = GPS_PP_BUFF_RESULT_NO_ERROR;
	
	ASSERT((gGPSPPBufferSema != NULL));
	//take the semaphore
	kal_take_sem(gGPSPPBufferSema, KAL_INFINITE_WAIT);
	if (pData == NULL || size == 0 || size > gGPSPPBufferCtrlBlk.Threshold)
	{
		//invalid parameters
		result = GPS_PP_BUFF_RESULT_INVALID_PARAM;
		goto _RelSema_Return_;
	}
	//save data anyway
	//if (gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status != GPS_PP_BUFF_STATUS_FULL)
	{		//not full
		kal_uint16 remaining, i;

		remaining = gGPSPPBufferCtrlBlk.Threshold - gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index;
		if (remaining < size)
		{//the remaining space is not enough
			//adjust the index to save all the input data
			gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index = gGPSPPBufferCtrlBlk.Threshold - size;
			//push ahead
			for (i=0; i<gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index; i++)
			{
				memcpy((void*)&(gGPSPPBuffer[GPS_PP_BUFF_PING].Body[i]),
						(void*)&(gGPSPPBuffer[GPS_PP_BUFF_PING].Body[i+(size-remaining)]),
						sizeof(GPS_GPRMC_Packed_Struct_t));
			}
		}
		memcpy((void*)&(gGPSPPBuffer[GPS_PP_BUFF_PING].Body[gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index]),
				(void*)pData, size*sizeof(GPS_GPRMC_Packed_Struct_t));
		gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index += size;
		gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status = \
			(gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index < gGPSPPBufferCtrlBlk.Threshold) ? \
			GPS_PP_BUFF_STATUS_STORING : GPS_PP_BUFF_STATUS_FULL;
	}
	//check full or not
	if (gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status == GPS_PP_BUFF_STATUS_FULL)
	{
		//check the other buffer status
		if (gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status != GPS_PP_BUFF_STATUS_LOADING)
		{
			GPS_GPRMC_Packed_Struct_t* pTmp;

			//exchange the buffers
			pTmp = gGPSPPBuffer[GPS_PP_BUFF_PONG].Body;
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Body = gGPSPPBuffer[GPS_PP_BUFF_PING].Body;
			//set PONG's status as FULL
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_FULL;
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index = gGPSPPBufferCtrlBlk.Threshold;
			//change PING's status to IDLE
			gGPSPPBuffer[GPS_PP_BUFF_PING].Body = pTmp;
			gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
			gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index = 0;
			if (gGPSPPBufferCtrlBlk.BufferFullCallbackFunc != NULL)
			{
				//set PONG's status as Loading
				gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_LOADING;
//				gGPSPPBufferCtrlBlk.LoadingBufferRefCount++;
				gGPSPPBufferCtrlBlk.BufferFullCallbackFunc(gGPSPPBuffer[GPS_PP_BUFF_PONG].Body, 
												gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index, 
												NULL);
			}
		}
	}
_RelSema_Return_:	
	//release semaphore
	kal_give_sem(gGPSPPBufferSema);
	return result;
}

/******************************************************************************
* Function:
*	GPSPPBufferSetThreshold
*
* Usage:
*	Set the threshold of the storing buffer. Once data amount of the buffer is reached at this 
*	threshold, it should be looked as buffer full.
*
* Parameters:
*	threshold - the new threshold value
*
* Return:
*	GPSPPBufferResult_t
******************************************************************************/
GPSPPBufferResult_t GPSPPBufferSetThreshold(kal_uint16 threshold)
{
	if (threshold > GPS_PP_BUFFER_SIZE || threshold == 0)
	{
		//invalid
		return GPS_PP_BUFF_RESULT_INVALID_PARAM;
	}
	ASSERT((gGPSPPBufferSema != NULL));
	//take the semaphore
	kal_take_sem(gGPSPPBufferSema, KAL_INFINITE_WAIT);
	//set new threshold
	gGPSPPBufferCtrlBlk.Threshold = threshold;
	//check full or not
	if ((gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index >= gGPSPPBufferCtrlBlk.Threshold))
	{
		gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status = GPS_PP_BUFF_STATUS_FULL;
		//check the other buffer status
		if (gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status != GPS_PP_BUFF_STATUS_LOADING)
		{
			GPS_GPRMC_Packed_Struct_t* pTmp;

			//exchange the buffers
			pTmp = gGPSPPBuffer[GPS_PP_BUFF_PONG].Body;
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Body = gGPSPPBuffer[GPS_PP_BUFF_PING].Body;
			//set PONG's status as FULL
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_FULL;
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index = gGPSPPBufferCtrlBlk.Threshold;
			//change PING's status to IDLE
			gGPSPPBuffer[GPS_PP_BUFF_PING].Body = pTmp;
			gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
			gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index = 0;
			if (gGPSPPBufferCtrlBlk.BufferFullCallbackFunc != NULL)
			{
				//set PONG's status as Loading
				gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_LOADING;
//				gGPSPPBufferCtrlBlk.LoadingBufferRefCount++;
				gGPSPPBufferCtrlBlk.BufferFullCallbackFunc(gGPSPPBuffer[GPS_PP_BUFF_PONG].Body, 
												gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index, 
												NULL);
			}
		}
	}
	//release semaphore
	kal_give_sem(gGPSPPBufferSema);
	return GPS_PP_BUFF_RESULT_NO_ERROR;
}

/******************************************************************************
* Function:
*	GPSPPBufferFinishLoading
*
* Usage:
*	Reset the status of the loading buffer. Once the caller finishing loading data, this function 
*	must be invoked.
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSPPBufferFinishLoading(void)
{
	ASSERT((gGPSPPBufferSema != NULL));
	//take the semaphore
	kal_take_sem(gGPSPPBufferSema, KAL_INFINITE_WAIT);
	if (gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status == GPS_PP_BUFF_STATUS_LOADING)
	{
		gGPSPPBufferCtrlBlk.LoadingBufferRefCount--;
		if (gGPSPPBufferCtrlBlk.LoadingBufferRefCount == 0)
		{
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_IDLE;
			gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index = 0;
		}
	}
	//release semaphore
	kal_give_sem(gGPSPPBufferSema);
}

/******************************************************************************
* Function:
*	GPSPPBufferGetStoringBuffer
*
* Usage:
*	Get the information of the storing buffer.
*
* Parameters:
*	pBuff - pointer to a buffer for returning the pointer of the storing buffer
*	pDepth - pointer to a buffer for returning the depth of the storing buffer (the amount of items)
*
* Return:
*	GPSPPBufferStatus_t - buffer status
******************************************************************************/
GPSPPBufferStatus_t GPSPPBufferGetStoringBuffer(GPS_GPRMC_Packed_Struct_t** pBuff, 
													kal_uint16* pDepth)
{
	GPSPPBufferStatus_t Status;
	
	ASSERT((gGPSPPBufferSema != NULL));
	//take the semaphore
	kal_take_sem(gGPSPPBufferSema, KAL_INFINITE_WAIT);
	if (pBuff != NULL)
	{
		*pBuff = gGPSPPBuffer[GPS_PP_BUFF_PING].Body;
	}
	if (pDepth != NULL)
	{
		*pDepth = gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Index;
	}
	Status = gGPSPPBuffer[GPS_PP_BUFF_PING].Header.Status; 
	//release semaphore
	kal_give_sem(gGPSPPBufferSema);
	return Status;
}

/******************************************************************************
* Function:
*	GPSPPBufferGetLoadingBuffer
*
* Usage:
*	Get the information of the loading buffer.
*
* Parameters:
*	Purpose - The purpose of getting this buffer
*	pBuff - pointer to a buffer for returning the pointer of the loading buffer
*	pDepth - pointer to a buffer for returning the depth of the loading buffer (the amount of items)
*
* Return:
*	GPSPPBufferStatus_t - orignial buffer status
******************************************************************************/
GPSPPBufferStatus_t GPSPPBufferGetLoadingBuffer(
	GPSPPBufferPurpose_t Purpose,
	GPS_GPRMC_Packed_Struct_t** pBuff,
	kal_uint16* pDepth
	)
{
	GPSPPBufferStatus_t OrgStatus;

	ASSERT((gGPSPPBufferSema != NULL));
	//take the semaphore
	kal_take_sem(gGPSPPBufferSema, KAL_INFINITE_WAIT);
	OrgStatus = gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status;
	if (Purpose == GPS_PP_BUFF_PURPOSE_LOADING)
	{
		gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Status = GPS_PP_BUFF_STATUS_LOADING;
		gGPSPPBufferCtrlBlk.LoadingBufferRefCount++;
	}
	if (pBuff != NULL)
	{
		*pBuff = gGPSPPBuffer[GPS_PP_BUFF_PONG].Body;
	}
	if (pDepth != NULL)
	{
		*pDepth = gGPSPPBuffer[GPS_PP_BUFF_PONG].Header.Index;
	}
	//release semaphore
	kal_give_sem(gGPSPPBufferSema);
	return OrgStatus;
}

