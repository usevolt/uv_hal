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
#if CONFIG_CANOPEN_LOG
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#endif
extern uw_errors_e __uw_save_previous_non_volatile_data();
extern uw_errors_e __uw_clear_previous_non_volatile_data();


/// @brief: Mask for CAN_ID field's node_id bits
#define UW_CANOPEN_NODE_ID_MASK		0x7F




/// @brief: Struct for general CANopen message. This includes all errors,
/// messsage types, as well as the message itself.
typedef struct {
	/// @brief: Tells the type of this message.
	/// This specifies what errors and message structures are
	/// stored in the unions.
	uw_canopen_protocol_ids_e type;
	/// @brief: Stores the CANopen node_id of the message
	uint8_t node_id;
	/// @brief: This holds the message data.
	/// Refer to the message type to fetch the right message
	union {
		uw_canopen_sdo_message_st sdo;
		struct {
			union {
				uint8_t as_8bit[8];
				uint16_t as_16bit[4];
				uint32_t as_32bit[2];
				uint64_t as_64bit;
			};
			uint8_t length;
			unsigned int id;
		} pdo_data;
		struct {
			uint8_t command;
			uint8_t node_id;
		} nmt;
		union {
			uint8_t heartbeat;
			uint8_t boot_up;
		};
	} msg;
} uw_canopen_msg_st;



/// @brief: A local declaration of static variable structure
typedef struct {
	const uw_canopen_object_st* obj_dict;
	unsigned int obj_dict_length;
	uw_canopen_node_states_e state;
	uint8_t node_id;
	void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry);
	int heartbeat_delay;
} this_st;

static this_st _this;
// A C++ style this-pointer definitions
#define this 	(&_this)

/// @brief: Executes the sdo command to either read or write the object dictionary.
/// Modifies the object dictionary as necessary, sends the answer message and executes
/// the callback function from object dictionary entry.
/// Possible errors are handled as well and the request message struct error field is updated
/// correspondly.
static void parse_sdo(uw_canopen_msg_st* req);

/// @brief: Executes RXPDO command.
/// Check's which objects are mapped to the specific PDO and assigns their values accordingly.
static void parse_rxpdo(uw_canopen_msg_st* req);

/// @brief: Parses the received NMT message and does the actions necessary
static void parse_nmt_message(uw_canopen_msg_st* req);

#define node_id()		(*(find_object(CONFIG_CANOPEN_NODEID_INDEX, 0)->data_ptr))
#define is_array(x)		(x & UW_ARRAY_MASK)
#define array_cell_size(x)	(x & (~UW_ARRAY_MASK))
#define get_array_cell_size(x) (x - (1 << 6))

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
		return *((uint16_t*)obj->data_ptr);
	}
	else if (obj->type == UW_UNSIGNED32) {
		return *((uint32_t*)obj->data_ptr);
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



uw_errors_e uw_canopen_init(const uw_canopen_object_st* obj_dict, unsigned int obj_dict_length,
		void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry)) {
	this->obj_dict = obj_dict;
	this->obj_dict_length = obj_dict_length;
	this->state = STATE_BOOT_UP;
	this->sdo_write_callback = sdo_write_callback;
	this->heartbeat_delay = 0;
	this->node_id = node_id();


	// check that node id and heartbeat time entries are found
	// in obj dict
	const uw_canopen_object_st *obj = find_object(CONFIG_CANOPEN_NODEID_INDEX, 0);
	if (!obj) {
		__uw_err_throw(ERR_CANOPEN_NODE_ID_ENTRY_INVALID | HAL_MODULE_CANOPEN);
	}
	if (! (obj = find_object(CONFIG_CANOPEN_HEARTBEAT_INDEX, 0))) {
		__uw_err_throw(ERR_CANOPEN_HEARTBEAT_ENTRY_INVALID| HAL_MODULE_CANOPEN);
	}

	// send boot up message
	uw_can_message_st msg = {
			.id = BOOTUP_ID + this->node_id,
			.data_length = 1,
			.data_8bit[0] = 0
	};
	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);

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

#if CONFIG_CANOPEN_LOG
	printf("canopen initialized with node id %x\n\r", this->node_id);
#endif

	return uw_err(ERR_NONE);
}

