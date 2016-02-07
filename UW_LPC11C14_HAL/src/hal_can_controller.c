/*
 * uw_can_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#include "hal_can_controller.h"
#include "LPC11xx.h"
#include "stdlib.h"
#include <stdint.h>
#include <stdio.h>

// for debugging can connection
#include "hal_uart_controller.h"
#include "hal_reset_controller.h"
#include "hal_debug.h"

/**************************************************************************
SDO Abort Codes
**************************************************************************/
#define SDO_ABORT_TOGGLE 			0x05030000UL // Toggle bit not alternated
#define SDO_ABORT_SDOTIMEOUT 		0x05040000UL // SDO protocol timed out
#define SDO_ABORT_UNKNOWN_COMMAND 	0x05040001UL // Client/server command specifier not valid or unknown
#define SDO_ABORT_UNSUPPORTED 		0x06010000UL // Unsupported access to an object
#define SDO_ABORT_WRITEONLY 		0x06010001UL // Attempt to read a write only object
#define SDO_ABORT_READONLY 			0x06010002UL // Attempt to write a read only object
#define SDO_ABORT_NOT_EXISTS 		0x06020000UL // Object does not exist in the object dictionary
#define SDO_ABORT_PARAINCOMP 		0x06040043UL // General parameter incompatibility reason
#define SDO_ABORT_ACCINCOMP 		0x06040047UL // General internal incompatibility in the device
#define SDO_ABORT_TYPEMISMATCH 		0x06070010UL // Data type does not match, length of service parameter does not match
#define SDO_ABORT_UNKNOWNSUB 		0x06090011UL // Sub-index does not exist
#define SDO_ABORT_VALUE_RANGE 		0x06090030UL // Value range of parameter exceeded (only for write access)
#define SDO_ABORT_TRANSFER 			0x08000020UL // Data cannot be transferred or stored to the application
#define SDO_ABORT_LOCAL 			0x08000021UL // Data cannot be transferred or stored to the application because of local control
#define SDO_ABORT_DEVSTAT 			0x08000022UL // Data cannot be transferred or stored to the application because of the present device state




// Return values for CANOPEN_sdo_req() callback
#define CAN_SDOREQ_NOTHANDLED 0 // process regularly, no impact
#define CAN_SDOREQ_HANDLED_SEND 1 // processed in callback, auto-send returned msg
#define CAN_SDOREQ_HANDLED_NOSEND 2 // processed in callback, don't send response

// Values for CANOPEN_sdo_seg_read/write() callback 'openclose' parameter
#define CAN_SDOSEG_SEGMENT 0 // segment read/write
#define CAN_SDOSEG_OPEN 1 // channel is opened
#define CAN_SDOSEG_CLOSE 2 // channel is closed

/// @brief: Time limit for pending message objects. If this exceedes, all message objects are
/// released from pending state.
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		50




void CAN_rx(uint8_t msg_obj_num);
void CAN_tx(uint8_t msg_obj_num);
void CAN_error(uint32_t error_info);
uint32_t CANOPEN_sdo_exp_read (uint16_t index, uint8_t subindex);
uint32_t CANOPEN_sdo_exp_write(uint16_t index, uint8_t subindex, uint8_t *dat_ptr);
uint32_t CANOPEN_sdo_seg_read(uint16_t index, uint8_t subindex, uint8_t openclose,
		uint8_t *length, uint8_t *data, uint8_t *last);
uint32_t CANOPEN_sdo_seg_write(uint16_t index, uint8_t subindex, uint8_t openclose,
		uint8_t length, uint8_t *data, uint8_t *fast_resp);
uint8_t CANOPEN_sdo_req(uint8_t length_req, uint8_t *req_ptr,
		uint8_t *length_resp, uint8_t *resp_ptr);



//************** CANopen configuration ***********************/

