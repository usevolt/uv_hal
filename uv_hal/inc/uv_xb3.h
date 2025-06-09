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
#include "uv_terminal.h"


#if CONFIG_SPI && CONFIG_XB3


#ifndef CONFIG_XB3_COORDINATOR_MAX_DEV_COUNT
#error "CONFIG_XB3_COORDINATOR_MAX_DEV_COUNT should define maximum count of end-devices\
	that can connect to this coordinator."
#endif

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
	XB3_MODEMSTATUS_JOINWINDOWCLOSED = 0x44,
	XB3_MODEMSTATUS_NONE = 0xFF
} uv_xb3_modem_status_e;

const char *uv_xb3_modem_status_to_str(uv_xb3_modem_status_e stat);


// when set, XB3 creates it's own network and operates as a coordinator
#define XB3_CONF_FLAGS_OPERATE_AS_COORDINATOR		(1 << 0)
// RX data echo enabled
#define XB3_CONF_FLAGS_RX_ECHO						(1 << 1)
#define XB3_CONF_FLAGS_AT_ECHO						(1 << 2)
#define XB3_CONF_FLAGS_AT_HEX						(1 << 3)
#define XB3_CONF_FLAGS_DEBUG						(1 << 4)
#define XB3_CONF_FLAGS_TX_ECHO						(1 << 5)

typedef struct {
	uint16_t flags;
	uint64_t epanid;
} uv_xb3_conf_st;

/// @brief: Resets the configuration structure
static inline void uv_xb3_conf_reset(uv_xb3_conf_st *conf, uint16_t flags_def) {
	memset(conf, 0, sizeof(uv_xb3_conf_st));
	conf->flags = flags_def;
}


/// @brief: Main struct for XB3 wireless module
typedef struct {
	uv_xb3_conf_st *conf;

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
	uv_streambuffer_st tx_streambuffer;
	// mutex that should belocked when AT command is ongoing
	uv_mutex_st atreq_mutex;
	// buffer for read data from XB3. Holds raw data parsed from API packages
	uv_streambuffer_st rx_data_streambuffer;
	// buffer for read AT commands from XB3. Holds raw data parsed from API packages
	uv_queue_st rx_at_queue;

	uv_xb3_modem_status_e modem_status;
	uv_xb3_modem_status_e modem_status_changed;

	uv_delay_st joinwindow_delay;

	// stores the currently active network settings.
	// packed struct to help mapping it to canopen object dictionary
	struct __attribute__((packed)) {
		uint64_t op;
		uint16_t oi;
		uint16_t ch;
	} network;

	// init function is executed
	bool initialized;
	uv_xb3_at_response_e at_response;
	uv_xb3_at_response_e at_response_req;
	int16_t rx_index;
	uint16_t rx_size;
	uint8_t rx_frame_type;
} uv_xb3_st;




/// @brief: Initializes the XB3 module
///
/// @ref: ERR_NONE if initialized succesfully
///
/// @param nodeid: Node Identifier, custom string
uv_errors_e uv_xb3_init(uv_xb3_st *this,
		uv_xb3_conf_st *conf,
		spi_e spi,
		uv_gpios_e ssel_gpio,
		uv_gpios_e spi_attn_gpio,
		uv_gpios_e reset_gpio,
		const char *nodeid);


uv_errors_e uv_xb3_set_nodename(uv_xb3_st *this, const char *name);


/// @brief: Should be called in rtos idle hook
///
/// @return: True if any RX data was going to be read, false otherwise
bool uv_xb3_poll(uv_xb3_st *this);


void uv_xb3_step(uv_xb3_st *this, uint16_t step_ms);



/// @brief: Gets received data from internal rx buffer.
///
/// @return True if data was available, otherwise false
///
/// @param dest: Destination where data is copied
static inline bool uv_xb3_get_data(uv_xb3_st *this, char *dest) {
	return uv_streambuffer_pop(&this->rx_data_streambuffer, dest, 1, 0);
}


/// @brief: Generic write function for internal use
uv_errors_e uv_xb3_generic_write(uv_xb3_st *this, char *data,
		uint16_t datalen, uint64_t destaddr, bool isr);


