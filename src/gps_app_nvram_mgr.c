/******************************************************************************
* File name: 
*	gps_app_nvram_mgr.c
*
* Description: 
*	This module provide the interface of NVRAM items for GPS application.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2007.11.19
*
******************************************************************************/
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

#include "kal_release.h"
#include "fs_type.h"
#include "fs_func.h"
#include "fs_errcode.h"
#include "app_str.h"
#include "bmt.h"

#include "gps_app_timer_mgr.h"
#include "gps_app_nvram_mgr.h"

#define GPS_NVRAM_DEBUG
#ifdef GPS_NVRAM_DEBUG
#define gps_nvram_log		trace_printf
#else
static void gps_nvram_log(kal_char *fmt, ...) { }
#endif

/******************************************************************************
* Local constants definition
******************************************************************************/
#define GPSLOCATE_NVRAMDATA_DRV  					FS_GetDrive(FS_DRIVE_I_SYSTEM, 2, FS_ONLY_ALT_SERIAL )
#define GPSLOCATE_NVRAMDATA_FOLDER_NAME				"GPSData\\"
#define GPSLOCATE_NVRAMDATA_FILE_NAME				"GPSData.bin"
#define GPSLOCATE_NVRAMDATA_FULLPATHNAME_LEN_MAX	32

#define GPSLOCATE_NVRAMDATA_DEFAULT_PWD_CHAR		'0'
#define GPSLOCATE_NVRAMDATA_DEFAULT_TMSETTING		0
#define GPSLOCATE_NVRAMDATA_DEFAULT_BEARERMODE		(GPS_BEARER_SMS << 8 | GPS_SMS_MODE_P2P)

/*
*	GPS Locate NVRAM Version Number
*
*	!!!
*	Once your update any record (add, delete, change size, change structure), you must change
*	this version number to force NVRAM system clean the storage.
*	It is recommended that increase the old value with 1.
*/
#define GPSLOCATE_NVRAM_VER_NO						(8)	//range 0 ~ 255

/*
*	Version number size
*
*	Use the first byte of NVRAM as version number
*/
#define GPSLOCATE_NVRAM_VER_NO_SIZE	(sizeof(unsigned char))

//add one record size to the total size
#define ADD_REC_SIZE(rec)	+ GPS_NVRAM_RECSIZE_##rec

/*
*	Definition of total size
*
*	Add your new record size to this total size definition.
*/
#define GPSLOCATE_NVRAM_TOTAL_SIZE	(GPSLOCATE_NVRAM_VER_NO_SIZE	\
	ADD_REC_SIZE(SERVICENUMBER)			\
	ADD_REC_SIZE(SERVICEPASSWORD)		\
	ADD_REC_SIZE(USERNUMBER)			\
	ADD_REC_SIZE(USERPASSWORD)			\
	ADD_REC_SIZE(TIMINGNUMBER)			\
	ADD_REC_SIZE(BSNUMBER)				\
	ADD_REC_SIZE(SETTINGS)				\
	ADD_REC_SIZE(SETTINGSBACKUP)		\
	ADD_REC_SIZE(BEARERMODE)			\
	ADD_REC_SIZE(SERVERADDR)			\
	ADD_REC_SIZE(GPRSUSER)				\
	ADD_REC_SIZE(GPRSPASSWORD)			\
	ADD_REC_SIZE(GPRSUPLOADSETTING)		\
	ADD_REC_SIZE(GPSONSETTING)			\
	ADD_REC_SIZE(HFREEONSETTING)		\
	ADD_REC_SIZE(POWERONCOUNT)			\
	ADD_REC_SIZE(DEFENCEONSETTING)		\
	ADD_REC_SIZE(VIBSENSOR_POLLINGINTERVAL)		\
	ADD_REC_SIZE(VIBSENSOR_SILENCETHRESHOLD)	\
	ADD_REC_SIZE(MTCALLPROFILE)					\
	ADD_REC_SIZE(FIXEDPOSITION)					\
	ADD_REC_SIZE(POSMONITORONFF)				\
	ADD_REC_SIZE(GPRSAPN)						\
	ADD_REC_SIZE(GPRSAPNUSER)					\
	ADD_REC_SIZE(GPRSAPNPWD)					\
	ADD_REC_SIZE(SOSPHONECALL)					\
	ADD_REC_SIZE(ITRACKFRONTMSG)				\
	ADD_REC_SIZE(ITRACKREARMSG)					\
	ADD_REC_SIZE(GPRSUPLOADSETTING2)			\
	ADD_REC_SIZE(RATELIMIT)						\
	ADD_REC_SIZE(ENDSIGN)						\
	)

