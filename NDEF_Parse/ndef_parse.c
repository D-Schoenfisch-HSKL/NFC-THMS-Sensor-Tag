
/*
 * thms_2_ndef.c
 *
 *  Created on: 12.07.2021
 *      Author: DavidSchönfisch
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ndef_parse.h>


#define THMS2NDEF_START_SIGN						0x03
#define THMS2NDEF_END_SIGN							0xFE


#ifndef NULL
#define NULL ((void *)0)
#endif


/* */
ndef_record_s_t * parse_ndef_record(uint8_t buffer_p[]) {

	ndef_record_s_t * new_record_p = malloc(sizeof(ndef_record_s_t));
	if (new_record_p == NULL){
		return NULL;
	} // Not enough space
	new_record_p->data_buffer_p = buffer_p;

	int n = 0; // Buffer Pointer

	uint8_t message_header = buffer_p[n++]; // 0

	new_record_p->type_length = buffer_p[n++]; // 1

	if(message_header & SHORT_RECORD) {
		new_record_p->payload_length = buffer_p[n++]; //2
	} else {
		new_record_p->payload_length = ((uint32_t) buffer_p[2]<<24) | ((uint32_t) buffer_p[3]<<16) | ((uint32_t) buffer_p[4]<<8) | buffer_p[5];
		n = 6;
	}

	if(message_header & ID_LENGTH_FIELD) {
		new_record_p->id_length = buffer_p[n++];
	} else {
		new_record_p->id_length = 0;
	}

	new_record_p->type_offset_pointer = n;

	new_record_p->id_offset_pointer = n + new_record_p->type_length;
	new_record_p->payload_offset_pointer = new_record_p->id_offset_pointer + new_record_p->id_length;
	new_record_p->data_length = new_record_p->payload_offset_pointer + new_record_p->payload_length;
	return new_record_p;
}

ndef_record_s_t * create_ndef_record(ndef_record_typedef_s_t * nrt_p) {
	// Speicher reservierung für Zuordnungsstruktur "new_record_p" (Memory Allocation)
	ndef_record_s_t * new_record_p = malloc(sizeof(ndef_record_s_t));
	if (new_record_p == NULL){
		return NULL;
	} // Not enough space

	// Calculate size of NDEF Record
	// [Header Byte ()]; [Type Lengt TL]; [Payload Length PL]; [Id lengt IL (if enabled in Header)]; TL*[Type]; IL*[ID (If enabled)]; PL*[Payload]
	// 1 Byte Header + 1 Byte type Length + 1/4 Byte Payload Length + (if ID) 1 Byte Id_Lengthfield
	// + (If Id) IDlength * 1Byte + TypeLength* 1Byte + PayloadLength * 1Byte
	new_record_p->data_length = 1 + 1 + (nrt_p->short_record ? 1: 4) + (nrt_p->id_field_available ? (1+nrt_p->id_length) : 0) + nrt_p->payload_length + nrt_p->type_length;

	// Allocate Memory for NDEF-Record Buffer.
	new_record_p->data_buffer_p = malloc(sizeof(uint8_t) * new_record_p->data_length);
	if (new_record_p->data_buffer_p == NULL){
		free(new_record_p);
		return NULL;
	} // Not enough space

	int n = 0; // Buffer Pointer

	new_record_p->data_buffer_p[n++] = (nrt_p->message_begin ? MESSAGE_BEGIN : 0x00) 		// n = 0
									| (nrt_p->message_end ? MESSAGE_END : 0x00)
									| (nrt_p->chunk_flag ? CHUNK_FLAG : 0x00)
									| (nrt_p->short_record ? SHORT_RECORD : 0x00)
									| (nrt_p->id_field_available ? ID_LENGTH_FIELD : 0x00)
									| nrt_p->tnf_type ;

	//Type Length
	new_record_p->type_length = nrt_p->type_length;
	new_record_p->data_buffer_p[n++] =  nrt_p->type_length; 		// n = 1

	//Payload Length
	new_record_p->payload_length = nrt_p->payload_length;
	if (nrt_p->short_record) {
		if(nrt_p->payload_length > 0xFF) { //>256
			free(new_record_p);
			free(new_record_p->data_buffer_p);
			return NULL;
		} // Tag to large for short-record;
		new_record_p->data_buffer_p[n++] = nrt_p->payload_length; //n = 2
	} else {
		new_record_p->data_buffer_p[n++] = (uint8_t) ((nrt_p->payload_length >> 24) & 0xFF); //n = 2
		new_record_p->data_buffer_p[n++] = (uint8_t) ((nrt_p->payload_length >> 16) & 0xFF); //n = 3
		new_record_p->data_buffer_p[n++] = (uint8_t) ((nrt_p->payload_length >>  8) & 0xFF); //n = 4
		new_record_p->data_buffer_p[n++] = (uint8_t) ((nrt_p->payload_length	  ) & 0xFF); //n = 5
	}

	// ID Length
	new_record_p->id_length = (nrt_p->id_field_available ? nrt_p->id_length : 0);
	if(nrt_p->id_field_available) {
		new_record_p->id_length = nrt_p->id_length;
		new_record_p->id_offset_pointer = n;
		new_record_p->data_buffer_p[n++] = nrt_p->id_length; // n = 3/6
	} else {
		new_record_p->id_length = 0;
		new_record_p->id_offset_pointer = 0;
	}

	// Set pointer to records Type, ID and Payload
	new_record_p->type_offset_pointer = n;
	new_record_p->id_offset_pointer = new_record_p->type_offset_pointer + new_record_p->type_length;
	new_record_p->payload_offset_pointer = new_record_p->id_offset_pointer + new_record_p->id_length;
	new_record_p->data_length = new_record_p->payload_offset_pointer + new_record_p->payload_length;

	// Set Type Record
	memcpy(&new_record_p->data_buffer_p[new_record_p->type_offset_pointer],nrt_p->type_p,(size_t) nrt_p->type_length);
	// Set ID Record
	if(nrt_p->id_field_available) {
		memcpy(&new_record_p->data_buffer_p[new_record_p->id_offset_pointer], nrt_p->id_p,(size_t) nrt_p->id_length);
	}
	// Set Payload Record
	memcpy(&new_record_p->data_buffer_p[new_record_p->payload_offset_pointer], nrt_p->payload_p,(size_t) nrt_p->payload_length);

	// ???? if (new_record_p->data_buffer_p)

	return new_record_p;
}

