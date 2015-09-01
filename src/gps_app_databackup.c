/******************************************************************************
* File name: 
*	gps_app_databackup.c
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
#include "kal_release.h"
#include "fs_type.h"
#include "fs_func.h"
#include "fs_errcode.h"
#include "app_str.h"

#include "gps_app_databackup.h"
#include "gps_app_configurable_const.h"
#include "gps_app_nema.h"

#define _GPSAPP_DATABACKUP_DBG_	(0)

/******************************************************************************
* Local constants definition
******************************************************************************/
#define GPSAPP_DATABACKUP_DRV  					FS_GetDrive(FS_DRIVE_V_NORMAL, 2, FS_ONLY_ALT_SERIAL )
//FS_GetDrive(FS_DRIVE_V_REMOVABLE, 1, FS_NO_ALT_DRIVE)	//for debug on D167
#define GPSAPP_DATABACKUP_ATTRIBUTE_FILE_NAME		"Backup.att"
#define GPSAPP_DATABACKUP_FOLDER_NAME				"Backup\\"
#define GPSAPP_DATABACKUP_FILE_PREFIX				"BK"
#define GPSAPP_DATABACKUP_FILE_SUFFIX				".dbk"

#define GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX	32
#define GPSAPP_DATABACKUP_ATTFILENAME_LEN_MAX		16

/******************************************************************************
* Local data structures definition
******************************************************************************/

typedef enum
{
	ATTRUPDATE_ADD,
	ATTRUPDATE_REMOVE,
	
	ATTRUPDATE_Invalid
}GPSAppDataBackupAttrUpdate_t;

typedef enum
{
	CELLATTR_FREE,
	CELLATTR_OCCUPIED,

	CELLATTR_Invalid
} GPSAppDataBackupCellAttr_t;

