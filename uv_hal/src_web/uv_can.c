/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2026 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2026 usevolt
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


/*
 * CAN on a web browser build: there is none. A page cannot reach a CAN
 * interface, so this is the interface of the Linux build answering that no
 * device is there - nothing is received, nothing goes out, and the device list
 * is empty. It lets the CANopen stack and the code built on it link unchanged.
 */

#include "uv_can.h"
#include <string.h>
#include <sys/time.h>

#if CONFIG_CAN && CONFIG_TARGET_WEB


static char dev_name[UV_CAN_IFNAME_MAX_LEN] = "none";
static unsigned int dev_baudrate = CONFIG_CAN0_BAUDRATE;


uv_errors_e _uv_can_init() {
	return ERR_NONE;
}


uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id, unsigned int mask, uv_can_msg_types_e type) {
	(void) channel;
	(void) id;
	(void) mask;
	(void) type;
	return ERR_NONE;
}


uv_errors_e uv_can_config_rx_message_no_callb(uv_can_channels_e chn,
		unsigned int id, unsigned int mask, uv_can_msg_types_e type) {
	(void) chn;
	(void) id;
	(void) mask;
	(void) type;
	return ERR_NONE;
}


uv_errors_e uv_can_set_rx_all(uv_can_channels_e chn, bool value) {
	(void) chn;
	(void) value;
	return ERR_NONE;
}


bool uv_can_create_vcan(const char *name, char *err, size_t err_len) {
	(void) name;
	if ((err != NULL) && (err_len > 0U)) {
		strncpy(err, "There is no CAN in a web browser.", err_len - 1U);
		err[err_len - 1U] = '\0';
	}
	return false;
}


bool uv_can_delete_vcan(const char *name, char *err, size_t err_len) {
	return uv_can_create_vcan(name, err, err_len);
}


void uv_can_set_rx_msg_callbacks(void (*config_callb)(uv_can_channels_e chn,
		unsigned int id, unsigned int mask, uv_can_msg_types_e type),
		void (*clear_callb)(uv_can_channels_e chn)) {
	(void) config_callb;
	(void) clear_callb;
}


void uv_can_clear_rx_messages(uv_can_chn_e chn) {
	(void) chn;
}


uv_errors_e uv_can_send_flags(uv_can_channels_e chn, uv_can_msg_st *msg,
		can_send_flags_e flags) {
	(void) chn;
	(void) msg;
	(void) flags;
	return ERR_HARDWARE_NOT_SUPPORTED;
}


bool uv_can_tx_idle(uv_can_channels_e chn) {
	(void) chn;
	return true;
}


bool uv_can_tx_full(uv_can_channels_e chn) {
	(void) chn;
	return false;
}


uv_errors_e uv_can_pop_message(uv_can_channels_e channel,
		uv_can_message_st *message) {
	(void) channel;
	(void) message;
	return ERR_BUFFER_EMPTY;
}


uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	(void) channel;
	return CAN_ERROR_HARDWARE_NOT_AVAILABLE;
}


uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	(void) channel;
	(void) callback_function;
	return ERR_NONE;
}


uv_errors_e uv_can_add_tx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg,
				can_send_flags_e flags)) {
	(void) channel;
	(void) callback_function;
	return ERR_NONE;
}


uv_errors_e uv_can_reset(uv_can_channels_e channel) {
	(void) channel;
	return ERR_NONE;
}


void uv_can_clear_rx_buffer(uv_can_channels_e channel) {
	(void) channel;
}


void uv_can_set_dev(uv_can_channels_e can_dev) {
	if (can_dev != NULL) {
		strncpy(dev_name, can_dev, sizeof(dev_name) - 1U);
		dev_name[sizeof(dev_name) - 1U] = '\0';
	}
}


uv_can_channels_e uv_can_get_dev(void) {
	return dev_name;
}


bool uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate) {
	(void) channel;
	dev_baudrate = baudrate;
	return false;
}


unsigned int uv_can_get_baudrate(uv_can_channels_e channel) {
	(void) channel;
	return dev_baudrate;
}


unsigned int uv_can_get_netdev_baudrate(uv_can_channels_e channel) {
	(void) channel;
	return 0U;
}


struct timeval uv_can_get_rx_time(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
}


char *uv_can_get_device_name(int32_t i) {
	(void) i;
	return "";
}


int32_t uv_can_get_device_count(void) {
	return 0;
}


bool uv_can_is_connected(void) {
	return false;
}


void uv_can_use_gui_password(bool enable) {
	(void) enable;
}


char *uv_can_set_up(bool force_set_up) {
	(void) force_set_up;
	return "There is no CAN in a web browser.";
}


void uv_can_close(void) {
}


void _uv_can_hal_step(unsigned int step_ms) {
	(void) step_ms;
}


#endif
