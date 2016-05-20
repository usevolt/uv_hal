/*
 * uw_canopen.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */


#include "uw_canopen.h"
#if CONFIG_CANOPEN

#include "uw_can.h"
#include "uw_reset.h"
#include "uw_utilities.h"
#include <string.h>
#if CONFIG_CANOPEN_LOG
#include <stdarg.h>
#include <stdio.h>
#endif
#if CONFIG_CANOPEN_IDENTITY_INDEX
#include "uw_memory.h"
#endif
extern uw_errors_e __uw_save_previous_non_volatile_data();
extern uw_errors_e __uw_clear_previous_non_volatile_data();

#if CONFIG_CANOPEN_LOG
extern bool canopen_log;
#endif

/// @brief: Mask for CAN_ID field's node_id bits
#define UW_CANOPEN_NODE_ID_MASK		0x7F


/// @brief: Returns true if this PDO message was enabled (bit 31 was not set)
#define is_pdo_enabled(x)			(!(((uw_txpdo_com_parameter_st*)x)->cob_id & (1 << 31)))


/// @brief: A local declaration of static variable structure
typedef struct {
	const uw_canopen_object_st* obj_dict;
	unsigned int obj_dict_length;
	uw_canopen_node_states_e state;
	uint8_t node_id;
	void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry);
	uw_canopen_emcy_msg_st temp_emcy;
	void (*emcy_callback)(void *user_ptr, uw_canopen_emcy_msg_st *msg);
#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
	uw_ring_buffer_st errors;
#endif
#if CONFIG_CANOPEN_HEARTBEAT_INDEX
	// stores the heartbeat object to fasten the step function
	const uw_canopen_object_st *heartbeat_obj;
	int heartbeat_delay;
#endif
} this_st;

static this_st _this = {
		.state = STATE_BOOT_UP,
};
// A C++ style this-pointer definitions
#define this 	(&_this)

/// @brief: Executes the sdo command to either read or write the object dictionary.
/// Modifies the object dictionary as necessary, sends the answer message and executes
/// the callback function from object dictionary entry.
/// Possible errors are handled as well and the request message struct error field is updated
/// correspondly.
void canopen_parse_sdo(uw_can_message_st* req);

/// @brief: Executes RXPDO command.
/// Check's which objects are mapped to the specific PDO and assigns their values accordingly.
static inline void parse_rxpdo(uw_can_message_st* msg);

/// @brief: Parses the received NMT message and does the actions necessary
static inline void parse_nmt_message(uw_can_message_st* msg);

#define node_id()		(*(find_object(CONFIG_CANOPEN_NODEID_INDEX, 0)->data_ptr))
#define is_array(x)		(x & UW_ARRAY_MASK)
#define array_element_size(x)	(x & (~UW_ARRAY_MASK))

/// @brief: Searches the object dictionary and returns a pointer to found
/// object. If object couldn't be found, returns NULL.
static const uw_canopen_object_st *find_object(uint16_t index, uint8_t sub_index) {
	unsigned int i;
	for (i = 0; i < this->obj_dict_length; i++) {
		if (this->obj_dict[i].main_index == index) {
			// object is of array type and sub index is dont_care
			if (is_array(this->obj_dict[i].type)) {
				return &this->obj_dict[i];
			}
			else {
				if (this->obj_dict[i].sub_index == sub_index) {
					return &this->obj_dict[i];
				}
			}
		}
	}
	return NULL;
}

static void array_write(const uw_canopen_object_st *obj, uint16_t index, unsigned int value) {
	if (index >= obj->array_max_size || !index) {
		return;
	}
	switch (obj->type) {
	case UW_ARRAY8:
		obj->data_ptr[index] = value;
		return;
	case UW_ARRAY16:
		((uint16_t*)(obj->data_ptr))[index] = value;
		return;
	case UW_ARRAY32:
		((uint32_t*)(obj->data_ptr))[index] = value;
		return;
	default:
		return;
	}
}

/// @brief: Can be used to index object which is of type array
static unsigned int array_index(const uw_canopen_object_st *obj, uint16_t index) {
	if (index >= obj->array_max_size) {
		return 0;
	}
	// index 0 means the length of this array
	else if (index == 0) {
		return obj->array_max_size;
	}
	switch (obj->type) {
	case UW_ARRAY8:
		return obj->data_ptr[index];
	case UW_ARRAY16:
		return ((uint16_t*) obj->data_ptr)[index];
	case UW_ARRAY32:
		return ((uint32_t*) obj->data_ptr)[index];
	default:
		return 0;
	}
}

