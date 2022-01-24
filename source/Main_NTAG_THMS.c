/*
 * Code Version: 1.3.3
 *
 * !!ACHTUNG:
 * Änderungen zu 1.3.1:
 *  - Sensorsignal jetzt in sqrt(ns)/LSB. 1LSB=1562500pV
 * Änderungen zu 1.3.2:
 *  - StateMachine Case für Get Info zu Codeversion, Sensor-Signal-Type, Measurement-Signal-Type
 *
 * ToDo:
 *  - Sturktur für Konfiguration.
 *  	- Einfacherers Übergeben als Pointer an Funktionen (-> thms2ndef_generateConfigInfoTextAndDoInstruction)
 *  - FSM_State für FSM_CHANGE_CONFIG
 *		- PULSE_LENGTH_MS als const änderbar über NFC
 *		- ADC Konfiguration (Gain, Samplerate) änderbar
 * 	- Handling of I2C block wärend der NFC Kommunikation
 *
 */

#define FIRMWARE_VERSION				"1.3.4"

// >>> CONFIG <<< //
#define SELF_RESET_AFTER_MEASUREMENT	false	// Reset NFC to trigger readout
#define AUTO_MEASUREMENT				false	// Automatic measurement after power-up
#define PULSE_LENGTH_MS					600		// Length of pulse. Max 3 Seconds. Adjust ADS Gain and DR in THMS.h to avoid sample overflow.
#define PULSE_FIT_START_MS				50		// Time to start LinReg fit for "THMS-SS" calculation.
#define AUTOMATIC_MEASUREMENT_INS		0		// To do automatic measurements if power is enabled. Set to zero to disable.
#define SENSOR_TCR						"6 10^-3 K^-1"		// In 10^-3 K^-1

// >>> DEBUG COONFIG <<< //
#define PRINT_UART_DEBUG_INFO			false
#define PRINT_UART_ERROR_INFO			false
#define REGISTER_CONFIG_CHECK_TO_UART	false 	// Only if debug enabled

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pin_mux.h"
#include "board.h"
#include "fsl_i2c.h"

#include "NTAG.h"
#include "ads1115.h"
#include "THMS.h"
#include "timer.h"
#include "thms_2_ndef.h"
#include "ndef_parse.h"

#include "fsl_debug_console.h"


/**********************************************************************c*********
 * Definitions
 ******************************************************************************/

#define I2C_MASTER_BASE    			(I2C0_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY	(12000000)
#define WAIT_TIME                 	10U
#define I2C_MASTER 					((I2C_Type *)I2C_MASTER_BASE)

#define I2C_BAUDRATE            	100000U


#define NDEF_MESSAGE_MAX_LENGTH		128

#define FSM_DEFAULT_SLOW_DOWN		1000	// Max 1000 ms

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PULSE_FITSTART_STRING 		STR(PULSE_FIT_START_MS) "ms"
#define PULSE_LENGTH_STRING			STR(PULSE_LENGTH_MS) "ms"

//States for Infinite State Machine in main
typedef enum {
	FSM_INIT							= 0x00U,
	FSM_IDLE 							= 0x01U,
	FSM_DO_SINGLE_MEASUREMENT			= 0x02U, // Do single measurement and go to NFC_I2C_RST (power cycle)
	FSM_NFCS_I2C_RST					= 0x04U, // Silence NFC (for 1 s) and Reset i2c
	//ToDo:
	//FSM_CHANGE_CONFIG					= 0x05U, // Changes Config of Tag due to instruction followed on "Do:"-Instruction
	FSM_GET_CONFIG						= 0x06U, // Get Config and Info for: Pulse Length, Sensor-Signal type, Measurement-Signal type, firmware version
	//FSM_DO_MULTIPLE_MEASUREMENTS		= 0x07U, // Do multiple measurements wit interval "delayBetweenMeasurementsInMs"
	//FSM_DO_SINGLE_MEASUREMENT_NO_RESET	= 0x08U,
	//FSM_NO_MEASUREMENT_AFTER_POWERUP	= 0x10U, // To be written on tag if no measurement should be done after power-cycle. As continuous Flag??

	FSM_ERROR							= 0xFFU
}fsm_states_e_t;

