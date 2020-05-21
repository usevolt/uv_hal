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

#ifndef UW_CANOPEN_H_
#define UW_CANOPEN_H_

#include "uv_hal_config.h"
#include "uv_errors.h"
#include "uv_can.h"
#include "uv_utilities.h"
#include "canopen/canopen_common.h"
#include "canopen/canopen_heartbeat.h"
#include "canopen/canopen_nmt.h"
#include "canopen/canopen_pdo.h"
#include "canopen/canopen_sdo.h"
#include "canopen/canopen_sdo_client.h"
#include "canopen/canopen_sdo_server.h"
#include "canopen/canopen_emcy.h"
#include "canopen/canopen_obj_dict.h"

/// @file: A software CANopen protocol implementation
/// @note: Relies on uv_can.h
///
/// ### IMPORTANT:
///
/// * In order to function properly, this CANopen stack requires CANOPEN_OBJ_DICT()-macro to be called
/// at the end of the object dictionary constant array, which defines the applications
/// objects. Read through this header file for more info.
///
/// ###DIFFERENCES TO A COMPLETE CANOPEN:
///
/// * SDO read and write requests of more than 4 bytes are not implemented (expedited transfer?)
///
/// * For array types, sub index 0 returns the max array size, not current size.
///		e.g. array sizes are constant.
///
/// * For all SDO read x bytes requests, response command identifier doesn't indicate
/// 	exactly how many bytes were returned, the byte 1 is always 0x42.
///
/// * For all SDO write x bytes requests, the request is handled assuming that
/// 	the whole object is being written. E.g. the write byte count is not checked.
///
/// * SDO write responses contain only 1 data byte rather than 4, to speed p the communication.
///
/// * Saving and restoring parameters can be done only with sub-index 1, meaning that
///   all parameters are saved / restored.
///
/// * EMCY messages do not provide any error states. It's completely up to the user application
/// to provide any NMT state changes when detecting errors. However, error listing to
/// predefined error field is implemented.
///



#if !defined(CONFIG_CANOPEN)
#error "CONFIG_CANOPEN should be defined as 1 or 0, depending if CANopen should be enabled or disabled"
#endif

#if CONFIG_CANOPEN

/* uv_hal_config.h symbol checks */
#if !defined(CONFIG_CANOPEN_SDO_SERVER)
// define CONFIG_CANOPEN_SDO_SERVER as 0 to disable the SDO server functionality.
// Useful for example on CAN conf tools that only listen and send commands to devices
#define CONFIG_CANOPEN_SDO_SERVER		1
#endif
#if !defined(CONFIG_CANOPEN_DEVICE_TYPE_INDEX)
#define CONFIG_CANOPEN_DEVICE_TYPE_INDEX	0x1000
#endif
#if ((!defined(CONFIG_CANOPEN_NMT_MASTER)) && (!defined(CONFIG_CANOPEN_NMT_SLAVE)))
#error "CANOPEN device should be defined as a NMT Master or Slave by defining either\
 CONFIG_CANOPEN_NMT_MASTER or CONFIG_CANOPEN_NMT_SLAVE as 1."
#endif
#if (CONFIG_CANOPEN_NMT_MASTER && CONFIG_CANOPEN_NMT_SLAVE)
#error "Both CONFIG_CANOPEN_NMT_MASTER and CONFIG_CANOPEN_NMT_SLAVE cannot be set."
#endif
#if !defined(CONFIG_CANOPEN_NODEID_INDEX)
#define CONFIG_CANOPEN_NODEID_INDEX			0x100B
#endif
#if !CONFIG_CANOPEN_DEFAULT_NODE_ID
#error "CONFIG_CANOPEN_DEFAULT_NODE_ID should define the default node ID assigned to this device"
#endif
#if !defined(CONFIG_CANOPEN_AUTO_PREOPERATIONAL)
#error "CONFIG_CANOPEN_AUTO_PREOPERATIONAL should be defined 1 or 0 depending if the canopen stack\
 is allowed to move itself to preoperational state at boot up. If it's defined as 0, the application\
 is responsible for moving the preoperational state after initialization."
