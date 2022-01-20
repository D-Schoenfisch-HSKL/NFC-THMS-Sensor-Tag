/*
 * NTAG.c
 *
 *  Created on: 24.03.2021
 *      Author: DavidSchönfisch
 */

#include <stdlib.h>
#include <string.h>

#include "fsl_i2c.h"
#include "NTAG.h"
#include "timer.h"
#include "ndef_parse.h"
#include "thms_2_ndef.h"

#include "board.h"

/*>>>------------------------------------------------------------*/
/* >> START: Symbols, Enums, Macros & Const Values */
/* Factory default value of memory block 0 */

#define NTAG_2K_VERSION	false

#if NTAG_2K_VERSION
	#define NTAG_SIZE_BYTE	0xEA
#else
	#define NTAG_SIZE_BYTE	0x6D
#endif

#define NDEF_START_SIGN						0x03
#define NDEF_END_SIGN						0xFE


#define NTAG_MEMORY_BLOCK_0_FACTORY_VALUE  { NTAG_DEFAULT_I2C_ADDRESS<<1, 0x00, 0x00, 0x00, \
                                      	  	 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00, \
											 0xE1, 0x10, 0x6D, 0x00 }  // Capability Container (CC) // 0x00, 0x00, 0x00, 0x00 } //
											// [1B, Address] [6B, UID] [3B, Internal] [2B, ??]
											// [4B, NDEF Definition (3rd Byte = Size Info) ]

#define NTAG_MEMORY_BLOCK_56_FACTORY_VALUE { 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0xFF }

#define NTAG_MEMORY_BLOCK_57_FACTORY_VALUE { 0x00, 0x00, 0x00, 0x00, \
											 0xFF, 0xFF, 0xFF, 0xFF, \
											 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00 }
//= 0x3A : Configuration Register: NC_REG | LAST_NDEF_BLOCK | SRAM_MIRROR_BLOCK | WDT_LS | WDT_MS | I"C_CLOCK_STR | REG_LOCK | RFU | 8* 0x00
#define NTAG_MEMORY_BLOCK_58_FACTORY_VALUE { 0x01, 0x00, 0xF8, 0x48, \
                                      	  	 0x08, 0x01, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00 }

#define NTAG_MEMORY_DEFAULT_NDEF_MESSAGE	{ 0x03, 0x0D, 0xD1, 0x01, \
											  0x09, 0x54, 0x02, 0x64, \
											  0x65, 0x44, 0x6F, 0x3A, \
											  0x30, 0x31, 0x3B, 0xFE } // NDEF Message to init


#define NTAG_MEMORY_BLOCK_OF_ZEROS 		   { 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00, \
											 0x00, 0x00, 0x00, 0x00 }

#define NTAG_USER_MEMORY_1_BLOCK_SIZE		56 								//0x38*16Byte = 888 Bytes
#define NTAG_USER_MEMORY_1_BLOCK(N) 			(N + 0x01)						//Beginnt bei 0; Abfrage?: ((N) <= (56) ? (N) : (a))
#define NTAG_USER_MEMORY_2_BLOCK_SIZE		64 								//64*16Byte = 1024 Bytes
#define NTAG_USER_MEMORY_2_BLOCK(N)			(N + 0x40)						//Beginnt bei 0; Abfrage?: ((N) <= (64) ? (N) : (a))
#if NTAG_2K_VERSION
	#define NTAG_USER_MEMORY_SIZE				56+64
#else
	#define NTAG_USER_MEMORY_SIZE				56 								//0x38*16Byte = 888 Bytes
#endif

/* CONFIG AND SESSION REGISTERS DEFINITIONS */
#define NTAG_CONFIG_REGISTERS_ADDRESS				0x3A
#define NTAG_SESSION_REGISTERS_ADDRESS				0xFE