static unsigned int get_object_value(const uw_canopen_object_st* obj) {
	if (obj->type == UW_UNSIGNED8) {
		return *obj->data_ptr;
	}
	else if (obj->type == UW_UNSIGNED16) {
		return *((uint16_t*) obj->data_ptr);
	}
	else if (obj->type == UW_UNSIGNED32) {
		return *((uint32_t*) obj->data_ptr);
	}
	else {
		return 0;
	}
}

uw_errors_e uw_canopen_pdos_init(uw_pdos_st *pdos, uint8_t node_id) {
	uint8_t i;
	for (i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		pdos->rxpdo_coms[i].cob_id = PDO_DISABLED + node_id +
				RXPDO_BEGIN_ID + 0x100 * i;
		pdos->rxpdo_coms[i].transmission_type = PDO_TRANSMISSION_ASYNC;
		uint8_t j;
		for (j = 0; j < UW_RXPDO_COM_ARRAY_SIZE; j++) {
			pdos->rxpdo_mappings[j][i].main_index = 0;
			pdos->rxpdo_mappings[j][i].sub_index = 0;
		}
	}
	for (i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		pdos->txpdo_coms[i].cob_id = PDO_DISABLED + this->node_id +
				TXPDO_BEGIN_ID + 0x100 * i;
		pdos->txpdo_coms[i].transmission_type = PDO_TRANSMISSION_ASYNC;
		uint8_t j;
		for (j = 0; j < UW_RXPDO_COM_ARRAY_SIZE; j++) {
			pdos->txpdo_mappings[j][i].main_index = 0;
			pdos->txpdo_mappings[j][i].sub_index = 0;
		}
	}
	return uw_err(ERR_NONE);
}



uw_errors_e uw_canopen_init(uint8_t node_id, const uw_canopen_object_st* obj_dict, unsigned int obj_dict_length,
		void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry),
		void (*emcy_callback)(void *user_ptr, uw_canopen_emcy_msg_st *msg)) {
	this->obj_dict = obj_dict;
	this->obj_dict_length = obj_dict_length;
	this->state = STATE_PREOPERATIONAL;
	this->sdo_write_callback = sdo_write_callback;
	this->node_id = node_id;
	this->emcy_callback = emcy_callback;


	// check that node id and heartbeat time entries are found
	// in obj dict
	const uw_canopen_object_st *obj = find_object(CONFIG_CANOPEN_NODEID_INDEX, 0);
	if (!obj) {
		__uw_err_throw(ERR_CANOPEN_NODE_ID_ENTRY_INVALID | HAL_MODULE_CANOPEN);
	}
#if CONFIG_CANOPEN_HEARTBEAT_INDEX
	if (! (obj = find_object(CONFIG_CANOPEN_HEARTBEAT_INDEX, 0))) {
		__uw_err_throw(ERR_CANOPEN_HEARTBEAT_ENTRY_INVALID| HAL_MODULE_CANOPEN);
	}
	this->heartbeat_obj = obj;
	this->heartbeat_delay = get_object_value(obj);
#endif

	// configure receive messages for NMT, SDO and PDO messages for this node id and
	// broadcast.
	// NMT broadcasting
	uw_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, NMT_ID,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// NMT node id
	uw_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, NMT_ID + this->node_id,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// SDO request broadcasting
	uw_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, SDO_REQUEST_ID,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// SDO request node id
	uw_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, SDO_REQUEST_ID + this->node_id,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// all other nodes' EMCY messages
	if (emcy_callback) {
		uw_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, EMCY_ID, 0xFFFFFF00, CAN_11_BIT_ID);
	}

	uint8_t i;
	for (i = 0; i < 255; i++) {
		// assume that the RXPDO communication parameters contain uw_rxpdo_com_parameter_st types.
		obj = find_object(CONFIG_CANOPEN_RXPDO_COM_INDEX + i, 0);
		if (!obj) {
			break;
		}
		uint32_t id = array_index(obj, 1);
		uw_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, id,
				CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	}

#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
	obj = find_object(CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX, 0);