/// @brief: C_CAN message data structure
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	uint8_t node_id;
	hal_can_msg_objs_t msgobj_rx;
	hal_can_msg_objs_t msgobj_tx;
	uint8_t isr_handled;
	uint32_t od_const_num;
	hal_canopen_obj_dict_const_entry_st *od_const_table;
	uint32_t od_num;
	hal_canopen_obj_dict_entry_st *od_table;

} CCAN_CANOPENCFG_T;

/// static variable for CANopen configuration
static CCAN_CANOPENCFG_T canopen_node_config;



/************ C_CAN hardware configuration ************************/

// LPC11C22 C_CAN driver struct. Do not change!
typedef struct CCAN_CALLBACKS {
	void (*CAN_rx)(uint8_t msg_obj_num);
	void (*CAN_tx)(uint8_t msg_obj_num);
	void (*CAN_error)(uint32_t error_info);
	uint32_t (*CANOPEN_sdo_read)(uint16_t index, uint8_t subindex);
	uint32_t (*CANOPEN_sdo_write)(uint16_t index, uint8_t subindex, uint8_t *dat_ptr);
	uint32_t (*CANOPEN_sdo_seg_read)(uint16_t index, uint8_t subindex, uint8_t openclose, uint8_t *length,
	uint8_t *data, uint8_t *last);
	uint32_t (*CANOPEN_sdo_seg_write)(uint16_t index, uint8_t subindex, uint8_t openclose, uint8_t length,
	uint8_t *data, uint8_t *fast_resp);
	uint8_t (*CANOPEN_sdo_req)(uint8_t length_req, uint8_t *req_ptr, uint8_t *length_resp, uint8_t *resp_ptr);
} CCAN_CALLBACKS_T;



// LPC11C22 C_CAN driver struct. Do not change!
typedef struct CCAN_API {
	void (*init_can)(uint32_t *can_cfg, uint8_t isr_ena);
	void (*isr)(void);
	void (*config_rxmsgobj)(hal_can_msg_obj_st *msg_obj);
	uint8_t (*can_receive)(hal_can_msg_obj_st *msg_obj);
	void (*can_transmit)(hal_can_msg_obj_st *msg_obj);
	void (*config_canopen)(CCAN_CANOPENCFG_T *canopen_cfg);
	void (*canopen_handler)(void);
	void (*config_calb)(CCAN_CALLBACKS_T *callback_cfg);
} CCAN_API_T;


// LPC11C22 C_CAN driver struct. Do not change!
typedef struct ROM_API {
	const uint32_t unused[2];
	const CCAN_API_T *pCCAND;
} ROM_API_T;



#define ROM_DRIVER_BASE (0x1FFF1FF8UL)
#define LPC_CCAN_API ((CCAN_API_T *) ((*(ROM_API_T * *) (ROM_DRIVER_BASE))->pCCAND))


/// application layers callback function for received can messages
static void (*rx_callback)(hal_can_msg_obj_st*) = NULL;
// application layer callback for CAN errors
static void(*error_callback)(uint32_t) = NULL;
/// application layer callback function for received CANopen PDO's
static void (*pdo_callback)(hal_canopen_pdo_types_e pdo) = NULL;
/// application layer callback function for received CANopen SDO reads
static void (*sdo_write_callback)(uint16_t index, uint8_t subindex, uint8_t* data) = NULL;
//application layer callback for nmt commands
static bool (*nmt_callback)(hal_canopen_nmt_messages_e command) = NULL;

static uw_device_status_e* device_status = NULL;

// message structs for all tx and rx pdos
hal_can_msg_obj_st hal_canopen_pdo_msg[CANOPEN_PDO_COUNT];

// message object for debug console
hal_can_msg_obj_st hal_can_debug_rx_msg;
hal_can_msg_obj_st hal_can_debug_tx_msg;

// message struct for CANopen NMT messages
hal_can_msg_obj_st hal_canopen_nmt_msg;