/******************************************************************************
* Global variables
******************************************************************************/
static kal_int8 gGPSAppDataBackupFolderName[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
static kal_uint16 gGPSAppDataBackupAttrFileName[GPSAPP_DATABACKUP_ATTFILENAME_LEN_MAX];
static kal_uint8 gGPSAppDataBackAttrCache[GPSAPP_DATABACKUP_FILES_MAX/8+1];
static kal_semid gGPSAppDataBackupSem = NULL;
static kal_int32 gGPSAppDataBackupCurrLoadingFileID = -1;

/******************************************************************************
* Static functions
******************************************************************************/
static GPSAppDataBackupError_t GPSAppDataBackupWriteAttrFile(kal_bool bNewCreate);
static kal_int32 GPSAppDataBackupApplyCell(
	kal_uint16* pFileName,
	GPSAppDataBackupCellAttr_t CellAttr
	);
static void __inline GPSAppDataBackupUpdateAttrCache(
	kal_int32 ID, 
	GPSAppDataBackupAttrUpdate_t Update
	);
static kal_uint16 __inline GPSAppDataBackupGetFileAmount(void);

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSAppDataBackupInit
*
* Usage:
*	Initialize data backup
*
* Parameters:
*	None
*
* Return:
*	void
******************************************************************************/
void GPSAppDataBackupInit(void)
{
	FS_HANDLE file_handle;
	int result;
	kal_uint16 folderName[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
	kal_int8 tmpBuffer[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
	unsigned int Size;
	GPSAppDataBackupError_t ErrCode;
	kal_int32 ErrExt1=0, ErrExt2=0, ErrExt3=0;

	if (gGPSAppDataBackupSem != NULL)
	{
		//has been initialized
		//do nothing, return error directly
		return;
	}
	//create semaphore
	gGPSAppDataBackupSem = kal_create_sem("Backup SEM", 1);
	ASSERT((gGPSAppDataBackupSem!=NULL));

	//initialize global data
	gGPSAppDataBackupFolderName[0] = 0x00;
	gGPSAppDataBackupAttrFileName[0] = 0x0000;
	memset(&gGPSAppDataBackAttrCache, 0x00, sizeof(gGPSAppDataBackAttrCache));

	//construct attr file name
	sprintf(tmpBuffer, "%c:\\%s", GPSAPP_DATABACKUP_DRV, GPSAPP_DATABACKUP_ATTRIBUTE_FILE_NAME);
	app_ansii_to_unicodestring((kal_int8*)gGPSAppDataBackupAttrFileName, (kal_int8*)tmpBuffer);
	//check the file exists or not
	file_handle = FS_Open(gGPSAppDataBackupAttrFileName, FS_READ_ONLY);
	if (file_handle < FS_NO_ERROR) 
	{
		//the attr file doesn't exist, create it.
		ErrCode = GPSAppDataBackupWriteAttrFile(KAL_TRUE);
		if (ErrCode != GPSAPP_DATABACKUP_ERROR_NONE)
		{
			ErrExt1 = 1;
			ErrExt2 = ErrCode;
			goto _INIT_FAIL_;
		}
	}
	else
	{
		//the attr file exists, read it
		result = FS_Read(file_handle, gGPSAppDataBackAttrCache, sizeof(gGPSAppDataBackAttrCache), &Size);
		FS_Close(file_handle);
		if (result != FS_NO_ERROR || Size != sizeof(gGPSAppDataBackAttrCache))
		{
			ErrExt1 = 2;
			ErrExt2 = result;
			ErrExt3 = Size;
			goto _INIT_FAIL_;
		}
	}
	
	//construct folder name
	sprintf(gGPSAppDataBackupFolderName, "%c:\\%s", GPSAPP_DATABACKUP_DRV, GPSAPP_DATABACKUP_FOLDER_NAME);
	app_ansii_to_unicodestring((kal_int8*)folderName, (kal_int8*)gGPSAppDataBackupFolderName);
	//check the folder exists or not
	file_handle = FS_Open(folderName, FS_OPEN_DIR | FS_READ_ONLY );
	if (file_handle < FS_NO_ERROR) 
	{
		//the folder doesn't exist, create it.
		result = FS_CreateDir(folderName);
		if (result < FS_NO_ERROR)
		{
			//fail to create
			ErrExt1 = 3;
			ErrExt2 = result;
			goto _INIT_FAIL_;
		}
	}
	else
	{
		FS_Close(file_handle);
		/*
		//sync attr file and backup files
		ErrCode = GPSAppDataBackupSyncAttr();
		if (ErrCode != GPSAPP_DATABACKUP_ERROR_NONE)
		{
			ErrExt1 = 4;
			ErrExt2 = ErrCode;
			goto _INIT_FAIL_;
		}
		*/
	}

	return;

_INIT_FAIL_:
	EXT_ASSERT(0, ErrExt1, ErrExt2, ErrExt3);
	return;
}

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
	)
{
	kal_uint16 FileName[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
	GPSAppDataBackupError_t ErrCode = GPSAPP_DATABACKUP_ERROR_NONE;
	FS_HANDLE file_handle;
	int result;
	kal_uint32 len;
	kal_int32 FileID;

	if (pBuff == NULL || Length == 0)
	{
		return GPSAPP_DATABACKUP_ERROR_INVALID_PARAM;
	}
	//take the semaphore
	kal_take_sem(gGPSAppDataBackupSem, KAL_INFINITE_WAIT);
	//apply file name
	if ((FileID = GPSAppDataBackupApplyCell(FileName, CELLATTR_FREE)) < 0)
	{
		ErrCode = GPSAPP_DATABACKUP_ERROR_STORAGE_FULL;
		goto _RelSem_Return_;
	}
	//create file
	file_handle = FS_Open(FileName, FS_CREATE | FS_READ_WRITE);
	if (file_handle < FS_NO_ERROR)
	{
		//fail to open
		ErrCode = GPSAPP_DATABACKUP_ERROR_OPEN_FAIL;
		#if _GPSAPP_DATABACKUP_DBG_
		trace_printf("GPSAppDataBackupStore: Open fail (%d)", file_handle);
		#endif //_GPSAPP_DATABACKUP_DBG_
		goto _RelSem_Return_;
	}
	//write
	result = FS_Write(file_handle, (void*)pBuff, Length, &len);
	FS_Close(file_handle);
	if (result != FS_NO_ERROR || len != Length)
	{
		ErrCode = GPSAPP_DATABACKUP_ERROR_WRITE_FAIL;
		goto _RelSem_Return_;
	}
	//update attr
	GPSAppDataBackupUpdateAttrCache(FileID, ATTRUPDATE_ADD);
	ErrCode = GPSAppDataBackupWriteAttrFile(KAL_FALSE);
	
_RelSem_Return_:
	//release semaphore
	kal_give_sem(gGPSAppDataBackupSem);
	return ErrCode;
}

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
	)
{
	GPSAppDataBackupError_t ErrCode = GPSAPP_DATABACKUP_ERROR_NONE;
	kal_int32 FileID;
	kal_uint16 FileName[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
	FS_HANDLE file_handle;
	int result;
	unsigned int len;

	if (pBuff == NULL && pLength == NULL)
	{
		return GPSAPP_DATABACKUP_ERROR_INVALID_PARAM;
	}
	//take the semaphore
	kal_take_sem(gGPSAppDataBackupSem, KAL_INFINITE_WAIT);

	gGPSAppDataBackupCurrLoadingFileID = -1;
	
	if (pBuff == NULL)
	{
		//just inquery file amount
		*pLength = GPSAppDataBackupGetFileAmount();
		ErrCode = GPSAPP_DATABACKUP_ERROR_NONE;
		goto _RelSem_Return_;
	}

	//apply file name
	if ((FileID = GPSAppDataBackupApplyCell(FileName, CELLATTR_OCCUPIED)) < 0)
	{
		ErrCode = GPSAPP_DATABACKUP_ERROR_STORAGE_EMPTY;
		goto _RelSem_Return_;
	}
	//save file ID
	gGPSAppDataBackupCurrLoadingFileID = FileID;
	//create file
	file_handle = FS_Open(FileName, FS_READ_ONLY);
	if (file_handle < FS_NO_ERROR)
	{
		//fail to open
		ErrCode = GPSAPP_DATABACKUP_ERROR_OPEN_FAIL;
		goto _RelSem_Return_;
	}
	//read
	result = FS_Read(file_handle, pBuff, *pLength, &len);
	FS_Close(file_handle);
	if (result != FS_NO_ERROR)
	{
		//read fail
		ErrCode = GPSAPP_DATABACKUP_ERROR_READ_FAIL;
		goto _RelSem_Return_;
	}
	*pLength = len;

#if 0
	{
	GPS_GPRMC_Packed_Struct_t *pPack = (GPS_GPRMC_Packed_Struct_t *)pBuff;
	int tmpLen = len / sizeof(GPS_GPRMC_Packed_Struct_t);
	char tmpStr[128];
	int i;
	Result_t result = RESULT_ERROR;
	for (i = 0; i < tmpLen; i++)
		{
		memset(tmpStr, 0, sizeof(tmpStr));
		result = GPS_APP_GPRMC_Packed2Str(tmpStr, pPack);
		pPack++;
		trace_printf("XXX: %s", tmpStr);
		}
	}
#endif

_RelSem_Return_:
	//release semaphore
	kal_give_sem(gGPSAppDataBackupSem);
	return ErrCode;
}

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
GPSAppDataBackupError_t GPSAppDataBackupDelCurrFile(void)
{
	kal_int8 tmpBuffer[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
	kal_uint16 FileName[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];

	//take the semaphore
	kal_take_sem(gGPSAppDataBackupSem, KAL_INFINITE_WAIT);

	if (gGPSAppDataBackupCurrLoadingFileID < 0)
	{
		goto _RelSem_Return_;
	}
	//construct the file name
#if (GPSAPP_DATABACKUP_FILES_MAX <= 100)	
	sprintf(tmpBuffer, "%s%s%02d%s", 
		gGPSAppDataBackupFolderName, 
		GPSAPP_DATABACKUP_FILE_PREFIX, 
		gGPSAppDataBackupCurrLoadingFileID, 
		GPSAPP_DATABACKUP_FILE_SUFFIX);
#elif (GPSAPP_DATABACKUP_FILES_MAX <= 1000)
	sprintf(tmpBuffer, "%s%s%03d%s", 
		gGPSAppDataBackupFolderName, 
		GPSAPP_DATABACKUP_FILE_PREFIX, 
		gGPSAppDataBackupCurrLoadingFileID, 
		GPSAPP_DATABACKUP_FILE_SUFFIX);
#else
	#error "Cannot define GPSAPP_DATABACKUP_FILES_MAX larger than 1000"
#endif
	app_ansii_to_unicodestring((kal_int8*)FileName, (kal_int8*)tmpBuffer);
	//delete the file
	FS_Delete(FileName);
	//update cache
	GPSAppDataBackupUpdateAttrCache(gGPSAppDataBackupCurrLoadingFileID, ATTRUPDATE_REMOVE);
	//update attr file
	GPSAppDataBackupWriteAttrFile(KAL_FALSE);
	gGPSAppDataBackupCurrLoadingFileID = -1;
	
_RelSem_Return_:
	//release semaphore
	kal_give_sem(gGPSAppDataBackupSem);
	return GPSAPP_DATABACKUP_ERROR_NONE;
}

static GPSAppDataBackupError_t GPSAppDataBackupWriteAttrFile(kal_bool bNewCreate)
{
	FS_HANDLE file_handle;
	int result;
	kal_uint32 len;
	
	//create file
	file_handle = FS_Open((kal_uint16*)gGPSAppDataBackupAttrFileName, (bNewCreate == KAL_TRUE ? FS_CREATE : 0) | FS_READ_WRITE);
	if (file_handle < FS_NO_ERROR)
	{
		//fail to create
		return GPSAPP_DATABACKUP_ERROR_OPEN_FAIL;
	}
	//write
	result = FS_Write(file_handle, gGPSAppDataBackAttrCache, sizeof(gGPSAppDataBackAttrCache), &len);
	FS_Close(file_handle);
	if (result != FS_NO_ERROR || len != sizeof(gGPSAppDataBackAttrCache))
	{
		return GPSAPP_DATABACKUP_ERROR_WRITE_FAIL;
	}
	else
	{
		return GPSAPP_DATABACKUP_ERROR_NONE;
	}
}

static kal_int32 GPSAppDataBackupApplyCell(
	kal_uint16* pFileName,
	GPSAppDataBackupCellAttr_t CellAttr
	)
{
	kal_int8 tmpBuffer[GPSAPP_DATABACKUP_FULLPATHNAME_LEN_MAX];
	kal_int32 BitIndx, ByteIndx, CellIndx = -1;

	if (pFileName == NULL || CellAttr >= CELLATTR_Invalid)
	{
		//invalid parameters
		return -2;
	}
	
	if (CellAttr == CELLATTR_FREE)
	{
		for (ByteIndx = 0; ByteIndx < sizeof(gGPSAppDataBackAttrCache); ByteIndx++)
		{
			for (BitIndx = 0; BitIndx < 8; BitIndx++)
			{
				if ((gGPSAppDataBackAttrCache[ByteIndx] & (1 << BitIndx)) == 0)
				{
					//found a free cell
					CellIndx = ByteIndx * 8 + BitIndx;
					break;
				}
			}
			if (CellIndx >= 0)
			{
				break;
			}
		}
	}
	else if (CellAttr == CELLATTR_OCCUPIED)
	{
		for (ByteIndx = 0; ByteIndx < sizeof(gGPSAppDataBackAttrCache); ByteIndx++)
		{
			for (BitIndx = 0; BitIndx < 8; BitIndx++)
			{
				if ((gGPSAppDataBackAttrCache[ByteIndx] & (1 << BitIndx)) != 0)
				{
					//found an occupied cell
					CellIndx = ByteIndx * 8 + BitIndx;
					break;
				}
			}
			if (CellIndx >= 0)
			{
				break;
			}
		}
	}
	if (CellIndx >= GPSAPP_DATABACKUP_FILES_MAX)
	{
		//beyond the range
		return -1;
	}
#if (GPSAPP_DATABACKUP_FILES_MAX <= 100)	
	sprintf(tmpBuffer, "%s%s%02d%s", 
		gGPSAppDataBackupFolderName, 
		GPSAPP_DATABACKUP_FILE_PREFIX, 
		CellIndx, 
		GPSAPP_DATABACKUP_FILE_SUFFIX);
#elif (GPSAPP_DATABACKUP_FILES_MAX <= 1000)
	sprintf(tmpBuffer, "%s%s%03d%s", 
		gGPSAppDataBackupFolderName, 
		GPSAPP_DATABACKUP_FILE_PREFIX, 
		CellIndx, 
		GPSAPP_DATABACKUP_FILE_SUFFIX);
#else
	#error "Cannot define GPSAPP_DATABACKUP_FILES_MAX larger than 1000"
#endif
	app_ansii_to_unicodestring((kal_int8*)pFileName, (kal_int8*)tmpBuffer);
	return CellIndx;
}

static void __inline GPSAppDataBackupUpdateAttrCache(
	kal_int32 ID, 
	GPSAppDataBackupAttrUpdate_t Update
	)
{
	kal_int32 BitIndx, ByteIndx;

	if (ID >= GPSAPP_DATABACKUP_FILES_MAX || Update >= ATTRUPDATE_Invalid)
	{
		//invalid parameters
		return;
	}
	BitIndx = ID % 8;
	ByteIndx = ID / 8;
	if (Update == ATTRUPDATE_ADD)
	{
		gGPSAppDataBackAttrCache[ByteIndx] |= 1 << BitIndx;
	}
	else if (Update == ATTRUPDATE_REMOVE)
	{
		gGPSAppDataBackAttrCache[ByteIndx] &= ~(1 << BitIndx);
	}
}

static kal_uint16 __inline GPSAppDataBackupGetFileAmount(void)
{
	kal_int32 BitIndx, ByteIndx;
	kal_uint16 FileCount = 0;
	
	for (ByteIndx = 0; ByteIndx < sizeof(gGPSAppDataBackAttrCache); ByteIndx++)
	{
		for (BitIndx = 0; BitIndx < 8; BitIndx++)
		{
			if ((gGPSAppDataBackAttrCache[ByteIndx] & (1 << BitIndx)) != 0)
			{
				//found a file
				FileCount++;
			}
		}
	}
	return FileCount;
}

