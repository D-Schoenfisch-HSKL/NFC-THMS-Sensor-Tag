/*
 * thms_2_ndef.c
 *
 *  Created on: 03.05.2021
 *      Author: DavidSchönfisch
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <thms_2_ndef.h>
#include <ndef_parse.h>


#define THMS2NDEF_START_SIGN						0x03
#define THMS2NDEF_END_SIGN							0xFE

#define THMS2NDEF_MEMORY_DEFAULT_NDEF_MESSAGE	  THMS2NDEF_START_SIGN, 13, 0xD1, 0x01, \
												  9, 'T', 0x02, 'd', \
												  'e', 'D', 'o', ':', \
												  '0', '1', ';', THMS2NDEF_END_SIGN // NDEF Message to init
												  //Byte[1] = Gesamtlänge
												  //Byte[4] = Länge Text?

#define DO_INSTRUCTION_HEADER						{'D', 'o', ':'}

/********************************
 * Prototypes
 ********************************/
static uint32_t hex2int(char * hex_string);



void thms2ndef_setInitialNDEFMessage(void) {
	uint8_t ndef_message[32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, THMS2NDEF_MEMORY_DEFAULT_NDEF_MESSAGE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	ntag_write_userMemory(ndef_message,32);
}


/*
 * Generates a Message String (Like Do: 0;No:1;SS:23;MS:45;RSQPB:67;).
 * Text message has to begin with language indicator (de)
 * Non Null-Terminated String.
 * Do State is given as Hex-String of uint8 with always 2 chars!
 */
ndef_message_uint8_array_t * thms2ndef_generateMeasTextAndDoInstruction(uint8_t doInstruction, uint8_t MeasurementNo,  int32_t SensorSignal, int32_t MeasurementSignal, uint32_t r_sqrt_by2pow32) {
	char message[60] ;//= "Do:0;No:   ;SS:         ;MS:         ;RSQPB:         ;\n"; //Max Length = 55

	int n = sprintf (message,
	        " deDo:%2x;No:%u;SS:%i;MS:%i;RSQPB:%u;",
			doInstruction,MeasurementNo,SensorSignal,MeasurementSignal,r_sqrt_by2pow32
	        );
	message[0] = 0x02 ;// ASCII Zeichen unklar
	ndef_message_uint8_array_t * ndefMessage = malloc(sizeof(ndef_message_uint8_array_t)+n*sizeof(char));
	if(ndefMessage == NULL) {return NULL;};
	//ndefMessage->buffer_vla[n*sizeof(char)];
	strcpy(ndefMessage->buffer_vla,message);
	ndefMessage->array_length = n;
	return ndefMessage;
}

// To be checked !!
// ToDo: Global Config Datei (to be included in main und hier)
/*
 * Generates a Config Info String (Like Do: 0;FWV:1.3.2;SST:23;MST:45).
 * FWS: Firmware Version ; SST: Sensorsignal Type; MeasurementSignal Type
 * Text message has to begin with language indicator (de)
 * Non Null-Terminated String.
 * Do State is given as Hex-String of uint8 with always 2 chars!
 */
ndef_message_uint8_array_t * thms2ndef_generateConfigInfoTextAndDoInstruction(uint8_t doInstruction) {
	char message[60] ;//= "Do:0;No:   ;SS:         ;MS:         ;RSQPB:         ;\n"; //Max Length = 55

	int n = sprintf (message,
	        " deDo:%2x;FWV:%s;SST:%s;MST:%s;",
			doInstruction,"1.3.2","sqrt(ns)/LSB","?"
	        );
	message[0] = 0x02 ;// ASCII Zeichen unklar
	ndef_message_uint8_array_t * ndefMessage = malloc(sizeof(ndef_message_uint8_array_t)+n*sizeof(char));
	if(ndefMessage == NULL) {return NULL;};
	//ndefMessage->buffer_vla[n*sizeof(char)];
	strcpy(ndefMessage->buffer_vla,message);
	ndefMessage->array_length = n;
	return ndefMessage;
}


// If "thms2ndef_generateMeasTextAndDoInstruction" uses malloc
void thms2ndef_freemMessTextArray(ndef_message_uint8_array_t * messageArrayToBeFreed_p) {
	free(messageArrayToBeFreed_p);
}


/*
 * Check message (byte array) ("ndefMessageFirst16Bytes") if there is NDEF Start sign and end sign.
 * @return Is true if NDEF-Start-Sign (0x03) and End-Sign found (0xFE).
 */
bool thms2ndef_getNDEFMessageStartAndEndPoint(uint8_t message[], uint8_t * ndefMessageStartPoint_p, uint8_t * ndefMessageEndPoint_p, uint8_t length) {
	// Code alternative
	/*uint8_t i=0;
	do {
		if(thms2ndef_getNDEFMessageStartPoint(&message[i],ndefMessageStartPoint_p,length-i)){
			i = *ndefMessageStartPoint_p;
			*ndefMessageEndPoint_p = i + message[i + 1] + 2;
			if(message[*ndefMessageEndPoint_p] == THMS2NDEF_END_SIGN ) {
				return true;
			}
		} else {
			return false;
		}
	} while( i < (length-5)); */
	for (uint8_t i = 0; i<length;i++) {
		if (message[i] == THMS2NDEF_START_SIGN) {
			*ndefMessageStartPoint_p = i;
			// Nach THMS2NDEF_START_SIGN steht NDEF Länge (ohne 0x3 und ohne 0xFE)
			*ndefMessageEndPoint_p = i + message[i+1] + 2;
			if(message[*ndefMessageEndPoint_p] == THMS2NDEF_END_SIGN ) {
				return true;
			}
		}
	}
	return false;
}

bool thms2ndef_getNDEFMessageStartPoint(uint8_t message[], uint8_t * ndefMessageStartPoint_p, uint8_t length) {
	for (uint8_t i = 0; i<length;i++) {
		if (message[i] == THMS2NDEF_START_SIGN) {
			*ndefMessageStartPoint_p = i;
			return true;
		}
	}
	return false;
}


//Check if value at NDEF-Message End Point is == NDEF_END_SIGN (0xFE);
bool thms2ndef_checkNDEFMessageEndPoint(uint8_t ndefMessageFirst16Bytes[], uint8_t * ndefMessageEndPoint_p) {
	return (ndefMessageFirst16Bytes[*ndefMessageEndPoint_p] == THMS2NDEF_END_SIGN) ? true : false;
}

/*
 * Check NDEF if there is a Do-Instruction. Write Instruction to "newIsmInstruction_p"
 * Return false if "Do:??" was not found
 */
bool thms2ndef_get_FSM_DoInstruction_inNDEF16Bytes(uint8_t ndefMessage[], uint8_t ndefMessageStartPoint, uint8_t ndefMessageEndPoint,uint8_t * newIsmInstruction_p) {
	for(int i = ndefMessageStartPoint; i < (ndefMessageEndPoint-5); i++) {
	//for(int i = ndefMessageStartPoint; i < (12); i++) {
		if ((ndefMessage[i]=='D') & (ndefMessage[i+1]=='o') & (ndefMessage[i+2]==':')) {
			char instructionHexVal[2] = {ndefMessage[i+3],ndefMessage[i+4]}; // With NULL at end ???
			//*newIsmInstruction_p = (uint8_t) atoi(instructionHexVal); //<< Very program-flash needy
			*newIsmInstruction_p = (uint8_t) hex2int(instructionHexVal); //Convert HEX-Char to uint8
			//sscanf(instructionHexVal,"%x",newIsmInstruction_p); //<< Very very very program-flash needy
			return true;
		}
	}
	return false;
}

/*
 * Write Do-Instruction after "Do:" in first 16 bytes of NDEF Text. Returns false if 'Do:' wasn't found.
 * Return false if "Do:??" was not found
 */
bool thms2ndef_set_FSM_DoInstruction(uint8_t ndefMessage[], uint8_t ndefMessageStartPoint, uint8_t ndefMessageEndPoint, uint8_t newInstruction) {
	for(int i = ndefMessageStartPoint; i < (ndefMessageEndPoint-4); i++) {
		if ((ndefMessage[i]=='D') & (ndefMessage[i+1]=='o') & (ndefMessage[i+2]==':')) {
			char instructionHexVal[3];
			int n = sprintf (instructionHexVal,"%2x",newInstruction);
			memcpy(&ndefMessage[i+3],instructionHexVal,2);
			return true;
		}
	}
	return false;
}


//ToDo:
bool thms2ndef_getValuesFromMeasText(ndef_message_uint8_array_t * message){
	char * sentence = (char *) message->buffer_vla; //OK?
	char indicator [20];
	int value;

	int sep_pointer = 0;
	int del_pointer = 0;


	while(del_pointer<strlen(sentence)){
		sep_pointer = (strchr(&sentence[del_pointer],':')-sentence);
		if(sep_pointer>strlen(sentence)) {return NULL;}
		sentence[sep_pointer] = ' ';
		sscanf (&sentence[del_pointer],"%s %d;",indicator,&value);
		//ToDo: Hier muss irgendeine Zuordnung stattfinden zu den variablen
			// Vorherige Übergabe von Pointern in einer Init Funktion?
		del_pointer = (strchr(&sentence[del_pointer],';')-sentence)+1;
	};
	return false;
}


/********************** - Internal Functions - ************************/
/**
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */
static uint32_t hex2int(char *hex) {
    uint32_t val = 0;
    while (*hex) {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}



//ALTE SACHEN

/*
bool thms2ndef_generateSimpleMeasurementTextMessage(uint8_t ndefMessage[], uint8_t * ndefMessageLength_p, int32_t SensorSignal) {
	uint8_t * messageEndPointer_p = ndefMessage;
	char defaultMessage[30] = THMS2NDEF_DEFALT_MESSAGE_TEXT_RECORD;
	memcpy(messageEndPointer_p, defaultMessage, 30);
	messageEndPointer_p += 30;

	char SensorSignalChar[10] = {0};
	uint32_t SensorSignalp1000 = (uint32_t)SensorSignal / 1000;
	itoa(SensorSignalp1000, SensorSignalChar, 10);
	uint8_t SensorSignalChar_length = strlen(SensorSignalChar);
	memcpy(messageEndPointer_p, SensorSignalChar, SensorSignalChar_length);
	messageEndPointer_p += SensorSignalChar_length;

	char ndefEndSign = THMS2NDEF_END_SIGN;
	memcpy(messageEndPointer_p, &ndefEndSign, 1);
	messageEndPointer_p += 1;

	*ndefMessageLength_p = (messageEndPointer_p - ndefMessage);

	ndefMessage[1] = *ndefMessageLength_p - 3; //Länge der Message ohne 0x03,0xFE und der Längenangabe
	ndefMessage[4] = 24 + SensorSignalChar_length;

	return true;
}

//Initial NDEF-String Message for NTAG ("Messung..." or App-Load)
bool thms2ndef_getInitialTextMessage(uint8_t ndefMessage[], uint8_t * ndefMessageLength_p) {
	uint8_t * messageEndPointer_p = ndefMessage;
	char initialMessage[] = THMS2NDEF_INITIAL_MESSAGE_TEXT_RECORD;
	memcpy(messageEndPointer_p, initialMessage, sizeof(initialMessage));

	messageEndPointer_p += sizeof(initialMessage);

	*ndefMessageLength_p = (messageEndPointer_p - ndefMessage);

	ndefMessage[1] = *ndefMessageLength_p - 3; //Länge der Message ohne 0x03,0xFE und der Längenangabe
	//ndefMessage[4] = ndefMessage[1] - 7;

	return true;
}
*/
