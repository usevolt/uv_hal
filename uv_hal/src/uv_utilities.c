/*
 * uv_utilities.c
 *
 *  Created on: Feb 18, 2015
 *      Author: usenius
 */


#include "uv_utilities.h"
#include "uv_terminal.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


void *user_ptr = NULL;


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


void uv_vector_init(uv_vector_st *this, void *buffer,
		uint16_t buffer_size, uint16_t element_size) {
	this->len = 0;
	this->element_size = element_size;
	this->buffer_size = buffer_size;
	this->buffer = buffer;
}


uv_errors_e uv_vector_push_back(uv_vector_st *this, void *data) {
	if (this->len >= this->buffer_size) {
		return uv_err(ERR_BUFFER_OVERFLOW | HAL_MODULE_UTILITIES);
	}
	memcpy(&this->buffer[this->len * this->element_size], data, this->element_size);
	this->len++;
	return uv_err(ERR_NONE);
}


uv_errors_e uv_vector_push_front(uv_vector_st *this, void *data) {
	if (this->len >= this->buffer_size) {
		return uv_err(ERR_BUFFER_OVERFLOW | HAL_MODULE_UTILITIES);
	}
	memmove(this->buffer + this->element_size, this->buffer, this->element_size);
	memcpy(this->buffer, data, this->element_size);
	this->len--;
	return uv_err(ERR_NONE);
}

uv_errors_e uv_vector_insert(uv_vector_st *this, uint16_t index, void *src) {
	if (this->len >= this->buffer_size) {
		return uv_err(ERR_BUFFER_OVERFLOW | HAL_MODULE_UTILITIES);
	}
	if (this->len <= index) {
		return uv_err(ERR_INDEX_OVERFLOW | HAL_MODULE_UTILITIES);
	}
	memmove(this->buffer + index * this->element_size + this->element_size,
			this->buffer + index * this->element_size, this->element_size * (this->len - index));
	memcpy(this->buffer + index * this->element_size, src, this->element_size);
	this->len++;

	return uv_err(ERR_NONE);
}


uv_errors_e uv_vector_pop_back(uv_vector_st *this, void *data) {
	if (!this->len) {
		return uv_err(ERR_BUFFER_EMPTY | HAL_MODULE_UTILITIES);
	}
	if (data) {
		memcpy(data, &this->buffer[(this->len - 1) * this->element_size], this->element_size);
	}
	this->len--;
	return uv_err(ERR_NONE);
}


uv_errors_e uv_vector_pop_front(uv_vector_st *this, void *data) {
	if (!this->len) {
		return uv_err(ERR_BUFFER_EMPTY | HAL_MODULE_UTILITIES);
	}
	if (data) {
		memcpy(data, this->buffer, this->element_size);
	}
	memmove(this->buffer, this->buffer + this->element_size, this->element_size);
	this->len--;
	return uv_err(ERR_NONE);
}



uv_errors_e uv_vector_remove(uv_vector_st *this, uint16_t index) {
	if (this->len <= index) {
		return uv_err(ERR_INDEX_OVERFLOW | HAL_MODULE_UTILITIES);
	}
	memmove(this->buffer + index * this->element_size,
			this->buffer + index * this->element_size + this->element_size,
			this->element_size * (this->len - index));
	this->len--;
	return uv_err(ERR_NONE);
}



/// @brief: Linear interpolation for floating points.
///
/// @param t: "Lerping scale". Should be between 0.0f ... 1.0f
/// @param a: The value at t=0.0f
/// @param b: The value at t=1.0f
float uv_lerpf(float t, float a, float b) {
	return (1-t)*a + t*b;
}

/// @brief: Linear interpolation for integers.
///
/// @param t: "Lerping scale". Should be between 0 ... 1000
/// @param a: The value at t=0
/// @param b: The value at t=1000
float uv_lerpi(int t, int a, int b) {
	return ((1000-t)*a + t*b) / 1000;
}


/// @brief: Returns the relative value of t in relation to a and b.
/// Typical use case: a = min value, b = max value, t = value between. Returns the relative
/// value of t.
///
/// @note: Should be min <= t <= max and min != max
float uv_relf(float t, float min, float max) {
	if (min == max) return 0;
	return (t-min)/(max-min);
}


/// @brief: Returns the relative value (parts per thousand) of t in relation to a and b.
/// Typical use case: a = min value, b = max value, t = value between. Returns the relative
/// value of t.
///
/// @note: Should be min <= t <= max and min != max
int uv_reli(int t, int min, int max) {
	if (min == max) return 0;
	return 1000 * (t-min)/(max-min);
}

/// @brief: Returns the bigger argument
int uv_maxi(int a, int b) {
	return (a > b) ? a : b;
}

/// @brief: Returns the smaller argument
int uv_mini(int a, int b) {
	return (a < b) ? a : b;
}


void _delay_ms (uint16_t ms)
{
	volatile uint16_t delay;
	volatile uint32_t i;
	for (delay = ms; delay > 0; delay--) {
		// with code optimization this is not at all precise!
		for (i = 0; i < SystemCoreClock / 15000; i++){ __NOP(); }
	}
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


void NMI_Handler(void) {
	printf(CLRL "NMI\r");
	_delay_ms(100);
}
void HardFault_Handler(void) {
	printf(CLRL "HardFault\r");
	_delay_ms(100);
}
void MemManage_Handler(void) {
	printf(CLRL "MemManage\r");
	_delay_ms(100);
}
void BusFault_Handler(void) {
	printf(CLRL "BusFault\r");
	_delay_ms(100);
}
void UsageFault_Handler(void) {
	printf(CLRL "UsageFault\r");
	_delay_ms(100);
}
void IntDefaultHandler(void) {
	printf(CLRL "Default\r");
	_delay_ms(100);
}

