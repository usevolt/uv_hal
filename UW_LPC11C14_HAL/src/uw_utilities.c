/*
 * uw_utilities.c
 *
 *  Created on: Feb 18, 2015
 *      Author: usenius
 */


#include "uw_utilities.h"
#include "uw_messages.h"
#include <stdint.h>
#include <string.h>


int uw_atoi (const char* str) {
	int i;
	int j;
	int result = 0;
	int base = 10;
	if (strstr(str, "0x\0") != NULL) {
		int k;
		for (k = 0; str[k] != '\0'; k++) {
			if (str[k] == 'x') {
				str = &str[k + 1];
				break;
			}
		}
		base = 16;
	}

	//calculate length of received value
	for (i = 0; str[i] != '\0'; i++) { }
	j = --i;
	// count backwards
	for (; i >= 0; i--) {
		uint8_t number;
		// convert hex characters into numbers
		if ( str[i] >= 'a' && str[i] <= 'f') {
			number = str[i] - 'a' + 10;
		}
		else if (str[i] >= 'A' && str[i] <= 'F') {
			number = str[i] - 'A' + 10;
		}
		else {
			number = str[i] - '0';
		}
		int p;
		int mult = 1;
		for (p = 0; p < j - i; p++) {
			mult *= base;
		}
		result += number * mult;
	}
	return result;
}


void uw_start_delay(unsigned int delay_ms, int* p) {
	*p = delay_ms;
}


bool uw_delay(unsigned int step_ms, int* p) {
	if (*p > 0) {
		*p -= step_ms;
		return false;
	}
	return true;
}


bool Debug_ParamOnOff (const char* arg, bool current, const char* param)
{
	bool state = true - current;

	if (strcmp(arg, "on") == 0)
		state = true;
	else if (strcmp(arg, "off") == 0)
		state = false;

	printf ("turning %s %s\r\n", param, state ? "on" : "off");

	return state;
}


void Debug_PrintMessage (hal_can_msg_obj_st* msg)
{
	int msgclass = msg->msg_id & 0xFF80;
	int nodeid   = msg->msg_id & 0x007F;
	int i;

	printf ("msg obj: %2u, class: ", msg->msgobj);

	switch (msgclass)
	{
	case 0x000:	printf ("nmt"); break;
	case 0x180:	printf ("pdo"); break;
	case 0x580:	printf ("sdo"); break;
	default:	printf ("%03x", msgclass); break;
	}

	printf (", node: ");

	switch (nodeid)
	{
	case MSB_DEFAULT_NODE_ID:		printf ("msb"); break;
	case CSB_DEFAULT_NODE_ID:		printf ("csb"); break;
	case KEYPAD_DEFAULT_NODE_ID:	printf ("kpd"); break;
	default:						printf ("%3u", nodeid); break;
	}

	printf (", data:");
	for (i = 0; i < msg->data_length; i++)
		printf(" %u", msg->data[i]);

	printf("\r\n");
}