/// @brief: global message object for CAN receive callback function parameter
static hal_can_msg_obj_st msg_obj;

/// @brief: variable remebering all message objects which wait for message to be sent
static uint32_t pending_msg_objs = 0;

/// @brief: Timer for pending message objects. If no message objects are set or released from pending
/// in a certain time limit, all pending objects are cleared. This needs to be done since the LPC11C2x
/// hardware misses some of the can tx callback calls and message objects may be left pending for
/// infinite time.
static int8_t pending_msg_obj_time_limit[MSG_OBJ_COUNT];


/* CAN receive callback */
/* Function is executed by the Callback handler after
a CAN message has been received */
void CAN_rx(uint8_t msg_obj_num) {

	/* Determine which CAN message has been received */
	msg_obj.msgobj = msg_obj_num;
	/* Now load up the msg_obj structure with the CAN message */
	LPC_CCAN_API->can_receive(&msg_obj);

	// check if message object was debug rx message
	if (msg_obj.msgobj == 14) {
		hal_debug_process_rx_msg(msg_obj.data, msg_obj.data_length);
		// don't call application callback
		return;
	}

	// call application callback if assigned
	if (rx_callback != NULL) {
		rx_callback(&msg_obj);
	}

	//check if message obj was any of CANopen PDO objects
	if (msg_obj.msgobj >= 8 && msg_obj.msgobj <= 13 &&
			pdo_callback != NULL) {
		int i;
		hal_canopen_pdo_msg[msg_obj.msgobj - 8].data_length = msg_obj.data_length;
		for (i = 0; i < msg_obj.data_length; i++) {
			hal_canopen_pdo_msg[msg_obj.msgobj - 8].data[i] = msg_obj.data[i];
		}
		for (i = msg_obj.data_length; i < 8; i++) {
			hal_canopen_pdo_msg[msg_obj.msgobj - 8].data[i] = 0;
		}
		pdo_callback(msg_obj.msgobj - 8);
	}
	//check if message object was CANopen NMT receiver object
	else if (msg_obj.msgobj == 5 && nmt_callback != NULL) {
		if (msg_obj.data[1] == 0 || msg_obj.data[1] == canopen_node_config.node_id) {
			//call the application layers callback
			if (!nmt_callback(msg_obj.data[0])) {
				return;
			}
			//check the received command if device status is not NULL
			if (device_status == NULL) {
				return;
			}

			switch (msg_obj.data[0]) {
			case CANOPEN_NMT_START_CMD:
				// start node
				*device_status = UW_DEVICE_STATUS_OPERATIONAL;
				break;
			case CANOPEN_NMT_STOP_CMD:
				//stop node
				*device_status = UW_DEVICE_STATUS_OFF;
				break;
			case CANOPEN_NMT_PREOP_CMD:
				//set node to preoperational
				*device_status = UW_DEVICE_STATUS_PREOPERATIONAL;
				break;
			case CANOPEN_NMT_RESET_CMD:
				//reset node
				hal_system_reset(true);
				break;
			case CANOPEN_NMT_RESET_COMMUNICATION_CMD:
				//reset node
				hal_system_reset(false);
				break;
			default:
				break;
			}
		}
	}

}



/* CAN transmit callback */
/* Function is executed by the Callback handler after
a CAN message has been transmitted */
void CAN_tx(uint8_t msg_obj_num) {
	// clear pending msg object
	pending_msg_objs &= ~(1 << msg_obj_num);
	pending_msg_obj_time_limit[msg_obj_num] = 0;
}