typedef enum {
		CONFIG_REGISTER							= NTAG_CONFIG_REGISTERS_ADDRESS,
		SESSION_REGISTER						= NTAG_SESSION_REGISTERS_ADDRESS
}config_or_session_register_t;

typedef enum {
		NTAG_CSR_NC_REG							=	0x00,
		NTAG_CSR_LAST_NDEF_BLOCK 				=	0x01,
		NTAG_CSR_SRAM_MIRROR_BLOCK				=	0x02,
		NTAG_CSR_WDT_LS							=	0x03,
		NTAG_CSR_WDT_MS							=	0x04,
		NTAG_CSR__I2C_CLOCK_STR					=	0x05,
		NTAG_CSR_NS_REG							=	0x06,
		NTAG_CSR__RFU							=	0x07
} ntag_config_and_session_registers_t;


#define NTAG_CSR_NC_REG_NFCS_I2C_RST_ON_OFF_MASK	0b10000000
#define NTAG_CSR_NC_REG_NFCS_I2C_RST_ON				0b10000000
#define NTAG_CSR_NC_REG_NFCS_I2C_RST_OFF			0b00000000
#define NTAG_CSR_NC_REG_PTHRU_ON_OFF_MASK			0b01000000
#define NTAG_CSR_NC_REG_FD_OFF_MASK					0b00110000 //0b00 = Field Off; 0b01 = Field of or tag at HALT ; 0b10 = Field off or NDEF Last Page read ; 0xb11 FD
#define NTAG_CSR_NC_REG_FD_ON_MASK					0b00001100
#define NTAG_CSR_NC_SRAM_MIRROR_ON_OFF_MASK			0b00000010
#define NTAG_CSR_NC_TRANSFER_DIR_MASK				0b00000001 // False for I2C to NFC


typedef struct {
	uint8_t	i2c_address_local;
}ntag_config_t;


typedef uint8_t * array_16bytes_p_t;

/* >> END: Symbols, Enums, Macros & Const Values */

/*>>>------------------------------------------------------------*/
/* >> START: Local Variables */
static I2C_Type * i2c_type_base_p_m;
static i2c_master_config_t * i2c_master_config_p_m;

static ntag_config_t ntag_config_m;

/* >> END: Local Variables */


/*>>>------------------------------------------------------------*/
/* >> START: Prototypes (Internal Functions) */
// I2C communication (for Hardware Abstraction)
static error_ntag_t config_i2c(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p);
static error_ntag_t read_i2c(uint8_t * valArray_p, uint8_t length);
static error_ntag_t write_i2c(uint8_t * valArray_p, uint8_t length);
static error_ntag_t read_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length);
static error_ntag_t write_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length);
static void wait_ms(uint32_t delay_in_ms);
error_ntag_t ntag_write_session_or_config_register(config_or_session_register_t mega, ntag_config_and_session_registers_t rega, uint8_t mask, uint8_t regdata);
error_ntag_t ntag_read_session_or_config_register(config_or_session_register_t mega, ntag_config_and_session_registers_t rega, uint8_t * regdata_p);
/* >> END: Prototypes */


/*>>>------------------------------------------------------------*/
/* >> START: External Functions */
error_ntag_t inti_ntag(I2C_Type * base_p, i2c_master_config_t * masterConfig_p) {
	config_i2c(base_p, masterConfig_p);
	ntag_config_m.i2c_address_local = NTAG_DEFAULT_I2C_ADDRESS;
	//status_t error_indicator = NTAG_NO_ERROR;
	//uint8_t memoryBlock0Value[16] = NTAG_MEMORY_BLOCK_0_FACTORY_VALUE;
	//error_indicator = write_i2c_with_command((uint8_t) 0x00, memoryBlock0Value,16);
	//if (error_indicator) {return NTAG_ERROR_I2C_COMMUNICATION;}
	return NTAG_NO_ERROR;
}

error_ntag_t ntag_change_local_i2c_address(uint8_t i2c_address) {
	ntag_config_m.i2c_address_local = i2c_address;
	return NTAG_NO_ERROR;
}

