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



#include <uv_i2c.h>

#if CONFIG_I2C


uv_errors_e _uv_i2c_init(void) {


	return ERR_NONE;
}


uv_errors_e uv_i2cm_read(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_i2cm_write(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}




uv_errors_e uv_i2cm_write_async(i2c_e i2c,
		uint8_t *tx_buffer, uint16_t tx_len) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}




#endif
