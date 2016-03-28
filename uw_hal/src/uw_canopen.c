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
	uw_canopen_object_st* obj_dict;
	unsigned int obj_dict_length;
	uw_canopen_node_states_e state;
	uint8_t node_id;
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


void uw_canopen_init(uint8_t node_id, uw_canopen_object_st* obj_dict, unsigned int obj_dict_length,
		void (*sdo_write_callback)(uw_canopen_object_st* obj_dict_entry)) {
	this->obj_dict = obj_dict;
	this->obj_dict_length = obj_dict_length;
	this->state = STATE_BOOT_UP;
	this->sdo_write_callback = sdo_write_callback;
	this->node_id = node_id;
}

void uw_canopen_step(unsigned int step_ms) {

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
		// these protocol messages with this device's node-ID don't cause any
		// actions. (PDO's are processed regardless of received message's node-ID,
		// heartbeat message shouldn't be received, only transmitted.
		case SDO_RESPONSE_ID:
		case TXPDO1_ID:
		case TXPDO2_ID:
		case TXPDO3_ID:
		case TXPDO4_ID:
		case RXPDO1_ID:
		case RXPDO2_ID:
		case RXPDO3_ID:
		case RXPDO4_ID:
		case HEARTBEAT_ID:
			break;
		case NMT_ID:
			m.msg.nmt = message->data_8bit[0];
			parse_nmt_message(&m);
			break;
		default:
			m.type = UNKNOWN_PROTOCOL;
		}
	}

	// regardless of what kind of message was received, check if it was mapped to any RXPDO
	// (RXPDO's can be mapped with any kind of message)
	m.msg.pdo_data.as_64bit = message->data_64bit;
	m.msg.pdo_data.length = message->data_length;
	parse_rxpdo(&m);
}

void __uw_canopen_set_state(uw_canopen_node_states_e state) {
	this->state = state;
}

uw_canopen_node_states_e __uw_canopen_get_state(void) {
	return this->state;
}


static void parse_sdo(uw_canopen_msg_st* req) {
	int i;
	for (i = 0; i < this->obj_dict_length; i++) {
		if (this->obj_dict[i].main_index == req->msg.sdo.main_index &&
				this->obj_dict[i].sub_index == req->msg.sdo.sub_index) {
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


static void parse_rxpdo(uw_canopen_msg_st* req) {

}

static void parse_nmt_message(uw_canopen_msg_st* req) {

}

