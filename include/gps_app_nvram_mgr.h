/******************************************************************************
* File name: 
*	gps_app_nvram_mgr.h
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
#ifndef _GPS_APP_NVRAM_MGR_H_
#define _GPS_APP_NVRAM_MGR_H_

#include "kal_non_specific_general_types.h"

#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app.h"

/*
*	Definition of record ID
*
* 	Add your new record ID in this enumeration.
*/
typedef enum
{
	//service number
	GPS_NVRAM_RECID_SERVICENUMBER,		// 0
	//service password
	GPS_NVRAM_RECID_SERVICEPASSWORD,	// 1
	//user number
	GPS_NVRAM_RECID_USERNUMBER,			// 2
	//user password
	GPS_NVRAM_RECID_USERPASSWORD,		// 3
	//timing number
	GPS_NVRAM_RECID_TIMINGNUMBER,		// 4
	//bs number
	GPS_NVRAM_RECID_BSNUMBER,			// 5
	//settings
	GPS_NVRAM_RECID_SETTINGS,			// 6
	//settings backup
	GPS_NVRAM_RECID_SETTINGSBACKUP,		// 7
	//bearer mode
	GPS_NVRAM_RECID_BEARERMODE,			// 8
	//server addr
	GPS_NVRAM_RECID_SERVERADDR,			// 9
	//gprs user
	GPS_NVRAM_RECID_GPRSUSER,			// 10
	//gprs password
	GPS_NVRAM_RECID_GPRSPASSWORD,		// 11
	//gprs upload settings
	GPS_NVRAM_RECID_GPRSUPLOADSETTING,	// 12
	//gps on/off setting
	GPS_NVRAM_RECID_GPSONSETTING,		// 13
	//handfree on/off
	GPS_NVRAM_RECID_HFREEONSETTING,		// 14
	//power on cycle count
	GPS_NVRAM_RECID_POWERONCOUNT,		// 15
	//defence on/off
	GPS_NVRAM_RECID_DEFENCEONSETTING,	// 16
	//vibration sensor status polling interval	(in 100ms)
	GPS_NVRAM_RECID_VIBSENSOR_POLLINGINTERVAL,
	//vibration sensor slience threshold		(in min)
	GPS_NVRAM_RECID_VIBSENSOR_SILENCETHRESHOLD,
	//mt call profile: normal/monitoring(silent)
	GPS_NVRAM_RECID_MTCALLPROFILE,
	//fixed position
	GPS_NVRAM_RECID_FIXEDPOSITION,
	//position monitor on/off
	GPS_NVRAM_RECID_POSMONITORONFF,
	//gprs apn
	GPS_NVRAM_RECID_GPRSAPN,
	//gprs apn user
	GPS_NVRAM_RECID_GPRSAPNUSER,
	//gprs apn password
	GPS_NVRAM_RECID_GPRSAPNPWD,
	//make phone call for sos or not
	GPS_NVRAM_RECID_SOSPHONECALL,
	//itrack front msg
	GPS_NVRAM_RECID_ITRACKFRONTMSG,
	//itrack rear msg
	GPS_NVRAM_RECID_ITRACKREARMSG,
	//gprs upload settings
	GPS_NVRAM_RECID_GPRSUPLOADSETTING2,
	//rate limit check
	GPS_NVRAM_RECID_RATELIMIT,

	/* Add new record ID before this line */

	//end sign
	GPS_NVRAM_RECID_ENDSIGN,			// xx

	GPS_NVRAM_RECID_Total
} GPSLocateNvramRecId_t;

/*
*	Definition of record size
*
*	Define your new record size following these existing samples.
*/
#define GPS_NVRAM_RECSIZE_SERVICENUMBER			\
	(GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_SERVICEPASSWORD		\
	(GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_USERNUMBER			\
	(GPSLOCATE_USER_NUMBER_MAX * GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_USERPASSWORD			\
	(GPSLOCATE_USER_PASSWORD_MAX * GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_TIMINGNUMBER			\
	(GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_BSNUMBER				\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_SETTINGS				\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_SETTINGSBACKUP		\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_BEARERMODE			\
	(sizeof(unsigned short))
#define GPS_NVRAM_RECSIZE_SERVERADDR			\
	(sizeof(GPSLocateServerAddr_t))
#define GPS_NVRAM_RECSIZE_GPRSUSER				\
	(GPSLOCATE_PRESAVED_NUMBER_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_GPRSPASSWORD			\
	(GPSLOCATE_PRESAVED_PASSWORD_BUFFER_LEN * sizeof(char))
#define GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING		\
	(sizeof(unsigned int))
#define GPS_NVRAM_RECSIZE_GPSONSETTING			\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_HFREEONSETTING		\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_POWERONCOUNT			\
	(sizeof(unsigned long))
#define GPS_NVRAM_RECSIZE_DEFENCEONSETTING		\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_VIBSENSOR_POLLINGINTERVAL		\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_VIBSENSOR_SILENCETHRESHOLD	\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_MTCALLPROFILE					\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_FIXEDPOSITION					\
	(sizeof(GPS_PostionRange_t))
#define GPS_NVRAM_RECSIZE_POSMONITORONFF				\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_GPRSAPN						\
	(sizeof(unsigned char) * GPSLOCATE_PRESAVED_APN_BUFFER_LEN)
#define GPS_NVRAM_RECSIZE_GPRSAPNUSER					\
	(sizeof(unsigned char) * GPSLOCATE_PRESAVED_APNUSER_BUFFER_LEN)
#define GPS_NVRAM_RECSIZE_GPRSAPNPWD					\
	(sizeof(unsigned char) * GPSLOCATE_PRESAVED_APNPWD_BUFFER_LEN)
#define GPS_NVRAM_RECSIZE_SOSPHONECALL					\
	(sizeof(unsigned char))
#define GPS_NVRAM_RECSIZE_ITRACKFRONTMSG				\
	(sizeof(unsigned char) * GPSLOCATE_PRESAVED_ITRACKMSG_LEN)
#define GPS_NVRAM_RECSIZE_ITRACKREARMSG					\
	(sizeof(unsigned char) * GPSLOCATE_PRESAVED_ITRACKMSG_LEN)
#define GPS_NVRAM_RECSIZE_GPRSUPLOADSETTING2			\
	(sizeof(unsigned int))
#define GPS_NVRAM_RECSIZE_RATELIMIT						\
	(sizeof(unsigned short))

#define GPS_NVRAM_RECSIZE_ENDSIGN						\
	(sizeof(unsigned char))
/*
*	API
*
*	Invoke these APIs to use this NVRAM sub-system
*/
kal_bool GPSLocateNvramInitDataFile(void);
kal_bool GPSLocateNvramReadRecord(
	GPSLocateNvramRecId_t RecId,
	void* pBuff,
	unsigned short BuffLen
	);
kal_bool GPSLocateNvramWriteRecord(
	GPSLocateNvramRecId_t RecId,
	const void* pBuff,
	unsigned short BuffLen
	);
void GPSLocateNvramWrData2FS(void);

#endif //_GPS_APP_NVRAM_MGR_H_