/* CAN error callback */
/* Function is executed by the Callback handler after
an error has occured on the CAN bus */
void CAN_error(uint32_t error_info) {
	hal_uart0_send_str("CAN error received:");
	if (error_info & CAN_ERROR_ACK) {
		hal_uart0_send_str(" ACK");
	}
	if (error_info & CAN_ERROR_BIT0) {
		hal_uart0_send_str(" BIT0");
	}
	if (error_info & CAN_ERROR_BIT1) {
		hal_uart0_send_str(" BIT1");
	}
	if (error_info & CAN_ERROR_BOFF) {
		hal_uart0_send_str(" BOFF");
		hal_uart0_send_str("\n\r** CAN bus off recovery started **\n\r\n\r");
		//set C_CAN up again. The CAN 2.0 specification of bus off recovery sequence will start
		LPC_CAN->CNTL &= ~0x1;
	}
	if (error_info & CAN_ERROR_CRC) {
		hal_uart0_send_str(" CRC");
	}
	if (error_info & CAN_ERROR_FORM) {
		hal_uart0_send_str(" FORM");
	}
	if (error_info & CAN_ERROR_NONE) {
		hal_uart0_send_str(" NONE");
	}
	if (error_info & CAN_ERROR_PASS) {
		hal_uart0_send_str(" PASS");
	}
	if (error_info & CAN_ERROR_STUF) {
		hal_uart0_send_str(" STUF");
	}
	if (error_info & CAN_ERROR_WARN) {
		hal_uart0_send_str(" WARN");
	}
	hal_uart0_send_str("\n\r");
	if (LPC_CAN->STAT & (1 <<7)) {
		hal_uart0_send_str("Device is in CAN bus off state\n\r");
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		hal_uart0_send_str("Device is in CAN error passive state\n\r");
	}

	if (error_callback) {
		error_callback(error_info);
	}
}

uint32_t CANOPEN_sdo_seg_read(uint16_t index, uint8_t subindex, uint8_t openclose,
		uint8_t *length, uint8_t *data, uint8_t *last) {
	return 0;
}
uint32_t CANOPEN_sdo_seg_write(uint16_t index, uint8_t subindex, uint8_t openclose,
		uint8_t length, uint8_t *data, uint8_t *fast_resp) {
	return 0;
}

uint8_t CANOPEN_sdo_req(uint8_t length_req, uint8_t *req_ptr,
		uint8_t *length_resp, uint8_t *resp_ptr) {
	return 0;
}
// CANopen sdo read callback
// Function is executed by the Callback handler after
// CANopen SDO read was called
uint32_t CANOPEN_sdo_exp_read (uint16_t index, uint8_t subindex) {
	return 0;
}



// CANopen sdo write callback
// Function is executed by the Callback handler after
// CANopen SDO write was requested
uint32_t CANOPEN_sdo_exp_write(uint16_t index, uint8_t subindex, uint8_t *dat_ptr) {
	if (sdo_write_callback) {
		sdo_write_callback(index, subindex, dat_ptr);
	}

	return 0;
}


inline bool hal_can_send_raw_msg(hal_can_msg_obj_st* message) {
	if (!(pending_msg_objs & (1 << message->msgobj))) {
		// mark message object as pending
		pending_msg_objs |= (1 << message->msgobj);
		pending_msg_obj_time_limit[message->msgobj] = 0;
		// send the message
		LPC_CCAN_API->can_transmit(message);

		return true;
	}
	return false;
}



inline void hal_can_config_rx_msg_obj(hal_can_msg_objs_t obj,
		uint32_t mode_id,
		uint32_t mask) {
	msg_obj.msgobj = obj;
	msg_obj.mask = mask;
	msg_obj.msg_id = mode_id;
	LPC_CCAN_API->config_rxmsgobj(&msg_obj);
}


inline void hal_can_register_rx_callback(
		void (*callback_function) (hal_can_msg_obj_st*)) {
	rx_callback = callback_function;
}


inline void hal_can_register_error_callback(void (*callback_function)(uint32_t)) {
	error_callback = callback_function;
}



void CAN_IRQHandler(void) {
	LPC_CCAN_API->isr();
}


inline bool hal_can_msg_obj_ready(hal_can_msg_objs_e msg_obj) {
	return !(pending_msg_objs & (1 << msg_obj));
}