uw_errors_e uw_canopen_step(unsigned int step_ms) {

	// in stopped state we do nothing
	if (this->state == STATE_STOPPED) {
		return uw_err(ERR_NONE);
	}

	// send heartbeat message
	const uw_canopen_object_st* obj = find_object(CONFIG_CANOPEN_HEARTBEAT_INDEX, 0);
	if (!obj) {
		__uw_err_throw(ERR_CANOPEN_HEARTBEAT_ENTRY_INVALID | HAL_MODULE_CANOPEN);
	}
	if (uw_delay(step_ms, &this->heartbeat_delay)) {
		// send heartbeat msg
		uw_can_message_st msg = {
				.id = HEARTBEAT_ID + this->node_id,
				.data_length = 1,
				.data_8bit[0] = this->state
		};
		if (uw_can_get_error_state(CONFIG_CANOPEN_CHANNEL) == CAN_ERROR_ACTIVE) {
			uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);
		}

		// start the delay again
		uw_start_delay(get_object_value(obj), &this->heartbeat_delay);
	}

	// PDO handling is done only in operational state
	if (this->state != STATE_OPERATIONAL) {
		return uw_err(ERR_NONE);
	}

	return uw_err(ERR_NONE);
}




void __uw_canopen_parse_message(uw_can_message_st* message) {
	uw_canopen_msg_st m;
	m.node_id = message->id & UW_CANOPEN_NODE_ID_MASK;
	m.type = message->id & (~UW_CANOPEN_NODE_ID_MASK);

	// process messages which are addressed to this node OR broadcasted
	if (this->node_id == m.node_id || m.node_id == 0) {

		m.type = message->id & (~UW_CANOPEN_NODE_ID_MASK);

		switch (message->id & (~UW_CANOPEN_NODE_ID_MASK)) {
		case SDO_REQUEST_ID:
			if (message->data_length < 5) {
				// EMCY message should be sent??
				// UW_CANOPEN_CORRUPTION_ERROR;
				break;
			}
			m.msg.sdo.request = message->data_8bit[0];
			m.msg.sdo.main_index = message->data_8bit[1] + (message->data_8bit[2] << 8);
			m.msg.sdo.sub_index = message->data_8bit[3];
			m.msg.sdo.data_length = message->data_length - 4;
			m.msg.sdo.data_32bit = message->data_32bit[1];
			parse_sdo(&m);
			break;
		case NMT_ID:
			m.msg.nmt.command = message->data_8bit[0];
			m.msg.nmt.node_id = message->data_8bit[1];
			parse_nmt_message(&m);
			break;
		default:
			break;
		}
	}

	// regardless of what kind of message was received, check if it was mapped to any RXPDO
	// (RXPDO's can be mapped with any kind of message)
	m.msg.pdo_data.as_64bit = message->data_64bit;
	m.msg.pdo_data.length = message->data_length;
	parse_rxpdo(&m);
}



uw_errors_e uw_canopen_set_state(uw_canopen_node_states_e state) {
	this->state = state;
	return uw_err(ERR_NONE);
}

uw_canopen_node_states_e uw_canopen_get_state(void) {
	return this->state;
}


/// @brief: Sends a SDO error response
static void sdo_send_error(uw_canopen_sdo_message_st *msg, uw_sdo_error_codes_e error) {
	// reply message struct
	uw_canopen_sdo_message_st  reply = {
			.main_index = msg->main_index,
			.sub_index = msg->sub_index,
			.data_length = 4,
			.request = SDO_CMD_ERROR,
			.data_32bit = error
	};

	__uw_canopen_send_sdo(&reply, this->node_id, SDO_ERROR_ID);
}

static void sdo_send_response(uw_canopen_sdo_message_st *msg, uw_canopen_sdo_commands_e cmd,
		const uint8_t *data, uint8_t data_length) {
	// reply message struct
	uw_canopen_sdo_message_st  reply = {
			.main_index = msg->main_index,
			.sub_index = msg->sub_index,
			.data_length = 4 + data_length,
			.request = cmd
	};
	uint8_t i = 0;
	for (i = 0; i < data_length; i++) {
		reply.data_8bit[i] = data[i];
	}
	while (i < 4) {
		i++;
		reply.data_8bit[i] = 0;
	}

	__uw_canopen_send_sdo(&reply, this->node_id, SDO_RESPONSE_ID);
}