#endif
#if CONFIG_CANOPEN_PDO_EXTINIT
#if !defined(CONFIG_CANOPEN_TXPDO_COM_INIT)
#error "CONFIG_CANOPEN_TXPDO_COM_INIT should define the name of the TXPDO communication array \
initialization structure."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_MAP_INIT)
#error "CONFIG_CANOPEN_TXPDO_MAP_INIT should define the name of the TXPDO mapping array \
initialization structure."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_COM_INIT)
#error "CONFIG_CANOPEN_RXPDO_COM_INIT should define the name of the RXPDO communication array \
initialization structure."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_MAP_INIT)
#error "CONFIG_CANOPEN_RXPDO_MAP_INIT should define the name of the RXPDO mapping array \
initialization structure."
#endif
#endif
#if !defined(CONFIG_CANOPEN_EMCY_INHIBIT_TIME_MS)
#error "CONFIG_CANOPEN_EMCY_INIHBIT_TIME_MS should define the inhibit delay time for EMCY messages"
#endif
#if !defined(CONFIG_CANOPEN_CONSUMER_HEARTBEAT_INDEX)
#define CONFIG_CANOPEN_CONSUMER_HEARTBEAT_INDEX	0x1016
#endif
#if !defined(CONFIG_CANOPEN_PRODUCER_HEARTBEAT_INDEX)
#define CONFIG_CANOPEN_PRODUCER_HEARTBEAT_INDEX	0x1017
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_COM_INDEX)
#define CONFIG_CANOPEN_TXPDO_COM_INDEX		0x1800
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_MAP_INDEX)
#define CONFIG_CANOPEN_TXPDO_MAP_INDEX		0x1A00
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_COM_INDEX)
#define CONFIG_CANOPEN_RXPDO_COM_INDEX 		0x1400
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_MAP_INDEX)
#define CONFIG_CANOPEN_RXPDO_MAP_INDEX		0x1600
#endif
#if !defined(CONFIG_CANOPEN_STORE_PARAMS_INDEX)
#define CONFIG_CANOPEN_STORE_PARAMS_INDEX	0x1010
#define CONFIG_CANOPEN_STORE_ALL_PARAMS_SUBINDEX	1
#endif
#if !defined(CONFIG_CANOPEN_RESTORE_PARAMS_INDEX)
#define CONFIG_CANOPEN_RESTORE_PARAMS_INDEX	0x1011
#define CONFIG_CANOPEN_RESTORE_ALL_PARAMS_SUBINDEX	1
#define CONFIG_CANOPEN_RESTORE_COMM_PARAMS_SUBINDEX	2
#define CONFIG_CANOPEN_RESTORE_APP_PARAMS_SUBINDEX	3
#endif
#if !defined(CONFIG_CANOPEN_IDENTITY_INDEX)
#define CONFIG_CANOPEN_IDENTITY_INDEX		0x1018
#endif
#if !defined(CONFIG_CANOPEN_VENDOR_ID)
#warning "CANOPEN VENDOR ID set by default to Usevolt Oy Vendor ID. Define ti with CONFIG_CANOPEN_VENDOR_ID."
#define CONFIG_CANOPEN_VENDOR_ID		CANOPEN_USEVOLT_VENDOR_ID
#endif
#if !defined(CONFIG_CANOPEN_PRODUCT_CODE)
#warning "CANopen Product code set to 0 by default. Define it with CONFIG_CANOPEN_PRODUCT_CODE."
#define CONFIG_CANOPEN_PRODUCT_CODE		0
#endif
#if !defined(CONFIG_CANOPEN_REVISION_NUMBER)
#warning "CANopen revision number set to 0 by default. Define it with CONFIG_CANOPEN_REVISION_NUMBER."
#define CONFIG_CANOPEN_REVISION_NUMBER	0
#endif
#if !defined(CONFIG_CANOPEN_PROGRAM_DATA_INDEX)
#define CONFIG_CANOPEN_PROGRAM_DATA_INDEX	0x1F50
#endif
#if !defined(CONFIG_CANOPEN_PROGRAM_CONTROL_INDEX)
#define CONFIG_CANOPEN_PROGRAM_CONTROL_INDEX	0x1F51
#endif
#if !defined(CONFIG_CANOPEN_PROGRAM_IDENTIF_INDEX)
#define CONFIG_CANOPEN_PROGRAM_IDENTIF_INDEX	0x1F56
#endif
#if !defined(CONFIG_CANOPEN_PROGRAM_FLASH_STATUS_INDEX)
#define CONFIG_CANOPEN_PROGRAM_FLASH_STATUS_INDEX	0x1F57
#endif
#if !defined(CONFIG_CANOPEN_DEVNAME_INDEX)
#define CONFIG_CANOPEN_DEVNAME_INDEX		0x5FFF
#endif
#define CONFIG_CANOPEN_EXMEM_DATA_INDEX			0x5FFE
#define CONFIG_CANOPEN_EXMEM_DATA_TYPE			CANOPEN_STRING
#define CONFIG_CANOPEN_EXMEM_BLOCKSIZE_INDEX	0x5FFD
#define CONFIG_CANOPEN_EXMEM_BLOCKSIZE_TYPE		CANOPEN_UNSIGNED32
#define CONFIG_CANOPEN_EXMEM_OFFSET_INDEX		0x5FFC
#define CONFIG_CANOPEN_EXMEM_OFFSET_TYPE		CANOPEN_UNSIGNED32
#define CONFIG_CANOPEN_EXMEM_FILENAME_INDEX		0x5FFB
#define CONFIG_CANOPEN_EXMEM_FILENAME_TYPE		CANOPEN_STRING
#define CONFIG_CANOPEN_EXMEM_FILESIZE_INDEX		0x5FFA
#define CONFIG_CANOPEN_EXMEM_FILESIZE_TYPE		CANOPEN_UNSIGNED32
#define CONFIG_CANOPEN_EXMEM_WRITEREQ_INDEX		0x5FF9
#define CONFIG_CANOPEN_EXMEM_WRITEREQ_TYPE		CANOPEN_UNSIGNED32
#define CONFIG_CANOPEN_EXMEM_CLEARREQ_INDEX		0x5FF8
#define CONFIG_CANOPEN_EXMEM_CLEARREQ_TYPE		CANOPEN_UNSIGNED8
#if CONFIG_TERMINAL
#if !defined(CONFIG_CANOPEN_TERMINAL_INDEX)
#define CONFIG_CANOPEN_TERMINAL_INDEX		0x5FFE
#endif
#endif
#if !defined(CONFIG_CANOPEN_PDO_MAPPING_COUNT)
#define CONFIG_CANOPEN_PDO_MAPPING_COUNT	8
#endif
#if !defined(CONFIG_CANOPEN_SDO_SEGMENTED)
#error "CONFIG_CANOPEN_SDO_SEGMENTED should be defined as 1 if SDO segmented transfers \
should be enabled. Defaults to 0. Segmented parth takes roughly 1k4 bytes of flash space."
#define CONFIG_CANOPEN_SDO_SEGMENTED		0
#endif
#if !defined(CONFIG_CANOPEN_SDO_BLOCK_TRANSFER)
#error "CONFIG_CANOPEN_SDO_BLOCK_TRANSFER should be defined as 1 or 0, depending if\
 block transfers are enabled or disabled."