typedef enum {
	ERROR_UNKNOWN					= 0x00U,
	ERROR_FSM_INSTRUCTION_UNKNOWN	= 0x01U,

	ERROR_NO_INSTRUCTION_FOUND		= 0x03U,
	ERROR_NDEF_MESSAGE_TO_LONG		= 0x04U,
	ERROR_NDEF_MESSAGE_CONSTRUCTION	= 0x05U,
	ERROR_NTAG_COMMUNICATION		= 0x06U,

	ERROR_CAN_NOT_SET_NTAG_FSM_INST	= 0x10U,
	ERROR_CAN_NOT_GET_NTAG_FSM_INST	= 0x20U
}errors_t;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
#if PRINT_UART_DEBUG_INFO
void readNTAGUserMemoryAndPrint(void);
void readNTAGRegisterAndPrint(uint8_t reg);
void findMyI2C_Devices(void);
void printConfig(uint8_t configArray_7Bytes[]);
#endif
void printHEX(uint8_t array[], size_t length);
void reanimateNTAG(void);

bool set_NDEF_Text_Record_on_NTAG(ndef_message_uint8_array_t * text_message_array_s_p);
bool set_NTAG_do_instruction(fsm_states_e_t newInstruction);
uint8_t get_NTAG_do_instructuion(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static int32_t SensorSignal;
static int32_t MeasurementSignal;
static uint32_t rsqrt_indicator; // in 1/2pow32

static i2c_master_config_t masterConfig;
static error_ntag_t ntag_error;
// static int delayBetweenMeasurementsInMs = DEFAULT_DELAY_BETWEEN_MEASUREMENTS_INS*1000;

static uint8_t fsm_state_m;
static uint8_t fsm_state_ntag;
static uint8_t next_doInstruction = FSM_ERROR;

static uint8_t ntagUserMemoryVal[NDEF_MESSAGE_MAX_LENGTH];	// Buffer for NTAG Memory (write/read)
static int ntagUserMemoryValLength = 0;		// Length of used "ntagUserMemoryVal" Message

static uint8_t lastError = ERROR_UNKNOWN;

static bool resetIndicator = false; // to indicate if system was reset (but without power-cycle (due to external power))

static uint8_t measurementCout = 0;
static int FSM_slowDown_ms = FSM_DEFAULT_SLOW_DOWN;

//ToDo: In Structur umformen für einfachere Übergabe an Funktionen
static char fw_version[10] = FIRMWARE_VERSION;
static char ss_type_m[20] = "sqrt(ns)/LSB"; // Should be Null-terminated
static char ms_type_m[20] = "nV";	// Should be Null-terminated
static char pulse_length_10_m[10] = PULSE_LENGTH_STRING; // Should be Null-terminated
static char pulse_fit_start_10_m[10] = PULSE_FITSTART_STRING;	// Should be Null-terminated
static char sensor_tcr_m[20] = SENSOR_TCR;

static uint32_t mulitplier_LSB_inPV; // Value to convert LSB (of ADC) into picoVolt.

/*******************************************************************************
 * Code
 ******************************************************************************/
#if PRINT_UART_DEBUG_INFO
void readNTAGUserMemoryAndPrint(void) {
	error_ntag_t ntag_error;
	uint8_t g_master_rxBuff[100] = {0x00};
	ntag_error = ntag_read_userMemory(g_master_rxBuff,100);
	if(ntag_error) {PRINTF("Error at read Memory: 0x%x \r\n",ntag_error);}
	else {
		PRINTF("User Memory:");
		printHEX(g_master_rxBuff,100);
		PRINTF("\r\n\r\n");
    }
}
void readNTAGRegisterAndPrint(uint8_t reg) {
	error_ntag_t ntag_error;
    uint8_t configMemory[16] = {0x00};
    ntag_error = ntag_get_16ByteBlock(reg, configMemory);
    if(ntag_error) {PRINTF("Error at read Memory: 0x%x \r\n",ntag_error);}
    else {
		PRINTF("Register 0x%02x Memory:", reg);
		printHEX(configMemory,16);
		PRINTF("\r\n");
    }
}
void findMyI2C_Devices(void) {
	uint8_t readTestData[1] = {0x00};
	for(uint8_t testAddress = 0x0; testAddress <0x80; testAddress++) {
	    status_t reVal = kStatus_Success;
	    reVal = I2C_MasterStart(I2C_MASTER, testAddress, kI2C_Write);
		if (reVal == kStatus_Success) {
		    reVal |= I2C_MasterWriteBlocking(I2C_MASTER, readTestData, 1, kI2C_TransferDefaultFlag);
		}
		reVal |= I2C_MasterStop(I2C_MASTER);
		if (reVal == kStatus_Success) {
			PRINTF("Device found with read address: 0x%x \r\n",testAddress);
		}
		timer_wait_ms(1);
	}
	for(uint8_t testAddress = 0x0; testAddress <0x80; testAddress++) {
	    status_t reVal = kStatus_Success;
	    reVal = I2C_MasterStart(I2C_MASTER, testAddress, kI2C_Read);
		if (reVal == kStatus_Success) {
		    reVal |= I2C_MasterReadBlocking(I2C_MASTER, readTestData, 1, kI2C_TransferDefaultFlag);
		}
		reVal |= I2C_MasterStop(I2C_MASTER);
		if (reVal == kStatus_Success) {
			PRINTF("Device found with write address: 0x%x \r\n",testAddress);
		}
		timer_wait_ms(1);
	}
}
void printConfig(uint8_t configArray_7Bytes[]) {
	PRINTF("Session Register:");
	printHEX(configArray_7Bytes,7);
	PRINTF("\r\n");
}
#endif

void printHEX(uint8_t array[], size_t length) {
	for (unsigned int i = 0; i < length; i++)    {
		if ((i % 16 == 0)&(i!=0)) {PRINTF("\r\n");}
		PRINTF("%02x ", array[i]);
	}
}


void reanimateNTAG(void) {
#if PRINT_UART_DEBUG_INFO
	PRINTF("Error init NTAG (1): 0x%x \r\n",ntag_error);
	PRINTF("Try to reset NTAG I2C and Reinit: 0x%x\r\n",ntag_i2c_rst_with_doubleStart());
	ntag_error = inti_ntag(I2C_MASTER, &masterConfig);
	PRINTF("Error init TAG (2): 0x%x \r\n",ntag_error);
	PRINTF("Try to reset NTAG I2C Address \r\n");
	findMyI2C_Devices();
#endif
	ntag_change_local_i2c_address(0x04);
	ntag_change_device_i2c_address(0x55);
	ntag_change_local_i2c_address(0x2A);
	ntag_change_device_i2c_address(0x55);
#if PRINT_UART_DEBUG_INFO
	ntag_error = inti_ntag(I2C_MASTER, &masterConfig);
	if(ntag_error == NTAG_NO_ERROR) PRINTF("NTAG init OK \r\n");
#endif
}


void thingsToDoAtInitialization(void) {
    /* Enable clock of uart0. */
    CLOCK_EnableClock(kCLOCK_Uart0);
    /* Ser DIV of uart0. */
    CLOCK_SetClkDivider(kCLOCK_DivUsartClk, 1U);
    /* Select the main clock as source clock of I2C0. */
    CLOCK_Select(kMAINCLK_From_Irc);
    /* Enable clock of i2c0. */
    CLOCK_EnableClock(kCLOCK_I2c0);

    BOARD_InitPins();
    BOARD_BootClockIRC12M();
    BOARD_InitDebugConsole(); //9600 Baud

    I2C_MasterGetDefaultConfig(&masterConfig);
    // masterConfig.baudRate_Bps = I2C_BAUDRATE;	/* To change the default baudrate configuration */
    I2C_MasterInit(I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);	/* Initialize the I2C master peripheral */

    /* Initialize NTAG, THMS, Timer  */
    init_ms_timer();
    timer_wait_ms(500); // Due to voltage stabilization
    init_thms(I2C_MASTER, &masterConfig);
    ntag_error = inti_ntag(I2C_MASTER, &masterConfig);

#if PRINT_UART_DEBUG_INFO
	PRINTF("System start ... \r\n");
	//ntag_factory_resest_of_memory();
	//readNTAGRegisterAndPrint(0x3A);
#endif

    if(ntag_error) reanimateNTAG();


#if PRINT_UART_ERROR_INFO
    uint8_t configArray_of_7Bytes[7] = {0x00}; // NTAG Session Config
	ntag_error = ntag_get_sessionConfig(configArray_of_7Bytes);
	if(ntag_error) {
		PRINTF("Error get NTAG Config: 0x%x \r\n",ntag_error);
	} else {
		printConfig(configArray_of_7Bytes);
	}
#endif
}


int main(void)
{
	thingsToDoAtInitialization();

	fsm_state_m = FSM_INIT;

    while (1) //Finite State Machine Infinite Loop
    {
#if PRINT_UART_DEBUG_INFO
		PRINTF("FSM:0x%x \r\n",fsm_state_m);
#endif
    	switch(fsm_state_m) //Finite State Machine
    	{
    	case FSM_INIT: {
    		fsm_state_ntag = get_NTAG_do_instructuion(); // Get FSM Do-Instruction from NTAG = Last instruction before Reset
    		if(fsm_state_ntag == FSM_NFCS_I2C_RST) {
    			resetIndicator = false;
    			set_NTAG_do_instruction(FSM_IDLE);
    			fsm_state_m = FSM_IDLE;
    		/*} else if (fsm_state_ntag == FSM_ERROR) {
    			fsm_state_m = FSM_ERROR; */  // If error (no instruction found) do measurement to reset instruction
    		} else {
#if AUTO_MEASUREMENT == true
    			fsm_state_m = FSM_DO_SINGLE_MEASUREMENT;
#else
    			fsm_state_m = FSM_IDLE;
#endif

    		}
    		break;}// Ende Case FSM_INIT

    	case FSM_IDLE: {
#if PRINT_UART_DEBUG_INFO & REGISTER_CONFIG_CHECK_TO_UART
    		uint8_t configArray_of_7Bytes[7] = {0x00}; // NTAG Session Config
    	    readNTAGRegisterAndPrint(0x3A);
			ntag_error = ntag_get_sessionConfig(configArray_of_7Bytes);
			if(ntag_error) {
				PRINTF("Error get NTAG Config: 0x%x \r\n",ntag_error);
			} else {
	    		printConfig(configArray_of_7Bytes);
			}
#endif
			if(resetIndicator) {		// Last operation was a soft-reset (no power cycle)
				resetIndicator = false;
				if (!set_NTAG_do_instruction(FSM_IDLE)) fsm_state_m = FSM_ERROR;
			}

			fsm_state_m = get_NTAG_do_instructuion(); // Update FSM-Instruction from NTAG Memory
			break;} // Ende Case FSM_IDLE

    	case FSM_DO_SINGLE_MEASUREMENT: {
    		measurementCout++;
    		thms_get_sensor_signal(&SensorSignal, &MeasurementSignal, &rsqrt_indicator, PULSE_LENGTH_MS, PULSE_FIT_START_MS,ss_type_m,ms_type_m,&mulitplier_LSB_inPV);
    		//thms2ndef_generateSimpleMeasurementTextMessage(ntagUserMemoryVal, &ntagUserMemoryValLength, SensorSignal);

#if SELF_RESET_AFTER_MEASUREMENT == true
    	    next_doInstruction = FSM_NFCS_I2C_RST;
#else
    	    next_doInstruction = FSM_IDLE;
#endif

    	    ndef_message_uint8_array_t * measurement_text_message_array_s_p = thms2ndef_generateMeasTextAndDoInstruction(next_doInstruction,measurementCout,SensorSignal, MeasurementSignal, rsqrt_indicator);
    	    if(measurement_text_message_array_s_p==NULL) {
    	    	fsm_state_m = FSM_ERROR;
    	    	lastError = ERROR_NDEF_MESSAGE_CONSTRUCTION;
    	    	break;
    	    }

    	    if(!set_NDEF_Text_Record_on_NTAG(measurement_text_message_array_s_p)){
    	    	fsm_state_m = FSM_ERROR;
    	    	lastError = ERROR_NDEF_MESSAGE_CONSTRUCTION;
    	    }

    		break;} // Ende Case FSM_DO_SINGLE_MEASUREMENT

    	/* Enables the NFC silence mode. Wait for a second and re-enable NFC. NFC device recognizes new NDEF TAG. I2C is reset.*/
    	case FSM_NFCS_I2C_RST: {
#if PRINT_UART_DEBUG_INFO
    	    PRINTF("Do NFC silence and i2c reset \r\n");
#endif
    	    // set_NTAG_do_instruction(FSM_IDLE); // No set to idle otherwise it will do no "single measurement"
    		fsm_state_m = FSM_IDLE;
    		resetIndicator = true;
    	    ntag_nfcs_i2c_rst(true);
    		timer_wait_ms(1200);
    		ntag_nfcs_i2c_rst(false);
			break;

    	case FSM_GET_CONFIG:
    		fsm_state_m = FSM_IDLE; // = Next state

    		next_doInstruction = FSM_IDLE;
    		ndef_message_uint8_array_t * config_text_message_array_s_p = thms2ndef_generateConfigInfoTextAndDoInstruction(next_doInstruction, fw_version, ss_type_m, ms_type_m,pulse_length_10_m,pulse_fit_start_10_m,sensor_tcr_m, mulitplier_LSB_inPV);
    		if(!set_NDEF_Text_Record_on_NTAG(config_text_message_array_s_p)) {
    	    	fsm_state_m = FSM_ERROR;
    	    	lastError = ERROR_NDEF_MESSAGE_CONSTRUCTION;
    		}
    		break; } // Ende Case FSM_NFCS_I2C_RST
		//ToDo: States: FSM_CHANGE_CONFIG, FSM_DO_MULTIPLE_MEASUREMENTS, FSM_DO_MULTIPLE_MEASUREMENTS, FSM_DO_SINGLE_MEASUREMENT_NO_RESET, FSM_NO_MEASUREMENT_AFTER_POWERUP

		/* If any error occurs, write it to UART inf if in Debug mode.*/
    	case FSM_ERROR: {
#if true //PRINT_UART_ERROR_INFO
    		PRINTF("LE:0x%x \r\n", lastError);
#endif
    		if ((lastError == ERROR_CAN_NOT_SET_NTAG_FSM_INST) | (lastError == ERROR_NTAG_COMMUNICATION)) reanimateNTAG();
    		ntag_factory_resest_of_memory();
    		if (lastError == ERROR_NO_INSTRUCTION_FOUND) thms2ndef_setInitialNDEFMessage();
    		lastError = ERROR_UNKNOWN;
    		fsm_state_m = FSM_IDLE;
    		break;

    		/* This should not happen. Unknown FSM-Instruction. */
    	default:
    		lastError = ERROR_FSM_INSTRUCTION_UNKNOWN;
    		fsm_state_m = FSM_ERROR;
    		break;} // Ende Case FSM_ERROR

    	}
    	timer_wait_ms(FSM_slowDown_ms);
#if AUTOMATIC_MEASUREMENT_INS
    	if((loop_counter*FSM_slowDown_ms) > (1000*AUTOMATIC_MEASUREMENT_INS)) {
    		fsm_state_m = FSM_DO_SINGLE_MEASUREMENT;
    		loop_counter = 0;
    	}
#endif
    }
}


// Function to create NDEF-Text-Record and write it on the NTAG.
// Used in "FSM_DO_SINGLE_MEASUREMENT", "FSM_GET_CONFIG" ...
bool set_NDEF_Text_Record_on_NTAG(ndef_message_uint8_array_t * text_message_array_s_p) {
	bool success_indicator = true;

    //Define structure for NDEF-Record (Settings and pointer to payload)
    ndef_record_typedef_s_t nrt_s;

    char type_of_record[1] = "T";

	nrt_s.message_begin = true;
    nrt_s.message_end = true;
    nrt_s.chunk_flag = false;
    nrt_s.short_record = true;
    nrt_s.id_field_available = false;
    nrt_s.tnf_type = TNF_Well_Known_Record;
    nrt_s.type_p = type_of_record;
    nrt_s.type_length = 1;
    nrt_s.id_p = NULL;
    nrt_s.id_length = 0;
    nrt_s.payload_p = text_message_array_s_p->buffer_vla;
    nrt_s.payload_length = text_message_array_s_p->array_length;

    //ndef_message_records_s_t * ndef_message_p = create_ndef_message(); //As "assemble_ndefMessage_to_given_uint8_array" does not work
    // This is the record including the message_array.
    ndef_record_s_t * new_record_p = create_ndef_record(&nrt_s);
    // Text array an be freed as it is included in new_record_p
    thms2ndef_freemMessTextArray(text_message_array_s_p);

    if(new_record_p==NULL) {
    	fsm_state_m = FSM_ERROR;
    	lastError = ERROR_NDEF_MESSAGE_CONSTRUCTION;
    	bool success_indicator = false;
    }

    // add_record_to_ndef_message(ndef_message_p,new_record_p); //As "assemble_ndefMessage_to_given_uint8_array" does not work

    // if assemble_ndefMessage_to_given_uint8_array(ntagUserMemoryVal, NDEF_MESSAGE_MAX_LENGTH, &ntagUserMemoryValLength, ndef_message_p)
    if(new_record_p->data_length < NDEF_MESSAGE_MAX_LENGTH) {
		ntagUserMemoryVal[0] = 0x03; //Start-sign
		ntagUserMemoryVal[1] = new_record_p->data_length; // Message length without message frame
	    memcpy(&ntagUserMemoryVal[2],new_record_p->data_buffer_p,new_record_p->data_length);
	    ntagUserMemoryVal[new_record_p->data_length + 2] = 0xFE;
	    ntagUserMemoryValLength = new_record_p->data_length + 3;;
		ntag_write_userMemory(ntagUserMemoryVal, ntagUserMemoryValLength);
		fsm_state_m = next_doInstruction;
    } else {
    	fsm_state_m = FSM_ERROR;
    	lastError = ERROR_NDEF_MESSAGE_TO_LONG;
    	bool success_indicator = false;
    }
    // Free record payload(data_buffer) and record struct with pointers
    free_ndef_record_struct_and_data_memory(new_record_p);

    // free_ndef_records_of_message(ndef_message_p); // Needed ????
    // free_ndef_record_struct_and_data_memory(new_record_p); // Needed ????

	return success_indicator;
}


//Find Do-instruction in NTAG NDEF Memory (only first 16 Bytes). Set Do-instruction. No NDEF read/change. !! Works only for SHORT_RECORD.
// To Do: Return type "errors_t". Add Error_state "ERROR_NO_ERROR"
bool set_NTAG_do_instruction(fsm_states_e_t newInstruction) {
	// ToDo: I2C read error abfangen (Bei NFC Zugriff)
	if (ntag_read_userMemory(ntagUserMemoryVal,60)) {
		uint8_t ndefMessageEndPoint; //In bytes
		uint8_t ndefMessageStartPoint;
		if(thms2ndef_getNDEFMessageStartAndEndPoint(ntagUserMemoryVal,&ndefMessageStartPoint,&ndefMessageEndPoint,16)) {
			if(thms2ndef_set_FSM_DoInstruction(ntagUserMemoryVal,ndefMessageStartPoint,ndefMessageEndPoint,newInstruction)) {
				if(ntag_write_userMemory(ntagUserMemoryVal,16) == NTAG_NO_ERROR) { //16->ndefMessageEndPoint
					PRINTF("FSM_NTAG_Set:0x%x \r\n",newInstruction);
					return true;
				} else {lastError = ERROR_NTAG_COMMUNICATION;}
			} else {lastError = ERROR_NTAG_COMMUNICATION;}
		} else {lastError = ERROR_NO_INSTRUCTION_FOUND;}
	} else {lastError = ERROR_NTAG_COMMUNICATION;}

#if PRINT_UART_DEBUG_INFO
	PRINTF("Set NDEF Message: \r\n");
	printHEX(ntagUserMemoryVal, 16);
	PRINTF("\r\n");
#endif
	lastError &= ERROR_CAN_NOT_SET_NTAG_FSM_INST;
	return false;
}

//Read 80 bytes of NTAG Memory to check if a Do-Instruction is available. No NDEF read
uint8_t get_NTAG_do_instructuion(void){
	uint8_t newIsmInstruction = FSM_ERROR;
	uint8_t ndefMessageEndPoint = 0; //In bytes
	uint8_t ndefMessageStartPoint = 0;
	ntagUserMemoryValLength = 0;
	uint8_t error_point = 0;
	if(ntag_read_userMemory(ntagUserMemoryVal,80) == NTAG_NO_ERROR) {
		error_point++;
		if(thms2ndef_getNDEFMessageStartAndEndPoint(ntagUserMemoryVal,&ndefMessageStartPoint,&ndefMessageEndPoint,80)) { //Search for Start+End-point
			ntagUserMemoryValLength = ndefMessageEndPoint - ndefMessageStartPoint;
			error_point++;
			if (thms2ndef_get_FSM_DoInstruction_inNDEF16Bytes(ntagUserMemoryVal,ndefMessageStartPoint,ndefMessageEndPoint,&newIsmInstruction)) {
#if PRINT_UART_DEBUG_INFO
				PRINTF("FSM_NTAG: 0x%x \r\n",newIsmInstruction);
#endif
				// newIsmInstruction = newIsmInstruction
			} else {lastError = (ERROR_NO_INSTRUCTION_FOUND&ERROR_CAN_NOT_GET_NTAG_FSM_INST);}
		}
	} else {lastError = (ERROR_NTAG_COMMUNICATION&ERROR_CAN_NOT_GET_NTAG_FSM_INST);}
	return newIsmInstruction;
}

