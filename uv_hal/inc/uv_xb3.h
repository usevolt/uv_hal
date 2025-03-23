/*
 * xb3.h
 *
 *  Created on: Dec 19, 2024
 *      Author: usevolt
 */

#ifndef UV_HAL_UV_HAL_INC_UV_XB3_H_
#define UV_HAL_UV_HAL_INC_UV_XB3_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_spi.h"
#include "uv_gpio.h"
#include "uv_rtos.h"


#if CONFIG_SPI


typedef enum {
	XB3_AT_RESPONSE_OK = 0,
	XB3_AT_RESPONSE_ERROR = 1,
	XB3_AT_RESPONSE_INVALID_COMMAND = 2,
	XB3_AT_RESPONSE_INVALID_PARAMETER = 3,
	XB3_AT_RESPONSE_COUNT
} uv_xb3_at_response_e;


typedef enum {
	XB3_MODEMSTATUS_POWERUP = 0x0,
	XB3_MODEMSTATUS_WDT = 0x1,
	XB3_MODEMSTATUS_JOINEDNETWORK = 0x2,
	XB3_MODEMSTATUS_LEFTNETWORK = 0x3,
	XB3_MODEMSTATUS_COORDINATORSTARTED = 0x6,
	XB3_MODEMSTATUS_NETWORKSECURITYKEYUPDATED = 0x7,
	XB3_MODEMSTATUS_NETWORKWOKEUP = 0xB,
	XB3_MODEMSTATUS_NETWORKWENTTOSLEEP = 0xC,
	XB3_MODEMSTATUS_VCCEXCEEDED = 0xD,
	XB3_MODEMSTATUS_REMOTEMANAGERCONNECTED = 0xE,
	XB3_MODEMSTATUS_REMOTEMANAGEDISCONNECTED = 0xF,
	XB3_MODEMSTATUS_MODEMCONFCHANGEDWHILEJOIN = 0x11,
	XB3_MODEMSTATUS_ACCESSFAULT = 0x12,
	XB3_MODEMSTATUS_FATALERROR = 0x13,
	XB3_MODEMSTATUS_COORDINATORDETECTEDPANIDCONFLICT = 0x3E,
	XB3_MODEMSTATUS_COORDINATORCHANGEDPANID = 0x3F,
	XB3_MODEMSTATUS_BLECONNECT = 0x32,
	XB3_MODEMSTATUS_BLEDISCONNECT = 0x33,
	XB3_MODEMSTATUS_ROUTERPANIDCHANGEDBYCOORDINATOR = 0x40,
	XB3_MODEMSTATUS_NETWORKWDT = 0x42,
	XB3_MODEMSTATUS_JOINWINDOWOPEN = 0x43,
	XB3_MODEMSTATUS_JOINWINDOWCLOSED = 0x44
} uv_xb3_modem_status_e;

const char *uv_xb3_modem_status_to_str(uv_xb3_modem_status_e stat);

/// @brief: Main struct for XB3 wireless module
typedef struct {
	// the spi channel used
	spi_e spi;
	// SPI SSEL gpio. Note that this HAS to be GPIO pin and this module
	// itself controls SSEL as a gpio pin, not via SPI modules
	uv_gpios_e ssel_gpio;
	// gpio pin for SPI_ATTN signal
	uv_gpios_e attn_gpio;
	// reset output
	uv_gpios_e reset_gpio;

	// buffer for writing data and AT commands to XB3.
	// This queue holds API packetized data
	uv_queue_st tx_queue;
	// mutex that should be locked when writing data to tx queue
	uv_mutex_st tx_mutex;
	// buffer for read data from XB3. Holds raw data parsed from API packages
	uv_queue_st rx_data_queue;
	// buffer for read AT commands from XB3. Holds raw data parsed from API packages
	uv_queue_st rx_at_queue;

	uint8_t modem_status;

	bool at_echo;
	bool at_echo_hex;
	bool initialized;
	uv_xb3_at_response_e at_response;
	int16_t rx_index;
	uint16_t rx_size;
	uint8_t rx_frame_type;
} uv_xb3_st;




/// @brief: Initializes the XB3 module
///
/// @ref: ERR_NONE if initialized succesfully
uv_errors_e uv_xb3_init(uv_xb3_st *this,
		spi_e spi,
		uv_gpios_e ssel_gpio,
		uv_gpios_e spi_attn_gpio,
		uv_gpios_e reset_gpio);


/// @brief: Should be called in rtos idle hook
void uv_xb3_poll(uv_xb3_st *this);



/// @brief: Gets received data from internal rx buffer.
///
/// @return True if data was available, otherwise false
///
/// @param dest: Destination where data is copied
static inline bool uv_xb3_get_data(uv_xb3_st *this, char *dest) {
	uv_errors_e e = uv_queue_pop(&this->rx_data_queue, dest, 0);
	return (e == ERR_NONE);
}



/// @brief: Writes a local AT command to XB3 module
///
/// @param data_len: The length of data string. This is given because
/// *data* can contain zero bytes, for example when writing PAN ID to device.
/// If *data_len* is 0 but *data* is not NULL, strlen(data) is used instead.
void uv_xb3_local_at_cmd_req(uv_xb3_st *this, char *atcmd, char *data, uint16_t data_len);


static inline void uv_xb3_set_at_echo(uv_xb3_st *this, bool value) {
	this->at_echo = value;
}

static inline void uv_xb3_set_at_echo_as_hex(uv_xb3_st *this, bool value) {
	this->at_echo_hex = value;
}
static inline bool uv_xb3_get_at_echo_as_hex(uv_xb3_st *this) {
	return this->at_echo_hex;
}

/// @brief: Sets the node identifier for this device
uv_errors_e uv_xb3_set_node_identifier(uv_xb3_st *this, char *name);


/// @brief: Returns the response to last AT command request sent with
/// *uv_xb3_local_at_cmd_req*. Will be cleared to CB3_AT_RESPONSE_COUNT
/// when sending new command.
static inline uv_xb3_at_response_e uv_xb3_get_at_response(uv_xb3_st *this) {
	return this->at_response;
}

#endif

#endif /* UV_HAL_UV_HAL_INC_UV_XB3_H_ */