#if CONFIG_CANOPEN_LOG
	if (!obj && canopen_log) {
		printf("Predefined error register not exists with CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX"
				" defined as %x\n\r", CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX);
	}
#endif

	uw_ring_buffer_init(&this->errors, obj->data_ptr,
			obj->array_max_size, array_element_size(obj->type));
	for (i = 0; i < obj->array_max_size * array_element_size(obj->type); i++) {
		obj->data_ptr[i] = 0;
	}
#endif

#if CONFIG_CANOPEN_LOG
	if (canopen_log) {
		printf("canopen initialized with node id %x\n\r", this->node_id);
	}
#endif


	// send boot up message
	uw_can_message_st msg = {
			.id = BOOTUP_ID + this->node_id,
			.data_length = 1,
			.data_8bit[0] = (uint8_t) STATE_BOOT_UP
	};
	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);

	return uw_err(ERR_NONE);
}

uw_errors_e uw_canopen_step(unsigned int step_ms) {

	// in stopped state we do nothing
	if (this->state == STATE_STOPPED) {
		return uw_err(ERR_NONE);
	}
	// send heartbeat message
	if (uw_delay(step_ms, &this->heartbeat_delay)) {
		// send heartbeat msg
		uw_can_message_st msg = {
				.id = HEARTBEAT_ID + this->node_id,
				.data_length = 1,
				.data_8bit[0] = this->state
		};
#if CONFIG_CANOPEN_LOG
	if (canopen_log) {
		printf("Heartbeat sent, node ID: %u, state: %x\n\r", this->node_id, this->state);
	}
#endif
		if (uw_can_get_error_state(CONFIG_CANOPEN_CHANNEL) == CAN_ERROR_ACTIVE) {
			uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);
		}

		// start the delay again
		uw_start_delay(get_object_value(this->heartbeat_obj), &this->heartbeat_delay);
	}

	// from this point forward is executed only in operational state
	if (this->state != STATE_OPERATIONAL) {
		return uw_err(ERR_NONE);
	}

	// TXPDO handling
	uint8_t i;
	const uw_canopen_object_st *obj;
	const uw_canopen_object_st *map_obj;
	uw_can_message_st msg;
	for (i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		obj = find_object(CONFIG_CANOPEN_TXPDO_COM_INDEX + i, 0);
		if (!obj) {

#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("TXPDO communication parameter couldn't be found with id %x\n\r",
						CONFIG_CANOPEN_TXPDO_COM_INDEX + i);
			}
#endif
			return uw_err(ERR_CANOPEN_PDO_COM_NOT_FOUND);
		}
		// if this PDO is enabled, fetch the data from mapping object and send the message
		if (is_pdo_enabled(obj)) {
			// fetch the message COB-ID
			msg.id = ((uw_txpdo_com_parameter_st*) obj)->cob_id;
			msg.data_length = 8;
			obj = find_object(CONFIG_CANOPEN_TXPDO_MAP_INDEX + i, 0);
			if (!obj) {
#if CONFIG_CANOPEN_LOG
				if (canopen_log) {
					printf("TXPDO mapping parameter couldn't found with id %x\n\r",
							CONFIG_CANOPEN_TXPDO_MAP_INDEX + i);
				}
#endif
				return uw_err(ERR_CANOPEN_PDO_MAP_NOT_FOUND);
			}
			uint8_t j;
			uint8_t byte_count = 0;
			// cycle trough all mapping parameters and fetch the PDO data
			for (j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {
				uw_pdo_mapping_parameter_st *map = &((uw_pdo_mapping_parameter_st*) obj)[i];
				// if all are zeroes, all data has been mapped to the PDO
				if (map->main_index == 0 && map->sub_index == 0) {
					break;
				}
				// otherwise map some data to the message
				map_obj = find_object(map->main_index, map->sub_index);
				if (!map_obj) {
#if CONFIG_CANOPEN_LOG
					if (canopen_log) {
						printf("TXPDO mapping parameter 0x%x points to a object 0x%x 0x%x which doens't exist\n\r",
								CONFIG_CANOPEN_TXPDO_MAP_INDEX + i, map->main_index, map->sub_index);
					}
#endif
					return uw_err(ERR_CANOPEN_MAPPED_OBJECT_NOT_FOUND);
				}
				if (is_array(map_obj->type)) {

				}

			}
			uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);
		}
	}


	return uw_err(ERR_NONE);
}