#endif
#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
#if !defined(CONFIG_CANOPEN_SDO_BLOCK_SIZE)
#error "CONFIG_CANOPEN_SDO_BLOCK_SIZE should define the block transfer segment size in bytes."
#endif
#if (CONFIG_CANOPEN_SDO_BLOCK_SIZE > 889)
#error "CONFIG_CANOPEN_SDO_BLOCK_SIZE cannot be greater than 889."
#endif
#endif
#if !defined(CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS)
#error "CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS should define the name of the canopen_object_st array\
 which contains all application's CANopen objects in object dictionary."
#endif
#if !defined(CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT)
#error "CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT should define a name of the function which\
 returns the number of application parameters in CANopen object dictionary"
#endif
#if !defined(CONFIG_CANOPEN_LOG)
#define CONFIG_CANOPEN_LOG					0
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_COUNT)
#error "CONFIG_CANOPEN_RXPDO_COUNT not defined. It should define the maximum number of receive\
 PDO's in this hardware."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_COUNT)
#error "CONFIG_CANOPEN_TXPDO_COUNT not defined. It should define the maximum number of transmit\
 PDO's in this hardware."
#endif
#if !defined(CONFIG_CANOPEN_INITIALIZER)
#if !defined(CONFIG_CANOPEN_PRODUCER_HEARTBEAT_TIME_MS)
#error "CONFIG_CANOPEN_PRODUCER_HEARTBEAT_TIME_MS should define the producer heartbeat time in ms"
#endif
#endif
#if !defined(CONFIG_CANOPEN_CHANNEL)
#error "CONFIG_CANOPEN_CHANNEL should define the uv_can channel to be used for CANopen communication"
#endif
#if !defined(CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE)
#error "CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE should define the buffer size for received EMCY messages"
#endif
#if !CONFIG_CANOPEN_SDO_TIMEOUT_MS
#error "CONFIG_CANOPEN_SDO_TIMEOUT_MS should define the SDO protocol timeout in segmented and block transfers\
 in milliseconds."
