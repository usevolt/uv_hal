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
/// * PDO message mapping lengths must be aligned to bytes. This means that
///   PDO mapping lengths should be either 8, 16, 32 or 64 bits long.
/// todo: correct this!
///
/// * EMCY messages do not provide any error states. It's completely up to the user application
/// to provide any NMT state changes when detecting errors. However, error listing to
/// predefined error field is implemented.



#if !defined(CONFIG_CANOPEN)
#error "CONFIG_CANOPEN should be defined as 1 or 0, depending if CANopen should be enabled or disabled"
#endif

#if CONFIG_CANOPEN

/* uw_hal_config.h symbol checks */
#if !defined(CONFIG_CANOPEN_CHANNEL)
#error "CONFIG_CANOPEN_CHANNEL not defined. It should define which CAN module to use (1, 2, 3, etc)"
#endif
#if !defined(CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX)
#error "CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX not defined. It should define the index from which\
 the EMCY error log can be found. Usually 0x1003, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_NODEID_INDEX)
#error "CONFIG_CANOPEN_NODEID_INDEX not defined. It should define which object dictionary index\
 the node id index is. Usually 0x100B, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_HEARTBEAT_INDEX)
#error "CONFIG_CANOPEN_HEARTBEAT_INDEX not defined. It should define which object dictionary index\
 the heartbeat index is. Usually 0x1017, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_COM_INDEX)
#error "CONFIG_CANOPEN_TXPDO_COM_INDEX not defined. It should define the index from which forward\
 transmit PDO communication parameters are found. Usually 0x1800, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_MAP_INDEX)
#error "CONFIG_CANOPEN_TXPDO_MAP_INDEX not defined. It should define the index from which forward\
 transmit PDO mapping parameters are found. Usually 0x1A00, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_COM_INDEX)
#error "CONFIG_CANOPEN_RXPDO_COM_INDEX not defined. It should define the index from which forward\
 receive PDO communication parameters are found. Usually 0x1400, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_MAP_INDEX)
#error "CONFIG_CANOPEN_RXPDO_MAP_INDEX not defined. It should define the index from which forward\
 receive PDO mapping parameters are found. Usually 0x1600, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_STORE_PARAMS_INDEX)
#error "CONFIG_CANOPEN_STORE_PARAMS_INDEX not defined. It should define the index which\
 saves the settings to flash memory. Usually 0x1010, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_RESTORE_PARAMS_INDEX)
#error "CONFIG_CANOPEN_RESTORE_PARAMS_INDEX not defined. It should define the index which\
 restores the settings from flash-memory. Usually 0x1011, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_IDENTITY_INDEX)
#error "CONFIG_CANOPEN_IDENTITY_INDEX not defined. It should define the index which\
 stores the identity field. Usually 0x1018, or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_LOG)
#error "CONFIG_CANOPEN_LOG not defined. It should be defined as 0 or 1, depending on if\
 debug information logging is wanted."
#endif
#if !defined(CONFIG_CANOPEN_RXPDO_COUNT)
#error "CONFIG_CANOPEN_RXPDO_COUNT not defined. It should define the maximum number of receive\
 PDO's in this hardware."
#endif
#if !defined(CONFIG_CANOPEN_TXPDO_COUNT)
#error "CONFIG_CANOPEN_TXPDO_COUNT not defined. It should define the maximum number of transmit\
 PDO's in this hardware."
#endif
#if !defined(CONFIG_CANOPEN_PDO_MAPPING_COUNT)
#error "CONFIG_CANOPEN_PDO_MAPPING_COUNT not defined. It should define the maximum number of\
 PDO mapping entries available to be mapped to the PDO messages (1...8)."
#endif


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
	EMCY_ID =				0x80,
	TXPDO_BEGIN_ID = 		0x180,
	TXPDO_END_ID =			0x480,
	RXPDO_BEGIN_ID = 		0x200,
	RXPDO_END_ID =			0x500,
	BOOTUP_ID = 			0x700,
	HEARTBEAT_ID = 			0x700,
	NMT_ID = 				0x0
} uw_canopen_protocol_ids_e;



