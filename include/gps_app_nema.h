/******************************************************************************
 * File name: gps_app_nema.h
 * Author: Robbie Cao
 * Date: 2008-3-11 10:40
 ******************************************************************************/

#ifndef _GPS_APP_NEMA_H
#define _GPS_APP_NEMA_H

#include "kal_release.h"

#include "gps_app_unconfigurable_const.h"
#include "gps_app_configurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "parser.h"

/*************************************************************************
 * GPS frame related data type, macro and function prototype
 *************************************************************************/

/**
 * GPS frame data field max length
 */
#define GPS_UTC_TIME_MAX_LEN	11
#define GPS_UTC_DATE_MAX_LEN	7
#define GPS_LATITUDE_MAX_LEN	10
#define GPS_LONGITUDE_MAX_LEN	11
#define GPS_RATE_MAX_LEN		7
#define GPS_DIRECTION_MAX_LEN	7
#define GPS_INCLINATION_MAX_LEN	7
#define GPS_CHECKSUM_MAX_LEN	3

/**
 * GPS frame Id
 */
typedef enum
{
	GPS_FRAME_ID_GPGGA,
	GPS_FRAME_ID_GPGLL,
	GPS_FRAME_ID_GSA,
	GPS_FRAME_ID_GSV,
	GPS_FRAME_ID_GPRMC,
	GPS_FRAME_ID_GPVTG,

	GPS_FRAME_ID_TOTAL
} GPS_Frame_ID_t;

/**
 * GPS GPRMC frame field
 */
typedef enum
{
	GPS_GPRMC_FLD_HEADER,
	GPS_GPRMC_FLD_UTC_TIME,
	GPS_GPRMC_FLD_STATUS,
	GPS_GPRMC_FLD_LATITUDE,
	GPS_GPRMC_FLD_LAT_NS,
	GPS_GPRMC_FLD_LONGITUDE,
	GPS_GPRMC_FLD_LON_EW,
	GPS_GPRMC_FLD_RATE,
	GPS_GPRMC_FLD_DIRECTION,
	GPS_GPRMC_FLD_UTC_DATE,
	GPS_GPRMC_FLD_INCLINATION,
	GPS_GPRMC_FLD_INC_EW,
	GPS_GPRMC_FLD_MODE,
	GPS_GPRMC_FLD_CHECKSUM,

	GPS_GPRMC_FLD_TOTAL
} GPS_GPRMC_Field_t;

/**
 * GPS GPRMC frame raw data struct
 */
typedef struct
{
	char utc_time[GPS_UTC_TIME_MAX_LEN];
	char status;
	char latitude[GPS_LATITUDE_MAX_LEN];
	char ns;
	char longitude[GPS_LONGITUDE_MAX_LEN];
	char ew;
	char rate[GPS_RATE_MAX_LEN];
	char direction[GPS_DIRECTION_MAX_LEN];
	char utc_date[GPS_UTC_DATE_MAX_LEN];
	char inclination[GPS_INCLINATION_MAX_LEN];
	char inc_ew;
	char mode;
	char checksum[GPS_CHECKSUM_MAX_LEN];
} GPS_GPRMC_RawData_t;

/**
 * GPS GPRMC frame packed data struct
 */
typedef __packed struct
{
	// header - 8 bits
	unsigned int header 			: 8;	// 0xff

	// leading sign '$'
	// NEMA type: "GPRMC"

	// utc time - 17 bits
	// hhmmss
	unsigned int time_h 			: 5;	// 0~23
	unsigned int time_m 			: 6;	// 0~59
	unsigned int time_s 			: 6;	// 0~59
	unsigned int time_sf 			: 10;	// 0~999

	// valid/invalid status - 1 bit
	// V/A
	unsigned int valid  			: 1;	// 0 - V, 1 - A

	// latitude - 28 bits
	// ddcc.ffff,N/S
	unsigned int latitude_d 		: 7;	// 0~89
	unsigned int latitude_c 		: 6;	// 0~59
	unsigned int latitude_cf 		: 14;	// 0~9999
	unsigned int latitude_ns 		: 1;	// 0 - N, 1 - S

	// longitude - 29 bits
	// dddcc.ffff,E/W
	unsigned int longitude_d 		: 8;	// 0~179
	unsigned int longitude_c 		: 6;	// 0~59
	unsigned int longitude_cf 		: 14;	// 0~9999
	unsigned int longitude_ew 		: 1;	// 0 - E, 1 - W

	// rate - 17 bits
	// rrr.ff
	unsigned int rate 				: 10;	// 0~999
	unsigned int rate_f 			: 7;	// 0~99

	// direction - 16 bits
	// ddd.ff
	unsigned int direction_d 		: 9;	// 0~359
	unsigned int direction_df 		: 7;	// 0~99

	// utc date - 16 bits
	// ddmmyy
	unsigned int date_d 			: 5;	// 0~31
	unsigned int date_m 			: 4;	// 0~12
	unsigned int date_y 			: 7;	// 0~99

	// inclination - 16 bits
	// ddd.ff,E/W
	unsigned int inclination_d 		: 8;	// 0~180
	unsigned int inclination_df 	: 7;	// 0~99
	unsigned int inclination_ew 	: 1;	// 0 - E, 1 - W

	// mode - 2 bits
	// A/D/E/N
	unsigned int mode 				: 2;	// (bin)00 - A, 01 - D, 10 - E, 11 - N

	// NEMA string end sign: '*'

	// checksum - 8 bits
	// cc(hex)
	unsigned int checksum 			: 8;	// 0~0xff

	// end
	/* 168 bits*/

	// current lac and cell id
	// fixme: temp put these info here
	unsigned int lac				: 16;	// lac
	unsigned int cid				: 16;	// cell id
	/* 32 bits */

	/* total 168+32 bits */
} GPS_GPRMC_Packed_Struct_t;

typedef struct
{
	// latitude
	// ddcc.ffff,N/S
	kal_uint8	latitude_d;		// 0~89
	kal_uint8	latitude_c;		// 0~59
	kal_uint16	latitude_cf;	// 0~9999
	kal_uint8	latitude_ns;	// 0 - N, 1 - S

	// longitude
	// dddcc.ffff,E/W
	kal_uint8	longitude_d;	// 0~179
	kal_uint8	longitude_c;	// 0~59
	kal_uint16	longitude_cf;	// 0~9999
	kal_uint8	longitude_ew;	// 0 - E, 1 - W

	kal_uint16	radius;			// 0~9999
	kal_uint16	radius_f;		// 0~9
} GPS_PostionRange_t;

/** Parse GPS frame into GPRMC data struct */
Result_t GPS_APP_ParseGPRMCData(const char *src, GPS_GPRMC_RawData_t *gprmc_data);

/** Convert GPS frame raw data to GPRMC packed data */
Result_t GPS_APP_GPRMC_Raw2Packed(GPS_GPRMC_Packed_Struct_t *gprmc_pack, const GPS_GPRMC_RawData_t *gprmc_raw);

/** Convert GPS packed data to string */
Result_t GPS_APP_GPRMC_Packed2Str(char *dest, const GPS_GPRMC_Packed_Struct_t *gprmc_pack);

/** Check GPRMC data valid or not */
kal_bool GPS_APP_CheckGPRMCData(const GPS_GPRMC_RawData_t *gprmc_data);

#endif /* _GPS_APP_NEMA_H */