#define msg_node_id(msg)		(msg->id & UW_CANOPEN_NODE_ID_MASK)
#define msg_type(msg)			(message->id & (~UW_CANOPEN_NODE_ID_MASK))

void __uw_canopen_parse_message(uw_can_message_st* message) {

	// process messages which are addressed to this node OR broadcasted
	if (this->node_id == msg_node_id(message) || msg_node_id(message) == 0) {

		switch (msg_type(message)) {
		case SDO_REQUEST_ID:
			// SDO messages are always 8 bytes long
			if (message->data_length < 8) {
				break;
			}
			// in stopped state SDO is disabled
			if (this->state == STATE_STOPPED) {
				break;
			}
			canopen_parse_sdo(message);
			break;
		case NMT_ID:
			parse_nmt_message(message);
			break;
		case EMCY_ID:
			// in stopped state EMCY message handling is disabled
			if (this->state == STATE_STOPPED) {
				break;
			}
			if (this->emcy_callback) {
				// EMCY message's length should always be 8 bits
				if (message->data_length == 8) {
					this->temp_emcy.node_id = message->id & UW_CANOPEN_NODE_ID_MASK;
					this->temp_emcy.error_code = message->data_16bit[0];
					this->temp_emcy.data_as_16_bit[0] = message->data_16bit[1];
					this->temp_emcy.data_as_16_bit[1] = message->data_16bit[2];
					this->temp_emcy.data_as_16_bit[2] = message->data_16bit[3];
					this->emcy_callback(__uw_get_user_ptr(), &this->temp_emcy);
				}
			}
			break;
		default:
			break;
		}
	}

	// regardless of what kind of message was received, check if it was mapped to any RXPDO
	// (RXPDO's can be mapped with any kind of message)
	if (this->state == STATE_OPERATIONAL) {
		parse_rxpdo(message);
	}
}



uw_errors_e uw_canopen_set_state(uw_canopen_node_states_e state) {
	this->state = state;
	return uw_err(ERR_NONE);
}

uw_canopen_node_states_e uw_canopen_get_state(void) {
	return this->state;
}


uw_errors_e uw_canopen_emcy_send(uw_canopen_emcy_msg_st *msg) {
	if (this->state == STATE_STOPPED) {
		return uw_err(ERR_CANOPEN_STACK_IN_STOPPED_STATE | HAL_MODULE_CANOPEN);
	}
	uw_can_message_st canmsg = {
			.data_length = 8,
			.id = EMCY_ID + msg->node_id,
			.data_16bit[0] = msg->error_code,
			.data_16bit[1] = msg->data_as_16_bit[0],
			.data_16bit[2] = msg->data_as_16_bit[1],
			.data_16bit[3] = msg->data_as_16_bit[2]
	};
	uw_errors_e err = ERR_NONE;

	uw_err_check(uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &canmsg)) {
#if CONFIG_CANOPEN_LOG
	if (canopen_log) {
		printf("EMCY message sending failed. CAN TX buffer full?\n\r");
	}
#endif
		err = ERR_BUFFER_OVERFLOW | HAL_MODULE_CANOPEN;
	}
#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
	uw_err_check (uw_ring_buffer_push(&this->errors, &msg->error_code)) {
		// pushing failed, array was full. Discard the oldest error and then put new one
		uw_ring_buffer_pop(&this->errors, NULL);
		uw_ring_buffer_push(&this->errors, &msg->error_code);
		err = ERR_BUFFER_OVERFLOW | HAL_MODULE_CANOPEN;
	}
#endif
	return uw_err(err);
}


#define sdo_main_index(msg)		(msg->data_8bit[1] + (msg->data_8bit[2] << 8))
#define sdo_sub_index(msg)		(msg->data_8bit[3])
#define sdo_command(msg)		(msg->data_8bit[0])
#define sdo_data(msg, index)	(msg->data_8bit[4 + index])

