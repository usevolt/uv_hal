/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
/// in the beginning of the object dictionary constant array, which defines the applications
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
#if !defined(CONFIG_CANOPEN_DEVICE_TYPE_INDEX)
#define CONFIG_CANOPEN_DEVICE_TYPE_INDEX	0x1000
#endif
#if !defined(CONFIG_CANOPEN_NODEID_INDEX)
#define CONFIG_CANOPEN_NODEID_INDEX			0x100B
#endif
#if !CONFIG_CANOPEN_DEFAULT_NODE_ID
#error "CONFIG_CANOPEN_DEFAULT_NODE_ID should define the default node ID assigned to this device"
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
#endif
#if !defined(CONFIG_CANOPEN_RESTORE_PARAMS_INDEX)
#define CONFIG_CANOPEN_RESTORE_PARAMS_INDEX	0x1011
#endif
#if !defined(CONFIG_CANOPEN_IDENTITY_INDEX)
#define CONFIG_CANOPEN_IDENTITY_INDEX		0x1018
#endif
#if CONFIG_INTERFACE_REVISION
#define CONFIG_CANOPEN_INTERFACE_REVISION_INDEX	0x2FF0
#endif
#if !defined(CONFIG_CANOPEN_PDO_MAPPING_COUNT)
#define CONFIG_CANOPEN_PDO_MAPPING_COUNT	8
#endif
#if !defined(CONFIG_CANOPEN_SDO_SEGMENTED)
#error "CONFIG_CANOPEN_SDO_SEGMENTED should be defined as 1 if SDO segmented transfers \
should be enabled. Defaults to 0."
#define CONFIG_CANOPEN_SDO_SEGMENTED		0
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
#if !defined(CONFIG_CANOPEN_PRODUCER_HEARTBEAT_TIME_MS)
#error "CONFIG_CANOPEN_PRODUCER_HEARTBEAT_TIME_MS should define the producer heartbeat time in ms"
#endif
#if !defined(CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT)
#error "CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT should defined the number of nodes whose heartbeat \
 this node monitors. If heartbeats are not monitored, set this to 0."
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
#if CONFIG_TARGET_LPC1785
#if !defined(CONFIG_CANOPEN_EMCY_MSG_COUNT)
#error "CONFIG_CANOPEN_EMCY_MSG_COUNT should define the count of different EMCY message ID's \
that the CANopen EMCY module is configured to receive. For each message, \
CONFIG_CANOPEN_EMCY_MSG_ID_x symbol should define the message ID, starting from 1."
#endif
#endif




typedef struct {
	canopen_pdo_mapping_st mappings[CONFIG_CANOPEN_PDO_MAPPING_COUNT];
} canopen_pdo_mapping_parameter_st;


typedef struct {
	uint16_t producer_heartbeat_time_ms;
#if CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT
	uint32_t consumer_heartbeats[CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT];
#endif

	canopen_rxpdo_com_parameter_st rxpdo_coms[CONFIG_CANOPEN_RXPDO_COUNT];
	canopen_pdo_mapping_parameter_st rxpdo_maps[CONFIG_CANOPEN_RXPDO_COUNT];

	canopen_txpdo_com_parameter_st txpdo_coms[CONFIG_CANOPEN_TXPDO_COUNT];
	canopen_pdo_mapping_parameter_st txpdo_maps[CONFIG_CANOPEN_TXPDO_COUNT];

} _canopen_non_volatile_st;



/// @brief: The main CANopen data structure.
/// A variable of this struct type should be created in a
/// RAM section which can be saved to the non-volatile flash. This way
/// CANopen configurations can be saved with the STORE_PARAMETERS object
typedef struct {
	canopen_node_states_e state;
	uint32_t device_type;
	uint32_t store_req;
	uint32_t restore_req;
	canopen_identity_object_st identity;
	uv_delay_st heartbeat_time;
	int32_t txpdo_time[CONFIG_CANOPEN_TXPDO_COUNT];
	int16_t inhibit_time[CONFIG_CANOPEN_TXPDO_COUNT];

	uv_ring_buffer_st emcy_rx;
	canopen_emcy_msg_st emcy_rx_buffer[CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE];

	// SDO member variables
	struct {
		struct {
			canopen_sdo_state_e state;
			uint8_t server_node_id;
			uint8_t sindex;
			uint8_t data_index;
			uint8_t toggle;
			uint16_t mindex;
			void *data_ptr;
			uv_delay_st delay;
		} client;
		struct {
			canopen_sdo_state_e state;
			uint16_t mindex;
			uint8_t sindex;
			// contains the index of next data to be transmitted
			uint8_t data_index;
			uint8_t toggle;
			uv_delay_st delay;
		} server;
	} sdo;
	void (*can_callback)(void *user_ptr, uv_can_message_st* msg);

#if CONFIG_INTERFACE_REVISION
	uint16_t if_revision;
#endif

} _uv_canopen_st;

extern _uv_canopen_st _canopen;






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
static inline uv_errors_e uv_canopen_sdo_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	return _uv_canopen_sdo_client_write(node_id, mindex, sindex, data_len, data);
}

static inline uv_errors_e uv_canopen_sdo_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *dest) {
	return _uv_canopen_sdo_client_read(node_id, mindex, sindex, data_len, dest);
}

/// @brief: Sets a CAN message callback. This can be used in order to manually receive messages
	void uv_canopen_set_can_callback(void (*callb)(void *user_ptr, uv_can_message_st *msg));


uint8_t uv_canopen_sdo_read8(uint8_t node_id, uint16_t mindex, uint8_t sindex);

uint16_t uv_canopen_sdo_read16(uint8_t node_id, uint16_t mindex, uint8_t sindex);

uint32_t uv_canopen_sdo_read32(uint8_t node_id, uint16_t mindex, uint8_t sindex);

uv_errors_e uv_canopen_sdo_write8(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint8_t data);

uv_errors_e uv_canopen_sdo_write16(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint16_t data);

uv_errors_e uv_canopen_sdo_write32(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data);


#endif

#endif /* UW_CANOPEN_H_ */