/*
*	The list of record size
*
*	Append your new record size to the tail of this list.
*/
static const unsigned short GPSLOCATE_REC_SIZE[GPS_NVRAM_RECID_Total] =
{
	//size of service number
	GPS_NVRAM_RECSIZE_SERVICENUMBER,
	//size of service password
	GPS_NVRAM_RECSIZE_SERVICEPASSWORD,
	//size of user number
	GPS_NVRAM_RECSIZE_USERNUMBER,
	//size of user password
	GPS_NVRAM_RECSIZE_USERPASSWORD,
	//size of timing number
	GPS_NVRAM_RECSIZE_TIMINGNUMBER,
	//size of bs number
	GPS_NVRAM_RECSIZE_BSNUMBER,
	//size of settings
	GPS_NVRAM_RECSIZE_SETTINGS,
	//size of settings backup
	GPS_NVRAM_RECSIZE_SETTINGSBACKUP,
	//size of bearer mode
	GPS_NVRAM_RECSIZE_BEARERMODE,
	//size of server addr
	GPS_NVRAM_RECSIZE_SERVERADDR,
	//size of gprs user
	GPS_NVRAM_RECSIZE_GPRSUSER,
	//size of gprs password
	GPS_NVRAM_RECSIZE_GPRSPASSWORD,
	//size of gprs upload settings
	GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING,
	//size of gps on/off setting
	GPS_NVRAM_RECSIZE_GPSONSETTING,
	//size of handfree on/off
	GPS_NVRAM_RECSIZE_HFREEONSETTING,
	//size of power on cycle count
	GPS_NVRAM_RECSIZE_POWERONCOUNT,
	//size of defence on/off
	GPS_NVRAM_RECSIZE_DEFENCEONSETTING,
	//size of vibration sensor polling interval
	GPS_NVRAM_RECSIZE_VIBSENSOR_POLLINGINTERVAL,
	//size of vibration sensor silence threshold
	GPS_NVRAM_RECSIZE_VIBSENSOR_SILENCETHRESHOLD,
	//size of mt call profile
	GPS_NVRAM_RECSIZE_MTCALLPROFILE,
	//size of fixed position
	GPS_NVRAM_RECSIZE_FIXEDPOSITION,
	//size of position monitor on/off
	GPS_NVRAM_RECSIZE_POSMONITORONFF,
	//size of gprs apn
	GPS_NVRAM_RECSIZE_GPRSAPN,
	//size of gprs apn user
	GPS_NVRAM_RECSIZE_GPRSAPNUSER,
	//size of gprs apn password
	GPS_NVRAM_RECSIZE_GPRSAPNPWD,
	//size of setting for make phone call or not for sos
	GPS_NVRAM_RECSIZE_SOSPHONECALL,
	//size of itrack front msg
	GPS_NVRAM_RECSIZE_ITRACKFRONTMSG,
	//size of itrack rear msg
	GPS_NVRAM_RECSIZE_ITRACKREARMSG,
	//size of gprs upload settings
	GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING2,
	//size of rate limit
	GPS_NVRAM_RECSIZE_RATELIMIT,

	//sizeof end sign
	GPS_NVRAM_RECSIZE_ENDSIGN
};

/******************************************************************************
* Local data structures definition
******************************************************************************/

typedef struct
{
	unsigned char Buff[GPSLOCATE_NVRAM_TOTAL_SIZE];
} GPSLocateNvramDataStruct_t;

