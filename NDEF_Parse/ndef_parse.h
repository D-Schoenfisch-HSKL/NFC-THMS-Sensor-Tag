/*
 * ndef_parse.h
 *
 *  Created on: 12.07.2021
 *      Author: DavidSchönfisch
 */


#ifndef NDEF_PARSE_H_
#define NDEF_PARSE_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	TNF_Empty_Record		= 0x00,
	TNF_Well_Known_Record	= 0x01,
	TNF_MIME_Media_Record	= 0x02,
	TNF_Absolute_URI_Record = 0x03,
	TNF_Uxternal_Record		= 0x04,
	TNF_Unknown_Record		= 0x05,
	TNF_Unchanged_Record	= 0x06
}TNF_Types_t;

typedef enum {
	MESSAGE_BEGIN		= 0x80,
	MESSAGE_END			= 0x40,
	CHUNK_FLAG			= 0x20,
	SHORT_RECORD		= 0x10,	// Wenn Short Record, dann Payload-Länge immer ein Byte lang, ansonsten 4 Bytes.
	ID_LENGTH_FIELD		= 0x08,
	TYPE_NAME_FORMAT	= 0x07
}HEADER_Byte_Structure_t;

/* Problematisch
const struct {
	uint8_t URI_RECORD[1]; //0x55
	uint8_t TEXT_RECORD[1];
	uint8_t SMART_POSTER_RECORD[2];
}RECORD_TYPE_s = {'U','T',"Sp"};
*/

typedef struct{
	bool 			message_begin;
	bool 			message_end;
	bool 			chunk_flag;
	bool			short_record;
	bool 			id_field_available;
	TNF_Types_t 	tnf_type;
	const uint8_t * type_p;
	uint8_t 		type_length;
	uint8_t * 		id_p;
	uint8_t 		id_length;
	uint8_t * 		payload_p;
	uint32_t 		payload_length;
}ndef_record_typedef_s_t;

typedef struct{
	uint8_t * data_buffer_p;		// the whole ndef_record as byte array
	size_t data_length;		// Length of the ndef_record

	uint8_t type_length;	// Length of the type field
	size_t type_offset_pointer;		// Offset to access the type

    uint8_t id_length;		// Length of the ID-Field (if available)
    size_t id_offset_pointer;		// Offset to access the length field (if available)

    uint32_t payload_length;// Length of the payload
    size_t payload_offset_pointer;	// Offset to access the payload
}ndef_record_s_t; //Structure definition for storing of ndef_records
// [Header Byte ()] [Type Lengt TL] [Payload Length PL] [Id lengt IL (if enabled in Header)] TL*[Type] IL*[ID (If enabled)] PL*[Payload]

typedef struct{
	uint8_t nuber_of_records;
	ndef_record_s_t *ndef_records_p[]; // VLA of pointers to ndef_records. Defined with malloc. (Must be set free if unused!)
}ndef_message_records_s_t; // Whole NDEF message with "flexible array member" ndef_records[]
// NDEF MESSAGE Aufbau: [Start Sign 0x03] [Length ohne 0x03 und 0xFE] N*[Records[]] [End Sign 0xFE]

typedef struct{
	size_t array_length;
	uint8_t buffer_vla[]; //Variable Length Array / Flexible Array Member
}ndef_message_uint8_array_t;

/* Function to parse a ndef message given as byte-array with length */
ndef_record_s_t * parse_ndef_record(uint8_t buffer_p[]); // Achtung!!!: Hier könnte mehr gelesen werden als eigentlich im Record ist!!!

/* Function to create a ndef record */
ndef_record_s_t * create_ndef_record(ndef_record_typedef_s_t * nrt_p);

void free_ndef_record_struct_and_data_memory(ndef_record_s_t * record_to_free_p);
void free_ndef_record_struct(ndef_record_s_t * record_to_free_p);

ndef_message_records_s_t * parse_ndef_message(uint8_t buffer_p[], size_t length);

ndef_message_records_s_t * create_ndef_message(void);
ndef_message_records_s_t * add_record_to_ndef_message(ndef_message_records_s_t * ndef_message_p, ndef_record_s_t * record_to_add_p);

void free_ndef_message_and_records(ndef_message_records_s_t * ndef_message_p);

void free_ndef_message_uint8_array(ndef_message_uint8_array_t * ndef_message_uint8_array_p);

void free_ndef_records_of_message(ndef_message_records_s_t * ndef_message_p);

//Not possible to assemble ndef_message_p to uint8_t array as VLA(of records) must be last but also "THMS2NDEF_END_SIGN"
// Therefore this "assemble function" due to costs of RAM
ndef_message_uint8_array_t * assemble_ndefMessage_to_uint8_array(ndef_message_records_s_t * ndef_message_p);

bool assemble_ndefMessage_to_given_uint8_array(uint8_t * message_array_p, size_t message_max_length, size_t * size_of_written_message_array, ndef_message_records_s_t * ndef_message_p) ;


#endif /* NDEF_PARSE_H_ */
