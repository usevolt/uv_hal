/*
 * uw_canopen.h
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#ifndef UW_CANOPEN_H_
#define UW_CANOPEN_H_

#include "uw_hal_config.h"
#include "uw_errors.h"
#include "uw_can.h"

/// @file: A software CANopen protocol implementation
/// @note: Relies on uw_can.h
///
/// DIFFERENCES TO A COMPLETE CANOPEN:
///
/// - SDO read and write requests of more than 4 bytes are not implemented
///
/// - For array types, sub index 0 returns the max array size, not current size.
///		e.g. array sizes are constant.
///
/// - For all SDO read x bytes requests, response command identifier doesn't indicate
/// 	exactly how many bytes were returned, the byte 1 is always 0x42.
///
/// - For all SDO write x bytes requests, the request is handled assuming that
/// 	the whole object is being written. E.g. the write byte count is not checked.
///



#if CONFIG_CANOPEN





/* uw_hal_config.h symbol checks */
#if !defined(CONFIG_CANOPEN_CHANNEL)
#error "CONFIG_CANOPEN_CHANNEL not defined. It should define which CAN module to use (1, 2, 3, etc)"
#endif
#if !defined(CONFIG_CANOPEN_HEARTBEAT_INDEX)
#error "CONFIG_CANOPEN_HEARTBEAT_INDEX not defined. It should define which object dictionary index\
 the heartbeat index is."
#endif
#if !defined(CONFIG_CANOPEN_NODEID_INDEX)
#error "CONFIG_CANOPEN_NODEID_INDEX not defined. It should define which object dictionary index\
 the node id index is."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_COM_INDEX)
#error "CONFIG_CANOPEN_TXPDO_COM_INDEX not defined. It should define the index from which forward\
 transmit PDO communication parameters are found."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_MAP_INDEX)
#error "CONFIG_CANOPEN_TXPDO_MAP_INDEX not defined. It should define the index from which forward\
 transmit PDO mapping parameters are found."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_COM_INDEX)
#error "CONFIG_CANOPEN_RXPDO_COM_INDEX not defined. It should define the index from which forward\
 receive PDO communication parameters are found."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_MAP_INDEX)
#error "CONFIG_CANOPEN_RXPDO_MAP_INDEX not defined. It should define the index from which forward\
 receive PDO mapping parameters are found."
#endif
#if !defined(CONFIG_CANOPEN_LOG)
#error "CONFIG_CANOPEN_LOG not defined. It should be defined as 0 or 1, depending on if\
 debug information logging is wanted."
#endif

/// @brief: Usewood Vendor ID.
/// Vendor ID can be got from CiA
#define USEWOOD_VENDOR_ID	0


enum {
	/// @brief: The maximum number of nodes
	/// The actual max value is this - 1
	CANOPEN_NODE_COUNT = 0x100,
	/// @brief: Broadcast messages can be sent with a zero received node_id
	CANOPEN_BROADCAST = 0
};

typedef enum {
	UNKNOWN_PROTOCOL,
	SDO_REQUEST_ID = 		0x600,
	SDO_RESPONSE_ID = 		0x580,
	SDO_ERROR_ID =			0x80,
	TXPDO_BEGIN_ID = 		0x180,
	TXPDO_END_ID =			0x480,
	RXPDO_BEGIN_ID = 		0x200,
	RXPDO_END_ID =			0x500,
	BOOTUP_ID = 			0x700,
	HEARTBEAT_ID = 			0x700,
	NMT_ID = 				0x0
} uw_canopen_protocol_ids_e;



typedef enum {
	STATE_BOOT_UP =			0x0,
	STATE_STOPPED = 		0x4,
	STATE_OPERATIONAL =		0x5,
	STATE_PREOPERATIONAL =	0x7F
} uw_canopen_node_states_e;



/// @brief: Enumeration of different permissions. Used when defining CANopen objects.
typedef enum {
	UW_RO = (1 << 0),
	UW_WO = (1 << 1),
	UW_RW = 0b11
} uw_permissions_e;
typedef uint8_t _uw_permissions_e;


/// @brief: Describes all possible NMT commands
typedef enum {
	NMT_START_NODE 			= 0x1,
	NMT_STOP_NODE 			= 0x2,
	NMT_SET_PREOPERATIONAL 	= 0x80,
	NMT_RESET_NODE 			= 0x81,
	NMT_RESET_COM 			= 0x82
} uw_canopen_nmt_commands_e;


