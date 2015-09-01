/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_nema.c
 *
 * Project:
 * --------
 *   Maui_Software
 *
 * Description:
 * ------------
 *   This file implements gps locate module component task create function
 *
 * Author:
 * -------
 * Robbie Cao
 * -------
 *
 *============================================================================
 *             HISTORY
 *============================================================================
 * 2008-3-10 15:13 - Initial create.
 ****************************************************************************/

/*************************************************************************
 * Include Statements
 *************************************************************************/
#include "kal_release.h"
#include "app_buff_alloc.h"
#include "custom_util.h"
#include "gpio_sw.h"
#include "em_struct.h"

#include "gps_app_configurable_const.h"
#include "gps_app_unconfigurable_const.h"
#include "GPSLocateMsgDefs.h"
#include "gps_app_nema.h"
#include "gps_app_util.h"
#include "gps_app_data.h"
#include "parser.h"

/*************************************************************************
 * Macro defines
 *************************************************************************/
//#define GPS_APP_DEBUG
#ifdef GPS_APP_DEBUG
#define gps_app_log		trace_printf
#else
static void gps_app_log(kal_char *fmt, ...) { }
#endif

#define char2hex(c)		(c >= '0' && c <= '9') ? (c - '0') : 		\
						(c >= 'a' && c <= 'f') ? (c - 'a' + 0xA) :	\
						(c >= 'A' && c <= 'F') ? (c - 'A' + 0xA) :	\
						0


/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Local variables
 *************************************************************************/
const Keyword_t gps_frm_kwdList [] = 
{
	{	GPS_FRAME_ID_GPGGA,					"~s*~c$GPGGA~s*,"	},
	{	GPS_FRAME_ID_GPGLL,					"~s*~c$GPGLL~s*,"	},
	{	GPS_FRAME_ID_GSA,					"~s*~c$GPGSA~s*,"	},
	{	GPS_FRAME_ID_GSV,					"~s*~c$GPGSV~s*,"	},
	{	GPS_FRAME_ID_GPRMC,					"~s*~c$GPRMC~s*,"	},
	{	GPS_FRAME_ID_GPVTG,					"~s*~c$GPVTG~s*,"	},
	{   0xFF,								0					} 
} ;

const Keyword_t gps_gprmc_kwdList [] = 
{
	{	GPS_GPRMC_FLD_HEADER,     	"~s*~c$GPRMC~s*,"				},
	{	GPS_GPRMC_FLD_UTC_TIME,   	"~s*(~w+)~s*,"					},	// "~s*(~d{6,6}~c.~d{3,3})~s*,"
	{	GPS_GPRMC_FLD_STATUS,     	"~s*(~l/AV/)~s*,"				},
	{	GPS_GPRMC_FLD_LATITUDE,   	"~s*(~w*)~s*,"					},	// "~s*(~d{1,4}~c.~d{4,4})~s*,"
	{	GPS_GPRMC_FLD_LAT_NS,     	"~s*(~l/NS/?)~s*,"				},
	{	GPS_GPRMC_FLD_LONGITUDE,  	"~s*(~w*)~s*,"					},	// "~s*(~d{1,5}~c.~d{4,4})~s*,"
	{	GPS_GPRMC_FLD_LON_EW,     	"~s*(~l/EW/?)~s*,"				},
	{	GPS_GPRMC_FLD_RATE,       	"~s*(~w*)~s*,"					},
	{	GPS_GPRMC_FLD_DIRECTION,  	"~s*(~w*)~s*,"					},
	{	GPS_GPRMC_FLD_UTC_DATE,   	"~s*(~d{6,6})~s*,"				},
	{	GPS_GPRMC_FLD_INCLINATION,	"~s*(~w*)~s*,"					},
	{	GPS_GPRMC_FLD_INC_EW,     	"~s*(~l/EW/?)~s*,"				},
	{	GPS_GPRMC_FLD_MODE,       	"~s*(~l/ADEN/)~s*"				},
	{	GPS_GPRMC_FLD_CHECKSUM,		"~s*~c*(~l/0-9A-Fa-f/{2,2})~s*$"},
	{	0xFF,						0								}
} ;