uint8_t ntag_get_local_i2c_address() {
	return ntag_config_m.i2c_address_local;
}

error_ntag_t ntag_change_device_i2c_address(uint8_t i2c_address) {
	status_t i2c_error_indicator;
	uint8_t memoryBlock0Value[16] = NTAG_MEMORY_BLOCK_0_FACTORY_VALUE;
	memoryBlock0Value[0] = i2c_address<<1;
	i2c_error_indicator = write_i2c_with_command((uint8_t) 0x00, memoryBlock0Value,16);
	if (i2c_error_indicator) {return NTAG_ERROR_I2C_COMMUNICATION;}
	ntag_config_m.i2c_address_local = i2c_address;
	return NTAG_NO_ERROR;
}

//Enables the NFC silence feature and enables soft reset through I2C repeated start.
error_ntag_t ntag_nfcs_i2c_rst(bool nfcsandrst) {
	// Bit7 in Configuration/Session register NC_REG (Byte number 0 in Config Register (=3Ah[58d] via I2C interface))
	error_ntag_t error_indicator;
	uint8_t regdata = (nfcsandrst) ? NTAG_CSR_NC_REG_NFCS_I2C_RST_ON : NTAG_CSR_NC_REG_NFCS_I2C_RST_OFF;
	error_indicator = ntag_write_session_or_config_register(SESSION_REGISTER,NTAG_CSR_NC_REG,NTAG_CSR_NC_REG_NFCS_I2C_RST_ON_OFF_MASK,regdata);
	return error_indicator;
}

//Reset I2C with double start condition
error_ntag_t ntag_i2c_rst_with_doubleStart() {
	error_ntag_t error_indicator = NTAG_NO_ERROR;
	uint8_t readDummyData[] = {0x00};
	error_indicator = I2C_MasterStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Read);
	error_indicator |= I2C_MasterReadBlocking(i2c_type_base_p_m, readDummyData, 1, kI2C_TransferRepeatedStartFlag);
	error_indicator |= I2C_MasterRepeatedStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Read);
	//wait_ms(4);
	error_indicator |= I2C_MasterStop(i2c_type_base_p_m);
	return error_indicator;
}

error_ntag_t ntag_get_sessionConfig(uint8_t configArray_7Bytes[]) {
	error_ntag_t error_indicator = NTAG_NO_ERROR;
	for(uint8_t reg = 0x00; reg< 0x07;reg++){
		error_indicator = ntag_read_session_or_config_register(SESSION_REGISTER, reg, (configArray_7Bytes+reg));
		if (error_indicator) {break;}
	}
	return error_indicator;
}

error_ntag_t ntag_factory_resest_of_memory() {
	error_ntag_t error_indicator = NTAG_NO_ERROR;;
	uint8_t memoryBlock0Value[16]= NTAG_MEMORY_BLOCK_0_FACTORY_VALUE;
	uint8_t memoryBlock56Value[16]= NTAG_MEMORY_BLOCK_56_FACTORY_VALUE;
	uint8_t memoryBlock57Value[16]= NTAG_MEMORY_BLOCK_57_FACTORY_VALUE;
	uint8_t memoryBlock58Value[16]= NTAG_MEMORY_BLOCK_58_FACTORY_VALUE;
	//uint8_t ndefMessageDoInit[16] = NTAG_MEMORY_DEFAULT_NDEF_MESSAGE;
	if(error_indicator == NTAG_NO_ERROR) {error_indicator = write_i2c_with_command((uint8_t) 0x00, memoryBlock0Value,16);wait_ms(4);}
	if(error_indicator == NTAG_NO_ERROR) {error_indicator = write_i2c_with_command((uint8_t) 56, memoryBlock56Value,16);wait_ms(4);}
	if(error_indicator == NTAG_NO_ERROR) {error_indicator = write_i2c_with_command((uint8_t) 57, memoryBlock57Value,16);wait_ms(4);}
	if(error_indicator == NTAG_NO_ERROR) {error_indicator = write_i2c_with_command((uint8_t) 58, memoryBlock58Value,16);wait_ms(4);}
	/*
	wait_ms(4); //NTAG needs about 4ms for writing
	uint8_t i2cRegister = NTAG_USER_MEMORY_1_BLOCK(0);
	error_indicator = write_i2c_with_command((uint8_t) i2cRegister, ndefMessageDoInit,16);
	*/
	return error_indicator;
}

