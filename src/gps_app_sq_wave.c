/******************************************************************************
* File name: 
*	gps_app_sq_wave.c
*
* Description: 
*	This module provide the interface of square wave generator for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.1.14
*
******************************************************************************/
#if defined(GPS_LOCATE_SUPPORT)

#include "drv_comm.h"
#include "reg_base.h"
#include "gpio_hw.h"
#include "gpio_sw.h"

#include "gps_app_timer_mgr.h"
#include "gps_app_sq_wave.h"
#include "gps_app_configurable_const.h"

/******************************************************************************
* Local constants definition
******************************************************************************/
enum {
	GSG_HOLD = 0,
	GSG_INVERSE
};

#if defined(M100)
//the wave table
static const unsigned char gGsgWaveTable[GSG_WAVE_TABLE_SIZE] =
{
GSG_INVERSE, // 0
GSG_INVERSE, // 1
GSG_INVERSE, // 2
GSG_INVERSE, // 3
GSG_INVERSE, // 4
GSG_HOLD, // 5
GSG_HOLD, // 6
GSG_HOLD, // 7
GSG_HOLD, // 8
GSG_HOLD, // 9
GSG_HOLD, // 10
GSG_HOLD, // 11
GSG_HOLD, // 12
GSG_HOLD, // 13
GSG_HOLD, // 14
GSG_HOLD, // 15
GSG_HOLD, // 16
GSG_HOLD, // 17
GSG_HOLD, // 18
GSG_INVERSE // 19
};
#endif //M100
/******************************************************************************
* Global variables
******************************************************************************/
#if defined(M100)
static unsigned char gGsgWaveTablePtr;
//static NU_TIMER gGsgTimer;
static GPSAppTimer_t gGsgTimer;
static GPSAppTimer_t gGsgTimer2;
#endif //M100
/******************************************************************************
* Static functions
******************************************************************************/
static void GSG_Callback(GPSAppTimerID_t);
static void GSG_WidthSQWCB(GPSAppTimerID_t);

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* GSG_Init
* 
* Intialize GSG
******************************************************************************/
void GSG_Init(void)
{
#if defined(M100)
	//initially, set output low
	DRV_Reg(GPIO_DOUT2) &= ~(1 << GSG_GPIO_SLOT);
	gGsgWaveTablePtr = 0;
	gGsgTimer = NULL;
#endif //M100
}

/******************************************************************************
* GSG_Start
* 
* Start GSG
*
* Return value:
*	KAL_TRUE - successful
*	KAL_FALSE - fail
******************************************************************************/
kal_bool GSG_Start(void)
{
#if defined(M100)
	if (gGsgTimer != NULL)
	{
		//the square wave is running
		return KAL_FALSE;
	}
	//start the timer
	gGsgTimer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GSG_Callback,
									GSG_WAVE_WIDTH,
									GSG_WAVE_WIDTH,
									KAL_TRUE);
	//initially, set output high
	DRV_Reg(GPIO_DOUT2) |= (1 << GSG_GPIO_SLOT);
	//set the wave table pointer to the header
	gGsgWaveTablePtr = 0;
	return KAL_TRUE;
#else
	return KAL_TRUE;
#endif //M100
}

/******************************************************************************
* GSG_Stop
* 
* Stop GSG
*
* Return value:
*	KAL_TRUE - successful
*	KAL_FALSE - fail
******************************************************************************/
kal_bool GSG_Stop(void)
{
#if defined(M100)
	if (gGsgTimer == NULL)
	{
		//the square wave is not running
		return KAL_FALSE;
	}
	//stop the timer
	GPSAppTimer_Stop(gGsgTimer, KAL_TRUE);
	gGsgTimer = NULL;
	//set output low
	DRV_Reg(GPIO_DOUT2) &= ~(1 << GSG_GPIO_SLOT);
	return KAL_TRUE;
#else
	return KAL_TRUE;
#endif //M100
}

/******************************************************************************
* GSG_Callback
* 
* Timer handler
******************************************************************************/
static void GSG_Callback(GPSAppTimerID_t Id)
{
#if defined(M100)
	if (gGsgWaveTable[gGsgWaveTablePtr] == GSG_INVERSE)
	{
		//According to the wave table, the output signal should be inversed.
		volatile kal_uint16 originalValue = DRV_Reg(GPIO_DOUT2);
		if ((originalValue & (1 << GSG_GPIO_SLOT)) == 0)
		{
			//change to high
			DRV_Reg(GPIO_DOUT2) = originalValue | (1 << GSG_GPIO_SLOT);
		}
		else
		{
			//change to low
			DRV_Reg(GPIO_DOUT2) = originalValue & ~(1 << GSG_GPIO_SLOT);
		}
	}
	//adjust the wave table pointer
	if (gGsgWaveTablePtr >= (GSG_WAVE_TABLE_SIZE-1))
	{
		gGsgWaveTablePtr = 0;
	}
	else
	{
		gGsgWaveTablePtr += 1;
	}
#endif //M100
}

void GSG_WidthSquareWave(kal_uint16 width)
{
	GPIO_WriteIO(1, GPS_SQWAVE_GPIO);
	if (gGsgTimer2 == NULL)
	{
		//start the timer
		gGsgTimer2 = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
									GSG_WidthSQWCB,
									KAL_TICKS_1_SEC * width,
									0,
									KAL_TRUE);
	}
	else
	{
		GPSAppTimer_Reset(gGsgTimer2, 
						GSG_WidthSQWCB,
						KAL_TICKS_1_SEC * width,
						0,
						KAL_TRUE);
	}

}

static void GSG_WidthSQWCB(GPSAppTimerID_t Id)
{
	GPIO_WriteIO(0, GPS_SQWAVE_GPIO);
	if (gGsgTimer2 != NULL)
	{
		GPSAppTimer_Stop(gGsgTimer2, KAL_TRUE);
		gGsgTimer2 = NULL;
	}
}

#endif // GPS_LOCATE_SUPPORT