typedef struct
{
	/*
	KAL_TRUE: the data in the cache is valid
	KAL_FALSE: the data in the cache is invalid
	*/
	kal_bool Valid;
	kal_bool Changed;
	GPSLocateNvramDataStruct_t Cache;
} GPSLocateNvramDataCacheStruct_t;

/******************************************************************************
* Global variables
******************************************************************************/
static kal_uint16 gGPSLocateNvramFullPathFileName[GPSLOCATE_NVRAMDATA_FULLPATHNAME_LEN_MAX];
static GPSLocateNvramDataCacheStruct_t gGPSLocateNvramDataCache;
static kal_semid gGPSLocateNvramSem = NULL;

static GPSLocateNvramDataStruct_t CacheBackup;
static GPSAppTimer_t gps_nvramstore_timer;

/* 
GPS locate restore flag
This flag will be set in INT_Initialize if SOS key is pressed during power on.
When using it to determine whether restoring pre-saved data or not, must check BMT.PWRon
simultaneously.
BMT.PWRon should be equal to PWRKEYPWRON, otherwise, don't issue restoring.
Use it like this:
if (gGPSLocateRestoreFlag == 1 && BMT.PWRon == PWRKEYPWRON)
{
	<do restoring>
}
*/
kal_uint32 gGPSLocateRestoreFlag;

/******************************************************************************
* Static functions
******************************************************************************/
static kal_bool GPSLocateNvramCreateDataFile(void);
static kal_bool GPSLocateNvramLoadDefault(FS_HANDLE file_handle, kal_bool newCreate);
static kal_bool GPSLocateNvramLoadData(void);
static kal_bool GPSLocateNvramStoreData(void);
static unsigned short GPSLocateNvramGetRecOffset(GPSLocateNvramRecId_t RecId);

static void GPSLocateNvramRepeatHandler(GPSAppTimerID_t Id);

/******************************************************************************
* Functions implementation
******************************************************************************/