/*
 * Reads the I2C Config Block 0x00 (Length of 16 bytes)
 */
error_ntag_t ntag_get_16ByteBlock(uint8_t blockNo, uint8_t * valArray_p){
	error_ntag_t error_indicator;
	uint8_t i2cRegister = blockNo;
	error_indicator = read_i2c_with_command(i2cRegister, valArray_p, 16);
	return error_indicator;
}

/*
 * Reads defined number of bytes of the NTAG user memory and saves it to array
 * @param length : Number of uint8_t bytes to read.
 * @param valArray_p : Pointer to array for read bytes.
 */
error_ntag_t ntag_read_userMemory(uint8_t * valArray_p, uint8_t length) {
	error_ntag_t error_indicator = NTAG_NO_ERROR;

	if (length > NTAG_USER_MEMORY_SIZE*16) {return NTAG_ERROR_MEMORY_SIZE;}

	uint8_t numberOf16ByteBlocks = length/16;
	uint8_t numberOfBytesInLastBlock = length%16;
	//Write Complete Blocks (16 Bytes)
	array_16bytes_p_t array_16bytes_p = valArray_p;
	uint8_t i2cRegister = 0;
	for (uint8_t blockNo = 0; blockNo < numberOf16ByteBlocks; blockNo++) {
		if (blockNo < NTAG_USER_MEMORY_1_BLOCK_SIZE) {
			i2cRegister = NTAG_USER_MEMORY_1_BLOCK(blockNo);
			error_indicator = read_i2c_with_command(i2cRegister, array_16bytes_p, 16);
#if NTAG_2K_VERSION
		} else if (blockNo < (NTAG_USER_MEMORY_1_BLOCK_SIZE + NTAG_USER_MEMORY_2_BLOCK_SIZE)) {
			i2cRegister = NTAG_USER_MEMORY_2_BLOCK(blockNo-NTAG_USER_MEMORY_1_BLOCK_SIZE);
			error_indicator = read_i2c_with_command(i2cRegister, array_16bytes_p, 16);
#endif
		} else {  // Sollte eigentlich nicht existieren, da oben schon Return mit Fehler Memory Size.
			return NTAG_ERROR_MEMORY_SIZE;
		}
		if (error_indicator) {break;}
		array_16bytes_p += 16;
	}
	// Rest Bytes
	if (numberOfBytesInLastBlock) {
		error_indicator = read_i2c_with_command(i2cRegister+1, array_16bytes_p, numberOfBytesInLastBlock);
	}
	return error_indicator;
}

