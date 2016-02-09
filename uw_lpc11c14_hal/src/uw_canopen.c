/*
 * uw_canopen.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */



#include "uw_canopen.h"
#include "uw_utilities.h"

/// @brief: A local declaration of static variable structure
typedef struct {
	uw_canopen_obj_dict_entry_st* obj_dict;
	unsigned int obj_dict_length;
	uint16_t heartbeat_time;
	uint16_t pdo_time;
	uw_canopen_node_states_e state;
} this_st;

static this_st _this;
// A C++ style this-pointer definitions
#define this 	(&_this)

/// @brief: Executes the sdo command to either read or write the object dictionary.
/// Modifies the object dictionary as necessary, sends the answer message and executes
/// the callback function from object dictionary entry.
/// Possible errors are handled as well and the request message struct error field is updated
/// correspondly.
static void exec_sdo_command(uw_canopen_msg_st* req);



void uw_canopen_init(uw_canopen_obj_dict_entry_st* obj_dict, unsigned int obj_dict_length) {
	this->obj_dict = obj_dict;
	this->obj_dict_length = obj_dict_length;
	this->state = UW_CANOPEN_STATE_BOOT_UP;
}

uw_canopen_msg_st uw_canopen_parse_message(uw_can_message_st* message) {
	uw_canopen_msg_st m;
	m.errors.general = UW_CANOPEN_NO_ERROR;
	m.node_id = message->id & UW_CANOPEN_NODE_ID_MASK;
	m.type = UW_CANOPEN_UNKNOWN_PROTOCOL;

	switch (message->id & (~UW_CANOPEN_NODE_ID_MASK)) {

	case UW_CANOPEN_SDO_REQUEST_ID:
		m.type = UW_CANOPEN_SDO_REQUEST_ID;
		if (message->data_length < 5) {
			m.errors.general = UW_CANOPEN_CORRUPTION_ERROR;
			return m;
		}
		m.msg.sdo.request = message->data_8bit[0];
		m.msg.sdo.index = message->data_8bit[1] + (message->data_8bit[2] << 8);
		m.msg.sdo.subindex = message->data_8bit[3];
		m.msg.sdo.data_length = message->data_length - 4;
		m.msg.sdo.data_32bit = message->data_32bit[1];
		exec_sdo_command(&m);
		return m;
	case UW_CANOPEN_SDO_RESPONSE_ID:
		m.type = UW_CANOPEN_SDO_RESPONSE_ID;
		if (message->data_length < 5) {
			m.errors.general = UW_CANOPEN_CORRUPTION_ERROR;
			return m;
		}
		m.msg.sdo.request = message->data_8bit[0];
		m.msg.sdo.index = message->data_8bit[1] + (message->data_8bit[2] << 8);
		m.msg.sdo.subindex = message->data_8bit[3];
		m.msg.sdo.data_32bit = message->data_32bit[1];
		m.msg.sdo.data_length = message->data_length;
		exec_sdo_command(&m);
		return m;
	case UW_CANOPEN_TXPDO1_ID:
		m.type = UW_CANOPEN_TXPDO1_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_TXPDO2_ID:
		m.type = UW_CANOPEN_TXPDO2_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_TXPDO3_ID:
		m.type = UW_CANOPEN_TXPDO3_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_TXPDO4_ID:
		m.type = UW_CANOPEN_TXPDO4_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_RXPDO1_ID:
		m.type = UW_CANOPEN_RXPDO1_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_RXPDO2_ID:
		m.type = UW_CANOPEN_RXPDO2_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_RXPDO3_ID:
		m.type = UW_CANOPEN_RXPDO3_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	case UW_CANOPEN_RXPDO4_ID:
		m.type = UW_CANOPEN_RXPDO4_ID;
		m.msg.pdo.data_64bit = message->data_64bit;
		m.msg.pdo.data_length = message->data_length;
		return m;
	// Also includes boot up message
	case UW_CANOPEN_HEARTBEAT_ID:
		m.type = UW_CANOPEN_HEARTBEAT_ID;
		m.msg.heartbeat = message->data_8bit[0];
		return m;
	case UW_CANOPEN_NMT_ID:
		m.type = UW_CANOPEN_NMT_ID;
		m.msg.nmt = message->data_8bit[0];
		return m;
	default:
		m.errors.general = UW_CANOPEN_ERROR_UNKNOWN_PROTOCOL;
		return m;
	}
}

void uw_canopen_step(unsigned int step_ms) {

}



static void exec_sdo_command(uw_canopen_msg_st* req) {
	int i;
	for (i = 0; i < this->obj_dict_length; i++) {
		if (this->obj_dict[i].index == req->msg.sdo.index &&
				this->obj_dict[i].subindex == req->msg.sdo.subindex) {
			switch (req->msg.sdo.request) {
			case UW_SDO_CMD_READ:
				break;
			case UW_SDO_CMD_WRITE_1_BYTE:
				break;
			case UW_SDO_CMD_WRITE_2_BYTES:
				break;
			case UW_SDO_CMD_WRITE_4_BYTES:
				break;
			}
		}
	}
}

void uw_canopen_set_state(uw_canopen_node_states_e state) {
	this->state = state;
}