#endif
#if !defined(CONFIG_CANOPEN_HEARTBEAT_CONSUMER)
#error "CONFIG_CANOPEN_HEARTBEAT_CONSUMER should be defined as 1 or 0 depending if this device\
 listens to any other node's heartbeats and consumes them."
#endif
#if !defined(CONFIG_CANOPEN_HEARTBEAT_PRODUCER)
#error "CONFIG_CANOPEN_HEARTBEAT_PRODUCER should be define as 1 or 0 depending if the device\
 sends the heartbeat messages. Usually this should be 1, as this is how it is defined by CiA 301."
#endif
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
#if !CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT
#error "CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT should define the number of Heartbeat producers\
 which are to be followed. Also CONFIG_CANOPEN_HEARTBEAT_PRODUCER_NODEIDx macros should be used to\
 define the Node ID's of the producers, as well as CONFIG_CANOPEN_HEARTBEAT_PRODUCER_TIMEx should define\
 the time limit which indicates a node connection loss."
#endif
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_TIMEOUT_MS)
#error "CONFIG_CANOPEN_RXPDO_TIMEOUT_MS should define the timeout for RXPDO's in ms."
#endif
#if CONFIG_TARGET_LPC1785
#if !defined(CONFIG_CANOPEN_EMCY_MSG_COUNT)
#error "CONFIG_CANOPEN_EMCY_MSG_COUNT should define the count of different EMCY message ID's \
that the CANopen EMCY module is configured to receive. For each message, \
CONFIG_CANOPEN_EMCY_MSG_ID_x symbol should define the message ID, starting from 1."
#endif
#endif
#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
#if !CONFIG_CANOPEN_SDO_BLOCK_SIZE
#error "CONFIG_CANOPEN_SDO_BLOCK_SIZE should define the size of SDO block transfers in bytes."
#endif
#endif



typedef struct {
	uint16_t cycle_time;
	uint8_t node_id;
	uint8_t _reserved;
} canopen_heartbeat_consumer_st;
#define CANOPEN_HEARTBEAT_CONSUMER_ST_TYPE	sizeof(canopen_heartbeat_consumer_st)