/******************************************************************************
* Function:
*	GPSLocateNvramInitDataFile
*
* Usage:
*	Initialize data file
*	If the file doesn't exist, create it and write the default data to the file. Otherwise, load the
*	content of the file to the cache.
*
* Parameters:
*	None
*
* Return:
*	KAL_TRUE - successfully
*	KAL_FALSE - failed
******************************************************************************/
kal_bool GPSLocateNvramInitDataFile(void)
{
	FS_HANDLE file_handle;
	int result;
	kal_uint16 folderName[GPSLOCATE_NVRAMDATA_FULLPATHNAME_LEN_MAX];
	char tmpBuffer[GPSLOCATE_NVRAMDATA_FULLPATHNAME_LEN_MAX];
	kal_bool bRet = KAL_TRUE, bNeedCreate = KAL_FALSE;
	unsigned int Size;
	unsigned char Ver;

	if (gGPSLocateNvramSem != NULL)
	{
		//has been initialized
		//do nothing, return error directly
		return KAL_FALSE;
	}
	//create semaphore
	gGPSLocateNvramSem = kal_create_sem("GPSNVRAM SEM", 1);
	ASSERT((gGPSLocateNvramSem!=NULL));
	/*
	* Taking the semaphore here will make the handset not able to power up.
	* Maybe, semaphore is not ready now.
	*/
	//take the semaphore
	//kal_take_sem(gGPSLocateNvramSem, KAL_INFINITE_WAIT);

	//initialize global data
	gGPSLocateNvramFullPathFileName[0] = 0x0000;
	gGPSLocateNvramDataCache.Valid = KAL_FALSE;
	
	//construct folder name
	sprintf(tmpBuffer, "%c:\\%s", GPSLOCATE_NVRAMDATA_DRV, GPSLOCATE_NVRAMDATA_FOLDER_NAME);
	app_ansii_to_unicodestring((kal_int8*)folderName, (kal_int8*)tmpBuffer);
	//check the folder exists or not
	file_handle = FS_Open(folderName, FS_OPEN_DIR | FS_READ_ONLY );
	if (file_handle < FS_NO_ERROR) 
	{
		//the folder doesn't exist, create it.
		result = FS_CreateDir(folderName);
		if (result < FS_NO_ERROR)
		{
			//fail to create
			bRet = KAL_FALSE;
			//return KAL_FALSE;
			goto _RelSem_Return_;
		}
	}
	else
	{
		FS_Close(file_handle);
	}

	/*
	1. construct full path file name
	2. check file exist or not
	3. check file valid or not (by file size)
	4. if file doesn't exist or it is invalid, re-create and restore default data for it
	*/

	//construct full path file name
	sprintf(tmpBuffer, "%c:\\%s%s", 
					GPSLOCATE_NVRAMDATA_DRV, 
					GPSLOCATE_NVRAMDATA_FOLDER_NAME,
					GPSLOCATE_NVRAMDATA_FILE_NAME);
	app_ansii_to_unicodestring((kal_int8*)gGPSLocateNvramFullPathFileName, (kal_int8*)tmpBuffer);
	//check files exist or not
	file_handle = FS_Open(gGPSLocateNvramFullPathFileName, FS_READ_ONLY);
	if (file_handle < FS_NO_ERROR)
	{
		//this file doesn't exist, need create it
		bNeedCreate = KAL_TRUE;
	}
	else 
	{
		if (FS_GetFileSize(file_handle, &Size) < FS_NO_ERROR)
		{
			//fail to get file size, consider the file invalid, need create it
			bNeedCreate = KAL_TRUE;
		}
		else if (Size != sizeof(GPSLocateNvramDataStruct_t))
		{
			//invalid size, need re-create it
			bNeedCreate = KAL_TRUE;
		}
		else
		{
			//check version number
			result = FS_Read(file_handle, &Ver, sizeof(unsigned char), &Size);
			if (result == FS_NO_ERROR && Size == sizeof(unsigned char))
			{
				//Got version number
				if (Ver != GPSLOCATE_NVRAM_VER_NO)
				{
					//old version, should be re-created
					bNeedCreate = KAL_TRUE;
				}
			}
		}
		//close the file
		FS_Close(file_handle);
		if (bNeedCreate == KAL_TRUE)
		{
			//delete the bad file
			FS_Delete(gGPSLocateNvramFullPathFileName);
		}
	}
	if (bNeedCreate == KAL_FALSE)
	{
#if 0//defined(M100)
		if (gGPSLocateRestoreFlag == 1 && BMT.PWRon == PWRKEYPWRON)
		{
			//should restore the default value
			file_handle = FS_Open(gGPSLocateNvramFullPathFileName, FS_READ_WRITE);
			if (file_handle < FS_NO_ERROR)
			{
				//fail to open
				bRet = KAL_FALSE;
				goto _RelSem_Return_;
			}
			else
			{
				//load default
				bRet = GPSLocateNvramLoadDefault(file_handle, KAL_FALSE);
				FS_Close(file_handle);
				goto _RelSem_Return_;
			}
		}
		else
#endif //	M100	
		{
			//valid file, load the content to the cache
			bRet = GPSLocateNvramLoadData();
			goto _RelSem_Return_;
		}
	}
	else
	{
		//create the file
		if (GPSLocateNvramCreateDataFile() != KAL_TRUE)
		{
			//fail to create, set the full path name empty
			gGPSLocateNvramFullPathFileName[0] = 0x0000;
			bRet = KAL_FALSE;
			//return KAL_FALSE;
			goto _RelSem_Return_;
		}
	}
_RelSem_Return_:
	//release the semaphore
	//kal_give_sem(gGPSLocateNvramSem);
	return bRet;
}

static kal_bool GPSLocateNvramCreateDataFile(void)
{
	FS_HANDLE file_handle;
	kal_bool result;

	if (app_ucs2_strlen((const kal_int8*)gGPSLocateNvramFullPathFileName) == 0)
	{
		//empty file name
		return KAL_FALSE;
	}

	//create file
	file_handle = FS_Open((kal_uint16*)gGPSLocateNvramFullPathFileName, FS_CREATE | FS_READ_WRITE);
	if (file_handle < FS_NO_ERROR)
	{
		//fail to create
		return KAL_FALSE;
	}
	//load default data
	result = GPSLocateNvramLoadDefault(file_handle, KAL_TRUE);
	//close the file
	FS_Close(file_handle);
	return result;
}

