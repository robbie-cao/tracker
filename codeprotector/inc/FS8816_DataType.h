/***********************************************************************
 *
 *                            FameG, Inc.
 *                     Building59, 461 Hongcao Road,
 *                     Shanghai 200233,P.R.China
 *                      All rights reserved.
 *
***********************************************************************/

/*! \file 
    \brief Define DataType.
 
 *\par File Name	
 	 FS8816_DataType.h

 *\par Decription
 
 
 *\par History	
 	\par 
 		  ----Time----------Author-------------Update Content--
 	\par
           -2006-09-19------Megan--------------Initializing

*/

#ifndef _FS8816_DATATYPE_H_
#define _FS8816_DATATYPE_H_

/*  CPU Type define ***************/
#define MCS51          1
#define OTHER          2

#define CPU         OTHER

#if (CPU==MCS51)
	#define 	code 	code
	#define 	data 	data
#elif (CPU==OTHER)
  	#define	code 
  	#define	data 
  	#define	xdata
#endif
/*  CPU Type define end************/

/*  Data Type define  *************/
#ifndef TRUE 
	#define	TRUE	1
#endif
#ifndef FALSE
	#define	FALSE	0
#endif
#ifndef NULL
	#define	NULL	0
#endif
#ifndef false 
	#define	false 	0
#endif
#ifndef true
	#define	true 	1
#endif

#ifndef BOOL 
	#define	BOOL	char
#endif
#ifndef PVOID
	#define	PVOID	(void*)
#endif
#ifndef UCHAR
	#define	UCHAR	unsigned char
#endif

#ifndef SCHAR
	#define	SCHAR	char
#endif

#ifndef  INT16
	#define	INT16	signed short
#endif

#ifndef UNIT16
	#define	UINT16	unsigned short
#endif

#ifndef PUINT16
	#define	PUINT16	UINT16*
#endif

#ifndef INT32
	#define	INT32	long
#endif

#ifndef UINT32
	#define	UINT32	unsigned long
#endif
/* For DES algorithm   **/
#ifndef  BYTE
	#define	BYTE	unsigned char
#endif

/* For I2C **********/
#ifndef WORD
	#define	WORD	unsigned short
#endif

#ifndef DWORD
	#define	DWORD	unsigned long  
#endif
/*  Data Type define  end**********/


// use define for compile size and function set      
#define LIGHTVER     0
#define LIGHTPLUSVER   0
#define COMPLETEVER	   1

void Delay_1ms(BYTE cnt);	 
void ReadCode(BYTE *pBuff,  BYTE*StAdd, UINT32 Len);

#endif //_FS8816_DATATYPE_H_