typedef struct {
	uint16_t producer_heartbeat_time_ms;
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	canopen_heartbeat_consumer_st consumer_heartbeats[CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT];
#endif

	canopen_rxpdo_com_parameter_st rxpdo_coms[CONFIG_CANOPEN_RXPDO_COUNT];
	canopen_pdo_mapping_parameter_st rxpdo_maps[CONFIG_CANOPEN_RXPDO_COUNT];

	canopen_txpdo_com_parameter_st txpdo_coms[CONFIG_CANOPEN_TXPDO_COUNT];
	canopen_pdo_mapping_parameter_st txpdo_maps[CONFIG_CANOPEN_TXPDO_COUNT];


	// crc indicating if the initial values have been changed.
	// with this it shouldnt be necessary to clear application non-volatie data
	// if canopen settings are changed while developing
	uint16_t crc;
} uv_canopen_non_volatile_st;


/// @brief: The main CANopen data structure.
/// A variable of this struct type should be created in a
/// RAM section which can be saved to the non-volatile flash. This way
/// CANopen configurations can be saved with the STORE_PARAMETERS object
typedef struct {
	canopen_node_states_e state;
	uint32_t device_type;
	uint32_t store_req[1];
	uint32_t restore_req[3];
	canopen_identity_object_st identity;
	uv_delay_st heartbeat_time;
	uint8_t current_node_id;
#if CONFIG_UV_BOOTLOADER
	uint8_t prog_control;
#endif
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	// stores the times for each heartbeat producer since last heartbeat message
	uint16_t consumer_heartbeat_times[CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT];
	canopen_node_states_e consumer_heartbeat_states[CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT];
#endif

	uv_ring_buffer_st emcy_rx;
	canopen_emcy_msg_st emcy_rx_buffer[CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE];
	uv_delay_st emcy_inihbit_delay;

	// SDO member variables
	struct {
		_uv_canopen_sdo_client_st client;
		_uv_canopen_sdo_server_st server;
	} sdo;

	// TXPDO member variables
	struct {
		int32_t time;
		int16_t inhibit_time;
	} txpdo[CONFIG_CANOPEN_TXPDO_COUNT];

	// RXPDO member variables
	struct {
		uv_delay_st def_delay;
	} rxpdo[CONFIG_CANOPEN_RXPDO_COUNT];

	void (*can_callback)(void *user_ptr, uv_can_message_st* msg);


} _uv_canopen_st;


extern _uv_canopen_st _canopen;



// The usevolt vendor ID assigned by CiA
#define CANOPEN_USEVOLT_VENDOR_ID			0x049B







#define CANOPEN_HEARTBEAT_ID	0x700
#define CANOPEN_SDO_REQUEST_ID	0x600
#define CANOPEN_SDO_RESPONSE_ID	0x580
#define CANOPEN_NMT_ID			0x0
#define CANOPEN_TXPDO1_ID		0x180
#define CANOPEN_TXPDO2_ID		0x280
#define CANOPEN_TXPDO3_ID		0x380
#define CANOPEN_TXPDO4_ID		0x480
#define CANOPEN_RXPDO1_ID		0x200
#define CANOPEN_RXPDO2_ID		0x300
#define CANOPEN_RXPDO3_ID		0x400
#define CANOPEN_RXPDO4_ID		0x500
#define CANOPEN_EMCY_ID			0x80