static kal_bool GPSLocateNvramLoadDefault(FS_HANDLE file_handle, kal_bool newCreate)
{
	int result;
	unsigned int len;
	GPSLocateNvramDataStruct_t tmpCache;
	char default_passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN];
	int i;
	unsigned char* pBuff;

	if (newCreate == KAL_TRUE)
	{
		memset(&tmpCache, 0x00, sizeof(GPSLocateNvramDataStruct_t));
	}
	else
	{
		//read from the original file
		FS_Seek(file_handle, 0, FS_FILE_BEGIN);
		result = FS_Read(file_handle, &tmpCache, sizeof(GPSLocateNvramDataStruct_t), &len);
		if (result != FS_NO_ERROR || len != sizeof(GPSLocateNvramDataStruct_t))
		{
			//read fail
			return KAL_FALSE;
		}
	}

	//set version number
	tmpCache.Buff[0] = GPSLOCATE_NVRAM_VER_NO;
	//set default passord to all zero(ascii '0')
	for (i = 0; i < GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN-1; i++)
	{
		default_passwd[i] = GPSLOCATE_NVRAMDATA_DEFAULT_PWD_CHAR;
	}
	default_passwd[GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN-1] = 0x00;
	//find service password space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_SERVICEPASSWORD);
	//set service password
	memcpy(pBuff, default_passwd, GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN);
	//find user password space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_USERPASSWORD);
	//set user password one by one
	for (i = 0; i < GPSLOCATE_USER_PASSWORD_MAX; i++)
	{
		memcpy(pBuff, default_passwd, GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN);
		pBuff += GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN;
	}
	//find service number space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_SERVICENUMBER);
	//set service number to null
	memset(pBuff, 0, GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN);
	//find user number space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_USERNUMBER);
	//set user number to null one by one
	for (i = 0; i < GPSLOCATE_USER_NUMBER_MAX; i++)
	{
		memset(pBuff, 0, GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN);
		pBuff += GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN;
	}
	//find timing number space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_TIMINGNUMBER);
	//set timing number to null
	memset(pBuff, 0, GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN);

	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_BSNUMBER)] = 1;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_SETTINGS)] = GPSLOCATE_NVRAMDATA_DEFAULT_TMSETTING;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPSONSETTING)] = 0;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_HFREEONSETTING)] = 0;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPRSUPLOADSETTING)] = 0;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_DEFENCEONSETTING)] = 0;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_MTCALLPROFILE)] = 0;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_SOSPHONECALL)] = 1;

	//find gprs user space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPRSUSER);
	//set service number to "PS"
	strcpy((char *)pBuff, "V300Q");
	//find gprs password space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPRSPASSWORD);
	//set gprs password
	memcpy(pBuff, default_passwd, GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN);
	//find server address space in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_SERVERADDR);
	//set server address to zero
	memset(pBuff, 0, sizeof(GPSLocateServerAddr_t));
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_POSMONITORONFF)] = 0;

	//find fixed position in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_FIXEDPOSITION);
	//set fixed position to zero
	memset(pBuff, 0, sizeof(GPS_PostionRange_t));

	if (newCreate == KAL_TRUE)
	{
		//load default value for the follwoing item only when new creating
		tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_BEARERMODE)] = GPSLOCATE_NVRAMDATA_DEFAULT_BEARERMODE;
	}

	//find gprs apn in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPRSAPN);
	strcpy((char *)pBuff, "CMNET");
	//find gprs apn user in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPRSAPNUSER);
	memset(pBuff, 0, GPS_NVRAM_RECSIZE_GPRSAPNUSER);
	//find gprs apn password in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_GPRSAPNPWD);
	memset(pBuff, 0, GPS_NVRAM_RECSIZE_GPRSAPNPWD);

	//find itrack front/rear msg in the data structure
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_ITRACKFRONTMSG);
	strcpy((char *)pBuff, "");
	pBuff = tmpCache.Buff + GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_ITRACKREARMSG);
	strcpy((char *)pBuff, "");

	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_VIBSENSOR_POLLINGINTERVAL)] = GPSLOCATE_VIBSENSOR_POLLINGINTERVAL_DEFAULT;
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_VIBSENSOR_SILENCETHRESHOLD)] = GPSLOCATE_VIBSENSOR_SILENCETHRESHOLD_DEFAULT;
	//set end sign to 0x55('U')
	tmpCache.Buff[GPSLocateNvramGetRecOffset(GPS_NVRAM_RECID_ENDSIGN)] = 0x55;

	//write default data to the file
	FS_Seek(file_handle, 0, FS_FILE_BEGIN);
	result = FS_Write(file_handle, &tmpCache, sizeof(GPSLocateNvramDataStruct_t), &len);
	if (result != FS_NO_ERROR || len != sizeof(GPSLocateNvramDataStruct_t))
	{
		return KAL_FALSE;
	}
	//load default data to the cache buffer
	memcpy(&(gGPSLocateNvramDataCache.Cache), &tmpCache, sizeof(GPSLocateNvramDataStruct_t));
	gGPSLocateNvramDataCache.Valid = KAL_TRUE;
	return KAL_TRUE;
}