/// @brief: Writes data to device specified by IEEE serial *dest_addr*
/// To be used inside ISRs
static inline uv_errors_e uv_xb3_write_isr(uv_xb3_st *this,
		char *data, uint16_t datalen, uint64_t destaddr) {
	return uv_xb3_generic_write(this, data, datalen, destaddr, true);
}


/// @brief: Writes data to device specified by IEEE serial *dest_addr*
static inline uv_errors_e uv_xb3_write(uv_xb3_st *this,
		char *data, uint16_t datalen, uint64_t destaddr) {
	return uv_xb3_generic_write(this, data, datalen, destaddr, false);
}




/// @brief: Structure defining zigbee devices that are found with "AT+AS" command
typedef struct {
	uint8_t channel;
	uint16_t pan16;
	uint64_t pan64;
	uint8_t allowjoin;
	uint8_t stackprofile;
	// link quality indicator, higher the better
	uint8_t lqi;
	// relative signal strength indicator, lower the better
	int8_t rssi;
} uv_xb3_network_st;


/// @brief: Performs an ATAS active scan and writes the result to *dest*.
///
/// @param dev_count: Pointer to where found network count is written
/// @param dest: Destination array of xb3 network structures
/// @param network_max_count: The length of *dest* in xb3_networks
uv_xb3_at_response_e uv_xb3_scan_networks(uv_xb3_st *this,
		uint8_t *network_count,
		uv_xb3_network_st *dest,
		uint8_t network_max_count);


/// @brief: data type for network_discovery command
typedef struct __attribute__((packed)) {
	uint16_t my;
	union {
		struct {
			uint32_t sl;
			uint32_t sh;
		};
		uint64_t serial;
	};
	char ni[21];
	uint16_t parent_panid;
	uint8_t device_type;
	uint8_t status;
	uint16_t profile_id;
	uint16_t manufacture_id;
} uv_xb3_nddev_st;


/// @brief: Performs AT+ND network discovery command and writes result to *dest*
///
/// @param dev_count: Pointer to where the discovered device count is stored
uv_xb3_at_response_e uv_xb3_network_discovery(uv_xb3_st *this,
		uint8_t *dev_count,
		uv_xb3_nddev_st *dest,
		uint8_t dev_max_count);




/// @brief: Returns the extended PAN ID with "ATID" command
uint64_t uv_xb3_get_epid(uv_xb3_st *this);


void uv_xb3_network_reset(uv_xb3_st *this);


/// @brief: Writes a local AT command to XB3 module
///
/// @param data_len: The length of data string. This is given because
/// *data* can contain zero bytes, for example when writing PAN ID to device.
/// If *data_len* is 0 but *data* is not NULL, strlen(data) is used instead.
void uv_xb3_local_at_cmd_req(uv_xb3_st *this, char *atcmd, char *data, uint16_t data_len);


static inline void uv_xb3_set_at_echo(uv_xb3_st *this, bool value) {
	if (value) {
		this->conf->flags |= XB3_CONF_FLAGS_AT_ECHO;
	}
	else {
		this->conf->flags &= ~XB3_CONF_FLAGS_AT_ECHO;
	}
}

static inline void uv_xb3_set_at_echo_as_hex(uv_xb3_st *this, bool value) {
	if (value) {
		this->conf->flags |= XB3_CONF_FLAGS_AT_HEX;
	}
	else {
		this->conf->flags &= ~XB3_CONF_FLAGS_AT_HEX;
	}
}
static inline bool uv_xb3_get_at_echo_as_hex(uv_xb3_st *this) {
	return !!(this->conf->flags & XB3_CONF_FLAGS_AT_HEX);
}


static inline void uv_xb3_set_rx_echo(uv_xb3_st *this, bool value) {
	this->conf->flags |= (XB3_CONF_FLAGS_RX_ECHO);
}


/// @brief: Returns the response to last AT command request sent with
/// *uv_xb3_local_at_cmd_req*. Will be cleared to CB3_AT_RESPONSE_COUNT
/// when sending new command.
static inline uv_xb3_at_response_e uv_xb3_get_at_response(uv_xb3_st *this) {
	return this->at_response;
}


/// @brief: Parses the "xb" termnal command
void uv_xb3_terminal(uv_xb3_st *this,
		unsigned int args, argument_st *argv);

#endif

#endif /* UV_HAL_UV_HAL_INC_UV_XB3_H_ */