/**** OBJECT DICTIONARY MANDATORY FIELDS ****/
/* List of all mandatory objects which the application
 * should implement to comply the CANopen standard.
 *
 * Refer to the CiA 301 document for more information
 *
 * Note: Application dependent entries should use IDs starting from 0x2000.
 *
 * 0x1000	Device type
 *
 * 0x1001	Error register
 *
 * 0x1003	Predefined error field
 *  0x1003.0	Number of actual errors (write 0 to clear all errors)
 *  0x1003.1	Most recent error
 *  ...
 * 0x100B	Node-ID
 *
 * 0x1010	Save parameters
 *  0x1010.0	Largest supported subindex (1)
 *  0x1010.1	Save parameters by writing "save"
 *
 * 0x1011	Restore parameters
 *  0x1011.0	Largest supported subindex (1)
 *  0x1011.1	Restore parameters by writing "load"
 *
 * 0x1017	Producer heartbeat time
 *
 * 0x1018	Identity
 * 	0x1018.0	number of entries (4)
 * 	0x1018.1	Vendor ID
 * 	0x1018.2	Product code
 * 	0x1018.3	Revision number
 * 	0x1018.4	Serial number
 *
 *
 *
 * 0x14xx 	Rx PDO communication parameter (mandatory for each PDO used)
 *  0x14xx.0	number of entries (2)
 *  0x14xx.1	RXPDOxx COB-ID
 *  0x14xx.2	transmission type (only asynchronous supported, set to 0xFF)
 *
 * 0x16xx	Rx PDO Mapping parameter (mandatory for each PDO used)
 *  0x16xx.0	number of mapped objects used
 *  0x16xx.1	1st mapped object
 *  0x16xx.2	2st mapped object
 *  	...
 *  0x16xx.8	8th mapped object
 *
 * 0x18xx	Tx PDO communication parameter (mandatory for each PDO used)
 *  0x18xx.0	number of entries (5)
 *  0x18xx.1	TXPDOxx COB-ID
 *  0x18xx.2	transmission type (only asynchronous supported, set to 0xFF)
 *  0x18xx.3	inhibit time (repeat delay)
 *  0x18xx.4	not used
 *  0x18xx.5	Event timer for cyclic transmission
 *
 * 0x1Axx	Tx PDO Mapping parameter (mandatory for each PDO used)
 *  0x1Axx.0	number of mapped objects used
 *  0x1Axx.1	1st mapped object
 *  0x1Axx.2	2st mapped object
 *  	...
 *  0x1Axx.8	8th mapped object
 *
 */




/// @brief: Initializes the CANopen node and the object dictionary
///
/// @param node_id: The node-id which is used to initiate the canopen stack. The node ID can be updated
/// only by restarting the canopen stack.
/// @param obj_dict: Pointer to the uv_canopen_object_st array containing the application specific objects
/// @param obj_dict_length: The length of the object array in objects
/// @param can_channel: The CAN channel which this CANopen instance uses
/// @param heartbeat_delay: Pointer to an int variable which is used for heart beat delay conting.
/// Giving this as a pointer makes sure that non-volatile memory is not flashed if nothing
/// else than heartbeat_delay is changed.
/// @param restore_defaults_callback: A callback function which restores the application parameter
/// object values to their default values. Implementing this is mandatory in order to ensure
/// correct CANopen behaviour.
/// @param sdo_write_callback: A function pointer to a callback function which will be called
/// when a SDO write request was received. If callback function is not required, NULL can be passed.
/// @param emcy_callback Callback function which is called when an EMCY message sent by any node
/// in the CAN-bus is received.
void _uv_canopen_init(void);


/// @brief: The CANopen step function. Makes sure that the txPDO and heartbeat messages
/// are sent cyclically in a right time step
void _uv_canopen_step(unsigned int step_ms);


void _uv_canopen_reset(void);


/// @brief: Used to set the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state after boot up is done.
/// From that point forward the device state is handled by this CANopen stack.
void uv_canopen_set_state(canopen_node_states_e state);

/// @brief: Used to get the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state arfter boot up is done.
canopen_node_states_e uv_canopen_get_state(void);