error_ntag_t ntag_write_userMemory(uint8_t * valArray_p, uint8_t length) {
	// Ein Block muss immer vollständig geschrieben (alle 16 Bytes). Sonst wird er nicht anerkannt.
	error_ntag_t error_indicator;

	if (length > NTAG_USER_MEMORY_SIZE*16) {return NTAG_ERROR_MEMORY_SIZE;}
#if	NTAG_2K_VERSION
	return NTAG_ERROR_NOT_YET_IMPLEMENTED;
#endif

	uint8_t numberOf16ByteBlocks = length/16;
	uint8_t numberOfBytesInLastBlock = length%16;
	//Write Complete Blocks (16 Bytes)
	array_16bytes_p_t array_16bytes_p = valArray_p;
	uint8_t i2cRegister = 0;
	for (uint8_t blockNo = 0; blockNo < numberOf16ByteBlocks; blockNo++) {
		i2cRegister = NTAG_USER_MEMORY_1_BLOCK(blockNo);
		error_indicator = write_i2c_with_command(i2cRegister, array_16bytes_p, 16);
		if (error_indicator) {break;}
		wait_ms(4); //NTAG needs about 4ms for writing
		array_16bytes_p += 16;
	}
	// Rest Bytes
	if (numberOfBytesInLastBlock) {
		uint8_t blockToWrite[16] = {0x00};
		memcpy(blockToWrite,array_16bytes_p,numberOfBytesInLastBlock);

		error_indicator = write_i2c_with_command(i2cRegister+1, blockToWrite, 16);
		wait_ms(4); //NTAG needs about 4ms for writing
	}
	return error_indicator;
}

//ToDo
/*
 * Reads first 16 bytes (Register) of NTAG, searches for NDEF Message Length indicator.
 * Reads the whole length NDEF Message.
 * Writes message to the defined structure given with pointer ndef_message_s_p.
 * Message includes start + end-sign and length-field
 */
//ToDo
/*
ndef_message_uint8_array_t * ntag_get_ndef_message(void) {
	error_ntag_t error_indicator = NTAG_NO_ERROR;
	uint8_t ntagUserMemoryVal[16];

	ntag_read_userMemory(ntagUserMemoryVal,16);
	uint8_t messageEndPoint;
	uint8_t messageStartPoint;

	if((thms2ndef_getNDEFMessageStartAndEndPoint(ntagUserMemoryVal, &messageStartPoint, &messageEndPoint))) {
		return NULL;
	}

	size_t message_length = (size_t) (messageEndPoint-messageStartPoint)+1; // Length with start and end sign

	uint8_t messagebuffer[messageEndPoint+1]; // Includes also empty fields of NTAG Memory

	uint8_t * ntag_message_buffer = malloc(message_length);
	ndef_message_uint8_array_t * new_ndef_message_array_s_p = malloc(sizeof(ndef_message_uint8_array_t)+message_length*sizeof(uint8_t));
	if (new_ndef_message_array_s_p == NULL){
		return false;
	} // Not enough space

	new_ndef_message_array_s_p->array_length = message_length;

	ntag_read_userMemory(messagebuffer,messageEndPoint+1); // With start and end sign. Includes also empty fields of NTAG Memory before start-sign

	memcpy(new_ndef_message_array_s_p->buffer_vla, ntag_message_buffer+messageStartPoint,message_length); //with start and end sign
}

*/


// Missing Code: Hier muss noch der User Memory 2 Block gelöscht werden
error_ntag_t ntag_erase_userMemory(void) {
	error_ntag_t error_indicator;
	for (uint8_t blockNo = 0; blockNo < NTAG_USER_MEMORY_1_BLOCK_SIZE; blockNo++) {
		uint8_t blockOfZeros[16] = NTAG_MEMORY_BLOCK_OF_ZEROS;
		error_indicator = write_i2c_with_command(NTAG_USER_MEMORY_1_BLOCK(blockNo), blockOfZeros, 16);
		if (error_indicator) {break;}
		wait_ms(4); //NTAG needs about 4ms for writing
	}
	return error_indicator;
}
/* >> END: External Functions */


/*>>>------------------------------------------------------------*/
/* >> START: Internal (Static) Functions */


error_ntag_t ntag_write_session_or_config_register(config_or_session_register_t mega, ntag_config_and_session_registers_t rega, uint8_t mask, uint8_t regdata) {
	uint8_t megau8 = mega;
	uint8_t regau8 = rega;
	uint8_t regdatamasked = mask & regdata;
	uint8_t i2c_message[] = {megau8,regau8,mask,regdatamasked};
	return write_i2c(i2c_message, sizeof(i2c_message));
}

