/******************************************************************************
* File name: 
*	gps_app_event_fifo.c
*
* Description:
*	Event FIFO for GPS locate application
*
* Author:
*	Hongji Zhou
*
* Date:
*	2007-12-01
******************************************************************************/
#ifdef GPS_LOCATE_SUPPORT

#include "kal_release.h"
#include "app_buff_alloc.h"

#include "gps_app_event_fifo.h"

/******************************************************************************
* Local constants definition
******************************************************************************/
//Event FIFO size
//should be 2/4/8/16/32/64/128/256
#define GPSAPP_EVENTFIFO_SIZE					16

/******************************************************************************
* Local data structure definition
******************************************************************************/
typedef struct
{
	GPSAppEvent_t Event;
	kal_bool AutoFreeLocalPara;
} GPSAppEventFifoItem_t;

typedef struct
{
	GPSAppEventFifoItem_t Entry[GPSAPP_EVENTFIFO_SIZE];
	kal_uint8 WrPtr;
	kal_uint8 RdPtr;
} GPSAppEventFifoCtrlBlock_t;

/******************************************************************************
* Static global variables
******************************************************************************/
static GPSAppEventFifoCtrlBlock_t gFifoCtrlBlock;

/******************************************************************************
* Static functions
******************************************************************************/

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSAppEventFifoInit
*
* Usage:
*	Initialize event fifo
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSAppEventFifoInit(void)
{
	//initialize control block
	gFifoCtrlBlock.RdPtr = gFifoCtrlBlock.WrPtr = 0;
}

/******************************************************************************
* Function:
*	GPSAppEventFifoPush
*
* Usage:
*	Push one item into the FIFO
*
* Parameters:
*	Event - pointer to the caller supplied event buffer
*	AutoFreeLocalPara:
*		KAL_TRUE - free the memory of local parameter when popping out the item.
*		KAL_FALSE - the caller take the responsibility of freeing the memory.
*
* Return:
*	KAL_TRUE - successful
*	KAL_FALSE - fail
******************************************************************************/
kal_bool GPSAppEventFifoPush(const GPSAppEvent_t* Event, 
									kal_bool AutoFreeLocalPara)
{
	GPSAppEventFifoItem_t* pItem = NULL;
	
	if (Event == NULL)
	{
		//invalid parameter
		return KAL_FALSE;
	}
	//check the FIFO full or not
	if ((kal_uint8)(gFifoCtrlBlock.WrPtr - gFifoCtrlBlock.RdPtr) >= GPSAPP_EVENTFIFO_SIZE)
	{
		//full
		return KAL_FALSE;
	}
	pItem = &(gFifoCtrlBlock.Entry[gFifoCtrlBlock.WrPtr & (GPSAPP_EVENTFIFO_SIZE-1)]);
	//fill the item with the caller supplied content
	memcpy(&(pItem->Event), Event, sizeof(GPSAppEvent_t));
	pItem->AutoFreeLocalPara = AutoFreeLocalPara;
	//move the write pointer to the next cell
	gFifoCtrlBlock.WrPtr++;

	return KAL_TRUE;
}

/******************************************************************************
* Function:
*	GPSAppEventFifoPop
*
* Usage:
*	Pop one item from the FIFO
*
* Parameters:
*	None
*
* Return:
*	KAL_TRUE - successful
*	KAL_FALSE - fail, the FIFO is empty
******************************************************************************/
kal_bool GPSAppEventFifoPop(void)
{
	GPSAppEventFifoItem_t* pItem = NULL;
	
	//check the FIFO empty or not
	if (gFifoCtrlBlock.WrPtr == gFifoCtrlBlock.RdPtr)
	{
		//empty
		return KAL_FALSE;
	}
	//pop the item from the FIFO
	pItem = &(gFifoCtrlBlock.Entry[gFifoCtrlBlock.RdPtr & (GPSAPP_EVENTFIFO_SIZE-1)]);
	gFifoCtrlBlock.RdPtr++;
	//free memory if necessary
	if (pItem->AutoFreeLocalPara == KAL_TRUE && pItem->Event.LocalPara != NULL)
	{
		free_ctrl_buffer(pItem->Event.LocalPara);
		pItem->Event.LocalPara = NULL;
	}

	return KAL_TRUE;
}

/******************************************************************************
* Function:
*	GPSAppEventFifoIsEmpty
*
* Usage:
*	Check the FIFO empty or not
*
* Parameters:
*	None
*
* Return:
*	KAL_TRUE - empty
*	KAL_FALSE - not empty
******************************************************************************/
kal_bool GPSAppEventFifoIsEmpty(void)
{
	return ((gFifoCtrlBlock.WrPtr == gFifoCtrlBlock.RdPtr) ? KAL_TRUE : KAL_FALSE);
}

/******************************************************************************
* Function:
*	GPSAppEventFifoIsFull
*
* Usage:
*	Check the FIFO full or not
*
* Parameters:
*	None
*
* Return:
*	KAL_TRUE - full
*	KAL_FALSE - not full
******************************************************************************/
kal_bool GPSAppEventFifoIsFull(void)
{
	return (((kal_uint8)(gFifoCtrlBlock.WrPtr - gFifoCtrlBlock.RdPtr) >= GPSAPP_EVENTFIFO_SIZE) \
				? KAL_TRUE : KAL_FALSE);
}

/******************************************************************************
* Function:
*	GPSAppEventFifoCurrItem
*
* Usage:
*	Get the current Item pointer in the FIFO
*
* Parameters:
*	None
*
* Return:
*	The current item pointer
*	NULL - the FIFO is empty
******************************************************************************/
GPSAppEvent_t* GPSAppEventFifoCurrItem(void)
{
	if (gFifoCtrlBlock.WrPtr == gFifoCtrlBlock.RdPtr)
	{
		//empty FIFO
		return NULL;
	}
	
	return &(gFifoCtrlBlock.Entry[gFifoCtrlBlock.RdPtr & (GPSAPP_EVENTFIFO_SIZE-1)].Event);
}

#endif //GPS_LOCATE_SUPPORT

