/*
 * uv_utilities.c
 *
 *  Created on: Feb 18, 2015
 *      Author: usenius
 */


#include "uv_utilities.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


void *user_ptr = NULL;



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


void Debug_PrintMessage (uv_can_message_st* msg)
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



void uv_set_application_ptr(void *ptr) {
	user_ptr = ptr;
}


char *uv_get_hardware_name() {
#if CONFIG_TARGET_LPC11C14
	return "CONFIG_TARGET_LPC11C14";
#elif CONFIG_TARGET_LPC1785
	return "CONFIG_TARGET_LPC1785";
#else
	#error "Error: Hardware name not specified in uv_utilities.c"
#endif
}


uv_errors_e uv_ring_buffer_init(uv_ring_buffer_st *buffer_ptr, void *buffer,
		uint16_t buffer_size, uint8_t element_size) {
	buffer_ptr->buffer = buffer;
	buffer_ptr->buffer_size = buffer_size;
	buffer_ptr->element_count = 0;
	buffer_ptr->element_size = element_size;
	buffer_ptr->head = buffer_ptr->tail = buffer_ptr->buffer;
	return uv_err(ERR_NONE);
}


uv_errors_e uv_ring_buffer_push(uv_ring_buffer_st *buffer, void *element) {
	if (buffer->element_count >= buffer->buffer_size) {
		return uv_err(ERR_BUFFER_OVERFLOW | HAL_MODULE_UTILITIES);
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
	return uv_err(ERR_NONE);
}


uv_errors_e uv_ring_buffer_pop(uv_ring_buffer_st *buffer, void *dest) {
	if (!buffer->element_count) {
		return uv_err(ERR_BUFFER_EMPTY | HAL_MODULE_UTILITIES);
	}
	uint8_t i;
	if (dest) {
		for (i = 0; i < buffer->element_size; i++) {
			*((char*)dest + i) = *(buffer->tail);
			buffer->tail++;
		}
	}
	if (buffer->tail == buffer->buffer + buffer->buffer_size * buffer->element_size) {
		buffer->tail = buffer->buffer;
	}
	buffer->element_count--;
	return uv_err(ERR_NONE);
}





uv_errors_e uv_set_int_priority(uv_int_sources_e src, unsigned int priority) {
	if (priority <= INT_LOWEST_PRIORITY) {
		NVIC_SetPriority(src, priority);
		return uv_err(ERR_NONE);
	}
	__uv_err_throw(ERR_INT_LEVEL | HAL_MODULE_UTILITIES);
}



void *__uv_get_user_ptr() {
	return user_ptr;
}
