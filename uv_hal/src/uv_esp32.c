/*
 * uv_esp32.c
 *
 *  Created on: Jan 21, 2026
 *      Author: usevolt
 */

#include "uv_esp32.h"

#if CONFIG_ESP32

static void rxtx_task(void *me_ptr) {
	// initialize ESP32

	// start rxtx loop
	while (true) {
		// send if anything is in tx queue
		// check if there's anything to receive
	}
}


uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_esp32_conf_st *conf,
		uv_gpios_e reset_io,
		uv_uarts_e uart) {
	this->conf = conf;
	this->uart = uart;
	this->reset_io = reset_io;
	this->state = ESP32_STATE_INIT;

	uv_rtos_task_create(&rxtx_task, "ESP",
						UV_RTOS_MIN_STACK_SIZE * 2,
						this,
						UV_RTOS_IDLE_PRIORITY + 1,
						NULL);

	return ERR_NONE;
}


/// @brief: Step function
void uv_esp32_step(uv_esp32_st *this, uint16_t step_ms) {

}











void uv_esp32_conf_reset(uv_esp32_conf_st *conf) {
	memset(conf, 0, sizeof(*conf));
}


uv_errors_e uv_esp32_get_data(uv_esp32_st *this, char *dest) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uv_errors_e uv_esp32_write(uv_esp32_st *this,
		char *data, uint16_t datalen, int32_t wait_ms) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uv_errors_e uv_esp32_write_isr(uv_esp32_st *this,
		char *data, uint16_t datalen) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uint64_t uv_esp32_get_mac(uv_esp32_st *this) {
	return 0;
}


char *uv_esp32_get_connected_ssid(uv_esp32_st *this) {
	return "";
}


void uv_esp32_network_reset(uv_esp32_st *this) {
}


void uv_esp32_network_leave(uv_esp32_st *this) {
}


void uv_esp32_network_join(uv_esp32_st *this, char ssid[32],
						   char passwd[64]) {
}


void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv) {
	printf("ESP32:\n");
}


#endif
