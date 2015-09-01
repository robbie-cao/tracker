
#ifndef _FS8816_APP_H_
#define _FS8816_APP_H_

#include "FS8816_DataType.h"

#ifndef IMPORT_LIB
  #define IMPORT_LIB 1
#endif

#if IMPORT_LIB 
  #define EXT extern
#else 
  #define EXT 
#endif


UCHAR InitFS8806Lib(BYTE I2cSlvAdr,BYTE *key ,BYTE Rand);
void SetDelayTime(UINT16 Response_d_time, UINT16 Write_d_time);
#if  COMPLETEVER
   EXT 	UCHAR Authentication(BYTE *MessageAdd, BYTE *isComplete, BYTE * flagMAC);
   EXT	UCHAR ResetChipOpr(void);
   EXT	UCHAR PowerDownChipOpr(void);
   EXT	void WakeFromPowerDownOpr(void);
   EXT	UCHAR WriteNVMOpr(BYTE Nvmoffset,BYTE len,BYTE * buffer);
   EXT	UCHAR ReadNVMOpr(BYTE Nvmoffset,BYTE len,BYTE * buffer);
   EXT    UCHAR Validation(void);
#endif

#if LIGHTPLUSVER 
   EXT	UCHAR WriteNVMOpr(BYTE Nvmoffset,BYTE len,BYTE * buffer);
   EXT	UCHAR ReadNVMOpr(BYTE Nvmoffset,BYTE len,BYTE * buffer);
   EXT  UCHAR Validation(void);
#endif

#if LIGHTPLUSVER | LIGHTVER
   EXT    UCHAR Validation();
#endif

#define FS8816API_SUCCESS	(0x00)

#endif //_FS8816_APP_H_