void hal_can_reset() {
	//clearing the C_CAN bit resets C_CAN hardware
	LPC_SYSCON->PRESETCTRL &= ~(1 << 3);
	//set bit to de-assert the reset
	LPC_SYSCON->PRESETCTRL |= (1 << 3);
}


uint8_t hal_can_get_error_state() {
	if (LPC_CAN->STAT & (1 << 7)) {
		return CAN_ERROR_BUS_OFF;
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		return CAN_ERROR_PASSIVE;
	}
	else if (LPC_CAN->STAT & (1 << 6)) {
		return CAN_ERROR_WARNING;
	}
	else return CAN_ERROR_ACTIVE;

}



inline void hal_init_can(uint32_t baudrate, uint32_t fosc) {
	uint8_t i;
	uint32_t CanApiClkInitTable[] = {
		//CANCLKDIV register
		0,
		//BTR register: TSEG2 = 2, TSEG1 = 1, SJW = 0 (equals to 0x2300)
		0x2300 + ((fosc / (8 * baudrate) - 1) & 0x3F)
	};
	/* Publish CAN Callback Functions */
	CCAN_CALLBACKS_T callbacks = {
		CAN_rx,
		CAN_tx,
		CAN_error,
		CANOPEN_sdo_exp_read,
		CANOPEN_sdo_exp_write,
		CANOPEN_sdo_seg_read,
		CANOPEN_sdo_seg_write,
		CANOPEN_sdo_req
	};

	LPC_CCAN_API->init_can(&CanApiClkInitTable[0], 1);
	/* Configure the CAN callback functions */
	LPC_CCAN_API->config_calb(&callbacks);
	/* Enable the CAN Interrupt */
	NVIC_EnableIRQ(CAN_IRQn);

#if CAN_LOOP_BACK_MODE
	// enable loop-back mode for testing
	LPC_CAN->CNTL |= 1 << 7;
	LPC_CAN->TEST |= 1 << 4;
#endif

	for (i = 0; i < MSG_OBJ_COUNT; i++) {
		pending_msg_obj_time_limit[i] = 0;
	}
}


void hal_can_step(unsigned int step_ms) {
	//check if the pending time limit has been exceeded.
	// If so, clear the pending message object.
	uint8_t i;
	for (i = 0; i < MSG_OBJ_COUNT; i++) {
		if (pending_msg_obj_time_limit[i] < PENDING_MSG_OBJ_TIME_LIMIT_MS) {
			pending_msg_obj_time_limit[i] += step_ms;
		}
		else {
			pending_msg_objs &= ~(1 << i);
		}
	}
}



