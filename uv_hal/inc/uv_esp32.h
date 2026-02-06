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



#define IPV6_STR_MAX_LEN	46
#define SSID_STR_MAX_LEN	33
#define PASSWD_STR_MAX_LEN	65

typedef struct {
	uint8_t flags;
	char ssid[SSID_STR_MAX_LEN];
	char passwd[PASSWD_STR_MAX_LEN];
	char destaddr_ipv6[IPV6_STR_MAX_LEN];
} uv_esp32_conf_st;

/// @brief: Resets the configuration structure
void uv_esp32_conf_reset(uv_esp32_conf_st *conf);


#define ESP32_TX_BUF_SIZE		(700)
#define ESP32_RX_BUF_SIZE		300



typedef enum {
	ESP32_STATE_INIT = 0,
	ESP32_STATE_JOINED_NETWORK,
	ESP32_STATE_LEFT_NETWORK
} uv_esp32_states_e;



/// @brief: Main struct for ESP32 wifi module
typedef struct {
	uv_esp32_conf_st *conf;

	uv_uarts_e uart;
	uv_gpios_e reset_io;

	uv_esp32_states_e state;
} uv_esp32_st;




/// @brief: Initializes the ESP32 module
///
/// @ref: ERR_NONE if initialized succesfully
///
/// @param nodeid: Node Identifier, custom string
uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_esp32_conf_st *conf,
		uv_gpios_e reset_io,
		uv_uarts_e uart);


/// @brief: Step function
void uv_esp32_step(uv_esp32_st *this, uint16_t step_ms);


static inline uv_esp32_states_e uv_esp32_state_get(uv_esp32_st *this) {
	return this->state;
}


/// @brief: Get data from connected device
uv_errors_e uv_esp32_get_data(uv_esp32_st *this, char *dest);

/// @brief: Writes data to ESP32
uv_errors_e uv_esp32_write(uv_esp32_st *this,
		char *data, uint16_t datalen, int32_t wait_ms);

uv_errors_e uv_esp32_write_isr(uv_esp32_st *this,
		char *data, uint16_t datalen);

/// @brief: Returns the ESP32 MAC address
uint64_t uv_esp32_get_mac(uv_esp32_st *this);

/// @brief: Returns the connected network's SSID
char *uv_esp32_get_connected_ssid(uv_esp32_st *this);


/// @brief: Resets the current network connection
void uv_esp32_network_reset(uv_esp32_st *this);

/// @brief: Leaves current network
void uv_esp32_network_leave(uv_esp32_st *this);

void uv_esp32_network_join(uv_esp32_st *this, char ssid[32],
						   char passwd[64]);



/// @brief: Parses the "esp" terminal command
void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv);

#endif

#endif /* UV_HAL_UV_HAL_INC_UV_ESP32_H_ */
