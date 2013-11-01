/*
 * COM_Parser.c
 *
 *   Created on: Sep 5, 2013
 *       Author: Rasmus Koldsø 
 * Organisation: University of Southern Denmark - MCI
 */

#include "COM_Parser.h"


/*************************************************************************
 *  Input parameters:
 *  datagram_t *datagram	- Pointer to a cleared datagram
 *  char *data			- Char data array
 *  int length			- Number of bytes in char data array
 *                                Make sure at least 1 byte is available
 *  int *offset                 - Offset to start from in data array
 *  int parserState		- current parser state of type parserState_t
 *
 *  Output parameters:
 *  ssize_t 			- New parser state
 *************************************************************************/
int ddCount;
ssize_t COM_parse_data(datagram_t *datagram, char *data, int length, int *offset, int _parserState)
{
	int _continue = 1;

	while(_continue) {
		switch(_parserState){
		case package_type_token:
			if(data[*offset] == Command){
//				printf("Type:\t%02X Command\n", data[*offset] & 0xff);
				fprintf(stderr, "Command type datagram received, but is not implemented\n");
				_continue = 0;
			}
			else if(data[*offset] == Event) {
//				printf("Type:\t%02X Event\n", data[*offset] & 0xff);
				datagram->type = Event;
				(*offset)++;
				_parserState = event_code_token;
			}
			else {
				fprintf(stderr, "Unknown type token: %02X\n", data[*offset] & 0xff);
				_continue = 0;
			}
			break;

		case event_code_token:
//			printf("Opcode:\t%02X\n", data[*offset] & 0xff);
			datagram->opcode = (long)data[(*offset)++];
			_parserState = length_token;
			break;

		case length_token:
//			printf("Length:\t%02X (%d)\n", data[*offset] & 0xff, (int)data[*offset]);
			datagram->data_length = (unsigned int)data[(*offset)++];
			_parserState = data_token;
			break;

		case data_token:
//			printf("Data:\t");

			if( length > STD_DATAGRAM_SIZE ) {
				fprintf(stderr, "Too much data (%d bytes) for storage buffer (%d bytes).\n", length, STD_DATAGRAM_SIZE);
				_continue = 0;
				break;
			}

			while(ddCount < datagram->data_length && (*offset) < length) {
//				printf("%02X ", data[*offset] & 0xff);
				datagram->data[ddCount++] = data[(*offset)++];

			}
//			printf("ddCount=%d\n", ddCount);

			if(ddCount == datagram->data_length) {
				ddCount = 0; // Clear counter for data_token loop
				_parserState = package_type_token;
			}
			break;

		default:
			/* Should never go here */
			_parserState = package_type_token;
			_continue = 0;
			break;
		}

		if(*offset >= length) {
			_continue = 0;
		}
	}
	return _parserState;
}

/*************************************************************************
 *  Input parameters:
 *  datagram_t *datagram	- Filled datagram to convert to char array
 *  char data[]			- Char array big enough to hold datagram
 *  int *length			- Number of bytes filled in char array
 *
 *  Output parameters:
 *  int				- Success (0)
 *************************************************************************/
int COM_compose_datagram(datagram_t *datagram, char data[], int *length)
{
	int i=0, j;

	*length = datagram->data_length + 3;
	if(datagram->type == Command) {
		(*length)++;
	}

	if(*length > STD_DATAGRAM_SIZE) {
		return -ENOMEM;
	}

	data[i++] = datagram->type;
	data[i++] = (char)datagram->opcode;

	if(datagram->type == Command) {
		data[i++] = (char)(datagram->opcode >> 8);
	}

	data[i++] = (char)datagram->data_length;

	for(j=0; j<datagram->data_length; j++) {
		data[i++] = datagram->data[j];
	}

	return 0;
}

void pretty_print_datagram(datagram_t *datagram)
{
#if (defined VERBOSITY) && (VERBOSITY >= 3)
	int i;

	printf("Type:\t(%02X) %s\n", datagram->type & 0xff, datagram->type==Command?"Command":"Event");

	printf("Opcode:\t(");
	if(datagram->type == Command)
		printf("%02X ", (char)(datagram->opcode >> 8) & 0xff);
	printf("%02X)\n", (char)datagram->opcode & 0xff);

	printf("Length:\t(%02X) %d bytes\n", datagram->data_length & 0xff, datagram->data_length);

	printf("Data:\t(");
	for(i=0; i<datagram->data_length; i++)
		printf("%02X%s", datagram->data[i] & 0xff, i<datagram->data_length-1?" ":")\n");
#endif
}