void hal_canopen_init_node(uint8_t node_id,
		uint32_t object_dictionary_const_entry_count,
		hal_canopen_obj_dict_const_entry_st* object_dictionary_const_entries,
		uint32_t object_dictionary_entry_count,
		hal_canopen_obj_dict_entry_st* object_dictionary_entries) {
	int i;
	for (i = 0; i < CANOPEN_PDO_COUNT; i++) {
		//set message id according to CANopen specification
		if (i < CANOPEN_RXPDO1) {
			hal_canopen_pdo_msg[i].msg_id = CANOPEN_TX_PDO1_ID + 0x100 * i + node_id;
		}
		else {
			hal_canopen_pdo_msg[i].msg_id = CANOPEN_RX_PDO1_ID + 0x100 * i + node_id;
		}
		//data length defaults to 8 bit
		hal_canopen_pdo_msg[i].data_length = 8;
		//message objects 8 - 13 are used for PDO's
		hal_canopen_pdo_msg[i].msgobj = 8 + i;
		//mask defaults to 11 bit CAN ID
		hal_canopen_pdo_msg[i].mask = 0x3FF;
		//every rx pdo should be configured as such
		if (i >= CANOPEN_RXPDO1) {
			LPC_CCAN_API->config_rxmsgobj(&hal_canopen_pdo_msg[i]);
		}
	}
	canopen_node_config.isr_handled = 1;
	canopen_node_config.msgobj_rx = 6;
	canopen_node_config.msgobj_tx = 7;
	canopen_node_config.node_id = node_id;
	canopen_node_config.od_const_num = object_dictionary_const_entry_count;
	canopen_node_config.od_const_table = object_dictionary_const_entries;
	canopen_node_config.od_num = object_dictionary_entry_count;
	canopen_node_config.od_table = object_dictionary_entries;
	LPC_CCAN_API->config_canopen(&canopen_node_config);

	hal_canopen_nmt_msg.data_length = 8;
	hal_canopen_nmt_msg.mask = 0x7FF;
	hal_canopen_nmt_msg.msg_id = 0;
	hal_canopen_nmt_msg.msgobj = 5;
	LPC_CCAN_API->config_rxmsgobj(&hal_canopen_nmt_msg);

	//set device status to preoperational
	if (device_status) {
		*device_status = UW_DEVICE_STATUS_PREOPERATIONAL;
	}

	//debug rx message object
	hal_can_debug_rx_msg.data_length = 8;
	hal_can_debug_rx_msg.mask = 0x1FFFFFFF;
	hal_can_debug_rx_msg.msg_id = hal_can_debug_rx_msg_id_base + canopen_node_config.node_id;
	hal_can_debug_rx_msg.msgobj = 14;
	LPC_CCAN_API->config_rxmsgobj(&hal_can_debug_rx_msg);

	// debug tx message object
	hal_can_debug_tx_msg.data_length = 8;
	hal_can_debug_tx_msg.mask = 0x1FFFFFFF;
	hal_can_debug_tx_msg.msg_id = hal_can_debug_tx_msg_id_base + canopen_node_config.node_id;
	hal_can_debug_tx_msg.msgobj = 15;


}

inline uw_device_status_e hal_canopen_get_device_status() {
	return *device_status;
}


inline void hal_canopen_set_device_status_ptr(uw_device_status_e* value) {
	device_status = value;
}


inline hal_can_pdo_return_values_e hal_canopen_send_pdo(hal_canopen_pdo_types_e pdo) {
	if (!device_status) {
		return PDO_DEVICE_STATUS_NULLPTR;
	}
	if (*device_status != UW_DEVICE_STATUS_OPERATIONAL) {
		return PDO_DEVICE_STATUS_NOT_OPERATIONAL;
	}
	if (pdo < CANOPEN_RXPDO1) {
		//send the message
		if (hal_can_send_raw_msg(&hal_canopen_pdo_msg[pdo])) {
			return PDO_SENT_SUCCESFULLY;
		}
		else {
			return PDO_MSG_OBJ_PENDING;
		}
	}
	else {
		return PDO_INVALID_PDO_NUM;
	}
}


inline bool hal_canopen_force_send_pdo(hal_canopen_pdo_types_e pdo) {
	if (device_status && *device_status != UW_DEVICE_STATUS_OPERATIONAL) {
		return false;
	}
	if (pdo < CANOPEN_RXPDO1) {
		//send the message
		hal_can_send_raw_msg(&hal_canopen_pdo_msg[pdo]);
		//wait until the sent message object has been removed from pending msg objs.
		while (!hal_can_msg_obj_ready(hal_canopen_pdo_msg[pdo].msgobj));
		return true;
	}
	return false;
}



inline void hal_canopen_register_pdo_callback(
		void (*callback_function)(hal_canopen_pdo_types_e pdo)) {

	pdo_callback = callback_function;
}

inline void hal_canopen_register_sdo_write_callback(void (*callback_function)
		(uint16_t index, uint8_t subindex, uint8_t* data)) {
	sdo_write_callback = callback_function;
}

