/*
 * thms_2_ndef.h
 *
 *  Created on: 03.05.2021
 *      Author: DavidSchönfisch
 */

#ifndef THMS_2_NDEF_H_
#define THMS_2_NDEF_H_

#include <stdbool.h>
#include <stdint.h>
#include <thms_2_ndef.h>
#include <ndef_parse.h>

/*/// Not used Code
const struct {
	char SEPARATOR[2];						// Seperator between indicator-value pairs
	char IND_VAL_SEP[2];					// Indicator and value seperator
	char IND_DO_STATE[3];				// Do instruction. See "fsm_states_e_t" in main. Value should be given as hex-string with always 2 chars
	char IND_MEAS_NO[3];				// Continuous number for measurement [LVC: 3 (1Byte to string)]
	char IND_SENSOR_SIGNAL[3];			// Sensor-Signal [LVC: 9 ?]
	char IND_MEASUREMENT_SIGNAL[3];	// Measurement-Signal at pulse begin (Correlates to temperature) [5 ?]
	char IND_R_SQRT[6]; 	//R² in 1/1E9 [9]
	// Configuration indicator //ToDo: Config implementieren
	char IND_PULSE_LENGTH[7];	// Pulse length in samples [2]
	char IND_ADS_SAMPLE_RATE[4];	// Sample rate (of ADS) [1]
	char IND_FIRST_FIT_SAMPLE[4];	// First sample for fit [2]
	char IND_ENABLE_UART_PULSE_OUTPUT[4];	// To enable output of Pulse-Curve via UART [1] (0 = disabled, rest = enabled)
}thms_message_parts_s = {";",":","Do","No","SS","MS","RSQPB","PLSLEN","SRT","FFT","UPO"};
/* Text Record example: Do:1;SS:1234;MS:1357 */

//bool thms2ndef_generateMeasurementMessageAndAppIndicator(uint8_t ndefMessage[], uint8_t * ndefMessageLength_p, uint8_t No, int32_t SensorSignal,uint32_t rsqrt_indicator, uint32_t pulseLenght_inms, uint32_t fitStart_inms);


void thms2ndef_setInitialNDEFMessage(void);

bool thms2ndef_generateSimpleMeasurementTextMessage(uint8_t ndefMessage[], uint8_t * ndefMessageLength_p, int32_t SensorSignal); // Old Version ??
bool thms2ndef_getInitialTextMessage(uint8_t ndefMessage[], uint8_t * ndefMessageLength_p);

bool thms2ndef_getNDEFMessageStartAndEndPoint(uint8_t ndefMessageFirst16Bytes[], uint8_t * ndefMessageEndPoint_p, uint8_t * ndefMessageStartPoint_p, uint8_t length);
bool thms2ndef_getNDEFMessageStartPoint(uint8_t message[], uint8_t * ndefMessageStartPoint_p, uint8_t length);
bool thms2ndef_checkNDEFMessageEndPoint(uint8_t ndefMessageFirst16Bytes[], uint8_t * ndefMessageEndPoint_p);
bool thms2ndef_get_FSM_DoInstruction_inNDEF16Bytes(uint8_t ndefMessage[], uint8_t ndefMessageStartPoint, uint8_t ndefMessageLength,uint8_t * newIsmInstruction_p);
bool thms2ndef_set_FSM_DoInstruction(uint8_t ndefMessage[], uint8_t ndefMessageStartPoint, uint8_t ndefMessageEndPoint, uint8_t newInstruction);

bool thms2ndef_getValuesFromMeasText(ndef_message_uint8_array_t * message);

// Prüfen ob Do: Instruction. in 128 Bytes
//bool thms2ndef_check4ismInstruction(uint8_t * newIsmInstruction_p, uint8_t ntagUserMemoryVal_p[]);

ndef_message_uint8_array_t * thms2ndef_generateMeasTextAndDoInstruction(uint8_t doInstruction, uint8_t MeasurementNo,  int32_t SensorSignal, int32_t MeasurementSignal, uint32_t r_sqrt_by2pow32) ;
ndef_message_uint8_array_t * thms2ndef_generateConfigInfoTextAndDoInstruction(uint8_t doInstructionchar, char fw_version[], char ss_type[], char ms_type[]);
void thms2ndef_freemMessTextArray(ndef_message_uint8_array_t * messageArrayToBeFreed_p);




#endif /* THMS_2_NDEF_H_ */
