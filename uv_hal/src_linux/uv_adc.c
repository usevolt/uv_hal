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

#include "uv_adc.h"

#include <stdio.h>
#include "uv_uart.h"
#include "uv_rtos.h"


#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1



uv_errors_e _uv_adc_init() {

	return ERR_NONE;
}



int16_t uv_adc_read(uv_adc_channels_e channel) {
	int16_t ret = -1;

	return ret;
}






int16_t uv_adc_read_average(uv_adc_channels_e channel, uint32_t conversion_count) {
	int32_t value = 0;

	return (int16_t) value;
}




void uv_adc_enable_ain(uv_adc_channels_e channel) {
}



void uv_adc_disable_ain(uv_adc_channels_e channel) {
}



uv_gpios_e uv_adc_get_gpio_pin(uv_adc_channels_e channel) {
	uv_gpios_e ret = 0;

	return ret;
}



#endif
