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

/// Depth of the publish slot pool. Each slot owns its own topic + payload
/// buffer, so this trades RAM for tolerance to bursts of uncoalesced events
/// before ERR_BUFFER_OVERFLOW kicks in.
#define ESP32_MQTT_PUBLISH_SLOT_COUNT	8


/// @brief: Publish priority. Lower numeric value drains first. Within a
/// priority, ties break by FIFO (oldest seq wins).
typedef enum {
	UV_ESP32_MQTT_PRIO_HIGH   = 0,	///< commands, alarms
	UV_ESP32_MQTT_PRIO_NORMAL = 1,	///< announce, status
	UV_ESP32_MQTT_PRIO_LOW    = 2,	///< periodic process data
	UV_ESP32_MQTT_PRIO_COUNT
} uv_esp32_mqtt_prio_e;


/// @brief: One entry in the publish slot pool.
typedef struct {
	bool in_use;
	uint8_t phase;	///< MQTT_PUB_PHASE_* internal to uv_esp32.c
	uv_esp32_mqtt_prio_e priority;
	uint16_t stream_id;	///< 0 = uncoalesced event; non-zero = coalescing key
	uint32_t seq;	///< monotonic insertion order, FIFO tiebreak within priority
	char topic[ESP32_MQTT_TOPIC_MAX_LEN];
	uint8_t data[ESP32_MQTT_PAYLOAD_MAX_LEN];
	uint16_t datalen;
	uint8_t qos;
	bool retain;
} uv_esp32_mqtt_slot_st;


/// @brief: MQTT scheme values are passed directly to AT+MQTTUSERCFG and
/// follow the Espressif ESP-AT spec:
///   1 = MQTT over TCP (no TLS)
///   2 = TLS, no certificate verification
///   3 = TLS, verify server cert (one-way TLS — typical MQTTS against a
///       public broker; provision a CA into the ESP32 customized certs
///       partition and set `ca_id` to that slot)
///   4 = TLS, provide client cert (no server verify); set `cert_key_id`
///   5 = TLS, mutual auth (verify server cert + provide client cert); set
///       both `ca_id` and `cert_key_id`




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


/// @brief: Main struct for ESP32 wifi module. All config strings/flags are
/// caller-owned: the WiFi side (wifi_flags / wifi_ssid / wifi_passwd) is
/// supplied via uv_esp32_init() and may be written back by the driver
/// (network_join / network_leave / terminal debug toggle). The MQTT side
/// (mqtt_broker_url / mqtt_client_id / mqtt_user / mqtt_passwd plus the
/// scalar settings) is supplied via uv_esp32_mqtt_init(); the driver only
/// reads them — caller storage must outlive the driver.
typedef struct {
	uint16_t *wifi_flags;
	char *wifi_ssid;
	char *wifi_passwd;

	const char *mqtt_broker_url;
	const char *mqtt_client_id;
	const char *mqtt_user;
	const char *mqtt_passwd;
	uint16_t mqtt_broker_port;
	uint16_t mqtt_scheme;
	uint16_t mqtt_ca_id;
	uint16_t mqtt_cert_key_id;
	uint16_t mqtt_keepalive_s;

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

	// Publish slot pool. Caller threads fill slots via uv_esp32_mqtt_publish
	// (non-blocking); the rxtx task drains them in priority + FIFO order.
	// mqtt_pub_mutex guards mqtt_pub_slots / mqtt_publish_seq mutations from
	// both sides.
	uv_mutex_st mqtt_pub_mutex;
	uint32_t mqtt_publish_seq;
	uv_esp32_mqtt_slot_st mqtt_pub_slots[ESP32_MQTT_PUBLISH_SLOT_COUNT];

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




/// @brief: Initializes the ESP32 module.
///
/// @param wifi_flags: Pointer to caller-owned flags word (ESP32_CONF_FLAGS_*).
///                    The driver reads and may write to this location.
/// @param wifi_ssid: Pointer to caller-owned SSID buffer of at least
///                   SSID_STR_MAX_LEN bytes. Written by uv_esp32_network_join
///                   and cleared by uv_esp32_network_leave.
/// @param wifi_passwd: Pointer to caller-owned WiFi password buffer of at
///                     least PASSWD_STR_MAX_LEN bytes. Same lifetime rules
///                     as wifi_ssid.
///
/// @ref: ERR_NONE if initialized succesfully
uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_gpios_e reset_io,
		uv_uarts_e uart,
		uint16_t *wifi_flags,
		char *wifi_ssid,
		char *wifi_passwd);


/// @brief: Configures the MQTT(S) client. Caller-owned string pointers must
/// outlive the driver; the driver reads them each time it transitions
/// through the MQTT state machine. Scheme follows the ESP-AT spec (see the
/// header-level scheme comment). May be called again to retarget — if MQTT
/// is currently connected, the change takes effect on the next reconnect.
void uv_esp32_mqtt_init(uv_esp32_st *this,
		const char *broker_url,
		uint16_t broker_port,
		const char *client_id,
		const char *user,
		const char *passwd,
		uint16_t scheme,
		uint16_t ca_id,
		uint16_t cert_key_id,
		uint16_t keepalive_s);


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


/// @brief: Queues an MQTT publish onto the slot pool. Returns immediately;
/// the rxtx task drains slots in priority order (lower numeric priority
/// first, FIFO within a priority).
///
/// @param priority: see uv_esp32_mqtt_prio_e — picks drain order so
///        infrequent high-priority traffic (alarms, announce) is never
///        starved by a high-rate low-priority stream.
/// @param stream_id: 0 = uncoalesced event — appended to the pool; returns
///        ERR_BUFFER_OVERFLOW if the pool is full. Non-zero = coalescing
///        key — if a pending slot already carries the same stream_id, its
///        payload is overwritten in place (latest-value semantics for
///        periodic data; a slow link can never accumulate stale samples).
///
/// @return ERR_NONE on success;
///         ERR_BUFFER_OVERFLOW if datalen > ESP32_MQTT_PAYLOAD_MAX_LEN,
///                             topic too long, or pool is full and
///                             stream_id is 0;
///         (Publish is queued even when not yet connected — it will be
///          sent once the MQTT state machine reaches CONNECTED.)
uv_errors_e uv_esp32_mqtt_publish(uv_esp32_st *this,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain,
		uv_esp32_mqtt_prio_e priority,
		uint16_t stream_id);


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
