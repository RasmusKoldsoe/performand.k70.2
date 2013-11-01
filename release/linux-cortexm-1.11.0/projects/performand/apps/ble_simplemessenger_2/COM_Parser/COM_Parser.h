/*
 * COM_Parser.h
 *
 *   Created on: Sep 5, 2013
 *       Author: Rasmus Koldsø
 * Organisation: University of Southern Denmark - MCI
 */

#ifndef PARSER_H_
#define PARSER_H_

#include "../HCI_Parser/HCI_Defs.h"

enum parserState_t {
	package_type_token,
	command_code_token,
	event_code_token,
	length_token,
	data_token
};

int COM_parse_data(datagram_t *datagram, char *data, int length, int *offset, int _parserState);
int compose_datagram(datagram_t *datagram, char data[], int *length);
void pretty_print_datagram(datagram_t *datagram);

#endif /* PARSER_H_ */