void free_ndef_record_struct_and_data_memory(ndef_record_s_t * record_to_free_p) {
	// Free data Buffer
	free(record_to_free_p->data_buffer_p);
	// Free record structure
	free(record_to_free_p);
}

void free_ndef_record_struct(ndef_record_s_t * record_to_free_p) {
	// Free record structure
	free(record_to_free_p);
}


ndef_message_records_s_t * parse_ndef_message(uint8_t buffer_p[], size_t length) {
	// Check if valid NDEF Message;
	ndef_message_records_s_t * new_message_s;
	if((buffer_p[0] == THMS2NDEF_START_SIGN)&(buffer_p[length-1] == THMS2NDEF_END_SIGN)){
		uint8_t nuber_of_records = 0;

		new_message_s = create_ndef_message();
		if (new_message_s == NULL) {return NULL;}

		//new_message_s->messsage_length = buffer_p[1];

		uint8_t message_pointer = 2;
		while(message_pointer<(length-2)) {  //sizeof(ndef_record_s_t)
			new_message_s = add_record_to_ndef_message(new_message_s,parse_ndef_record(buffer_p + message_pointer)); // < alternative
			/* new_message_s = (ndef_message_s_t*) realloc(new_message_s, sizeof(ndef_message_s_t) + (nuber_of_records+1)*sizeof(ndef_record_s_t) );
			if (new_message_s == NULL) {return NULL;}
			new_message_s->ndef_records_p[nuber_of_records] = parse_ndef_record(buffer_p + message_pointer); */ //If alternative does not work use this
			if (new_message_s == NULL) {return NULL;}
			message_pointer = message_pointer + new_message_s->ndef_records_p[nuber_of_records]->data_length;
			nuber_of_records++;
			if(new_message_s->ndef_records_p[nuber_of_records] == NULL) {
				break;
			}
		}
		new_message_s->nuber_of_records = nuber_of_records;
	} else {
		return NULL;
	}
	return new_message_s;
}

ndef_message_records_s_t * create_ndef_message(void) {
	ndef_message_records_s_t * ndefmessage_p;
	ndefmessage_p = malloc(sizeof(ndef_message_records_s_t));
	ndefmessage_p->nuber_of_records=0;
	return ndefmessage_p;
}

ndef_message_records_s_t * add_record_to_ndef_message(ndef_message_records_s_t * ndef_message_p, ndef_record_s_t * record_to_add_p) {
	uint8_t number_of_records = ndef_message_p->nuber_of_records;
	ndef_message_records_s_t * new_message_s_p = (ndef_message_records_s_t*) realloc(ndef_message_p, sizeof(ndef_message_records_s_t) + (number_of_records+1)*sizeof(ndef_record_s_t));
	if (new_message_s_p == NULL) {return NULL;}
	new_message_s_p->ndef_records_p[number_of_records+1] = record_to_add_p;
	ndef_message_p->nuber_of_records++;
	return new_message_s_p; //
}

void free_ndef_message_and_records(ndef_message_records_s_t * ndef_message_p) {
	int i=0;
	while(i<ndef_message_p->nuber_of_records) {
		free_ndef_record_struct_and_data_memory(ndef_message_p->ndef_records_p[i]);
		i++;
	}
	free(ndef_message_p);
}

void free_ndef_records_of_message(ndef_message_records_s_t * ndef_message_p) {
	int i=0;
	while(i<ndef_message_p->nuber_of_records) {
		free_ndef_record_struct_and_data_memory(ndef_message_p->ndef_records_p[i]);
		i++;
	}
}


/*
 * Creates a uint8_t NDEF message from an NDEF structure (ndef_message_s_t).
 * @param ndef_message_p : Pointer to the NDEF-Message Structure given as "ndef_message_s_t" type
 * @return : The NDEF-Message given as uint8_t variable length array. Must be set free if no longer needed (free_ndef_message_uint8_array).
 */