/// @brief: Sends a SDO error response
static void sdo_send_error(uw_can_message_st *msg, uw_sdo_error_codes_e error) {
	uw_can_message_st reply = {
			.id = SDO_ERROR_ID + this->node_id,
			.data_length = 8,
			.data_8bit[0] = SDO_CMD_ERROR,
			.data_8bit[1] =	sdo_main_index(msg),
			.data_8bit[2] = sdo_main_index(msg) >> 8,
			.data_8bit[3] = sdo_sub_index(msg),
			.data_8bit[4] = (uint8_t) error,
			.data_8bit[5] = (uint8_t) (error >> 8),
			.data_8bit[6] = (uint8_t) (error >> 16),
			.data_8bit[7] = (uint8_t) (error >> 24)
	};
	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &reply);
}

static void sdo_send_response(uw_can_message_st *msg, uw_canopen_sdo_commands_e cmd,
		const uint8_t *data, uint8_t data_length) {
	uw_can_message_st reply = {
			.id = SDO_RESPONSE_ID + this->node_id,
			.data_length = 8,
			.data_8bit[0] = cmd,
			.data_8bit[1] =	sdo_main_index(msg),
			.data_8bit[2] = sdo_main_index(msg) >> 8,
			.data_8bit[3] = sdo_sub_index(msg)
	};
	uint8_t i = 0;
	for (i = 0; i < data_length; i++) {
		reply.data_8bit[4 + i] = data[i];
	}
	while (i < 4) {
		i++;
		reply.data_8bit[4 + i] = 0;
	}

	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &reply);
}


