/*
 * uv_canopen.h
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#ifndef UW_CANOPEN_H_
#define UW_CANOPEN_H_

#include "uv_hal_config.h"
#include "uv_errors.h"
#include "uv_can.h"
#include "uv_utilities.h"

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
#error "CONFIG_CANOPEN_DEVICE_TYPE_INDEX not defined. It should define the index from which\
 the device type constant should be found. Usually 0x1000 or NULL to disable."
#endif
#if !defined(CONFIG_CANOPEN_ERROR_REGISTER_INDEX)
#error "CONFIG_CANOPEN_ERROR_REGISTER_INDEX not defined. It should define the index from where\
 the Error register array can be found. Usually 0x1001, or NULL to disable."
#endif
#if (CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX && !defined(CONFIG_CANOPEN_PREDEFINED_ERROR_SIZE))
#error "CONFIG_CANOPEN_PREDEFINED_ERROR_SIZE not defined. It should define the maximum number of\
 errors which can be saved in the predefined error register."
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
	CANOPEN_UNKNOWN_PROTOCOL,
	CANOPEN_SDO_REQUEST_ID = 		0x600,
	CANOPEN_SDO_RESPONSE_ID = 		0x580,
	CANOPEN_SDO_ERROR_ID =			0x80,
	CANOPEN_EMCY_ID =				0x80,
	CANOPEN_TXPDO1_ID = 			0x180,
	CANOPEN_TXPDO2_ID = 			0x280,
	CANOPEN_TXPDO3_ID = 			0x380,
	CANOPEN_TXPDO4_ID = 			0x480,
	CANOPEN_RXPDO1_ID = 			0x200,
	CANOPEN_RXPDO2_ID = 			0x300,
	CANOPEN_RXPDO3_ID = 			0x400,
	CANOPEN_RXPDO4_ID = 			0x500,
	CANOPEN_BOOTUP_ID = 			0x700,
	CANOPEN_HEARTBEAT_ID = 			0x700,
	CANOPEN_NMT_ID = 				0x0
} uv_canopen_protocol_ids_e;



typedef enum {
	/// @brief: CANopen stack is in boot up state until
	/// it has been initialized with uv_canopen_init
	CANOPEN_BOOT_UP =			0x0,
	/// @brief: CANopen device is stopped. It can only receive NMT and
	/// heartbeat messages.
	CANOPEN_STOPPED = 			0x4,
	/// @brief: In operational state even PDO messages can be sent
	/// and transmitted.
	CANOPEN_OPERATIONAL =		0x5,
	/// @brief: Configuration state where PDO messages are not sent.
	/// The device can be configured with SDO messages.
	/// The device ends up to this state after a call to uv_canopen_init.
	CANOPEN_PREOPERATIONAL =	0x7F
} uv_canopen_node_states_e;



/// @brief: Enumeration of different permissions. Used when defining CANopen objects.
typedef enum {
	CANOPEN_RO = (1 << 0),
	CANOPEN_WO = (1 << 1),
	CANOPEN_RW = 0b11
} _uv_permissions_e;
typedef uint8_t uv_permissions_e;


/// @brief: Describes all possible NMT commands
typedef enum {
	CANOPEN_NMT_START_NODE 			= 0x1,
	CANOPEN_NMT_STOP_NODE 			= 0x2,
	CANOPEN_NMT_SET_PREOPERATIONAL 	= 0x80,
	CANOPEN_NMT_RESET_NODE 			= 0x81,
	CANOPEN_NMT_RESET_COM 			= 0x82
} uv_canopen_nmt_commands_e;


/// @brief: Enumeration of different CANopen object types
/// @note: Used for defining object type in object dictionary.
/// If the object is constant (saved in flash), UW_CONST should be OR'red
/// with the object type.
///
/// Objects of type UW_ARRAY_XX are CANopen object dictionary arrays. They
/// are mapped to object dictionary only with main_index and they use
/// sub_indexes for indexing the array elements. Index 0 returns the
/// length of the array.
#define CANOPEN_ARRAY_MASK 	0b11000000
#define CANOPEN_NUMBER_MASK	0b00111111
typedef enum {
	CANOPEN_UNSIGNED8 = 1,
	CANOPEN_SIGNED8 = 1,
	CANOPEN_UNSIGNED16 = 2,
	CANOPEN_SIGNED16 = 2,
	CANOPEN_UNSIGNED32 = 4,
	CANOPEN_SIGNED32 = 4,
	CANOPEN_ARRAY8 = (1 << 6) + 1,
	CANOPEN_ARRAY16 = (1 << 6) + 2,
	CANOPEN_ARRAY32 = (1 << 6) + 4
} uv_object_types_e;
typedef uint8_t _uv_object_types_e;





typedef enum {
	CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND =					0x05040001,
	CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 			0x06010000,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST = 					0x06020000,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 				0x06090032,
	CANOPEN_SDO_ERROR_GENERAL = 								0x08000000
} _uv_sdo_error_codes_e;
typedef uint32_t uv_sdo_error_codes_e;



/// @brief: SDO command specifier, e.g. the MSB data byte of the CAN message
typedef enum {
	CANOPEN_SDO_CMD_READ = 						0x40,
	CANOPEN_SDO_CMD_READ_RESPONSE_1_BYTE =		0x4F,
	CANOPEN_SDO_CMD_READ_RESPONSE_2_BYTES =		0x4B,
	CANOPEN_SDO_CMD_READ_RESPONSE_4_BYTES =		0x43,
	CANOPEN_SDO_CMD_READ_RESPONSE_BYTES =		0x42,
	CANOPEN_SDO_CMD_WRITE_1_BYTE = 				0x2F,
	CANOPEN_SDO_CMD_WRITE_2_BYTES = 			0x2B,
	CANOPEN_SDO_CMD_WRITE_4_BYTES = 			0x23,
	CANOPEN_SDO_CMD_WRITE_BYTES = 				0x22,
	CANOPEN_SDO_CMD_WRITE_RESPONSE =			0x60,
	CANOPEN_SDO_CMD_ERROR = 					0x80
} _uv_canopen_sdo_commands_e;
typedef uint8_t uv_canopen_sdo_commands_e;

/// @brief: Defines the indexspaces for CANopen object main indexes.
/// 0x1000 - 0x1FFF objects are reserved for CANopen communication parameters
/// and many of them are defined in CiA documents.
/// 0x2000 - 0x2FFF is mainly for application objects.
typedef enum {
	CANOPEN_SDO_COMMUNICATION_INDEXSPACE =		0x1000,
	CANOPEN_SDO_APPLICATION_INDEXSPACE =		0x2000
} uv_sdo_mindex_namespaces_e;


typedef struct {
	/// @brief: Request regarding this sdo entry, as a uv_canopen_sdo_commands_e
	uv_canopen_sdo_commands_e request;
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

} uv_canopen_sdo_message_st;


typedef enum {
	CANOPEN_EMCY_NO_ERROR				= 0x0000,
	CANOPEN_EMCY_GENERIC 				= 0x1000,
	CANOPEN_EMCY_CURRENT_GENERIC		= 0x2000,
	CANOPEN_EMCY_CURRENT_INPUT 			= 0x2100,
	CANOPEN_EMCY_CURRENT_DEVICE			= 0x2200,
	CANOPEN_EMCY_CURRENT_OUTPUT			= 0x2300,
	CANOPEN_EMCY_VOLTAGE_GENERIC		= 0x3000,
	CANOPEN_EMCY_VOLTAGE_MAINS			= 0x3100,
	CANOPEN_EMCY_VOLTAGE_DEVICE			= 0x3200,
	CANOPEN_EMCY_VOLTAGE_OUTPUT			= 0x3300,
	CANOPEN_EMCY_TEMP_GENERIC			= 0x4000,
	CANOPEN_EMCY_TEMP_AMBIENT			= 0x4100,
	CANOPEN_EMCY_TEMP_DEVICE			= 0x4200,
	CANOPEN_EMCY_HARDWARE_GENERIC		= 0x5000,
	CANOPEN_EMCY_SOFTWARE_GENERIC		= 0x6000,
	CANOPEN_EMCY_SOFTWARE_INTERNAL		= 0x6100,
	CANOPEN_EMCY_SOFTWARE_USER			= 0x6200,
	CANOPEN_EMCY_DATA_SET_GENERIC		= 0x6300,
	CANOPEN_EMCY_ADDITIONAL_MODULES		= 0x7000,
	CANOPEN_EMCY_MONITORING_GENERIC		= 0x8000,
	CANOPEN_EMCY_COMMUNICATION_GENERIC	= 0x8100,
	CANOPEN_EMCY_CAN_OVERRUN			= 0x8110,
	CANOPEN_EMCY_CAN_ERROR_PASSIVE		= 0x8120,
	CANOPEN_EMCY_LIFE_GUARD_HEARTBEAT	= 0x8130,
	CANOPEN_EMCY_RECOVERED_FROM_BUS_OFF = 0x8140,
	CANOPEN_EMCY_CAN_ID_COLLISION		= 0x8150,
	CANOPEN_EMCY_PROTOCOL_GENERIC		= 0x8200,
	CANOPEN_EMCY_PDO_LENGTH_ERROR		= 0x8210,
	CANOPEN_EMCY_PDO_LENGTH_EXCEEDED	= 0x8220,
	CANOPEN_EMCY_DAM_MPDO_NOT_PROCESSED = 0x8230,
	CANOPEN_EMCY_UNEXPECT_SYNC_LENGTH	= 0x8240,
	CANOPEN_EMCY_RPDO_TIMEOUT			= 0x8250,
	CANOPEN_EMCY_EXTERNAL_ERROR			= 0x9000,
	CANOPEN_EMCY_ADDITIONAL_FUNCTIONS	= 0xF000,
	CANOPEN_EMCY_DEVICE_SPECIFIC		= 0xFF00


} _uv_emcy_codes_e;
typedef uint16_t uv_emcy_codes_e;

/// @brief: Defines a EMCY message structure.
/// This is used as a EMCY callback parameter, where
/// the user application can read the EMCY error codes.
typedef struct {
	/// @brief: The sender node ID of this EMCY message
	uint8_t node_id;
	/// @brief: CANopen EMCY error code
	uv_emcy_codes_e error_code;
	/// @brief: Application dependent error data.
	/// This can contain any data the application wants.
	union {
		uint8_t data_as_8bit[6];
		uint16_t data_as_16_bit[3];
	};
} uv_canopen_emcy_msg_st;



//************ OBJECT DICTIONARY **************

/// @brief: Structure for an individual object dictionary entry
/// Object dictionary consist of an array of these.
typedef struct {
	/// @brief: Index for this CANopen object dictionary entry
	uint16_t main_index;
	union {
		/// @brief: Subindex for this CANopen object dictionary entry
		///
		/// @note: For object of type UW_ARRAY this is dont-care.
		/// Since arrays don't use sub-index, the same data location
		/// can be used for sub_index and array_max_size.
		uint8_t sub_index;
		/// @brief: If this object is used as an CANopen array,
		/// this is used to define the maximum length of it
		///
		/// @note: CANopen devices can read this value from sub_index 0.
		/// This is an exception to the CANopen standard: All arrays
		/// appear with a prefixed length to the CAN-bus.
		/// Since arrays don't use sub-index, the same data location
		/// can be used for sub_index and array_max_size.
		uint8_t array_max_size;
	};
	/// @brief: Data type for this CANopen object dictionary entry.
	_uv_object_types_e type;
	/// @brief: Pointer to the location where data of this object is saved
	uint8_t* data_ptr;
	/// @brief: Type for this CANopen object dictionary entry
	/// Can be read, write or read-write.
	_uv_permissions_e permissions;
//	uint8_t array_max_size;
} uv_canopen_object_st;


typedef enum {
	/// @brief: PDO is transmitted asynchronously
	CANOPEN_PDO_TRANSMISSION_ASYNC = 0xFF,
} _uv_pdo_transmission_types_e;
typedef uint32_t uv_pdo_transmission_types_e;


/// @brief: Values for PDO communication parameters
/// These are used when defining PDO COB-ID's. Currently this
/// CANopen stack doesn't support RTR. Also all PDO's are
/// asynchronous.
typedef enum {
	CANOPEN_PDO_ENABLED = 0,
	/// @brief: PDO transmission is disabled. This should be OR'red with the PDO
	/// communication parameter's COB-ID field.
	CANOPEN_PDO_DISABLED = 0x80000000,
	CANOPEN_PDO_RTR_ALLOWED = 0x40000000
} pdo_cob_id_mapping_e;



/// @brief: a nice way for defining a RXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint32_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this should be set to 0xFF
	/// by the application.
	uv_pdo_transmission_types_e transmission_type;
} uv_rxpdo_com_parameter_st;
#define CANOPEN_RXPDO_COM_ARRAY_SIZE	2

/// @brief: a nice way for defining a TXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint32_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this must be set to PDO_TRANSMISSION_ASYNC
	/// by the application.
	uv_pdo_transmission_types_e transmission_type;
	// currently not supported
	uint32_t inhibit_time;
	// reserved data for internal use
	uint32_t _reserved;
	// the time delay for sending the PDO messages
	uint32_t event_timer;
} uv_txpdo_com_parameter_st;
#define CANOPEN_TXPDO_COM_ARRAY_SIZE		5

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
} uv_pdo_mapping_parameter_st;

/// @brief: Enables the PDO message by clearing the 31'th bit from the cob id
static inline void canopen_txpdo_enable(uv_txpdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}
static inline void canopen_rxpdo_enable(uv_rxpdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}




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
} uv_identity_object_st;
#define CANOPEN_IDENTITY_OBJECT_ARRAY_SIZE	7
#endif


/// @brief: CANopen communication parameters in the object dictionary
typedef struct {
#if CONFIG_CANOPEN_DEVICE_TYPE_INDEX
	uint32_t device_type;
#endif
#if CONFIG_CANOPEN_ERROR_REGISTER_INDEX
	uint8_t error_register;
#endif
#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
	uint32_t predefined_errors[CONFIG_CANOPEN_PREDEFINED_ERROR_SIZE];
#endif
#if CONFIG_CANOPEN_NODEID_INDEX
	uint8_t node_id;
#endif
#if CONFIG_CANOPEN_STORE_PARAMS_INDEX
	uint32_t store_params;
#endif
#if CONFIG_CANOPEN_RESTORE_PARAMS_INDEX
	uint32_t restore_params;
#endif
#if CONFIG_CANOPEN_HEARTBEAT_INDEX
	uint16_t heartbeat_time;
#endif
#if CONFIG_CANOPEN_IDENTITY_INDEX
	uv_identity_object_st identity;
#endif
#if CONFIG_CANOPEN_TXPDO_COM_INDEX && CONFIG_CANOPEN_TXPDO_MAP_INDEX
	uv_txpdo_com_parameter_st txpdo_coms[CONFIG_CANOPEN_TXPDO_COUNT];
	uv_pdo_mapping_parameter_st txpdo_mappings[CONFIG_CANOPEN_TXPDO_COUNT][CONFIG_CANOPEN_PDO_MAPPING_COUNT];
#endif
#if CONFIG_CANOPEN_RXPDO_COM_INDEX && CONFIG_CANOPEN_RXPDO_MAP_INDEX
	uv_rxpdo_com_parameter_st rxpdo_coms[CONFIG_CANOPEN_RXPDO_COUNT];
	uv_pdo_mapping_parameter_st rxpdo_mappings[CONFIG_CANOPEN_RXPDO_COUNT][CONFIG_CANOPEN_PDO_MAPPING_COUNT];
#endif

} uv_canopen_communication_params_st;


/// @brief: The CANopen object dictionary structure. Used in uv_canopen_st
typedef struct {
	// CANopen communication params defined here in HAL
	uv_canopen_communication_params_st com_params;

	// application specific parameters defined by the user application
	const uv_canopen_object_st *app_parameters;

	// the length of the app parameters
	uint16_t app_parameters_length;

} uv_canopen_object_dict_st;


/// @brief: The main CANopen data structure.
/// A variable of this struct type should be created in a
/// RAM section which can be saved to the non-volatile flash. This way
/// CANopen configurations can be saved with the STORE_PARAMETERS object
typedef struct {
	/// @brief: The object dictionary
	uv_canopen_object_dict_st obj_dict;

	/// @brief: The state of this CANopen node
	uv_canopen_node_states_e state;

	/// @brief: SDO write callback function
	void (*sdo_write_callback)(uv_canopen_object_st* obj_dict_entry);

	/// @brief: EMCY callback function
	void (*emcy_callback)(void *user_ptr, uv_canopen_emcy_msg_st *msg);

	/// @brief: Temporary EMCY message for internal use
	uv_canopen_emcy_msg_st temp_emcy;

	uv_canopen_object_st temp_obj;

	/// @brief: The CAN channel
	uv_can_channels_e can_channel;

#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
	uv_ring_buffer_st errors;
#endif

	int *heartbeat_delay;

} uv_canopen_st;






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
uv_errors_e uv_canopen_init(uv_canopen_st *me,
		const uv_canopen_object_st *obj_dict,
		uint16_t obj_dict_length,
		uv_can_channels_e can_channel,
		int *heartbeat_delay,
		void (*sdo_write_callback)(uv_canopen_object_st* obj_dict_entry),
		void (*emcy_callback)(void *user_ptr, uv_canopen_emcy_msg_st *msg));


/// @brief: Restores the object dictionary communication settings to their defaults.
/// Note that this doesn't restore application dependent objects to their default values by its own.
/// However, it does call restore_defaults() function pointer which should be defined by the application
/// to restore the application parameters to their default values.
uv_errors_e uv_canopen_restore_defaults(uv_canopen_st *me);


/// @brief: The CANopen step function. Makes sure that the txPDO and heartbeat messages
/// are sent cyclically in a right time step
uv_errors_e uv_canopen_step(uv_canopen_st *me, unsigned int step_ms);


/// @brief: Used to set the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state after boot up is done.
/// From that point forward the device state is handled by this CANopen stack.
static inline void uv_canopen_set_state(uv_canopen_st *me, uv_canopen_node_states_e state) {
	me->state = state;
}

/// @brief: Used to get the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state arfter boot up is done.
static inline uv_canopen_node_states_e uv_canopen_get_state(uv_canopen_st *me) {
	return me->state;
}


/// @brief: Sends an EMCY (emergency) message
uv_errors_e uv_canopen_emcy_send(uv_canopen_st *me, uv_canopen_emcy_msg_st *msg);

/// @brief: Sends an SDO request to another node
///
/// @param sdo: The SDO message data structure which is sent.
/// @param node_id: The destination node's node id
uv_errors_e uv_canopen_send_sdo(uv_canopen_st *me, uv_canopen_sdo_message_st *sdo, uint8_t node_id);

/// @brief: Quick way for sending a SDO write request
static inline uv_errors_e uv_canopen_sdo_write(uv_canopen_st *me,
		uv_canopen_sdo_commands_e sdoreq, uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data) \
{
	uv_canopen_sdo_message_st sdo;
	sdo.request = sdoreq;
	sdo.data_32bit = data;
	sdo.main_index = mindex;
	sdo.sub_index = sindex;
	return uv_canopen_send_sdo(me, &sdo, node_id);
}



#endif

#endif /* UW_CANOPEN_H_ */


