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


#include "uv_pwm.h"


#if CONFIG_PWM



uv_errors_e _uv_pwm_init() {


	return ERR_NONE;
}


uv_errors_e uv_pwmext_module_init(
		uint8_t module_index,
		void *module_ptr,
		void (*set_callb)(void *module_ptr, uint32_t chn, uint16_t value),
		uint16_t (*get_callb)(void *module_ptr, uint32_t chn),
		void (*freq_callb)(void *module_ptr, uint32_t chn, uint32_t freq)) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}



uv_errors_e uv_pwm_set(uv_pwm_channel_t chn, uint16_t value) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uint16_t uv_pwm_get(uv_pwm_channel_t chn) {
	uint16_t ret = 0;

	return ret;
}



void uv_pwm_set_freq(uv_pwm_channel_t chn, uint32_t value) {
}



#endif