// note that this shouldn't be static since it's called from uw_terminal.c
void canopen_parse_sdo(uw_can_message_st *msg) {
	const uw_canopen_object_st *obj = find_object(sdo_main_index(msg), sdo_sub_index(msg));
#if CONFIG_CANOPEN_LOG
	if (canopen_log) {
		printf("SDO received: %x, %x\n\r", sdo_main_index(msg), sdo_sub_index(msg));
	}
#endif

	if (!obj) {
#if CONFIG_CANOPEN_LOG
		if (canopen_log) {
			printf("SDO object doesn't exist: %x, %x\n\r", sdo_main_index(msg), sdo_sub_index(msg));
		}
#endif
		sdo_send_error(msg, SDO_ERROR_OBJECT_DOES_NOT_EXIST);
		return;
	}
	// SDO READ requests...
	if (sdo_command(msg) == SDO_CMD_READ) {
#if CONFIG_CANOPEN_LOG
		if (canopen_log) {
			printf("SDO read object %x command received\n\r", sdo_main_index(msg));
		}
#endif

		// error with read permissions
		if (!(obj->permissions & UW_RO)) {
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("SDO error: object %x is write only\n\r", sdo_main_index(msg));
			}
#endif
			sdo_send_error(msg, SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
		}
		// for array type objects
		else if (is_array(obj->type)) {
			// indexing the array
			if (sdo_sub_index(msg)) {
				if (sdo_sub_index(msg) <= obj->array_max_size) {
					sdo_send_response(msg, SDO_CMD_READ_RESPONSE_BYTES,
							&obj->data_ptr[(sdo_sub_index(msg) - 1) * array_element_size(obj->type)],
							array_element_size(obj->type));
				}
				else {
#if CONFIG_CANOPEN_LOG
					if (canopen_log) {
						printf("SDO error: overindexing object %x with sub-index %x\n\r",
								sdo_main_index(msg), sdo_sub_index(msg));
					}
#endif
					sdo_send_error(msg, SDO_ERROR_OBJECT_DOES_NOT_EXIST);
				}
			}
			// index 0 is the array max size
			else {
				sdo_send_response(msg, SDO_CMD_READ_RESPONSE_BYTES,
						&obj->array_max_size, 1);
			}
		}
		// normal data
		else {
			// obj->type should straight contain the number of bytes
			sdo_send_response(msg, SDO_CMD_READ_RESPONSE_BYTES, obj->data_ptr, obj->type);
		}
	}

	/*
	 * all SDO write requests...
	 */
	else if (sdo_command(msg) == SDO_CMD_WRITE_1_BYTE ||
			sdo_command(msg) == SDO_CMD_WRITE_2_BYTES ||
			sdo_command(msg) == SDO_CMD_WRITE_4_BYTES ||
			sdo_command(msg) == SDO_CMD_WRITE_BYTES) {

		if (obj->permissions & UW_WO) {
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("SDO write to object %x\n\r", sdo_main_index(msg));
			}
#endif
			// Predefined error register clearing
#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
			if (sdo_main_index(msg) == CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX &&
					sdo_sub_index(msg) == 0) {
				// clear all errors
				uint8_t i;
				uw_ring_buffer_clear(&this->errors);
				for (i = 0; i < obj->array_max_size * array_element_size(obj->type); i++) {
					obj->data_ptr[i] = 0;
				}
#if CONFIG_CANOPEN_LOG
				if (canopen_log) {
					printf("SDO: cleared errors from predefined error field %x\n\r",
							CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX);
				}
#endif
				sdo_send_response(msg, SDO_CMD_WRITE_RESPONSE, &i, 1);
				return;

			}
#endif
			// writing to arrays
			if (is_array(obj->type)) {
				// writing to array subindex 0 as well as overindexing is not permitted
				if (sdo_sub_index(msg) == 0 || sdo_sub_index(msg) >= obj->array_max_size) {
					sdo_send_error(msg, SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					return;
				}
#if CONFIG_CANOPEN_LOG
				if (canopen_log) {
					printf("SDO error: Unsupported access to array type object %x\n\r", sdo_main_index(msg));
				}
#endif
				array_write(obj, sdo_sub_index(msg), msg->data_32bit[1]);
			}
			// writing to all other objects
			uint8_t i;
			for (i = 0; i < obj->type; i++) {
				obj->data_ptr[i] = msg->data_8bit[4 + i];
			}
			i = 0;
			sdo_send_response(msg, SDO_CMD_WRITE_RESPONSE, &i, 1);

			// parameter saving and loading
#if CONFIG_CANOPEN_STORE_PARAMS_INDEX
			if (sdo_main_index(msg) == CONFIG_CANOPEN_STORE_PARAMS_INDEX &&
					sdo_sub_index(msg) == 1) {
				if (obj->data_ptr[0] == 's' &&
						obj->data_ptr[1] == 'a' &&
						obj->data_ptr[2] == 'v' &&
						obj->data_ptr[3] == 'e') {
#if CONFIG_CANOPEN_LOG
					if (canopen_log) {
						printf("Saving settings to flash memory\n\r");
					}
#endif
					*((unsigned int*)obj->data_ptr) = 0;
#if !defined(CONFIG_NON_VOLATILE_MEMORY)
#warning "CANopen store parameters object needs CONFIG_MEMORY defined as 1"
#else
					__uw_save_previous_non_volatile_data();
#endif

				}
			}
#endif
#if CONFIG_CANOPEN_RESTORE_PARAMS_INDEX
			if (sdo_main_index(msg) == CONFIG_CANOPEN_RESTORE_PARAMS_INDEX &&
					sdo_sub_index(msg) == 1) {
				if (obj->data_ptr[0] == 'l' &&
						obj->data_ptr[1] == 'o' &&
						obj->data_ptr[2] == 'a' &&
						obj->data_ptr[3] == 'd') {
#if CONFIG_CANOPEN_LOG
					if (canopen_log) {
						printf("Loading factory settings from flash memory\n\r");
					}
#endif
#if !defined(CONFIG_NON_VOLATILE_MEMORY)
#warning "CANopen restore parameters object needs CONFIG_MEMORY defined as 1"
#else
					*((unsigned int*)obj->data_ptr) = 0;
					__uw_clear_previous_non_volatile_data();
#endif

				}

			}
#endif
		}
		else {
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("SDO error: object %x is write only\n\r", sdo_main_index(msg));
			}
#endif
			sdo_send_error(msg, SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
		}
	}
	else {
#if CONFIG_CANOPEN_LOG
		if (canopen_log) {
			printf("SDO error: invalid command %x\n\r", sdo_command(msg));
		}
#endif
		sdo_send_error(msg, SDO_ERROR_CMD_SPECIFIER_NOT_FOUND);
	}

}

