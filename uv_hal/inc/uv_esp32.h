/*
 * esp32.h
 *
 *  Created on: Jan 21, 2026
 *      Author: usevolt
 */

#ifndef UV_HAL_UV_HAL_INC_UV_ESP32_H_
#define UV_HAL_UV_HAL_INC_UV_ESP32_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_uart.h"
#include "uv_gpio.h"
#include "uv_rtos.h"
#include "uv_terminal.h"


#if CONFIG_ESP32 && CONFIG_UART

#define ESP32_UART_BAUDRATE		921600




typedef struct {

} uv_esp32_conf_st;

/// @brief: Resets the configuration structure
void uv_esp32_conf_reset(uv_esp32_conf_st *conf);


#define ESP32_TX_BUF_SIZE		(700)
#define ESP32_RX_BUF_SIZE		300


/// @brief: Main struct for ESP32 wifi module
typedef struct {
	uv_esp32_conf_st *conf;

	uv_uarts_e uart;
	uv_gpios_e reset_io;
	uv_gpios_e rts_io;
	uv_gpios_e cts_io;
} uv_esp32_st;




/// @brief: Initializes the ESP32 module
///
/// @ref: ERR_NONE if initialized succesfully
///
/// @param nodeid: Node Identifier, custom string
uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_esp32_conf_st *conf,
		uv_gpios_e reset_gpio,
		uv_uarts_e uart);


/// @brief: Step function
void uv_esp32_step(uv_esp32_st *this, uint16_t step_ms);



/// @brief: Parses the "esp" terminal command
void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv);

#endif

#endif /* UV_HAL_UV_HAL_INC_UV_ESP32_H_ */
