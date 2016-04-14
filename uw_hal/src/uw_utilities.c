/*
 * uw_utilities.c
 *
 *  Created on: Feb 18, 2015
 *      Author: usenius
 */


#include "uw_utilities.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#if CONFIG_TARGET_LPC11CXX
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC178X
#include "LPC177x_8x.h"
#endif


void *user_ptr = NULL;




void uw_start_delay(unsigned int delay_ms, int* p) {
	*p = delay_ms;
}


bool uw_delay(unsigned int step_ms, int* p) {
	if (*p >= step_ms) {
		*p -= step_ms;
		return false;
	}
	*p = -1;
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


void Debug_PrintMessage (uw_can_message_st* msg)
{
	int msgclass = msg->id & 0xFF80;
	int nodeid   = msg->id & 0x007F;
	int i;

	switch (msgclass)
	{
	case 0x000:	printf ("nmt"); break;
	case 0x180:	printf ("pdo"); break;
	case 0x580:	printf ("sdo"); break;
	case 0x700: printf("heartbeat"); break;
	default:	printf ("%03x", msgclass); break;
	}

	printf (", node: %3u", nodeid);

	printf (", data:");
	for (i = 0; i < msg->data_length; i++)
		printf(" %u", msg->data_8bit[i]);

	printf("\r\n");
}



void uw_set_application_ptr(void *ptr) {
	user_ptr = ptr;
}


char *uw_get_hardware_name() {
#if CONFIG_TARGET_LPC11CXX
	return "CONFIG_TARGET_LPC11CXX";
#elif CONFIG_TARGET_LPC178X
	return "CONFIG_TARGET_LPC178X";
#else
	#error "Error: Hardware name not specified in uw_utilities.c"
#endif
}


uw_errors_e uw_ring_buffer_init(uw_ring_buffer_st *buffer_ptr, void *buffer,
		uint16_t buffer_size, uint8_t element_size) {
	buffer_ptr->buffer = buffer;
	buffer_ptr->buffer_size = buffer_size;
	buffer_ptr->element_count = 0;
	buffer_ptr->element_size = element_size;
	buffer_ptr->head = buffer_ptr->tail = buffer_ptr->buffer;
	return uw_err(ERR_NONE);
}


uw_errors_e uw_ring_buffer_push(uw_ring_buffer_st *buffer, void *element) {
	if (buffer->element_count == buffer->buffer_size) {
		return uw_err(ERR_BUFFER_OVERFLOW | HAL_MODULE_UTILITIES);
	}
	uint8_t i;
	for (i = 0; i < buffer->element_size; i++) {
		*(buffer->head) = *((char*) element + i);
		buffer->head++;
	}
	if (buffer->head == buffer->buffer + buffer->buffer_size * buffer->element_size) {
		buffer->head = buffer->buffer;
	}
	buffer->element_count++;
	return uw_err(ERR_NONE);
}


uw_errors_e uw_ring_buffer_pop(uw_ring_buffer_st *buffer, void *dest) {
	if (!buffer->element_count) {
		return uw_err(ERR_BUFFER_EMPTY | HAL_MODULE_UTILITIES);
	}
	uint8_t i;
	for (i = 0; i < buffer->element_size; i++) {
		*((char*)dest + i) = *(buffer->tail);
		buffer->tail++;
	}
	if (buffer->tail == buffer->buffer + buffer->buffer_size * buffer->element_size) {
		buffer->tail = buffer->buffer;
	}
	buffer->element_count--;
	return uw_err(ERR_NONE);
}





uw_errors_e uw_set_int_priority(uw_int_sources_e src, unsigned int priority) {
	if (priority <= INT_LOWEST_PRIORITY) {
		NVIC_SetPriority(src, priority);
		return uw_err(ERR_NONE);
	}
	__uw_err_throw(ERR_INT_LEVEL | HAL_MODULE_UTILITIES);
}



void *__uw_get_user_ptr() {
	return user_ptr;
}
