/*
 * uw_canopen.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */



#include "uw_canopen.h"
#include "uw_reset.h"
#include "uw_utilities.h"



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
	uw_canopen_object_st* obj_dict;
	unsigned int obj_dict_length;
	uw_canopen_node_states_e state;
	void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry);
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
#define heartbeat()		(*(find_object(CONFIG_CANOPEN_HEARTBEAT_INDEX, 0)->data_ptr))
#define is_array(x)		(x & UW_ARRAY_MASK)
#define get_array_cell_size(x) (x - (1 << 6))

/// @brief: Searches the object dictionary and returns a pointer to found
/// object. If object couldn't be found, returns NULL.
static uw_canopen_object_st *find_object(uint16_t index, uint8_t sub_index) {
	unsigned int i;
	for (i = 0; i < this->obj_dict_length; i++) {
		if (this->obj_dict[i].main_index == index) {
			// object is of array type
			if (is_array(this->obj_dict[i].type)) {
				if (sub_index < this->obj_dict[i].array_max_size) {
					return &this->obj_dict[i];
				}
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


uw_errors_e uw_canopen_init(uw_canopen_object_st* obj_dict, unsigned int obj_dict_length,
		void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry)) {
	this->obj_dict = obj_dict;
	this->obj_dict_length = obj_dict_length;
	this->state = STATE_BOOT_UP;
	this->sdo_write_callback = sdo_write_callback;

	// check that node id and heartbeat time entries are found
	// in obj dict
	uw_canopen_object_st *obj = find_object(CONFIG_CANOPEN_NODEID_INDEX, 0);
	if (!obj) {
		__uw_err_throw(ERR_CANOPEN_NODE_ID_ENTRY_INVALID | HAL_MODULE_CANOPEN);
	}
	if (! (obj= find_object(CONFIG_CANOPEN_HEARTBEAT_INDEX, 0))) {
		__uw_err_throw(ERR_CANOPEN_HEARTBEAT_ENTRY_INVALID| HAL_MODULE_CANOPEN);
	}

	// send boot up message
	uw_can_message_st msg = {
			.id = BOOTUP_ID + node_id(),
			.data_length = 1,
			.data_8bit[0] = 0
	};
	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);

	return ERR_NONE;
}

uw_errors_e uw_canopen_step(unsigned int step_ms) {
	// PDO handling is done only in operational state
	if (this->state == STATE_OPERATIONAL) {

	}

	// send heartbeat message

	return ERR_NONE;
}




void __uw_canopen_parse_message(uw_can_message_st* message) {
	uw_canopen_msg_st m;
	m.node_id = message->id & UW_CANOPEN_NODE_ID_MASK;
	m.type = message->id & (~UW_CANOPEN_NODE_ID_MASK);

	// process messages which are addressed to this node OR broadcasted
	if (node_id() == m.node_id || m.node_id == 0) {

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
	return ERR_NONE;
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

	__uw_canopen_send_sdo(&reply, node_id(), SDO_ERROR_ID);
}

static void sdo_send_response(uw_canopen_sdo_message_st *msg, uw_canopen_sdo_commands_e cmd,
		uint8_t *data, uint8_t data_length) {
	// reply message struct
	uw_canopen_sdo_message_st  reply = {
			.main_index = msg->main_index,
			.sub_index = msg->sub_index,
			.data_length = data_length,
			.request = cmd
	};
	uint8_t i = 0;
	for (i = 0; i < data_length; i++) {
		reply.data_8bit[i] = data[i];
	}

	__uw_canopen_send_sdo(&reply, node_id(), SDO_RESPONSE_ID);
}

static void parse_sdo(uw_canopen_msg_st* req) {
	uw_canopen_object_st *obj = find_object(req->msg.sdo.main_index, req->msg.sdo.sub_index);

	if (!obj) {
		sdo_send_error(&req->msg.sdo, SDO_ERROR_OBJECT_DOES_NOT_EXIST);
		return;
	}
	// SDO READ requests...
	if (req->msg.sdo.request == SDO_CMD_READ) {
		// error with read permissions
		if (!obj->permissions & UW_RO) {
			sdo_send_error(&req->msg.sdo, SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
		}
		// for array type objects
		if (is_array(obj->type)) {
			// indexing the array
			if (req->msg.sdo.sub_index) {
				sdo_send_response(&req->msg.sdo, SDO_CMD_READ_RESPONSE_BYTES,
						&obj->data_ptr[req->msg.sdo.sub_index - 1], get_array_cell_size(obj->type));
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

		}
		else {
			sdo_send_error(&req->msg.sdo, SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
		}
	}

}


static void parse_rxpdo(uw_canopen_msg_st* req) {
	// todo: RX pdo parsing

}

static void parse_nmt_message(uw_canopen_msg_st* req) {
	// check if the NMT message was broadcasted or dedicated to this node
	if (req->msg.nmt.node_id == node_id() || !req->msg.nmt.node_id) {
		switch (req->msg.nmt.command) {
		case NMT_START_NODE:
			uw_canopen_set_state(STATE_OPERATIONAL);
			break;
		case NMT_STOP_NODE:
			uw_canopen_set_state(STATE_STOPPED);
			break;
		case NMT_SET_PREOPERATIONAL:
			uw_canopen_set_state(STATE_PREOPERATIONAL);
			break;
		case NMT_RESET_NODE:
			uw_system_reset(false);
			break;
		case NMT_RESET_COM:
			uw_canopen_init(this->obj_dict, this->obj_dict_length, this->sdo_write_callback);
			break;
		default:
			break;
		}
	}
}

uw_errors_e __uw_canopen_send_sdo(uw_canopen_sdo_message_st *sdo, uint8_t node_id, unsigned int req_response) {

	uw_can_message_st msg;
	printf("Sending SDO...\n\r");
	msg.id = req_response + node_id;
	msg.data_8bit[0] = sdo->request;
	msg.data_8bit[1] = (sdo->main_index >> 8);
	msg.data_8bit[2] = (uint8_t) sdo->main_index;
	msg.data_8bit[3] = sdo->sub_index;
	msg.data_32bit[1] = sdo->data_32bit;
	msg.data_length = sdo->data_length;

	uw_can_send_message(CONFIG_CANOPEN_CHANNEL, &msg);

	return ERR_NONE;
}
