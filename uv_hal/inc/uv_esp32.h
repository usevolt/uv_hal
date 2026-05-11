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



#define ESP32_CONF_FLAGS_DEBUG		(1 << 0)
#define ESP32_CONF_FLAGS_ECHO		(1 << 1)

#define SSID_STR_MAX_LEN	34
#define PASSWD_STR_MAX_LEN	66
#define IPV6_STR_MAX_LEN	46


#define ESP32_MQTT_URL_MAX_LEN			64
#define ESP32_MQTT_CLIENT_ID_MAX_LEN	32
#define ESP32_MQTT_USER_MAX_LEN			32
#define ESP32_MQTT_PASSWD_MAX_LEN		64
#define ESP32_MQTT_TOPIC_MAX_LEN		64
#define ESP32_MQTT_PAYLOAD_MAX_LEN		256
#define ESP32_MQTT_DEFAULT_KEEPALIVE_S	60
#define ESP32_MQTT_LINK_ID				0


/// @brief: MQTT(S) client configuration consumed by the ESP-AT MQTT commands.
/// Default scheme is 4 (TLS without server-cert verification). For scheme 5
/// (server-cert verification), provision a CA cert into the ESP32 customized
/// certs partition and set ca_id to that slot.
/// All fields are 16-bit aligned so the struct can be exposed verbatim as a
/// CANOPEN_ARRAY16 dictionary entry.
typedef struct {
	char broker_url[ESP32_MQTT_URL_MAX_LEN];
	uint16_t broker_port;
	char client_id[ESP32_MQTT_CLIENT_ID_MAX_LEN];
	char user[ESP32_MQTT_USER_MAX_LEN];
	char passwd[ESP32_MQTT_PASSWD_MAX_LEN];
	uint16_t scheme;
	uint16_t ca_id;
	uint16_t cert_key_id;
	uint16_t keepalive_s;
} uv_esp32_mqtt_conf_st;


void uv_esp32_mqtt_conf_reset(uv_esp32_mqtt_conf_st *conf);


typedef struct {
	uint16_t flags;
	char ssid[SSID_STR_MAX_LEN];
	char passwd[PASSWD_STR_MAX_LEN];
	uv_esp32_mqtt_conf_st mqtt;
} uv_esp32_conf_st;


/// @brief: Resets the configuration structure
void uv_esp32_conf_reset(uv_esp32_conf_st *conf);




typedef enum {
	ESP32_STATE_INIT = 0,
	ESP32_STATE_WAIT_READY,
	ESP32_STATE_TEST_AT,
	ESP32_STATE_DISABLE_ECHO,
	ESP32_STATE_SET_CWMODE,
	ESP32_STATE_CONNECT_WIFI,
	ESP32_STATE_JOINED_NETWORK,
	ESP32_STATE_LEFT_NETWORK,
	ESP32_STATE_SCAN_NETWORKS,
	ESP32_STATE_GET_MAC
} uv_esp32_states_e;


/// @brief: MQTT client state, advances independently of the main esp32 state
/// once Wi-Fi has joined a network. Future protocol modules (ESPNOW, etc.)
/// follow the same pattern with their own *_state field.
typedef enum {
	ESP32_MQTT_STATE_DISABLED = 0,
	ESP32_MQTT_STATE_INIT,
	ESP32_MQTT_STATE_USERCFG,
	ESP32_MQTT_STATE_CONNCFG,
	ESP32_MQTT_STATE_CONN,
	ESP32_MQTT_STATE_CONNECTED,
	ESP32_MQTT_STATE_ERROR,
	ESP32_MQTT_STATE_COUNT
} uv_esp32_mqtt_states_e;


const char *uv_esp32_mqtt_state_to_str(uv_esp32_mqtt_states_e state);


/// @brief: Invoked from the rxtx task when an MQTT message arrives on a
/// subscribed topic. The buffers are owned by the driver and only valid for
/// the duration of the callback.
typedef void (*uv_esp32_mqtt_rx_callb_t)(
		const char *topic, const uint8_t *data, uint16_t len);


#define ESP32_MAC_STR_LEN	18




#define ESP32_SCAN_MAX_NETWORKS		8

typedef struct {
	char ssid[SSID_STR_MAX_LEN];
	int8_t rssi;
} uv_esp32_network_st;


#define ESP32_TX_BUF_SIZE		(700)
#define ESP32_RX_BUF_SIZE		(300)
#define ESP32_AT_RESP_LEN		(96)