static inline void parse_rxpdo(uw_can_message_st* msg) {
	uint8_t i;
	const uw_canopen_object_st *obj;
	// cycle trough all RXPDO communication parameters
	for (i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		obj = find_object(CONFIG_CANOPEN_RXPDO_COM_INDEX + i, 0);
		if (obj) {
			if (array_index(obj, 1) == msg->id) {
				// RXPDO configured with this message's ID has been found
				obj = find_object(CONFIG_CANOPEN_RXPDO_MAP_INDEX + i, 0);
				if (!obj) {
#if CONFIG_CANOPEN_LOG
					if (canopen_log) {
						printf("RXPDO error: PDO%u configured with ID %x found but corresponding"
							" mapping parameter not found.\n\r", i, msg->id);
					}
#endif
					return;
				}
				uint8_t j;
				uint8_t bytes = 0;
				// cycle trough all mapping entries used
				for (j = 1; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT + 1; j++) {
					unsigned int p = array_index(obj, j);
					if (p) {
						// object index which will be written
						uint16_t index = (p >> 16);
						// object sub-index which will be written
						uint8_t subindex = (p >> 8) & 0xFF;
						// the length of the write operation in bytes
						uint8_t byte_length = (p & 0xFF) / 8;
						const uw_canopen_object_st *trg = find_object(index, subindex);
						if (!trg) {
#if CONFIG_CANOPEN_LOG
							if (canopen_log) {
								printf("RXPDO error: object with index %x and subindex %x not found\n\r",
										index, subindex);
							}
#endif
							break;
						}
#if CONFIG_CANOPEN_LOG
						if (bytes + byte_length > 8) {
							if (canopen_log) {
								printf("RXPDO error: PDO mapping length exceeds 8 bytes\n\r");
							}
						}
#endif
						// write bits to the object
						memcpy(trg->data_ptr, &obj->data_ptr[bytes], byte_length);
						bytes += byte_length;


					}
					else {
						// if array index was zero, PDO mappings have been ended and we return.
						break;
					}
				}
#if CONFIG_CANOPEN_LOG
				if (canopen_log) {
					printf("RXPDO with id %x written\n\r", msg->id);
				}
#endif
				return;
			}
		}
	}

}


#define nmt_command(msg)		(msg->data_8bit[0])
#define nmt_target_node(msg)	(msg->data_8bit[1])
static inline void parse_nmt_message(uw_can_message_st* msg) {
	// check if the NMT message was broadcasted or dedicated to this node
	if (nmt_target_node(msg) == this->node_id || !nmt_target_node(msg)) {
		switch (nmt_command(msg)) {
		case NMT_START_NODE:
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("NMT start\n\r");
			}
#endif
			uw_canopen_set_state(STATE_OPERATIONAL);
			break;
		case NMT_STOP_NODE:
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("NMT stop\n\r");
			}
#endif
			uw_canopen_set_state(STATE_STOPPED);
			break;
		case NMT_SET_PREOPERATIONAL:
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("NMT preoperational\n\r");
			}
#endif
			uw_canopen_set_state(STATE_PREOPERATIONAL);
			break;
		case NMT_RESET_NODE:
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("NMT reset\n\r");
			}
#endif
			uw_system_reset(false);
			break;
		case NMT_RESET_COM:
#if CONFIG_CANOPEN_LOG
			if (canopen_log) {
				printf("NMT reset communication\n\r");
			}
#endif
			uw_canopen_init(this->node_id, this->obj_dict, this->obj_dict_length,
					this->sdo_write_callback, this->emcy_callback);
			break;
		default:
			break;
		}
	}
}

uw_errors_e __uw_canopen_send_sdo(uw_canopen_sdo_message_st *sdo, uint8_t node_id, unsigned int req_response) {

	// to CAN bus bytes are sent as 0, 1, 2, 3 (although CAN should be big-endian)
	// in CANopen data is little-endian, so for example main-index of 0x1017 is sent as 17 10 bytes
	uw_can_message_st msg;
	msg.id = req_response + node_id;
	msg.data_8bit[0] = sdo->request;
	msg.data_8bit[1] = (uint8_t) sdo->main_index;
	msg.data_8bit[2] = (uint8_t) (sdo->main_index >> 8);
	msg.data_8bit[3] = sdo->sub_index;
	msg.data_32bit[1] = sdo->data_32bit;
	msg.data_length = 8;

	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);

	return uw_err(ERR_NONE);
}

#if CONFIG_CANOPEN_IDENTITY_INDEX
void uw_canopen_identity_init(uw_identity_object_st* identity_obj,
		uint32_t product_code, uint32_t revision_number) {
	identity_obj->vendor_id = USEWOOD_VENDOR_ID;
	identity_obj->revision_number = revision_number;
	identity_obj->product_code = product_code;
	uw_get_device_serial(identity_obj->serial_number);
}
#endif


#endif