error_ntag_t ntag_read_session_or_config_register(config_or_session_register_t mega, ntag_config_and_session_registers_t rega, uint8_t * regdata_p) {
	error_ntag_t error_indicator = NTAG_NO_ERROR;
	uint8_t megau8 = mega;
	uint8_t regau8 = rega;
	uint8_t i2c_message[] = {megau8,regau8};
	error_indicator = write_i2c(i2c_message, sizeof(i2c_message));
	if(!error_indicator) {
		return read_i2c(regdata_p, 1);
	}
	else {
		return error_indicator;
	}
}

static error_ntag_t config_i2c(I2C_Type * base_p, i2c_master_config_t * i2c_master_config_p) {
	i2c_type_base_p_m = base_p;
	i2c_master_config_p_m = i2c_master_config_p;
	return NTAG_NO_ERROR;
}

static error_ntag_t read_i2c(uint8_t * valArray_p, uint8_t length){
    status_t reVal = kStatus_Success;
    reVal = I2C_MasterStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Read);
	if (reVal == kStatus_Success) {
	    reVal = I2C_MasterReadBlocking(i2c_type_base_p_m, valArray_p, length, kI2C_TransferDefaultFlag);
	}
	reVal |= I2C_MasterStop(i2c_type_base_p_m);
	return (reVal == kStatus_Success) ? NTAG_NO_ERROR : NTAG_ERROR_I2C_COMMUNICATION;
}

static error_ntag_t read_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length){
    status_t reVal = kStatus_Success;
    reVal = I2C_MasterStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Write);
	if (reVal == kStatus_Success) {
	    reVal = I2C_MasterWriteBlocking(i2c_type_base_p_m, &command, 1, kI2C_TransferNoStopFlag);
	    if (reVal == kStatus_Success) {
	    	reVal = I2C_MasterRepeatedStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Read);
	    	if (reVal == kStatus_Success) {
	    		reVal = I2C_MasterReadBlocking(i2c_type_base_p_m, valArray_p, length, kI2C_TransferDefaultFlag);
	    	}
	    }
	}
	reVal |= I2C_MasterStop(i2c_type_base_p_m);
	return (reVal == kStatus_Success) ? NTAG_NO_ERROR : NTAG_ERROR_I2C_COMMUNICATION;
}

static error_ntag_t write_i2c(uint8_t * valArray_p, uint8_t length) {
    status_t reVal = kStatus_Success;
    reVal = I2C_MasterStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Write);
	if (reVal == kStatus_Success) {
	    reVal = I2C_MasterWriteBlocking(i2c_type_base_p_m, valArray_p, length, kI2C_TransferDefaultFlag);
	}
	reVal |= I2C_MasterStop(i2c_type_base_p_m);
	return (reVal == kStatus_Success) ? NTAG_NO_ERROR : NTAG_ERROR_I2C_COMMUNICATION;
}

static error_ntag_t write_i2c_with_command(uint8_t command, uint8_t * valArray_p, uint8_t length) {
    status_t reVal = kStatus_Success;
    reVal = I2C_MasterStart(i2c_type_base_p_m, ntag_config_m.i2c_address_local, kI2C_Write);
	if (reVal == kStatus_Success) {
	    reVal |= I2C_MasterWriteBlocking(i2c_type_base_p_m, &command, 1, kI2C_TransferNoStopFlag);
	    if (reVal == kStatus_Success) {
	    	reVal |= I2C_MasterWriteBlocking(i2c_type_base_p_m, valArray_p, length, kI2C_TransferDefaultFlag);
	    }
	}
	reVal |= I2C_MasterStop(i2c_type_base_p_m);
	return (reVal == kStatus_Success) ? NTAG_NO_ERROR : NTAG_ERROR_I2C_COMMUNICATION;
}

static void wait_ms(uint32_t delay_in_ms) {
	timer_wait_ms(delay_in_ms);
}
/* >> END: Internal (Static) Functions */