inline void hal_canopen_register_nmt_callback(bool (*callback_function)
		(hal_canopen_nmt_messages_e command)) {
	nmt_callback = callback_function;
}


void hal_canopen_set_pdo_data(hal_canopen_pdo_types_e pdo,
		uint8_t index,
		uint8_t* data,
		uint8_t data_length) {
	// CAN message can have 8 bytes max
	int i;
	//return if pdo number was invalid
	if (pdo > CANOPEN_PDO_COUNT) {
		return;
	}
	// increase data length if necessary
	if (hal_canopen_pdo_msg[pdo].data_length < index + data_length) {
		hal_canopen_pdo_msg[pdo].data_length = data_length;
	}
	//write the data into the message object
	for ( i = index; i <  index + data_length; i++) {
		hal_canopen_pdo_msg[pdo].data[i] = data[i - index];
	}
}


uint8_t* hal_canopen_get_pdo_data(hal_canopen_pdo_types_e pdo) {
	return hal_canopen_pdo_msg[pdo].data;
}



bool hal_canopen_send_boot_up_msg(hal_can_msg_obj_st* msg) {
	if (msg->msgobj >= MSG_OBJ_COUNT) {
		//message object not valid. Set some message object
		msg->msgobj = MSG_OBJ_28;
	}
	msg->msg_id = CANOPEN_BOOTUP_ID + canopen_node_config.node_id;
	msg->data_length = 1;
	msg->data[0] = 0;
	return hal_can_send_raw_msg(msg);
}


bool hal_canopen_send_nmt_command(hal_can_msg_obj_st* msg, hal_canopen_nmt_messages_e cmd,
		uint8_t target_node_id) {
	if (msg->msgobj >= MSG_OBJ_COUNT) {
		//message object not valid. Set some message object
		msg->msgobj = MSG_OBJ_28;
	}
	msg->msg_id = CANOPEN_NMT_ID;
	msg->data_length = 2;
	msg->data[1] = target_node_id;
	msg->data[0] = cmd;
	return hal_can_send_raw_msg(msg);
}



bool hal_canopen_send_sdo_read_request(hal_can_msg_obj_st* msg,
		uint8_t target_node_id,
		uint16_t index,
		uint8_t sub_index) {
	if (msg->msgobj >= MSG_OBJ_COUNT) {
		//message object not valid. Message sending failed
		return false;
	}
	msg->data_length = 8;
	msg->data[0] = 0x40;
	msg->data[1] = index & 0xFF;
	msg->data[2] = index >> 8;
	msg->data[3] = sub_index;
	msg->data[4] = msg->data[5] = msg->data[6] = msg->data[7] = 0;
	msg->msg_id = CANOPEN_SDO_REQUEST_ID + target_node_id;
	return hal_can_send_raw_msg(msg);
}



bool hal_canopen_send_sdo_write_request(hal_can_msg_obj_st* msg,
		uint8_t target_node_id,
		uint16_t index,
		uint8_t sub_index,
		uint8_t* data,
		uint8_t data_length) {
	if (msg->msgobj >= MSG_OBJ_COUNT) {
		//message object not valid. Message sending failed
		return false;
	}
	msg->data_length = 8;
	switch (data_length) {
	case 1:
		msg->data[0] = 0x2F;
		break;
	case 2:
		msg->data[0] = 0x2B;
		break;
	case 4:
		msg->data[0] = 0x23;
		break;
	default: return false;
	}
	int i;
	msg->data[1] = index & 0xFF;
	msg->data[2] = index >> 8;
	msg->data[3] = sub_index;
	//set data to message
	for (i = 0; i < data_length; i++) {
		msg->data[4 + i] = data[i];
	}
	//set all rest bytes to 0
	for (i = data_length; i < 4; i++) {
		msg->data[4 + i] = 0;
	}
	msg->msg_id = CANOPEN_SDO_REQUEST_ID + target_node_id;
	return hal_can_send_raw_msg(msg);
}