/// @brief: Quick way for sending a SDO write request
///
/// @param node_id: The nodeID of the device to be written
/// @param mindex: The main index of the object to be written
/// @param sindex: The sub index of the object to be written
/// @param data_len: The length of the data to be written. This should match the
/// object, otherwise the server device will respond with a SDO Abort message.
/// If this is set to 0, the SDO request is done as an expedited transfer without
/// indicating the data length. Thus 4 bytes of data is copied from *data* to
/// the CAN message and the server device is responsible to read the correct
/// numbre of bytes from the message.
/// @param data: A pointer to the data that is written. In case of *data_len* == 0,
/// this is expected to be a pointer to uint32_t variable where 4 bytes of data is read.
static inline uv_errors_e uv_canopen_sdo_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	return _uv_canopen_sdo_client_write(node_id, mindex, sindex, data_len, data);
}

static inline uv_errors_e uv_canopen_sdo_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *dest) {
	return _uv_canopen_sdo_client_read(node_id, mindex, sindex, data_len, dest);
}


#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER

static inline uv_errors_e uv_canopen_sdo_block_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *dest) {
	return _uv_canopen_sdo_client_block_read(node_id, mindex, sindex, data_len, dest);
}

static inline uv_errors_e uv_canopen_sdo_block_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *src) {
	return _uv_canopen_sdo_client_block_write(node_id, mindex, sindex, data_len, src);
}

#endif

/// @brief: Sets a CAN message callback. This can be used to manually receive messages.
/// Note that the callback will be called from the HAL task.
void uv_canopen_set_can_callback(void (*callb)(void *user_ptr, uv_can_message_st *msg));

/// @brief: Sets an SDO write callback. This can be used to get a notification of when an
/// SDO write request has been received.
///
/// @note: The callback will be called from the HAL task
static inline void uv_canopen_set_sdo_write_callback(void (*callb)(uint16_t mindex, uint8_t sindex)) {
	_uv_canopen_sdo_server_add_write_callb(callb);
}

static inline void uv_canopen_set_sdo_read_callback(void (*callb)(uint16_t mindex, uint8_t sindex)) {
	_uv_canopen_sdo_server_add_read_callb(callb);
}

uint8_t uv_canopen_sdo_read8(uint8_t node_id, uint16_t mindex, uint8_t sindex);

uint16_t uv_canopen_sdo_read16(uint8_t node_id, uint16_t mindex, uint8_t sindex);

uint32_t uv_canopen_sdo_read32(uint8_t node_id, uint16_t mindex, uint8_t sindex);

uv_errors_e uv_canopen_sdo_write8(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint8_t data);

uv_errors_e uv_canopen_sdo_write16(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint16_t data);

uv_errors_e uv_canopen_sdo_write32(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data);


/// @brief: Configures all CAN rx messages to be received that are required in CANopen stack
void uv_canopen_config_rx_msgs(void);

typedef uint8_t memory_scope_e_;

/// @brief: Sends a restore params request to node *node_id*.
///
/// @param param_scope: Defines if the app or communication parameters are restored.
/// Refer to uv_memory.h for values of this.
uv_errors_e uv_canopen_sdo_restore_params(uint8_t node_id, memory_scope_e_ param_scope);


/// @brief: Sends a store params request to node *node_id*.
///
/// @param param_scope: Defines if the app or communication parameters are stored.
/// Refer to uv_memory.h for values of this.
uv_errors_e uv_canopen_sdo_store_params(uint8_t node_id, memory_scope_e_ param_scope);


/// @brief: Returns the last error code received while sending read or write SDO requests.
static inline uv_sdo_error_codes_e uv_canopen_sdo_get_error(void) {
	return _uv_canopen_sdo_get_error_code();
}

/// @brief: Returns the current nodeid of this device
static inline uint8_t uv_canopen_get_our_nodeid(void) {
	return _canopen.current_node_id;
}

/// @brief: Sets the nodeid of this device. The change comes valid
/// after saving non-volatile settings and resetting the device.
void uv_canopen_set_our_nodeid(uint8_t nodeid);


#endif

#endif /* UW_CANOPEN_H_ */