static kal_bool GPSLocateNvramLoadData(void)
{
	FS_HANDLE file_handle;
	int result;
	unsigned int len;
	GPSLocateNvramDataStruct_t tmpCache;

	if (app_ucs2_strlen((const kal_int8*)gGPSLocateNvramFullPathFileName) == 0)
	{
		//empty file name
		return KAL_FALSE;
	}
	//open file
	file_handle = FS_Open(gGPSLocateNvramFullPathFileName, FS_READ_ONLY);
	if (file_handle < FS_NO_ERROR)
	{
		//fail to open
		return KAL_FALSE;
	}
	//read
	result = FS_Read(file_handle, &tmpCache, sizeof(GPSLocateNvramDataStruct_t), &len);
	FS_Close(file_handle);
	if (result != FS_NO_ERROR || len != sizeof(GPSLocateNvramDataStruct_t))
	{
		//read fail
		return KAL_FALSE;
	}
	//copy data to global cache
	memcpy(&(gGPSLocateNvramDataCache.Cache), &tmpCache, sizeof(GPSLocateNvramDataStruct_t));
	gGPSLocateNvramDataCache.Valid = KAL_TRUE;
	return KAL_TRUE;
}

static kal_bool GPSLocateNvramStoreData(void)
{
	FS_HANDLE file_handle;
	int result;
	unsigned int len;

	if (app_ucs2_strlen((const kal_int8*)gGPSLocateNvramFullPathFileName) == 0)
	{
		//empty file name
		return KAL_FALSE;
	}
	//open file
	file_handle = FS_Open(gGPSLocateNvramFullPathFileName, FS_READ_WRITE);
	if (file_handle < FS_NO_ERROR)
	{
		//fail to open
		return KAL_FALSE;
	}
	//write
	result = FS_Write(file_handle, &(gGPSLocateNvramDataCache.Cache), sizeof(GPSLocateNvramDataStruct_t), &len);
	FS_Close(file_handle);
	if (result != FS_NO_ERROR || len != sizeof(GPSLocateNvramDataStruct_t))
	{
		return KAL_FALSE;
	}
	else
	{
		return KAL_TRUE;
	}
}

static unsigned short GPSLocateNvramGetRecOffset(GPSLocateNvramRecId_t RecId)
{
	unsigned short Offset = GPSLOCATE_NVRAM_VER_NO_SIZE;
	int i;
	
	for (i=0; i<RecId; i++)
	{
		Offset += GPSLOCATE_REC_SIZE[i];
	}
	return Offset;
}