typedef enum {
	/// @brief: CANopen stack is in boot up state until
	/// it has been initialized with uw_canopen_init
	STATE_BOOT_UP =			0x0,
	/// @brief: CANopen device is stopped. It can only receive NMT and
	/// heartbeat messages.
	STATE_STOPPED = 		0x4,
	/// @brief: In operational state even PDO messages can be sent
	/// and transmitted.
	STATE_OPERATIONAL =		0x5,
	/// @brief: Configuration state where PDO messages are not sent.
	/// The device can be configured with SDO messages.
	/// The device ends up to this state after a call to uw_canopen_init.
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
	UW_SIGNED8 = 1,
	UW_UNSIGNED16 = 2,
	UW_SIGNED16 = 2,
	UW_UNSIGNED32 = 4,
	UW_SIGNED32 = 4,
	UW_ARRAY8 = (1 << 6) + 1,
	UW_ARRAY16 = (1 << 6) + 2,
	UW_ARRAY32 = (1 << 6) + 4,
	UW_PDO_COM_ARRAY = UW_ARRAY32,
	UW_PDO_MAP_ARRAY = UW_ARRAY32,
	UW_IDENTITY_ARRAY = UW_ARRAY32
} uw_object_types_e;
typedef uint8_t _uw_object_types_e;





typedef enum {
	SDO_ERROR_CMD_SPECIFIER_NOT_FOUND =					0x05040001,
	SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 			0x06010000,
	SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	SDO_ERROR_OBJECT_DOES_NOT_EXIST = 					0x06020000,
	SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 				0x06090032,
	SDO_ERROR_GENERAL = 								0x08000000
} _uw_sdo_error_codes_e;
typedef uint32_t uw_sdo_error_codes_e;



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


typedef enum {
	EMCY_NO_ERROR				= 0x0000,
	EMCY_GENERIC 				= 0x1000,
	EMCY_CURRENT_GENERIC		= 0x2000,
	EMCY_CURRENT_INPUT 			= 0x2100,
	EMCY_CURRENT_DEVICE			= 0x2200,
	EMCY_CURRENT_OUTPUT			= 0x2300,
	EMCY_VOLTAGE_GENERIC		= 0x3000,
	EMCY_VOLTAGE_MAINS			= 0x3100,
	EMCY_VOLTAGE_DEVICE			= 0x3200,
	EMCY_VOLTAGE_OUTPUT			= 0x3300,
	EMCY_TEMP_GENERIC			= 0x4000,
	EMCY_TEMP_AMBIENT			= 0x4100,
	EMCY_TEMP_DEVICE			= 0x4200,
	EMCY_HARDWARE_GENERIC		= 0x5000,
	EMCY_SOFTWARE_GENERIC		= 0x6000,
	EMCY_SOFTWARE_INTERNAL		= 0x6100,
	EMCY_SOFTWARE_USER			= 0x6200,
	EMCY_DATA_SET_GENERIC		= 0x6300,
	EMCY_ADDITIONAL_MODULES		= 0x7000,
	EMCY_MONITORING_GENERIC		= 0x8000,
	EMCY_COMMUNICATION_GENERIC	= 0x8100,
	EMCY_CAN_OVERRUN			= 0x8110,
	EMCY_CAN_ERROR_PASSIVE		= 0x8120,
	EMCY_LIFE_GUARD_HEARTBEAT	= 0x8130,
	EMCY_RECOVERED_FROM_BUS_OFF = 0x8140,
	EMCY_CAN_ID_COLLISION		= 0x8150,
	EMCY_PROTOCOL_GENERIC		= 0x8200,
	EMCY_PDO_LENGTH_ERROR		= 0x8210,
	EMCY_PDO_LENGTH_EXCEEDED	= 0x8220,
	EMCY_DAM_MPDO_NOT_PROCESSED = 0x8230,
	EMCY_UNEXPECT_SYNC_LENGTH	= 0x8240,
	EMCY_RPDO_TIMEOUT			= 0x8250,
	EMCY_EXTERNAL_ERROR			= 0x9000,
	EMCY_ADDITIONAL_FUNCTIONS	= 0xF000,
	EMCY_DEVICE_SPECIFIC		= 0xFF00


} _uw_emcy_codes_e;
typedef _uw_emcy_codes_e uw_emcy_codes_e;

/// @brief: Defines a EMCY message structure.
/// This is used as a EMCY callback parameter, where
/// the user application can read the EMCY error codes.
typedef struct {
	/// @brief: The sender node ID of this EMCY message
	uint8_t node_id;
	/// @brief: CANopen EMCY error code
	uw_emcy_codes_e error_code;
	/// @brief: Application dependent error data.
	/// This can contain any data the application wants.
	union {
		uint8_t data_as_8bit[6];
		uint16_t data_as_16_bit[3];
	};
} uw_canopen_emcy_msg_st;



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


typedef enum {
	/// @brief: PDO is transmitted asynchronously
	PDO_TRANSMISSION_ASYNC = 0xFF,
} _uw_pdo_transmission_types_e;
typedef uint32_t uw_pdo_transmission_types_e;