static void parse_sdo(uw_canopen_msg_st* req) {
	const uw_canopen_object_st *obj = find_object(req->msg.sdo.main_index, req->msg.sdo.sub_index);
#if CONFIG_CANOPEN_LOG
	printf("SDO received: %x, %x\n\r", req->msg.sdo.main_index, req->msg.sdo.sub_index);
#endif

	if (!obj) {
#if CONFIG_CANOPEN_LOG
	printf("SDO object doesn't exist: %x, %x\n\r", req->msg.sdo.main_index, req->msg.sdo.sub_index);
#endif
		sdo_send_error(&req->msg.sdo, SDO_ERROR_OBJECT_DOES_NOT_EXIST);
		return;
	}
	// SDO READ requests...
	if (req->msg.sdo.request == SDO_CMD_READ) {
#if CONFIG_CANOPEN_LOG
			printf("SDO read object %x command received\n\r", req->msg.sdo.main_index);
#endif
		// error with read permissions
		if (!obj->permissions & UW_RO) {
#if CONFIG_CANOPEN_LOG
			printf("SDO error: object %x is write only\n\r", req->msg.sdo.main_index);
#endif
			sdo_send_error(&req->msg.sdo, SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
		}
		// for array type objects
		if (is_array(obj->type)) {
			// indexing the array
			if (req->msg.sdo.sub_index) {
				if (req->msg.sdo.sub_index <= obj->array_max_size) {
					sdo_send_response(&req->msg.sdo, SDO_CMD_READ_RESPONSE_BYTES,
							&obj->data_ptr[(req->msg.sdo.sub_index - 1) * array_cell_size(obj->type)],
							get_array_cell_size(obj->type));
				}
				else {
#if CONFIG_CANOPEN_LOG
			printf("SDO error: overindexing object %x with sub-index %x\n\r",
					req->msg.sdo.main_index, req->msg.sdo.sub_index);
#endif
					sdo_send_error(&req->msg.sdo, SDO_ERROR_OBJECT_DOES_NOT_EXIST);
				}
			}
			// index 0 is the array max size
			else {
				sdo_send_response(&req->msg.sdo, SDO_CMD_READ_RESPONSE_BYTES,
						&obj->array_max_size, 1);
			}
		}
		// normal data
		else {
			// obj->type should straight contain the number of bytes
			sdo_send_response(&req->msg.sdo, SDO_CMD_READ_RESPONSE_BYTES, obj->data_ptr, obj->type);
		}
	}
	// all SDO write requests...
	else if (req->msg.sdo.request == SDO_CMD_WRITE_1_BYTE ||
			req->msg.sdo.request == SDO_CMD_WRITE_2_BYTES ||
			req->msg.sdo.request == SDO_CMD_WRITE_4_BYTES ||
			req->msg.sdo.request == SDO_CMD_WRITE_BYTES) {

		if (obj->permissions & UW_WO) {
#if CONFIG_CANOPEN_LOG
			printf("SDO write to object %x\n\r", req->msg.sdo.main_index);
#endif
			// writing to arrays
			if (is_array(obj->type)) {
				// writing to array subindex 0 as well as overindexing is not permitted
				if (req->msg.sdo.sub_index == 0 || req->msg.sdo.sub_index >= obj->array_max_size) {
					sdo_send_error(&req->msg.sdo, SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					return;
				}
#if CONFIG_CANOPEN_LOG
			printf("SDO error: Unsupported access to array type object %x\n\r", req->msg.sdo.main_index);
#endif
				array_write(obj, req->msg.sdo.sub_index, req->msg.sdo.data_32bit);
			}
			// writing to all other objects
			uint8_t i;
			for (i = 0; i < obj->type; i++) {
				obj->data_ptr[i] = req->msg.sdo.data_8bit[i];
			}
			i = 0;
			sdo_send_response(&req->msg.sdo, SDO_CMD_WRITE_RESPONSE, &i, 1);

			// parameter saving and loading
#if CONFIG_CANOPEN_STORE_PARAMS_INDEX
			if (req->msg.sdo.main_index == CONFIG_CANOPEN_STORE_PARAMS_INDEX &&
					req->msg.sdo.sub_index == 1) {
				if (obj->data_ptr[0] == 's' &&
						obj->data_ptr[1] == 'a' &&
						obj->data_ptr[2] == 'v' &&
						obj->data_ptr[3] == 'e') {
#if CONFIG_CANOPEN_LOG
					printf("Saving settings to flash memory\n\r");
					*((unsigned int*)obj->data_ptr) = 0;
					__uw_save_previous_non_volatile_data();
#endif

				}
			}
#endif
#if CONFIG_CANOPEN_RESTORE_PARAMS_INDEX
			if (req->msg.sdo.main_index == CONFIG_CANOPEN_RESTORE_PARAMS_INDEX &&
					req->msg.sdo.sub_index == 1) {
				if (obj->data_ptr[0] == 'l' &&
						obj->data_ptr[1] == 'o' &&
						obj->data_ptr[2] == 'a' &&
						obj->data_ptr[3] == 'd') {
#if CONFIG_CANOPEN_LOG
					printf("Loading factory settings from flash memory\n\r");
					*((unsigned int*)obj->data_ptr) = 0;
					__uw_clear_previous_non_volatile_data();
#endif

				}

			}
#endif
		}
		else {
#if CONFIG_CANOPEN_LOG
			printf("SDO error: object %x is write only\n\r", req->msg.sdo.main_index);
#endif
			sdo_send_error(&req->msg.sdo, SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
		}
	}
	else {
#if CONFIG_CANOPEN_LOG
			printf("SDO error: invalid command %x\n\r", req->msg.sdo.request);
#endif
		sdo_send_error(&req->msg.sdo, SDO_ERROR_CMD_SPECIFIER_NOT_FOUND);
	}

}

static void parse_rxpdo(uw_canopen_msg_st* req) {
	uint8_t i;
	const uw_canopen_object_st *obj;
	// cycle trough all RXPDO communication parameters
	for (i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		obj = find_object(CONFIG_CANOPEN_RXPDO_COM_INDEX + i, 0);
		if (obj) {
			if (array_index(obj, 1) == req->msg.pdo_data.id) {
				// RXPDO configured with this message's ID has been found
				obj = find_object(CONFIG_CANOPEN_RXPDO_MAP_INDEX + i, 0);
				if (!obj) {
#if CONFIG_CANOPEN_LOG
					printf("RXPDO error: PDO%u configured with ID %x found but corresponding"
						" mapping parameter not found.\n\r", i, req->msg.pdo_data.id);
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
				printf("RXPDO error: object with index %x and subindex %x not found\n\r", index, subindex);
#endif
							break;
						}
						if (bytes + byte_length > 8) {
#if CONFIG_CANOPEN_LOG
				printf("RXPDO error: PDO mapping length exceeds 8 bytes\n\r");
#endif
						}
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
				printf("RXPDO with id %x written\n\r", req->msg.pdo_data.id);
#endif
				return;
			}
		}
	}

}

static void parse_nmt_message(uw_canopen_msg_st* req) {
	// check if the NMT message was broadcasted or dedicated to this node
	if (req->msg.nmt.node_id == this->node_id || !req->msg.nmt.node_id) {
		switch (req->msg.nmt.command) {
		case NMT_START_NODE:
#if CONFIG_CANOPEN_LOG
			printf("NMT start\n\r");
#endif
			uw_canopen_set_state(STATE_OPERATIONAL);
			break;
		case NMT_STOP_NODE:
#if CONFIG_CANOPEN_LOG
			printf("NMT stop\n\r");
#endif
			uw_canopen_set_state(STATE_STOPPED);
			break;
		case NMT_SET_PREOPERATIONAL:
#if CONFIG_CANOPEN_LOG
			printf("NMT preoperational\n\r");
#endif
			uw_canopen_set_state(STATE_PREOPERATIONAL);
			break;
		case NMT_RESET_NODE:
#if CONFIG_CANOPEN_LOG
			printf("NMT reset\n\r");
#endif
			uw_system_reset(false);
			break;
		case NMT_RESET_COM:
#if CONFIG_CANOPEN_LOG
			printf("NMT reset communication\n\r");
#endif
			uw_canopen_init(this->obj_dict, this->obj_dict_length, this->sdo_write_callback);
			uw_canopen_set_state(STATE_PREOPERATIONAL);
			break;
		default:
			break;
		}
	}
}

uw_errors_e __uw_canopen_send_sdo(uw_canopen_sdo_message_st *sdo, uint8_t node_id, unsigned int req_response) {

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



#endif