/******************************************************************************
* Function:
*	GPSLocateNvramReadRecord
*
* Usage:
*	Read one record from the storage (NVRAM or FS)
*
* Parameters:
*	RecId - Record ID
*	pBuff - Pointer to the caller's memory buffer. On a successful request, the record data
*					 is copied to the buffer.
*	BuffLen - the buffer length
*
* Return:
*	KAL_TRUE - successfully
*	KAL_FALSE - failed
******************************************************************************/
kal_bool GPSLocateNvramReadRecord(
	GPSLocateNvramRecId_t RecId,
	void* pBuff,
	unsigned short BuffLen
	)
{
	kal_bool bRet = KAL_TRUE;
	
	//check parameters
	if (RecId >= GPS_NVRAM_RECID_Total || 
		pBuff == NULL ||
		BuffLen < GPSLOCATE_REC_SIZE[RecId])
	{
		return KAL_FALSE;
	}
	ASSERT((gGPSLocateNvramSem!=NULL));
	//take the semaphore
	kal_take_sem(gGPSLocateNvramSem, KAL_INFINITE_WAIT);
	if (gGPSLocateNvramDataCache.Valid == KAL_TRUE)
	{
		//read from cache
		memcpy(pBuff, 
			&(gGPSLocateNvramDataCache.Cache.Buff[GPSLocateNvramGetRecOffset(RecId)]), 
			GPSLOCATE_REC_SIZE[RecId]);
		bRet = KAL_TRUE;
		//return KAL_TRUE;
		goto _RelSem_Return_;
	}
	//read the file
	if (GPSLocateNvramLoadData() == KAL_FALSE)
	{
		bRet = KAL_FALSE;
		//return KAL_FALSE;
		goto _RelSem_Return_;
	}
	else
	{
		//load successfully, read from cache
		memcpy(pBuff, 
			&(gGPSLocateNvramDataCache.Cache.Buff[GPSLocateNvramGetRecOffset(RecId)]), 
			GPSLOCATE_REC_SIZE[RecId]);
		bRet = KAL_TRUE;
		//return KAL_TRUE;
		goto _RelSem_Return_;
	}
_RelSem_Return_:
	//release semaphore
	kal_give_sem(gGPSLocateNvramSem);
	return bRet;
}

/******************************************************************************
* Function:
*	GPSLocateNvramWriteRecord
*
* Usage:
*	Write one record to the storage (NVRAM or FS)
*
* Parameters:
*	RecId - Record ID
*	pBuff - Pointer to the caller's memory buffer. On a successful request, the record data
*					 is written to the storage.
*	BuffLen - the buffer length
*
* Return:
*	KAL_TRUE - successfully
*	KAL_FALSE - failed
******************************************************************************/
kal_bool GPSLocateNvramWriteRecord(
	GPSLocateNvramRecId_t RecId,
	const void* pBuff,
	unsigned short BuffLen
	)
{
	//check parameters
	if (RecId >= GPS_NVRAM_RECID_Total || 
		pBuff == NULL ||
		BuffLen < GPSLOCATE_REC_SIZE[RecId])
	{
		return KAL_FALSE;
	}

	ASSERT((gGPSLocateNvramSem!=NULL));
	//take the semaphore
	kal_take_sem(gGPSLocateNvramSem, KAL_INFINITE_WAIT);
	//backup the cache data
	memcpy(&CacheBackup, &(gGPSLocateNvramDataCache.Cache), sizeof(GPSLocateNvramDataStruct_t));
	if (gGPSLocateNvramDataCache.Valid == KAL_FALSE)
	{
		//the cache data is invalid, clear it firstly
		memset(&(gGPSLocateNvramDataCache.Cache), 0x00, sizeof(GPSLocateNvramDataStruct_t));
	}
	//write to cache
	memcpy(&(gGPSLocateNvramDataCache.Cache.Buff[GPSLocateNvramGetRecOffset(RecId)]),
			pBuff, GPSLOCATE_REC_SIZE[RecId]);
	gGPSLocateNvramDataCache.Changed = KAL_TRUE;
	//release semaphore
	kal_give_sem(gGPSLocateNvramSem);

	if (!gps_nvramstore_timer)
	{
		gps_nvramstore_timer = GPSAppTimer_Create(GENERAL_GPSAPP_TIMERID,
								GPSLocateNvramRepeatHandler,
								KAL_TICKS_1_MIN,
								KAL_TICKS_1_MIN,
								KAL_TRUE);
	}

	return KAL_TRUE;
}

