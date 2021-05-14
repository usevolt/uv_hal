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


#include "uv_eeprom.h"
#include "uv_rtos.h"

#if CONFIG_EEPROM


#define FILENAME	STRINGIFY(__UV_PROJECT_NAME) ".eeprom"

static struct {

} _this;
#undef this
#define this (&_this)


uv_errors_e _uv_eeprom_init() {
	uv_errors_e ret = ERR_NONE;

	printf("Loading eeprom %s\n", FILENAME);
	printf("WARNING: EEPROM not implemented on Linux target");

	return ret;
}


uv_errors_e uv_eeprom_write(const void *data, uint16_t len, uint16_t eeprom_addr) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_eeprom_read(void *dest, uint16_t len, uint16_t eeprom_addr) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}





#if CONFIG_EEPROM_RING_BUFFER

void uv_eeprom_ring_buffer_init(const uint16_t entry_len) {

}


uv_errors_e uv_eeprom_ring_buffer_push_back(const void *src) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_eeprom_ring_buffer_push_back_force(const void *src) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_eeprom_ring_buffer_pop_back(void *dest) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_eeprom_ring_buffer_pop_front(void *dest) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_eeprom_ring_buffer_at(void *dest, uint16_t *eeprom_addr, uint16_t index) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uint32_t uv_eeprom_ring_buffer_get_count(void) {
	uint32_t ret = 0;

	return ret;
}

#endif


void uv_eeprom_clear(void) {

}


#endif



