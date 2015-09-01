/******************************************************************************
* File name: 
*	gps_app_databackup.h
*
* Description: 
*	This module provide the interface of data backup for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.04.08
*
******************************************************************************/
#ifndef _GPS_APP_DATABACKUP_H_
#define _GPS_APP_DATABACKUP_H_

#include "kal_release.h"

typedef enum
{
	//no error
	GPSAPP_DATABACKUP_ERROR_NONE = 0,
	//invalid parameters
	GPSAPP_DATABACKUP_ERROR_INVALID_PARAM,
	//open file failed
	GPSAPP_DATABACKUP_ERROR_OPEN_FAIL,
	//write file failed
	GPSAPP_DATABACKUP_ERROR_WRITE_FAIL,
	//read file failed
	GPSAPP_DATABACKUP_ERROR_READ_FAIL,
	//storage full
	GPSAPP_DATABACKUP_ERROR_STORAGE_FULL,
	//storage empty
	GPSAPP_DATABACKUP_ERROR_STORAGE_EMPTY,
	
	GPSAPP_DATABACKUP_ERROR_Total
} GPSAppDataBackupError_t;


/******************************************************************************
* Function:
*	GPSAppDataBackupInit
* 
* Usage:
*	Initialize data backup.
*
* Parameters:
*	None
*
* Return:
*	None
******************************************************************************/
void GPSAppDataBackupInit(void);

/******************************************************************************
* Function:
*	GPSAppDataBackupStore
* 
* Usage:
*	Store data to backup files.
*
* Parameters:
*	pBuff - pointer to caller's data buffer, which contains data to be stored.
*	Length - data length (in byte)
*
* Return:
*	Error code
******************************************************************************/
GPSAppDataBackupError_t GPSAppDataBackupStore(
	const kal_uint8* pBuff,
	kal_uint16 Length
	);

/******************************************************************************
* Function:
*	GPSAppDataBackupLoad
* 
* Usage:
*	Load data from backup files.
*
* Parameters:
*	pBuff - pointer to caller's data buffer, which is used to store loading data.
*	pLength - (Input) the buffer size (in byte)
*			 (Output) data length loaded from the backup file (in byte)
*
*	NOTE: If just want to check whether backup files existing, the caller is allowed to assign a
*			NULL pointer to pBuff; the other parameter pLength is used to return the number
*			of these backup files, so it should not be NULL.
*
* Return:
*	Error code
******************************************************************************/
GPSAppDataBackupError_t GPSAppDataBackupLoad(
	kal_uint8* pBuff,
	kal_uint16* pLength
	);

/******************************************************************************
* Function:
*	GPSAppDataBackupDelCurrFile
* 
* Usage:
*	Delete the current loading backup files.
*
* Parameters:
*	None
*
* Return:
*	Error code
******************************************************************************/
GPSAppDataBackupError_t GPSAppDataBackupDelCurrFile(void);


#endif //_GPS_APP_DATABACKUP_H_