void GPSLocateNvramWrData2FS(void)
{
	if (!gGPSLocateNvramDataCache.Changed)
		return;
		
	//write to file
	if (GPSLocateNvramStoreData())
	{
		//set cache valid
		gGPSLocateNvramDataCache.Valid = KAL_TRUE;
		gGPSLocateNvramDataCache.Changed = KAL_FALSE;
	}
}


void GPSLocateNvramRepeatHandler(GPSAppTimerID_t Id)
{
    ilm_struct *send_ilm;
	kal_uint16 src_mod;

	src_mod = stack_int_get_active_module_id();
	send_ilm = allocate_ilm(src_mod);
    send_ilm->src_mod_id = src_mod;
#ifdef GPS_NVRAM_TASK
    send_ilm->dest_mod_id = MOD_GPS_NVRAM;
#else
    send_ilm->dest_mod_id = MOD_GPS_APP_TASK;
#endif
    send_ilm->msg_id = MSG_ID_GPSLOCATE_NVRAMSTORE_IND;
    send_ilm->local_para_ptr = NULL;

    msg_send_ext_queue(send_ilm);
}

#ifdef GPS_NVRAM_TASK
static void gps_nvram_main(task_entry_struct * task_entry_ptr);
static kal_bool gps_nvram_init(task_indx_type task_indx);
static kal_bool gps_nvram_reset(task_indx_type task_index);

static void gps_nvram_wr2fs_ind_hdlr(ilm_struct *ilm_ptr);

/*************************************************************************
* FUNCTION
*  gps_nvram_create
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
kal_bool gps_nvram_create(comptask_handler_struct **handle)
{
   static const comptask_handler_struct gps_nvram_handler_info =
   {
      gps_nvram_main,  /* task entry function */
      gps_nvram_init,  /* task initialization function */
      NULL,  /* task configuration function */
      gps_nvram_reset,  /* task reset handler */
      NULL,  /* task termination handler */
   };

   *handle = (comptask_handler_struct *)&gps_nvram_handler_info;

   return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_nvram_main
* DESCRIPTION
*   This function is the main function of GPS NVRAM task
* PARAMETERS
*   task_entry_ptr  IN   taks entry of GPS NVRAM
* RETURNS
*   None.
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static void gps_nvram_main(task_entry_struct * task_entry_ptr)
{
   ilm_struct current_ilm;

   while ( 1 ) {
      receive_msg_ext_q(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);

      switch (current_ilm.msg_id) {
	  case MSG_ID_GPSLOCATE_NVRAMSTORE_IND:
      	gps_nvram_log("GPS NVRAM receive msg: MSG_ID_GPSLOCATE_NVRAMSTORE_IND");
		gps_nvram_wr2fs_ind_hdlr(&current_ilm);
		break;

	//release semaphore
	kal_give_sem(gGPSLocateNvramSem);

      default:
         break;
      }
      free_ilm(&current_ilm);
   }
}

/*****************************************************************************
* FUNCTION
*   gps_nvram_init
* DESCRIPTION
*   Init function if GPS NVRAM task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static kal_bool gps_nvram_init(task_indx_type task_indx)
{
    /* Do task's initialization here.
     * Notice that: shouldn't execute modules reset handler since 
     * stack_task_reset() will do. */

    return KAL_TRUE;
}

/*****************************************************************************
* FUNCTION
*   gps_nvram_reset
* DESCRIPTION
*   Reset function if GPS NVRAM task
* PARAMETERS
*   task_index  IN   index of the taks
* RETURNS
*   TRUE if reset successfully
* GLOBALS AFFECTED
*   external_global
*****************************************************************************/
static kal_bool gps_nvram_reset(task_indx_type task_index)
{
   return KAL_TRUE;
}

void gps_nvram_wr2fs_ind_hdlr(ilm_struct *ilm_ptr)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/

    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
	GPSLocateNvramWrData2FS();
}
#endif

