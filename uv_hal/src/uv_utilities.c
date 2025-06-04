/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "uv_utilities.h"
#include "uv_terminal.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "uv_rtos.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


void *user_ptr = NULL;


void uv_set_application_ptr(void *ptr) {
	user_ptr = ptr;
}



bool uv_delay(uv_delay_st* p, uint16_t step_ms) {
	bool ret = true;
	if (p != NULL) {
		if (*p > step_ms) {
			*p -= step_ms;
			ret = false;
		}
		else if (*p <= 0) {
			// for rest of the time p is negative to indicate that
			// delay ended long time ago
			ret = (*p == 0) ? true : false;
			*p = -1;
		}
		else {
			// p is 0 for 1 step cycle when delay ends
			ret = false;
			*p = 0;
		}
	}
	return ret;
}


char *uv_get_hardware_name() {
#if CONFIG_TARGET_LPC15XX
	return "CONFIG_TARGET_LPC15XX";
#elif CONFIG_TARGET_LPC40XX
	return "CONFIG_TARGET_LPC40XX";
#elif CONFIG_TARGET_LINUX
	return "CONFIG_TARGET_LINUX";
#elif CONFIG_TARGET_WIN
	return "CONFIG_TARGET_WIN";
#else
	#error "Error: Hardware name not specified in uv_utilities.c"
#endif
}


uv_errors_e uv_ring_buffer_init(uv_ring_buffer_st *buffer_ptr, void *buffer,
		uint16_t buffer_size, uint16_t element_size) {
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
		if (element) {
			memcpy(buffer->head, element, buffer->element_size);
		}
		buffer->head += buffer->element_size;
		if (buffer->head == buffer->buffer + buffer->buffer_size * buffer->element_size) {
			buffer->head = buffer->buffer;
		}
		buffer->element_count++;
	}
	return ret;
}

void uv_ring_buffer_push_force(uv_ring_buffer_st *buffer, void *element) {
	if (uv_ring_buffer_is_full(buffer)) {
		uv_ring_buffer_pop(buffer, NULL);
	}
	uv_ring_buffer_push(buffer, element);
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
		if (dest) {
			memcpy(dest, buffer->tail, buffer->element_size);
		}
		buffer->tail += buffer->element_size;
		if (buffer->tail == buffer->buffer + buffer->buffer_size * buffer->element_size) {
			buffer->tail = buffer->buffer;
		}
		buffer->element_count--;
	}
	return ret;
}


uv_errors_e uv_ring_buffer_pop_front(uv_ring_buffer_st *buffer, void *dest) {
	uv_errors_e ret = ERR_NONE;

	if (buffer == NULL) {
		ret = ERR_NULL_PTR;
	}
	else if (!buffer->element_count) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		if (dest) {
			memcpy(dest, buffer->head, buffer->element_size);
		}
		buffer->head += buffer->element_size;
		if (buffer->head == buffer->buffer + buffer->buffer_size * buffer->element_size) {
			buffer->head = buffer->buffer;
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
	else if (index >= this->len) {
		uv_vector_push_back(this, src);
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
int32_t uv_reli(int32_t t, int32_t min, int32_t max) {
	if (min == max) return 0;
	return 1000 * (t-min)/(max-min);
}

/// @brief: Returns the bigger argument
int32_t uv_maxi(int32_t a, int32_t b) {
	return (a > b) ? a : b;
}

/// @brief: Returns the smaller argument
int32_t uv_mini(int32_t a, int32_t b) {
	return (a < b) ? a : b;
}


uint32_t uv_ctz(uint32_t a) {
	uint32_t c = 32; // c will be the number of zero bits on the right
	a &= - ((int32_t) a);
	if (a) c--;
	if (a & 0x0000FFFF) c -= 16;
	if (a & 0x00FF00FF) c -= 8;
	if (a & 0x0F0F0F0F) c -= 4;
	if (a & 0x33333333) c -= 2;
	if (a & 0x55555555) c -= 1;

	return c;
}


uint64_t uv_isqrt(uint64_t value) {
    uint64_t op  = value;
    uint64_t res = 0;
    uint64_t one = (1uLL << 62); // The second-to-top bit is set:
    // use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type

    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
    {
        one >>= 2;
    }

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }

    /* Do arithmetic rounding to nearest integer */
    if (op > res)
    {
        res++;
    }

    return res;
}



uint32_t uv_countofbit(uint32_t a, uint8_t bit) {
	uint32_t ret = 0;

	for (uint32_t i = 0; i < 32; i++) {
		if (!!(a & (1 << i)) == !!bit) {
			ret++;
		}
	}
	return ret;
}




void *__uv_get_user_ptr() {
	return user_ptr;
}

#if (CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC15XX || \
CONFIG_TARGET_LPC1785 || CONFIG_TARGET_LPC40XX)

void NMI_Handler(void) {
	printf("NMI\r");
}

#if defined(CONFIG_HARDFAULT_CALLBACK)
extern void CONFIG_HARDFAULT_CALLBACK (void);
#endif

extern uv_mutex_st printf_mutex;
void HardFault_Handler(void) {
#if defined(CONFIG_HARDFAULT_CALLBACK)
	CONFIG_HARDFAULT_CALLBACK ();
#endif
	uv_mutex_unlock(&printf_mutex);
	printf("HardFault\r");
}
void MemManage_Handler(void) {
	printf("MemManage\r");
}
void BusFault_Handler(void) {
	printf("BusFault\r");
}
void UsageFault_Handler(void) {
	printf("UsageFault\r");
}
void IntDefaultHandler(void) {
	printf("Default\r");
}

#endif

bool uv_isdigit(char c) {
	return (c >= '0') && (c <= '9');
}


uint64_t ntouint64(uint64_t netdata) {
	uint64_t ret = 0;

	ret |= (((uint64_t) ((uint8_t*) &netdata)[0]) << 56);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[1]) << 48);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[2]) << 40);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[3]) << 32);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[4]) << 24);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[5]) << 16);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[6]) << 8);
	ret |= (((uint64_t) ((uint8_t*) &netdata)[7]) << 0);

	return ret;
}


uint16_t ntouint16(uint16_t netdata) {
	uint16_t ret = 0;

	ret |= (((uint16_t) ((uint8_t*) &netdata)[0]) << 8);
	ret |= (((uint16_t) ((uint8_t*) &netdata)[1]) << 0);

	return ret;
}