/// @brief: Main struct for ESP32 wifi module
typedef struct {
	uv_esp32_conf_st *conf;

	uv_uarts_e uart;
	uv_gpios_e reset_io;

	// buffer for writing data to ESP32
	uv_streambuffer_st tx_streambuffer;
	char tx_buffer[ESP32_TX_BUF_SIZE];
	uv_staticstreambuffer_st tx_staticstreambuffer;
	uv_mutex_st txstream_mutex;
	uv_mutex_st tx_mutex;

	// streambuffer for received data from ESP32
	uv_streambuffer_st rx_datastream;
	uint8_t rx_datastream_buffer[ESP32_RX_BUF_SIZE];
	uv_staticstreambuffer_st rx_static_datastream;

	// AT response line parser
	char at_resp[ESP32_AT_RESP_LEN];
	uint8_t at_resp_i;
	bool at_resp_escape;
	const char *rx_at_cmd;

	uv_esp32_states_e state;
	uv_esp32_states_e scan_return_state;
	uv_delay_st timeout;

	uv_esp32_mqtt_states_e mqtt_state;
	uv_delay_st mqtt_timeout;
	uint8_t mqtt_retry_backoff_s;
	uv_esp32_mqtt_rx_callb_t mqtt_rx_callb;

	// scratch for parsing +MQTTSUBRECV payload (binary, raw bytes follow header)
	struct {
		char topic[ESP32_MQTT_TOPIC_MAX_LEN];
		uint8_t data[ESP32_MQTT_PAYLOAD_MAX_LEN];
		uint16_t expected_len;
		uint16_t received_len;
		bool active;
	} mqtt_subrecv;

	// Pending publish request handed off to the rxtx task. Only one publish
	// in flight at a time; uv_esp32_mqtt_publish blocks while phase != IDLE.
	struct {
		bool pending;
		uint8_t phase;
		char topic[ESP32_MQTT_TOPIC_MAX_LEN];
		uint8_t data[ESP32_MQTT_PAYLOAD_MAX_LEN];
		uint16_t datalen;
		uint8_t qos;
		bool retain;
	} mqtt_publish_req;

	// Pending-line buffer used by at_get_line. When pump_mqtt_async pops a
	// completed line that is not an MQTT async event, the line is held here
	// so the state machine's next at_get_line call can consume it.
	char at_resp_pending[ESP32_AT_RESP_LEN];
	bool at_resp_has_pending;

	union {
		struct {
			uv_esp32_network_st networks[ESP32_SCAN_MAX_NETWORKS];
			uint8_t network_count;
		} scan;
	} state_data;

	uint64_t mac;

	uint32_t written_byte_count;
	uint32_t transmitted_byte_count;

} uv_esp32_st;


static inline uint32_t uv_esp32_get_transmitted_byte_count(uv_esp32_st *this) {
	return this->transmitted_byte_count;
}

static inline uint8_t uv_esp32_get_network_count(uv_esp32_st *this) {
	return this->state_data.scan.network_count;
}




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


const char *uv_esp32_state_to_str(uv_esp32_states_e state);

static inline uv_esp32_states_e uv_esp32_state_get(uv_esp32_st *this) {
	return this->state;
}


/// @brief: Get data from connected device
uv_errors_e uv_esp32_get_data(uv_esp32_st *this, char *dest);

/// @brief: Writes data to ESP32
uv_errors_e uv_esp32_write(uv_esp32_st *this,
		char *data, uint16_t datalen, int32_t wait_ms,
		uint32_t *transmitting_index);

uv_errors_e uv_esp32_write_isr(uv_esp32_st *this,
		char *data, uint16_t datalen, uint32_t *transmitting_index);

/// @brief: Returns the ESP32 MAC address as uint64_t.
/// The MAC is read during ESP32 initialization.
static inline uint64_t uv_esp32_get_mac(uv_esp32_st *this) {
	return this->mac;
}

/// @brief: Converts the ESP32 MAC address to a string
/// (e.g. "aa:bb:cc:dd:ee:ff"). *dest* should be at least
/// ESP32_MAC_STR_LEN (18) bytes.
void uv_esp32_mac_get_str(uv_esp32_st *this, char *dest);

/// @brief: Returns the connected network's SSID
char *uv_esp32_get_connected_ssid(uv_esp32_st *this);


/// @brief: Resets the current network connection
void uv_esp32_reset(uv_esp32_st *this);

/// @brief: Leaves current network
void uv_esp32_network_leave(uv_esp32_st *this);

void uv_esp32_network_join(uv_esp32_st *this, char ssid[32],
						   char passwd[64]);



/// @brief: Starts a WiFi network scan. Results are stored in
/// Results are stored in state_data.scan when scan completes.
uv_errors_e uv_esp32_network_scan(uv_esp32_st *this, bool blocking);


/// @brief: Parses the "esp" terminal command
void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv);


/// @brief: Returns the current MQTT state.
static inline uv_esp32_mqtt_states_e uv_esp32_mqtt_state_get(uv_esp32_st *this) {
	return this->mqtt_state;
}


/// @brief: Publishes a binary payload to a topic via AT+MQTTPUBRAW.
/// Returns ERR_NOT_READY if MQTT is not connected, ERR_BUFFER_OVERFLOW if
/// datalen exceeds ESP32_MQTT_PAYLOAD_MAX_LEN.
uv_errors_e uv_esp32_mqtt_publish(uv_esp32_st *this,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain);


/// @brief: Subscribes to a topic via AT+MQTTSUB.
/// Returns ERR_NOT_READY if MQTT is not connected.
uv_errors_e uv_esp32_mqtt_subscribe(uv_esp32_st *this,
		const char *topic, uint8_t qos);


/// @brief: Unsubscribes from a topic via AT+MQTTUNSUB.
/// Returns ERR_NOT_READY if MQTT is not connected.
uv_errors_e uv_esp32_mqtt_unsubscribe(uv_esp32_st *this, const char *topic);


/// @brief: Registers a callback to be invoked when an MQTT message arrives
/// on a subscribed topic. Pass NULL to clear.
void uv_esp32_mqtt_set_rx_callb(uv_esp32_st *this, uv_esp32_mqtt_rx_callb_t cb);


#endif

#endif /* UV_HAL_UV_HAL_INC_UV_ESP32_H_ */