/// @brief: Enumeration of different CANopen object types
/// @note: Used for defining object type in object dictionary.
/// If the object is constant (saved in flash), UW_CONST should be OR'red
/// with the object type.
///
/// Objects of type UW_ARRAY_XX are CANopen object dictionary arrays. They
/// are mapped to object dictionary only with main_index and they use
/// sub_indexes for indexing the array elements. Index 0 returns the
/// length of the array.
#define UW_ARRAY_MASK 	0b11000000
#define UW_NUMBER_MASK	0b00111111
typedef enum {
	UW_UNSIGNED8 = 1,
	UW_UNSIGNED16 = 2,
	UW_UNSIGNED32 = 4,
	UW_ARRAY8 = (1 << 6) + 1,
	UW_ARRAY16 = (1 << 6) + 2,
	UW_ARRAY32 = (1 << 6) + 4,
	UW_PDO_COM_ARRAY = UW_ARRAY16,
	UW_PDO_MAP_ARRAY = UW_ARRAY32,
	UW_IDENTITY_ARRAY = UW_ARRAY32
} uw_object_types_e;
typedef uint8_t _uw_object_types_e;

/// @brief: Values for PDO communication parameters
/// These are used when defining PDO COB-ID's. Currently this
/// CANopen stack doesn't support RTR. Also all PDO's are
/// asynchronous.
typedef enum {
	PDO_ENABLED = 0,
	PDO_DISABLED = 0x80000000,
	PDO_RTR_ALLOWED = 0x40000000
} pdo_cob_id_mapping_e;





typedef enum {
	SDO_ERROR_COMMAND_NOT_VALID = 						0x05040001,
	SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 			0x06010001,
	SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	SDO_ERROR_OBJECT_DOES_NOT_EXIST = 					0x06020000,
	SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 				0x06090032,
	SDO_ERROR_CMD_SPECIFIER_NOT_FOUND =					0x05040001,
	SDO_ERROR_GENERAL = 								0x08000000
} uw_sdo_error_codes_e;



/// @brief: SDO command specifier, e.g. the MSB data byte of the CAN message
typedef enum {
	SDO_CMD_READ = 						0x40,
	SDO_CMD_READ_RESPONSE_1_BYTE =		0x4F,
	SDO_CMD_READ_RESPONSE_2_BYTES =		0x4B,
	SDO_CMD_READ_RESPONSE_4_BYTES =		0x43,
	SDO_CMD_READ_RESPONSE_BYTES =		0x42,
	SDO_CMD_WRITE_1_BYTE = 				0x2F,
	SDO_CMD_WRITE_2_BYTES = 			0x2B,
	SDO_CMD_WRITE_4_BYTES = 			0x23,
	SDO_CMD_WRITE_BYTES = 				0x22,
	SDO_CMD_WRITE_RESPONSE =			0x60,
	SDO_CMD_ERROR = 					0x80
} _uw_canopen_sdo_commands_e;
typedef uint8_t uw_canopen_sdo_commands_e;

typedef struct {
	/// @brief: Request regarding this sdo entry, as a uw_canopen_sdo_commands_e
	uw_canopen_sdo_commands_e request;
	/// @brief: Index for this CANopen object dictionary entry
	uint16_t main_index;
	/// @brief: Subindex for this CANopen object dictionary entry
	uint8_t sub_index;
	/// @brief: Data length for this CANopen object dictionary entry.
	/// @note: Currently only expedited entries are supported => this has to be betweeen 1 to 4.
	uint8_t data_length;
	/// @brief: Data of this CANopen object dictionary entry
	union {
		uint8_t data_8bit[4];
		uint16_t data_16bit[2];
		uint32_t data_32bit;
	};

} uw_canopen_sdo_message_st;




//************ OBJECT DICTIONARY **************

/// @brief: Structure for an individual object dictionary entry
/// Object dictionary consist of an array of these.
typedef struct {
	/// @brief: Index for this CANopen object dictionary entry
	uint16_t main_index;
	/// @brief: Subindex for this CANopen object dictionary entry
	/// @note: For object of type UW_ARRAY this is dont-care
	uint8_t sub_index;
	/// @brief: Data type for this CANopen object dictionary entry.
	_uw_object_types_e type;
	/// @brief: Pointer to the location where data of this object is saved
	uint8_t* data_ptr;
	/// @brief: Type for this CANopen object dictionary entry
	/// Can be read, write or read-write.
	_uw_permissions_e permissions;
	/// @brief: If this object is used as an CANopen array,
	/// this is used to define the maximum length of it
	/// @note: CANopen devices can read this value from sub_index 0.
	/// This is an exception to the CANopen standard: All arrays
	/// appear with a prefixed length to the CAN-bus.
	uint8_t array_max_size;
} uw_canopen_object_st;


