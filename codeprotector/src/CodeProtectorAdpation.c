/******************************************************************************
* File name:
*	CodeProtectorAdpation.c
*
* Description:
*	This module implement I2C and other platform related functions for FS8816 core library.
*	FS8816 core library will invoke these functions to communicate with FS8816 chip and issue
*	handset software protection.
*
* Author:
*	Hongji Zhou
*
* Date:
*	2008.03.11
*
******************************************************************************/
#ifdef CODEPROTECTOR_FS8816_SUPPORT

#include "sccb_v2.h"

#include "FS8816_DataType.h"

#define FS8816_DBG
#undef FS8816_DBG


/******************************************************************************
* Function name:
*	I2c_Init
*
* Description:
*	Initiate I2C slave address and transfer mode
*
* Parameters:
*	SlvAdr - Slave device address
*
* Return:
*	None
*
******************************************************************************/
void I2c_Init(BYTE SlvAdr)
{
	sccb_config_struct config;

	config.slave_address = SlvAdr;
	config.get_handle_wait = KAL_FALSE;
	config.transaction_mode = SCCB_TRANSACTION_FAST_MODE;
	config.Fast_Mode_Speed = 400;
#ifdef I2C_DMA_ENABLED
	config.is_DMA_enabled = KAL_TRUE;
#endif //I2C_DMA_ENABLED
	i2c_config(SCCB_OWNER_FS8816, &config);

#ifdef FS8816_DBG
	trace_printf("I2c_Init: SlvAdr = %02X", SlvAdr);
#endif //FS8816_DBG
}

/******************************************************************************
* Function name:
*	I2cSendByte
*
* Description:
*	Send a byte data.
*	This function just sends one byte, not send I2C Slave Address automatically.
*
* Parameters:
*	c - a byte data
*
* Return:
*	1 (TRUE) - success
*	0 (FALSE) - failed
*
******************************************************************************/
BOOL I2cSendByte(BYTE c)
{
#ifdef FS8816_DBG
	trace_printf("I2cSendByte: c = %02X", c);
#endif //FS8816_DBG

	return TRUE;
}

/******************************************************************************
* Function name:
*	I2c_SendStr
*
* Description:
*	Send multiple bytes data.
*	1. This function need send I2C Slave Address automatically before send data in s.
*	2. Please return 0 or 0x20 always, not return any other error number for it may be incur 
*	FS8816 core library operate abnormally.
*
* Parameters:
*	s - multibyte data to send
*	no - data length
*
* Return:
*	0 - success
*	0x20 - ACK detected error
*
******************************************************************************/
BYTE I2c_SendStr(BYTE *s,UINT16 no)
{
	SCCB_TRANSACTION_RESULT res = SCCB_TRANSACTION_FAIL;

#ifdef I2C_DMA_ENABLED
	res = i2c_write(SCCB_OWNER_FS8816, s, no);
#else //I2C_DMA_ENABLED
	while (no/8)
	{
		res = i2c_write(SCCB_OWNER_FS8816, s, 8);
		if (res == SCCB_TRANSACTION_FAIL)
		{
			return 0x20;
		}
		s += 8;
		no -= 8;
	}
	if (no > 0)
	{
		res = i2c_write(SCCB_OWNER_FS8816, s, no);
		if (res == SCCB_TRANSACTION_FAIL)
		{
			return 0x20;
		}
	}
#endif //I2C_DMA_ENABLED	

#ifdef FS8816_DBG
{
	unsigned char i;
	char tmp[128];

	memset(tmp, 0, 128);
	for (i=0; i<no && i < 128/3; i++)
	{
		sprintf(&tmp[3*i], "%02X ", s[i]);
	}
	trace_printf("I2c_SendStr(%d), %d: %s", no, res, tmp);
}
#endif //FS8816_DBG

	return (res == SCCB_TRANSACTION_COMPLETE ? 0x00 : 0x20);
}

/******************************************************************************
* Function name:
*	I2c_RcvStr
*
* Description:
*	Receive multiple bytes data.
*	1. This function need send I2C Slave Address automatically before receive data .
*	2. Please return 0 or 0x20 always, not return any other error number for it may be incur 
*	FS8816 core library operate abnormally.
*
* Parameters:
*	s - Data Array pointer to store data
*	Len - data length
*
* Return:
*	0 - success
*	0x20 - ACK detected error
*
******************************************************************************/
BYTE I2c_RcvStr(BYTE *s,UINT16 Len)
{
	SCCB_TRANSACTION_RESULT res = SCCB_TRANSACTION_FAIL;

#ifdef I2C_DMA_ENABLED
	res = i2c_read(SCCB_OWNER_FS8816, s, Len);
#else //I2C_DMA_ENABLED
	while (Len/8)
	{
		res = i2c_read(SCCB_OWNER_FS8816, s, 8);
		if (res == SCCB_TRANSACTION_FAIL)
		{
			return 0x20;
		}
		s += 8;
		Len -= 8;
	}
	if (Len > 0)
	{
		res = i2c_read(SCCB_OWNER_FS8816, s, Len);
		if (res == SCCB_TRANSACTION_FAIL)
		{
			return 0x20;
		}
	}
#endif //I2C_DMA_ENABLED	

#ifdef FS8816_DBG
{
	unsigned char i;
	char tmp[128];

	memset(tmp, 0, 128);
	for (i=0; i<Len && i < 128/3; i++)
	{
		sprintf(&tmp[3*i], "%02X ", s[i]);
	}
	trace_printf("I2c_RcvStr(%d), %d: %s", Len, res, tmp);
}
#endif //FS8816_DBG

	return (res == SCCB_TRANSACTION_COMPLETE ? 0x00 : 0x20);
}

/******************************************************************************
* Function name:
*	ReadCode
*
* Description:
*	Read code from memory or flash.
*	1. This function need send I2C Slave Address automatically before receive data .
*	2. Please return 0 or 0x20 always, not return any other error number for it may be incur 
*	FS8816 core library operate abnormally.
*
* Parameters:
*	pBuff - Data Array pointer to store the data
*	StAdd - The pointer to Start Address of code range
*	Len - How much data need be read
*
* Return:
*	None
*
******************************************************************************/
void ReadCode(BYTE *pBuff , BYTE* StAdd, UINT32 Len)
{
#ifdef FS8816_DBG
	trace_printf("ReadCode: StAdd=%X, Len=%d", (UINT32)StAdd, Len);
#endif //FS8816_DBG
}

/******************************************************************************
* Function name:
*	Delay_1ms
*
* Description:
*	Delay 1 millisecond.
*
* Parameters:
*	cnt - How much time should be delayed
*
* Return:
*	None
*
******************************************************************************/
void Delay_1ms(BYTE cnt)
{
	while (cnt--);
}

#endif //CODEPROTECTOR_FS8816_SUPPORT