/*************************************************************************
 * Global variables
 *************************************************************************/


/*************************************************************************
 * Function declaration
 *************************************************************************/
extern void gps_set_gprs_account_direct(unsigned char indx,
									  const char* apn,
									  const char* usr,
									  const char* psw);


/**
 * Function: GPS_APP_ParseGPRMCData
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_ParseGPRMCData(const char *src, GPS_GPRMC_RawData_t *gprmc_data)
{
	ParserMatch_t	match ;				//	pattern-matching results
	ParserToken_t	token [3] ;			//	pattern-matching tokens
	char			tknBuf [64] ;
	Result_t result = RESULT_ERROR;
	const Keyword_t* kwd = gps_gprmc_kwdList;

	if (src == NULL || gprmc_data == NULL)
		return RESULT_ERROR;
	
	gps_app_log("GPS_APP_ParseGPRMCData, src: %s", src);

	memset(&match, 0, sizeof(match));
	memset(token, 0, sizeof(token));
	memset(tknBuf, 0, sizeof(tknBuf));
	memset(gprmc_data, 0, sizeof(GPS_GPRMC_RawData_t));

	ParserInitMatch( src, &match ) ;

	// Header: $GPRMC
	result = ParserMatchPattern( kwd->pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("header not match");
		return RESULT_ERROR;
	}

	// UTC Time
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("utc time not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	strncpy(gprmc_data->utc_time, tknBuf, token[0].tknLen);
	gps_app_log("utc time: %s", gprmc_data->utc_time);

	// Locating status: A for valid, V for invalid
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("status(A/V) not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	gprmc_data->status = *tknBuf;
	gps_app_log("status: %c", gprmc_data->status);

	// Latitude degree
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("latitude not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	strncpy(gprmc_data->latitude, tknBuf, token[0].tknLen);
	gps_app_log("latitude: %s", gprmc_data->latitude);

	// Latitude North/South
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("latitude N/S not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	gprmc_data->ns = (*tknBuf) ? (*tknBuf) : ' ';	// set it as space(ascii 0x20 if null char)
	gps_app_log("latitude N/S: %c", gprmc_data->ns);

	// Longitude degree
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("longitude not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	strncpy(gprmc_data->longitude, tknBuf, token[0].tknLen);
	gps_app_log("longitude: %s", gprmc_data->longitude);

	// Longitude East/West
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("longitude E/W not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	gprmc_data->ew = (*tknBuf) ? (*tknBuf) : ' ';	// set it as space(ascii 0x20 if null char)
	gps_app_log("longitude E/W: %c", gprmc_data->ew);

	// if there's no latitude and longtitude in GPRMC frame, 
	// there will be no rate and direction, so ignore them
	if (strlen(gprmc_data->latitude) == 0 && 
		strlen(gprmc_data->longitude) == 0)
	{
		kwd += 2;
	}
	else
	{
		// Rate
		kwd++;
		result = ParserMatch ( kwd->pattern, &match, token );
		if (result != RESULT_OK)
		{
			gps_app_log("rate not match");
			return RESULT_ERROR;
		}
		ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
		strncpy(gprmc_data->rate, tknBuf, token[0].tknLen);
		gps_app_log("rate: %s", gprmc_data->rate);

		// Direction
		kwd++;
		result = ParserMatch ( kwd->pattern, &match, token );
		if (result != RESULT_OK)
		{
			gps_app_log("direction not match");
			return RESULT_ERROR;
		}
		ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
		strncpy(gprmc_data->direction, tknBuf, token[0].tknLen);
		gps_app_log("direction: %s", gprmc_data->direction);
	}

	// UTC Date
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("utc date not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	strncpy(gprmc_data->utc_date, tknBuf, token[0].tknLen);
	gps_app_log("utc date: %s", gprmc_data->utc_date);

	// Inclination
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("inclination not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	strncpy(gprmc_data->inclination, tknBuf, token[0].tknLen);
	gps_app_log("inclination: %s", gprmc_data->inclination);

	// Inclination East/West
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("inclination E/W not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	gprmc_data->inc_ew = (*tknBuf) ? (*tknBuf) : ' ';	// set it as space(ascii 0x20 if null char)
	gps_app_log("inclination E/W: %c", gprmc_data->inc_ew);

	// Mode
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("mode not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	gprmc_data->mode = *tknBuf;
	gps_app_log("mode: %c", gprmc_data->mode);

	// Checksum
	kwd++;
	result = ParserMatch ( kwd->pattern, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("checksum not match");
		return RESULT_ERROR;
	}
	ParserTknToStr ( &token[0], tknBuf, sizeof(tknBuf) ) ;
	strncpy(gprmc_data->checksum, tknBuf, token[0].tknLen);
	gps_app_log("checksum: %s", gprmc_data->checksum);

	return RESULT_OK;
}

/**
 * Function: GPS_APP_GPRMC_Raw2Packed
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_GPRMC_Raw2Packed(GPS_GPRMC_Packed_Struct_t *gprmc_pack, const GPS_GPRMC_RawData_t *gprmc_raw)
{
	ParserMatch_t	match ;				//	pattern-matching results
	ParserToken_t	token [4] ;			//	pattern-matching tokens
	char			tknBuf [16] ;
	kal_uint32		tmpVal ;

	const char *pattern = NULL;
	const char *src = NULL;
	Result_t result = RESULT_ERROR;

	ASSERT(gprmc_pack != NULL);

	gps_app_log("GPS_APP_GPRMC_Raw2Packed");

	memset(&match, 0, sizeof(match));
	memset(token, 0, sizeof(token));
	memset(tknBuf, 0, sizeof(tknBuf));
	memset((void *)gprmc_pack, 0, sizeof(GPS_GPRMC_Packed_Struct_t));

	gprmc_pack->header = 0xff;

	if (gprmc_raw == NULL)
	{
		return RESULT_ERROR;
	}

	// utc time
	pattern = "(~d{2,2})(~d{2,2})(~d{2,2})~c.(~d{3,3})";
	src = gprmc_raw->utc_time;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("utc time not match: %s", match.pLine);
		return RESULT_ERROR;
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->time_h = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->time_m = tmpVal;

		ParserTknToUInt ( &token[2], &tmpVal ) ;
		gprmc_pack->time_s = tmpVal;

		ParserTknToUInt ( &token[3], &tmpVal ) ;
		gprmc_pack->time_sf = tmpVal;

		gps_app_log("utc time(hhmmss.sss): %02d%02d%02d.%03d", 
					gprmc_pack->time_h, 
					gprmc_pack->time_m, 
					gprmc_pack->time_s, 
					gprmc_pack->time_sf);
	}

	// valid/invalid status
	gprmc_pack->valid = (gprmc_raw->status == 'A') ? 1 : 0;

	// latitude
	pattern = "(~d{2,2})(~d{2,2})~c.(~d{4,4})";
	src = gprmc_raw->latitude;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("latitude not match, %s", match.pLine);
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->latitude_d = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->latitude_c = tmpVal;

		ParserTknToUInt ( &token[2], &tmpVal ) ;
		gprmc_pack->latitude_cf = tmpVal;

		gps_app_log("latitude(ddcc.ffff): %02d%02d.%04d", 
					gprmc_pack->latitude_d, 
					gprmc_pack->latitude_c, 
					gprmc_pack->latitude_cf);
	}
	gprmc_pack->latitude_ns = (gprmc_raw->ns == 'S') ? 1 : 0;
	gps_app_log("latitude N/S: %d", gprmc_pack->latitude_ns);

	// longitude
	pattern = "(~d{1,3})(~d{2,2})~c.(~d{4,4})";
	src = gprmc_raw->longitude;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("longitude not match, %s", match.pLine);
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->longitude_d = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->longitude_c = tmpVal;

		ParserTknToUInt ( &token[2], &tmpVal ) ;
		gprmc_pack->longitude_cf = tmpVal;

		gps_app_log("longitude(dddcc.ffff): %03d%02d.%04d", 
					gprmc_pack->longitude_d, 
					gprmc_pack->longitude_c, 
					gprmc_pack->longitude_cf);
	}
	gprmc_pack->longitude_ew = (gprmc_raw->ew == 'W') ? 1 : 0;
	gps_app_log("longitude E/W: %d", gprmc_pack->longitude_ew);

	// rate
	pattern = "(~d{1,3})~c.(~d{2,2})";
	src = gprmc_raw->rate;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("rate not match, %s", match.pLine);
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->rate = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->rate_f = tmpVal;

		gps_app_log("rate(rrr.ff): %03d.%02d", 
					gprmc_pack->rate, 
					gprmc_pack->rate_f);
	}

	// direction
	pattern = "(~d{1,3})~c.(~d{2,2})";
	src = gprmc_raw->direction;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("direction not match, %s", match.pLine);
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->direction_d = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->direction_df = tmpVal;

		gps_app_log("direction(ddd.ff): %03d.%02d", 
					gprmc_pack->direction_d, 
					gprmc_pack->direction_df);
	}

	// utc date
	pattern = "(~d{2,2})(~d{2,2})(~d{2,2})";
	src = gprmc_raw->utc_date;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("utc date not match: %s", match.pLine);
		return RESULT_ERROR;
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->date_d = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->date_m = tmpVal;

		ParserTknToUInt ( &token[2], &tmpVal ) ;
		gprmc_pack->date_y = tmpVal;

		gps_app_log("date(ddmmyy): %02d%02d%02d", 
					gprmc_pack->date_d, 
					gprmc_pack->date_m, 
					gprmc_pack->date_y);
	}

	// inclination
	pattern = "(~d{1,3})~c.(~d{2,2})";
	src = gprmc_raw->inclination;
	ParserInitMatch( src, &match ) ;
	result = ParserMatchPattern( pattern, src, &match, token );
	if (result != RESULT_OK)
	{
		gps_app_log("inclination not match, %s", match.pLine);
		gprmc_pack->inclination_d = 0;
		gprmc_pack->inclination_df = 0;
	}
	else
	{
		ParserTknToUInt ( &token[0], &tmpVal ) ;
		gprmc_pack->inclination_d = tmpVal;

		ParserTknToUInt ( &token[1], &tmpVal ) ;
		gprmc_pack->inclination_df = tmpVal;

		gps_app_log("inclination(ddd.ff): %03d.%02d", 
					gprmc_pack->inclination_d, 
					gprmc_pack->inclination_df);
	}
	gprmc_pack->inclination_ew = (gprmc_raw->inc_ew == 'W') ? 1 : 0;
	gps_app_log("inclination E/W: %d", gprmc_pack->inclination_ew);

	// mode
	switch (gprmc_raw->mode)
	{
	case 'A': gprmc_pack->mode = 0; break;
	case 'D': gprmc_pack->mode = 1; break;
	case 'E': gprmc_pack->mode = 2; break;
	case 'N': gprmc_pack->mode = 3; break;
	default:  gprmc_pack->mode = 0; break;
	}
	gps_app_log("mode: %d", gprmc_pack->mode);

	// checksum
	if (strlen(gprmc_raw->checksum) == 0)
	{
		gps_app_log("checksum not match");
		return RESULT_ERROR;
	}
	else
	{
		gprmc_pack->checksum =  char2hex(gprmc_raw->checksum[0]);
		gprmc_pack->checksum <<= 4;
		gprmc_pack->checksum |= char2hex(gprmc_raw->checksum[1]);
	}
	gps_app_log("checksum: %#02X", gprmc_pack->checksum);

	return RESULT_OK;
}

/**
 * Function: GPS_APP_GPRMC_Packed2Str
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_GPRMC_Packed2Str(char *dest, const GPS_GPRMC_Packed_Struct_t *gprmc_pack)
{
	char *pos = dest;
	char *pBuff;
	char tmpMode;
	kal_uint8 checksum = 0;

	ASSERT(dest != NULL && gprmc_pack != NULL);

	*pos = '\0';
	if (gprmc_pack->header != 0xff)
	{
		return RESULT_ERROR;
	}

	// leading sign
	sprintf(pos, "$GPRMC,");

	// utc time
	pos = dest + strlen(dest);
	sprintf(pos, "%02d%02d%02d.%03d,", 
			gprmc_pack->time_h, 
			gprmc_pack->time_m, 
			gprmc_pack->time_s, 
			gprmc_pack->time_sf);

	// valid/invalid status
	pos = dest + strlen(dest);
	sprintf(pos, "%c,", gprmc_pack->valid ? 'A' : 'V');

	// latitude
	pos = dest + strlen(dest);
	if (gprmc_pack->latitude_d || 
		gprmc_pack->latitude_c || 
		gprmc_pack->latitude_cf)
	{
		sprintf(pos, "%d%02d.%04d,%c,", 
				gprmc_pack->latitude_d, 
				gprmc_pack->latitude_c, 
				gprmc_pack->latitude_cf, 
				gprmc_pack->latitude_ns ? 'S' : 'N');
	}
	else
	{
		sprintf(pos, ",,");
	}

	// longitude
	pos = dest + strlen(dest);
	if (gprmc_pack->longitude_d || 
		gprmc_pack->longitude_c || 
		gprmc_pack->longitude_cf)
	{
		sprintf(pos, "%d%02d.%04d,%c,", 
				gprmc_pack->longitude_d, 
				gprmc_pack->longitude_c, 
				gprmc_pack->longitude_cf, 
				gprmc_pack->longitude_ew ? 'W' : 'E');
	}
	else
	{
		sprintf(pos, ",,");
	}

	// rate and direction
	// if there's no latitude and longitude, there will be no rate and direction
	if ((gprmc_pack->latitude_d || gprmc_pack->latitude_c || gprmc_pack->latitude_cf) &&
		(gprmc_pack->longitude_d || gprmc_pack->longitude_c || gprmc_pack->longitude_cf))
	{
		pos = dest + strlen(dest);
	#if 0
		sprintf(pos, "%d.%02d,%d.%02d,", 
				gprmc_pack->rate, 
				gprmc_pack->rate_f, 
				gprmc_pack->direction_d, 
				gprmc_pack->direction_df);
	#else
		if (gprmc_pack->direction_d || gprmc_pack->direction_df)
		{
		sprintf(pos, "%d.%02d,%d.%02d,", 
				gprmc_pack->rate, 
				gprmc_pack->rate_f, 
				gprmc_pack->direction_d, 
				gprmc_pack->direction_df);
		}
		else
		{
		sprintf(pos, ",,");
		}
	#endif
	}

	// utc date
	pos = dest + strlen(dest);
	sprintf(pos, "%02d%02d%02d,", 
			gprmc_pack->date_d, 
			gprmc_pack->date_m, 
			gprmc_pack->date_y);

	// inclination
	pos = dest + strlen(dest);
	if (gprmc_pack->inclination_d || 
		gprmc_pack->inclination_df)
	{
		sprintf(pos, "%d.%02d,%c,", 
				gprmc_pack->inclination_d, 
				gprmc_pack->inclination_df, 
				gprmc_pack->inclination_ew ? 'W' : 'E');
	}
	else
	{
		sprintf(pos, ",,");
	}

	// mode
	pos = dest + strlen(dest);
	switch (gprmc_pack->mode)
	{
	case 0:  tmpMode = 'A'; break;
	case 1:  tmpMode = 'D'; break;
	case 2:  tmpMode = 'E'; break;
	case 3:  tmpMode = 'N'; break;
	default: tmpMode = 'N'; break;
	}
	sprintf(pos, "%c", tmpMode);

	// checksum
	pos = dest + strlen(dest);
	pBuff = dest + 1;	// dest point to "$GPRMC...X", 
						// pBuff point to "GPRMC....X"
						// X is A/D/E/N
	checksum = GPS_APP_Checksum8((kal_uint8 *)pBuff, strlen(pBuff));
	gps_app_log("%s checksum: 0X%02X", pBuff, checksum);
	gps_app_log("Raw checksum: 0X%02X", gprmc_pack->checksum);
	sprintf(pos, "*%02X\r\n", checksum);

	gps_app_log("Packed2Str: %s", dest);

	return RESULT_OK;
}

/**
 * Function: GPS_APP_CheckGPRMCData
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_bool GPS_APP_CheckGPRMCData(const GPS_GPRMC_RawData_t *gprmc_data)
{
	if (gprmc_data->status != 'A')
		return KAL_FALSE;

	return KAL_TRUE;
}

