/*
 * nt3h.h
 *
 *  Created on: 17.03.2021
 *      Author: DavidSchönfisch
 */

#ifndef _NTAG_H_
#define _NTAG_H_

#include <stdint.h>
#include <stddef.h>
#include "NTAG.h"
#include "fsl_i2c.h"

/**************************************************************************/
/* >> START: Symbols, Enums and Macros */

#define NTAG_DEFAULT_I2C_ADDRESS        0x55

//typedef struct {uint8_t Bytes[16];} NTAG_I2C_Block_t;

typedef uint32_t error_ntag_t;

enum ntag_errors_e{
	NTAG_NO_ERROR 					= 0x00U,
	NTAG_ERROR_I2C_COMMUNICATION	= 0x01U,
	NTAG_ERROR_MEMORY_SIZE			= 0x02U,
	NTAG_ERROR_NO_NDEF_FOUND		= 0x03U,
	NTAG_ERROR_NOT_YET_IMPLEMENTED	= 0xFEU,
	NTAG_ERROR_UNSPECIFIC			= 0xFFU
};



/* >> END: Symbols, Enums and Macros */



/**************************************************************************/
/* >> START: External Functions */
error_ntag_t inti_ntag(I2C_Type *base_p, i2c_master_config_t * masterConfig_p);

error_ntag_t ntag_change_local_i2c_address(uint8_t i2c_address);

uint8_t ntag_get_local_i2c_address(void);

error_ntag_t ntag_nfcs_i2c_rst(bool nfcsandrst);
error_ntag_t ntag_i2c_rst_with_doubleStart(void);

error_ntag_t ntag_change_device_i2c_address(uint8_t i2c_address);

error_ntag_t ntag_factory_resest_of_memory(void);


error_ntag_t ntag_get_16ByteBlock(uint8_t blockNo, uint8_t * valArray_p);

error_ntag_t ntag_get_sessionConfig(uint8_t configArray_7Bytes[]);

error_ntag_t ntag_write_userMemory(uint8_t * valArray, uint8_t length);

error_ntag_t ntag_read_userMemory(uint8_t * valArray, uint8_t length);

//ToDo:
//ndef_message_uint8_array_t * ntag_get_ndef_message(void);

error_ntag_t ntag_erase_userMemory(void);



/*For future Use:
 * error_ntag_t ntag_writeNdefMessage(uint8_t * valArray, uint8_t length);
 */




/* >> END: External Functions */

#endif /* NT3H_DEFS_H_ */
/** @}*/
