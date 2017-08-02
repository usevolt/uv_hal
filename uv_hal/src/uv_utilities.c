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



bool uv_delay(unsigned int step_ms, int* p) {
	if (p != NULL) {
		if (*p >= step_ms) {
			*p -= step_ms;
			return false;
		}
		if (*p == -1) {
			return false;
		}
		*p = -1;
	}
	return true;
}


char *uv_get_hardware_name() {
#if CONFIG_TARGET_LPC11C14
	return "CONFIG_TARGET_LPC11C14";
#elif CONFIG_TARGET_LPC1785
	return "CONFIG_TARGET_LPC1785";
#elif CONFIG_TARGET_LPC1549
	return "CONFIG_TARGET_LPC1549";
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
	return ERR_NONE;
}


uv_errors_e uv_ring_buffer_push(uv_ring_buffer_st *buffer, void *element) {
	uv_errors_e ret = ERR_NONE;

	if (buffer == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (buffer->element_count >= buffer->buffer_size) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		uint8_t i;
		for (i = 0; i < buffer->element_size; i++) {
			*(buffer->head) = *((char*) element + i);
			buffer->head++;
		}
		if (buffer->head == buffer->buffer + buffer->buffer_size * buffer->element_size) {
			buffer->head = buffer->buffer;
		}
		buffer->element_count++;
	}
	return ret;
}

uv_errors_e uv_ring_buffer_peek(uv_ring_buffer_st *buffer, void *dest) {
	uv_errors_e ret = ERR_NONE;

	if (buffer == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (!buffer->element_count) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		if (dest) {
			for (int16_t i = 0; i < buffer->element_size; i++) {
				*((char*)dest + i) = *(buffer->tail + i);
			}
		}
	}
	return ret;

}

uv_errors_e uv_ring_buffer_pop(uv_ring_buffer_st *buffer, void *dest) {
	uv_errors_e ret = ERR_NONE;

	if (buffer == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (!buffer->element_count) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		uint8_t i;
			for (i = 0; i < buffer->element_size; i++) {
				if (dest) {
					*((char*)dest + i) = *(buffer->tail);
				}
				buffer->tail++;
		}
		if (buffer->tail == buffer->buffer + buffer->buffer_size * buffer->element_size) {
			buffer->tail = buffer->buffer;
		}
		buffer->element_count--;
	}
	return ret;
}


void uv_vector_init(uv_vector_st *this, void *buffer,
		uint16_t buffer_size, uint16_t element_size) {
	this->len = 0;
	this->element_size = element_size;
	this->buffer_size = buffer_size;
	this->buffer = buffer;
}


uv_errors_e uv_vector_push_back(uv_vector_st *this, void *data) {
	uv_errors_e ret = ERR_NONE;

	if (this == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (this->len >= this->buffer_size) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		if (data) {
			memcpy(&this->buffer[this->len * this->element_size], data, this->element_size);
		}
		this->len++;
	}
	return ret;
}


uv_errors_e uv_vector_push_front(uv_vector_st *this, void *data) {
	uv_errors_e ret = ERR_NONE;
	if (this == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (this->len >= this->buffer_size) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		memmove(this->buffer + this->element_size, this->buffer, this->element_size);
		memcpy(this->buffer, data, this->element_size);
		this->len--;
	}
	return ret;
}

uv_errors_e uv_vector_insert(uv_vector_st *this, uint16_t index, void *src) {
	uv_errors_e ret = ERR_NONE;

	if (this == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (this->len >= this->buffer_size) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else if (this->len <= index) {
		ret = ERR_INDEX_OVERFLOW;
	}
	else {
		memmove(this->buffer + index * this->element_size + this->element_size,
				this->buffer + index * this->element_size, this->element_size * (this->len - index));
		memcpy(this->buffer + index * this->element_size, src, this->element_size);
		this->len++;
	}

	return ret;
}


uv_errors_e uv_vector_pop_back(uv_vector_st *this, void *data) {
	uv_errors_e ret = ERR_NONE;

	if (this == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (!this->len) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		if (data) {
			memcpy(data, &this->buffer[(this->len - 1) * this->element_size], this->element_size);
		}
		this->len--;
	}
	return ret;
}


uv_errors_e uv_vector_pop_front(uv_vector_st *this, void *data) {
	uv_errors_e ret = ERR_NONE;

	if (this == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (!this->len) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		if (data) {
			memcpy(data, this->buffer, this->element_size);
		}
		memmove(this->buffer, this->buffer + this->element_size, this->element_size);
		this->len--;
	}
	return ret;
}



uv_errors_e uv_vector_remove(uv_vector_st *this, uint16_t index, uint16_t count) {
	uv_errors_e ret = ERR_NONE;

	if (this == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (this->len < index + count) {
		ret = ERR_INDEX_OVERFLOW;
	}
	else {
		if (count > 0) {
			memmove(this->buffer + index * this->element_size,
					this->buffer + index * this->element_size + count * this->element_size,
					this->element_size * (this->len - index - (count - 1)));
			this->len -= count;
		}
	}
	return ret;
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
int uv_lerpi(int t, int a, int b) {
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