/// @brief: a nice way for defining a RXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint16_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this should be set to 0xFF
	/// by the application.
	uint16_t transmission_type;
} uw_rxpdo_com_parameter_st;
#define UW_RXPDO_COM_ARRAY_SIZE	2

/// @brief: a nice way for defining a TXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint16_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this must be set to 0xFF
	/// by the application.
	uint16_t transmission_type;
	uint16_t inhibit_time;
	uint16_t _reserved;
	uint16_t event_timer;
	uint16_t _reserved2;
} uw_txpdo_com_parameter_st;
#define UW_TXPDO_COM_ARRAY_SIZE	5


/// @brief: A nice way for defining a CANopen PDO mapping parameter
/// PDO mapping parameter object should be an array of these
typedef struct {
	/// @brief: Mapped object's main_index
	uint16_t main_index;
	/// @brief: Mapped object's sub_index
	uint8_t sub_index;
	/// @brief: Mapped bit length
	/// @note: This length is in bits!
	uint8_t length;
} uw_pdo_mapping_parameter_st;


/// @brief: A nice way for defining object dictionary's identity object
/// @note: Every CANopen device should specify an identity object at index 0x1018.
/// Difference from Cia 307 CANopen: Serial number is 16 bytes long instead of 4
typedef struct {
	/// @brief: CANopen vendor ID. Manufacturer should have a vendor ID specified
	/// for a valid CANopen device.
	uint32_t vendor_id;
	/// @brief: Vendor specific product code. Describes the type of this device
	uint32_t product_code;
	/// @brief: This device's vendor specific revision number
	uint32_t revision_number;
	/// @brief: This device's specific serial number. Unique for each device
	uint32_t serial_number[4];
} uw_identity_object_st;
#define UW_IDENTITY_OBJECT_ARRAY_SIZE	7




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




/**** PROTECTED FUNCTIONS *****/
/* These are meant only for  hal library's private use and user application shouldn't call these */

/// @brief: parses the CAN message and
/// makes the appropriate actions depending on the message received
void __uw_canopen_parse_message(uw_can_message_st* message);


/// @brief: Really sends the sdo message
///
/// @param req_response: The CAN message ID field indicating if this SDO is a request, response or error
/// @param node_id: The destination node's node id
uw_errors_e __uw_canopen_send_sdo(uw_canopen_sdo_message_st *sdo, uint8_t node_id, unsigned int req_response);

/***** END OF PROTECTED FUNCTIONS ***/





/// @brief: Initializes the CANopen node and the object dictionary
///
/// @param node_id: Pointer to a variable which holds this device's node_id.
/// Usually node id is stored in the object dictionary, index 0x100B
/// @param obj_dict: Pointer to the uw_canopen_object_st array.
/// Object dictionary should be defined by the application.
/// @param obj_dict_length: The length of the object dictionary,
/// e.g. the number of different indexes.
/// @param sdo_write_callback: A function pointer to a callback function which will be called
/// when a SDO write request was received. If callback function is not required, NULL can be passed.
uw_errors_e uw_canopen_init(const uw_canopen_object_st* obj_dict, unsigned int obj_dict_length,
		void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry));


/// @brief: The CANopen step function. Makes sure that the txPDO and heartbeat messages
/// are sent cyclically in a right time step
uw_errors_e uw_canopen_step(unsigned int step_ms);


/// @brief: Used to set the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state after boot up is done.
/// From that point forward the device state is handled by this CANopen stack.
uw_errors_e uw_canopen_set_state(uw_canopen_node_states_e state);


/// @brief: Used to get the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state arfter boot up is done.
uw_canopen_node_states_e uw_canopen_get_state(void);


/// @brief: Sends a SDO message.
/// The message is sent asynchronously
///
/// @param node_id: The destination node's node id
static inline uw_errors_e uw_canopen_send_sdo(uw_canopen_sdo_message_st *sdo, uint8_t node_id) {
	return __uw_canopen_send_sdo(sdo, node_id, SDO_REQUEST_ID);
}

#endif

#endif /* UW_CANOPEN_H_ */