/// @brief: Values for PDO communication parameters
/// These are used when defining PDO COB-ID's. Currently this
/// CANopen stack doesn't support RTR. Also all PDO's are
/// asynchronous.
typedef enum {
	PDO_ENABLED = 0,
	/// @brief: PDO transmission is disabled. This should be OR'red with the PDO
	/// communication parameter's COB-ID field.
	PDO_DISABLED = 0x80000000,
	PDO_RTR_ALLOWED = 0x40000000
} pdo_cob_id_mapping_e;



/// @brief: a nice way for defining a RXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint32_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this should be set to 0xFF
	/// by the application.
	uw_pdo_transmission_types_e transmission_type;
} uw_rxpdo_com_parameter_st;
#define UW_RXPDO_COM_ARRAY_SIZE	2

/// @brief: a nice way for defining a TXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint32_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this must be set to PDO_TRANSMISSION_ASYNC
	/// by the application.
	uw_pdo_transmission_types_e transmission_type;
	// currently not supported
	uint32_t inhibit_time;
	// reserved data for internal use
	uint32_t _reserved;
	// the time delay for sending the PDO messages
	uint32_t event_timer;
} uw_txpdo_com_parameter_st;
#define UW_TXPDO_COM_ARRAY_SIZE	5

/// @brief: Enables the PDO message by clearing the 31'th bit from the cob id
static inline void canopen_txpdo_enable(uw_txpdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}
static inline void canopen_rxpdo_enable(uw_rxpdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}


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



/// @brief: A struct which the user application should use when defining
/// the object dictionary. A member of this type variable should be
/// created in a place which can be saved to non-volatile memory
typedef struct {
	uw_rxpdo_com_parameter_st rxpdo_coms[CONFIG_CANOPEN_RXPDO_COUNT];
	uw_pdo_mapping_parameter_st rxpdo_mappings[CONFIG_CANOPEN_PDO_MAPPING_COUNT][CONFIG_CANOPEN_RXPDO_COUNT];
	uw_txpdo_com_parameter_st txpdo_coms[CONFIG_CANOPEN_TXPDO_COUNT];
	uw_pdo_mapping_parameter_st txpdo_mappings[CONFIG_CANOPEN_PDO_MAPPING_COUNT][CONFIG_CANOPEN_TXPDO_COUNT];
} uw_pdos_st;



#if CONFIG_CANOPEN_IDENTITY_INDEX
/// @brief: Usewood Vendor ID.
/// Vendor ID can be got from CiA
#define USEWOOD_VENDOR_ID	0

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
#endif



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
/// @param node_id: The node-id which is used to initiate the canopen stack. The node ID can be updated
/// only by restarting the canopen stack.
/// @param obj_dict: Pointer to the uw_canopen_object_st array.
/// Object dictionary should be defined by the application.
/// @param obj_dict_length: The length of the object dictionary,
/// e.g. the number of different indexes.
/// @param sdo_write_callback: A function pointer to a callback function which will be called
/// when a SDO write request was received. If callback function is not required, NULL can be passed.
uw_errors_e uw_canopen_init(uint8_t node_id,
		const uw_canopen_object_st* obj_dict,
		unsigned int obj_dict_length,
		void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry),
		void (*emcy_callback)(void *user_ptr, uw_canopen_emcy_msg_st *msg));


/// @brief: Initializes the PDO struct to it's initial state. All PDO's are disabled
/// and set to asynchronous mode, with the ID being the default CANopen PDO ID + NODE-ID.
///
/// @note: The user application should configure needed PDO messages after calling this function.
/// This only ensures that no uninitialized entries remain in the PDO configurations.
uw_errors_e uw_canopen_pdos_init(uw_pdos_st *pdos, uint8_t node_id);


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


/// @brief: Sends an EMCY (emergency) message
uw_errors_e uw_canopen_emcy_send(uw_canopen_emcy_msg_st *msg);



/// @brief: Sends a SDO message.
/// The message is sent asynchronously
///
/// @param node_id: The destination node's node id
static inline uw_errors_e uw_canopen_send_sdo(uw_canopen_sdo_message_st *sdo, uint8_t node_id) {
	return __uw_canopen_send_sdo(sdo, node_id, SDO_REQUEST_ID);
}

#if CONFIG_CANOPEN_IDENTITY_INDEX
/// @brief: Initializes the identity object to the right values.
/// Assigns the vendor_id as Usewood and reads the LPC hardware for unique serial number .
/// Product code and revision number needs to be specified in the user application.
void uw_canopen_identity_init(uw_identity_object_st* identity_obj,
		uint32_t product_code, uint32_t revision_number);
#endif


#endif

#endif /* UW_CANOPEN_H_ */