ndef_message_uint8_array_t * assemble_ndefMessage_to_uint8_array(ndef_message_records_s_t * ndef_message_p){
	ndef_message_uint8_array_t * ndef_message_uint8_array_p =  malloc(sizeof(ndef_message_uint8_array_t));
	if (ndef_message_uint8_array_p == NULL) {return NULL;}
	ndef_message_uint8_array_p->array_length = 0;
	int buffer_write_pointer = 0;

	// Make space for start sign > Add start sign >
	ndef_message_uint8_array_p->array_length += 1;
	ndef_message_uint8_array_p = realloc(ndef_message_uint8_array_p, sizeof(ndef_message_uint8_array_t) + ndef_message_uint8_array_p->array_length);
	if (ndef_message_uint8_array_p == NULL) {return NULL;}
	ndef_message_uint8_array_p->buffer_vla[buffer_write_pointer] = THMS2NDEF_START_SIGN;
	buffer_write_pointer = ndef_message_uint8_array_p->array_length; //Set Pointer after actual length

	// Write the records to uint8 array
	int i=0;
	while(i<ndef_message_p->nuber_of_records) {
		// Check size of next record
		size_t record_length = ndef_message_p->ndef_records_p[i]->data_length;
		// Increase array_length and size of struct
		ndef_message_uint8_array_p->array_length += record_length;
		ndef_message_uint8_array_p = realloc(ndef_message_uint8_array_p, sizeof(ndef_message_uint8_array_t) + ndef_message_uint8_array_p->array_length);
		if (ndef_message_uint8_array_p == NULL) {return NULL;}
		// Add record to buffer
		memcpy(&ndef_message_uint8_array_p->buffer_vla[buffer_write_pointer],ndef_message_p->ndef_records_p[i]->data_buffer_p,record_length);
		// set buffer pointer to end
		buffer_write_pointer = ndef_message_uint8_array_p->array_length;
	}


	// Make space for end sign > Add end sign
	ndef_message_uint8_array_p->array_length += 1;
	ndef_message_uint8_array_p = realloc(ndef_message_uint8_array_p, sizeof(ndef_message_uint8_array_t) + ndef_message_uint8_array_p->array_length);
	if (ndef_message_uint8_array_p == NULL) {return NULL;}
	ndef_message_uint8_array_p->buffer_vla[buffer_write_pointer] = THMS2NDEF_END_SIGN;

	// buffer_write_pointer = ndef_message_uint8_array->array_length;
	return ndef_message_uint8_array_p;

}

/*
 * Function to free the variable length array structure generated e.g. with "assemble_ndefMessage_to_uint8_array()"
 */
void free_ndef_message_uint8_array(ndef_message_uint8_array_t * ndef_message_uint8_array_p) {
	free(ndef_message_uint8_array_p->buffer_vla);
	free(ndef_message_uint8_array_p);
}

/*
 * To Be Debugged!!!! Does not work
 * Creates a uint8_t NDEF message from an NDEF structure (ndef_message_s_t) and writes it to given uint8_t array (message_array_p) with given length (message_max_length).
 * @param message_array_p : Pointer to array for assembled ndef uint8 array start
 * @param message_max_length : Length of message_array_p. If assembled ndef message would be longer, the function returns false.
 * @param ndef_message_p : Input of the NDEF message to be assembled.
 * @return : Returns true if assembling was OK, false if there was an error.
 */
bool assemble_ndefMessage_to_given_uint8_array(uint8_t * message_array_p, size_t message_max_length, size_t * size_of_written_message_array, ndef_message_records_s_t * ndef_message_p) {
	int buffer_write_pointer = 0;
	if((buffer_write_pointer+1)>message_max_length) {return false;} // Check for space
	message_array_p[buffer_write_pointer] = THMS2NDEF_START_SIGN;
	buffer_write_pointer++; //Set Pointer after start sign

	// Write the records to uint8 array
	int i=0;

    //PRINTF("OK1 \r\n");
	while(i < (ndef_message_p->nuber_of_records)) {
		// Check size of next record
		ndef_record_s_t * actual_record_p = (ndef_record_s_t *) ndef_message_p->ndef_records_p[i];
		// WTF!!!!!!!!!!!! HIER GEHT WAS NICHT   !???????!?!?!!
		size_t record_length = actual_record_p->data_length;
		// Check size of output array
		if((buffer_write_pointer+record_length)>message_max_length) {return false;} // Check for space
		// Add record to array
		memcpy(&message_array_p[buffer_write_pointer],ndef_message_p->ndef_records_p[i]->data_buffer_p,record_length);
		// set buffer pointer to end
		buffer_write_pointer += record_length;
	}

	// Add end sign
	if((buffer_write_pointer+1)>message_max_length) {return false;} // Check for space
	message_array_p[buffer_write_pointer] = THMS2NDEF_END_SIGN;
	buffer_write_pointer++;

	*size_of_written_message_array = buffer_write_pointer;

	return true;
}
