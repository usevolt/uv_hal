/*
 * canopen_common.h
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_COMMON_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_COMMON_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"

#if CONFIG_CANOPEN


enum {
	/// @brief: The maximum number of nodes
	/// The actual max value is this - 1
	CANOPEN_NODE_COUNT = 0x100,
	/// @brief: Broadcast messages can be sent with a zero received node_id
	CANOPEN_BROADCAST = 0
};


/// @brief: Enumeration of different permissions. Used when defining CANopen objects.
enum {
	CANOPEN_RO = (1 << 0),
	CANOPEN_WO = (1 << 1),
	CANOPEN_RW = 0b11
};
typedef uint8_t canopen_permissions_e;
#define CANOPEN_IS_READABLE(permissions) ((permissions) & CANOPEN_RO)
#define CANOPEN_IS_WRITABLE(permissions) ((permissions) & CANOPEN_WO)



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
#define CANOPEN_STRING_MASK 0b00110000
#define CANOPEN_NUMBER_MASK	0b00000111
/// @brief: returns the length of this type variable in bytes
#define CANOPEN_TYPE_LEN(type)	((type) & (CANOPEN_NUMBER_MASK))
/// @brief: Additional way of returning the type length in bytes
#define CANOPEN_SIZEOF(type)	CANOPEN_TYPE_LEN(type)

#define CANOPEN_IS_ARRAY(type)	((type) & (CANOPEN_ARRAY_MASK))
#define CANOPEN_IS_INTEGER(type) (!((type) & (CANOPEN_ARRAY_MASK | CANOPEN_STRING_MASK)))
#define CANOPEN_IS_STRING(type) ((type) == CANOPEN_STRING)

enum {
	CANOPEN_UNDEFINED = 0,
	CANOPEN_UNSIGNED8 = 1,
	CANOPEN_SIGNED8 = 1,
	CANOPEN_UNSIGNED16 = 2,
	CANOPEN_SIGNED16 = 2,
	CANOPEN_UNSIGNED32 = 4,
	CANOPEN_SIGNED32 = 4,
	CANOPEN_STRING = (1 << 4) + 1,
	CANOPEN_ARRAY8 = (1 << 6) + 1,
	CANOPEN_ARRAY16 = (1 << 6) + 2,
	CANOPEN_ARRAY32 = (1 << 6) + 4
};
typedef uint8_t canopen_object_type_e;


/// @brief: Declarations of stdint variable types for each CANOPEN parameter type
#define CANOPEN_UNSIGNED8_TYPE		uint8_t
#define CANOPEN_SIGNED8_TYPE		int8_t
#define CANOPEN_UNSIGNED16_TYPE		uint16_t
#define CANOPEN_SIGNED16_TYPE		int16_t
#define CANOPEN_UNSIGNED32_TYPE		uint32_t
#define CANOPEN_SIGNED32_TYPE		int32_t
#define CANOPEN_STRING_TYPE			char
#define CANOPEN_ARRAY8_TYPE			uint8_t
#define CANOPEN_ARRAY16_TYPE		uint16_t
#define CANOPEN_ARRAY32_TYPE		uint32_t

/// @brief: Macro for getting the parameter type
#define CANOPEN_TYPEOF(type)		CAT(type, _TYPE)



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
	canopen_object_type_e type;
	/// @brief: Pointer to the location where data of this object is saved
	void* data_ptr;
	/// @brief: If this object is string type,
	/// this indicates the string length.
	uint16_t string_len;
	/// @brief: Type for this CANopen object dictionary entry
	/// Can be read, write or read-write.
	canopen_permissions_e permissions;
} canopen_object_st;






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
} canopen_identity_object_st;
#define CANOPEN_IDENTITY_OBJECT_ARRAY_SIZE	7




#if CONFIG_CANOPEN_LOG
#define DEBUG_LOG(...)	\
	printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)	//__VA_ARGS__
#endif

/// @brief: Mask for CAN_ID field's node_id bits
#define CANOPEN_NODE_ID_MASK		0x7F


/// @brief: Returns the node ID from the COB-ID
static inline uint8_t uv_canopen_get_node_id(unsigned int cob_id) {
	return cob_id & CANOPEN_NODE_ID_MASK;
}

static inline bool uv_canopen_is_array(const canopen_object_st *obj) {
	return (obj->type & CANOPEN_ARRAY_MASK);
}

static inline bool uv_canopen_is_string(const canopen_object_st *obj) {
	return (obj->type & CANOPEN_STRING_MASK);
}
static inline uint8_t uv_canopen_get_object_data_size(const canopen_object_st *obj) {
	return (obj->type & (CANOPEN_NUMBER_MASK));
}


#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_COMMON_H_ */
