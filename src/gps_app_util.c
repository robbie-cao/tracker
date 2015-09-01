/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gps_app_util.c
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
 * 2008-3-10 16:31 - Initial create.
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
#include "parser.h"
#include "gps_app_data.h"

/*************************************************************************
 * Macro defines
 *************************************************************************/
#define GPS_COMP_NUMBER_LEN_MAX			9

#ifndef max
#define max(a,b)		((a)>(b)?(a):(b))
#endif
#ifndef min
#define min(a,b)		((a)<(b)?(a):(b))
#endif

/*************************************************************************
 * Type defines
 *************************************************************************/


/*************************************************************************
 * Local variables
 *************************************************************************/


/*************************************************************************
 * Global variables
 *************************************************************************/


/*************************************************************************
 * Function declaration
 *************************************************************************/

/**
 * Function: GPS_APP_FindNumberInPresavdList
 *
 * Usage: Find phone number in presaved list 
 *
 * Parameters:
 *
 * Returns:
 *	0 			- not found
 *	non-zero	- found, index in the presaved list, index based 1
 *
 */
kal_uint8 GPS_APP_FindNumberInPresavdList(const GPSLocatePhoneNumber_t *pNumber)
{
	int i;

	for (i = 0; i < GPS_PRESAVED_NUMBER_TOTAL; i++)
	{
	#if 0
		if (!strcmp((char *)pNumber->Number, gps_usr_numbers[i].number))
		{
			return (i + 1);
		}
	#else
		int j, k, m;
		kal_bool found = KAL_TRUE;

		j = strlen((char *)pNumber->Number);
		k = strlen(gps_usr_numbers[i].number);
		m = min(GPS_COMP_NUMBER_LEN_MAX, min(j, k));

		if (m <= 0) continue;

		while (m > 0)
		{
			if (pNumber->Number[j-1] != gps_usr_numbers[i].number[k-1])
			{
				found = KAL_FALSE;
				break;
			}
			m--;
			j--;
			k--;
		}

		if (found == KAL_TRUE)
		{
			return (i + 1);
		}
	#endif
	}

	return 0;
}

/**
 * Function: GPS_APP_CheckNumberPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_CheckNumberPassword(const GPSLocatePhoneNumber_t *pNumber, const char *passwd)
{
#ifdef GPS_SEPARATE_USER_PASSWOR
	kal_uint8 found = 0;

	found = GPS_APP_FindNumberInPresavdList(pNumber);
	if (found > 0)
	{
		if (!strcmp(passwd, gps_usr_numbers[found-1].passwd))
			return RESULT_OK;
	}
#else
	if (!strcmp(passwd, gps_usr_passwd))
		return RESULT_OK;
#endif

	return RESULT_ERROR;
}

/**
 * Function: GPS_APP_CheckServicePassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_CheckServicePassword(const GPSLocatePhoneNumber_t *pNumber, const char *service_passwd)
{
#ifdef GPS_SEPARATE_USER_PASSWOR
	kal_uint8 found = 0;

	found = GPS_APP_FindNumberInPresavdList(pNumber);
	if (!found)
	{
		return RESULT_ERROR;
	}
#endif

	if (!strcmp(service_passwd, gps_sc_number.passwd))
	{
		return RESULT_OK;
	}

	return RESULT_ERROR;
}

/**
 * Function: GPS_APP_CheckGprsPassword
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
Result_t GPS_APP_CheckGprsPassword(const GPSLocatePhoneNumber_t *pNumber, const char *passwd)
{
#ifdef GPS_SEPARATE_USER_PASSWOR
	kal_uint8 found = 0;

	found = GPS_APP_FindNumberInPresavdList(pNumber);
	if (!found)
	{
		return RESULT_ERROR;
	}
#endif

	if (!strcmp(passwd, gps_gprs_userpwd))
	{
		return RESULT_OK;
	}

	return RESULT_ERROR;
}

/**
 * Function: GPS_APP_GetNumberType
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_uint8 GPS_APP_GetNumberType(const char *pNumber)
{
	ASSERT(pNumber != NULL);

	if (pNumber[0] == GPS_INTERNATIONAL_CODE)
	{
		return GPS_INTERNA_TON_ISDN_NPI;
	}

	// Number start with country code should be treat as international ton,
	// Current only check number in China, should extend the check in future.
	if (pNumber[0] == '8' && pNumber[1] == '6')
	{
		return GPS_INTERNA_TON_ISDN_NPI;
	}

	return GPS_UNKNOWN_TON_ISDN_NPI;
}

/**
 * Function: GPS_APP_RmcDegree2Rad
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
double GPS_APP_RmcDegree2Rad(kal_uint8 d, kal_uint8 c, kal_uint16 cf)
{
	const double PI = 3.14159265;

	return PI * (d + c / 60.0 + cf / 600000.0) / 180.0;
}

/**
 * Function: GPS_APP_Checksum8
 *
 * Usage: 
 *
 * Parameters:
 *
 * Returns:
 *
 */
kal_uint8 GPS_APP_Checksum8(kal_uint8 *buff, kal_uint32 size)
{
	kal_uint8  check_sum = 0;
	kal_uint32 i ;

	ASSERT(buff != NULL);

	for (i = 0; i < size; i++)
	{
		check_sum ^= buff[i]; 
	}

	return check_sum ;
}

/**
 * Function: GPS_APP_StrReplace
 *
 * Usage: Replace the specify string in the line with new
 *
 * Parameters:
 *	A - original string line
 *	B - pattern to be replaced
 *	C - new string will change to
 *
 * Returns:
 *
 */
char* GPS_APP_StrReplace(char* A, char* B, char* C)
{
	int lenA, lenB, lenC;
	int i, j, k;
	int found;     //   bool
	char *pA, *pB, *pC, *pY, *p;
	const int MAX_REPLACE_COUNT = 128;   //At most 128 replacement
	char Y[MAX_REPLACE_COUNT];

	if (!A || !B || !C) return NULL;

	lenA = strlen(A);
	lenB = strlen(B);
	lenC = strlen(C);   
	  
	memset(Y, 0, sizeof(Y));

	if (lenA && lenB)
	{
		pA = A;
		pY = Y;
		while (*pA)
		{
			pB = B;
			if (*pA == *pB)
			{
				p = pA;   // p used for compare
				found = 1;
				while (*p && *pB)
				{
					if(*p != *pB)
					{
						found   =   0;
						break;
					}
					p++;
					pB++;
				}
				if (found)
				{
					// We find one! Replace from here!
					for(pC = C; *pC; pC++  )
					{
						*pY = *pC;
						pY++;
					}
					pA += lenB;
				}
			}
			*pY = *pA;
			pA++;
			pY++;
		}
	}
	*pY = '\0';
	strcpy(A, Y);

	return A;
}

