/*
 * uw_can.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#include "uw_can.h"


#include "uw_utilities.h"
#include <stdio.h>
#if CONFIG_TARGET_LPC178X
#include "LPC177x_8x.h"
#warning "Implementation needs verification for CONFIG_TARGET_LPC178X"
#endif
#if CONFIG_TARGET_LPC11CXX
#include "LPC11xx.h"
enum {
	MSG_OBJ_COUNT = 32
};
#endif

typedef struct {
#ifdef CAN_ENABLE_ASYNCHRONOUS_MODE
#ifdef CAN_TX_BUFFER_SIZE
	uw_can_message_st tx_buffer[CAN_TX_BUFFER_SIZE];
#else
	uw_can_message_st tx_buffer[CAN_ENABLE_ASYNCHRONOUS_MODE];
#endif
#ifdef CAN_RX_BUFFER_SIZE
	uw_can_message_st tx_buffer[CAN_RX_BUFFER_SIZE];
#else
	uw_can_message_st tx_buffer[CAN_ENABLE_ASYNCHRONOUS_MODE];
#endif
#else

#endif
#if CONFIG_TARGET_LPC11CXX
	uint32_t pending_msg_objs;
	uint32_t used_msg_objs;
	int8_t pending_msg_obj_time_limit[MSG_OBJ_COUNT];
#elif CONFIG_TARGET_LPC178X

#endif
	void (*rx_callback[CAN_COUNT])(void *user_ptr, uw_can_message_st *msg);
} this_st;
static this_st _this;
#define this (&_this)





static bool check_channel(uw_can_channels_e channel) {
	if (channel < CAN_COUNT) {
		return true;
	}
	else {
		printf("Error: CAN channel %u don't exist on this hardware\n\r", channel);
		return false;
	}
}



// to keep this file clear, first all function are defined for CONFIG_TARGET_LPC11CXX and
// after that to CONFIG_TARGET_LPC178X
#if CONFIG_TARGET_LPC11CXX

/// @brief: Time limit for pending message objects. If this exceedes, all message objects are
/// released from pending state.
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		50

#define CAN_ERROR_NONE 		0x00000000UL
#define CAN_ERROR_PASS 		0x00000001UL
#define CAN_ERROR_WARN 		0x00000002UL
#define CAN_ERROR_BOFF 		0x00000004UL
#define CAN_ERROR_STUF 		0x00000008UL
#define CAN_ERROR_FORM 		0x00000010UL
#define CAN_ERROR_ACK 		0x00000020UL
#define CAN_ERROR_BIT1 		0x00000040UL
#define CAN_ERROR_BIT0 		0x00000080UL
#define CAN_ERROR_CRC 		0x00000100UL



void CAN_rx(uint8_t msg_obj_num);
void CAN_tx(uint8_t msg_obj_num);
void CAN_error(uint32_t error_info);

/// control bits for hal_can_msg_obj_st.msg_id
enum {
	CAN_MSGOBJ_STD = 0x00000000UL, /* CAN 2.0a 11-bit ID */
	CAN_MSGOBJ_EXT = 0x20000000UL, /* CAN 2.0b 29-bit ID */
	CAN_MSGOBJ_DAT = 0x00000000UL, /* data frame */
	CAN_MSGOBJ_RTR = 0x40000000UL /* rtr frame */
};


/************ C_CAN hardware configuration ************************/

/// @brief: Basic CAN message data structure
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	/// @brief: Messages ID and mode.
	/// Mode defines if message is 11 bit, 29 bit data frame or rtr frame.
	/// Control bits are OR'red with message ID, for example
	/// message.msg_id = CAN_MSGOBJ_STD | 0x123;
	/// Mode is CAN_MSGOBJ_STD by default.
	uint32_t msg_id;
	/// @brief: Message ID mask. Multiple ID's can be masked
	/// to be received into a single message object by masking
	/// out the least significant bits from message ID. This mask is AND'ed
	/// with the received messages ID => use 0x7FF is default mask for 11 bit ID's.
	uint32_t mask;
	/// @brief: Message data bytes
	uint8_t data[8];
	/// @brief: Defines how many bytes of data is sent
	uint8_t data_length;
	/// @brief: Defines which hardware message object is used for this message.
	/// Message objects should not be multiplexed among many messages.
	/// For receiving multiple messages with a single message object, use mask bits.
	uint8_t msgobj;

} hal_can_msg_obj_st;

/// @brief: CANopen object dictionary constant entry
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	uint16_t index;
	uint8_t subindex;
	uint8_t len;
	uint32_t val;
} hal_canopen_obj_dict_const_entry_st;



/// @brief: CANopen object dictionary entry
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	uint16_t index;
	uint8_t subindex;
	uint8_t entrytype_len;
	uint8_t *val;
} hal_canopen_obj_dict_entry_st;

// CONFIG_TARGET_LPC11CXX C_CAN driver struct. Do not change!
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

/// @brief: C_CAN message data structure
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	uint8_t node_id;
	uint8_t msgobj_rx;
	uint8_t msgobj_tx;
	uint8_t isr_handled;
	uint32_t od_const_num;
	hal_canopen_obj_dict_const_entry_st *od_const_table;
	uint32_t od_num;
	hal_canopen_obj_dict_entry_st *od_table;

} CCAN_CANOPENCFG_T;

// CONFIG_TARGET_LPC11CXX C_CAN driver struct. Do not change!
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




void CAN_rx(uint8_t msg_obj_num) {
	hal_can_msg_obj_st msg_obj;
	/* Determine which CAN message has been received */
	msg_obj.msgobj = msg_obj_num;
	/* Now load up the msg_obj structure with the CAN message */
	LPC_CCAN_API->can_receive(&msg_obj);
	uw_can_message_st msg = {
			.data_length = msg_obj.data_length,
			.id = msg_obj.msg_id
	};
	uint8_t i;
	for (i = 0; i < msg.data_length; i++) {
		msg.data_8bit[i] = msg_obj.data[i];
	}
	// call application callback if assigned
	if (this->rx_callback[CAN1] != NULL) {
		this->rx_callback[CAN1](__uw_get_user_ptr(), &msg);
	}

}

void CAN_tx(uint8_t msg_obj_num) {
	// clear pending msg object
	this->pending_msg_objs &= ~(1 << msg_obj_num);
	this->pending_msg_obj_time_limit[msg_obj_num] = 0;
}

void CAN_error(uint32_t error_info) {
	printf("CAN error received:");
	if (error_info & CAN_ERROR_ACK) {
		printf(" ACK");
	}
	if (error_info & CAN_ERROR_BIT0) {
		printf(" BIT0");
	}
	if (error_info & CAN_ERROR_BIT1) {
		printf(" BIT1");
	}
	if (error_info & CAN_ERROR_BOFF) {
		printf(" BOFF");
		//set C_CAN up again. The CAN 2.0 specification
		// of bus off recovery sequence will start
		LPC_CAN->CNTL &= ~0x1;
	}
	if (error_info & CAN_ERROR_CRC) {
		printf(" CRC");
	}
	if (error_info & CAN_ERROR_FORM) {
		printf(" FORM");
	}
	if (error_info & CAN_ERROR_NONE) {
		printf(" NONE");
	}
	if (error_info & CAN_ERROR_PASS) {
		printf(" PASS");
	}
	if (error_info & CAN_ERROR_STUF) {
		printf(" STUF");
	}
	if (error_info & CAN_ERROR_WARN) {
		printf(" WARN");
	}
	printf("\n\r");
	if (LPC_CAN->STAT & (1 <<7)) {
		printf("Device is in CAN bus off state\n\r");
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		printf("Device is in CAN error passive state\n\r");
	}
}


void CAN_IRQHandler(void) {
	LPC_CCAN_API->isr();
}



bool uw_can_init(uw_can_channels_e channel, unsigned int baudrate, unsigned int fosc,
		uw_can_message_st *tx_buffer, unsigned int tx_buffer_size,
		uw_can_message_st *rx_buffer, unsigned int rx_buffer_size) {
	if (!check_channel(channel)) return false;
	uint8_t i;
	for (i = 0; i < CAN_COUNT; i++) {
		this->rx_callback[i] = NULL;
	}
	for (i = 0; i < MSG_OBJ_COUNT; i++) {
		this->pending_msg_obj_time_limit[i] = 0;
	}
	this->pending_msg_objs = 0;
	this->used_msg_objs = 0;
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
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
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
	return true;

}



bool uw_can_step(uw_can_channels_e channel, unsigned int step_ms) {
	if (!check_channel(channel)) return false;
	//check if the pending time limit has been exceeded.
	// If so, clear the pending message object.
	uint8_t i;
	for (i = 0; i < MSG_OBJ_COUNT; i++) {
		if (this->pending_msg_obj_time_limit[i] < PENDING_MSG_OBJ_TIME_LIMIT_MS) {
			this->pending_msg_obj_time_limit[i] += step_ms;
		}
		else {
			this->pending_msg_objs &= ~(1 << i);
		}
	}

	return true;
}



bool uw_can_config_rx_message(uw_can_channels_e channel,
		unsigned int id,
		unsigned int mask) {
	if (!check_channel(channel)) return false;

	// if any message objects are still not in use,
	// config them with settings requested
	// the last message object is reserved for sending messages
	uint8_t i;
	for (i = 0; i < MSG_OBJ_COUNT - 1; i++) {
		// search for a unused message object to be used for receiving the messages
		if (!GET_MASKED(this->used_msg_objs, (1 << i))) {
			hal_can_msg_obj_st obj = {
					.msg_id = id,
					.msgobj = i,
					.mask = mask
			};
		}
	}
	return true;
}



uw_can_errors_e uw_can_send_message(uw_can_channels_e channel, uw_can_message_st* message) {
	if (!check_channel(channel)) return false;

	hal_can_msg_obj_st obj = {
			.data_length = message->data_length,
			.msg_id = message->id,
			.msgobj = MSG_OBJ_COUNT - 1
	};
	uint8_t i;
	for (i = 0; i < obj.data_length; i++) {
		obj.data[i] = message->data_8bit[i];
	}
	// mark message object as pending
	this->pending_msg_objs |= (1 << obj.msgobj);
	this->pending_msg_obj_time_limit[obj.msgobj] = 0;
	// send the message
	LPC_CCAN_API->can_transmit(&obj);

	return true;
}


uint8_t uw_can_get_error_state(uw_can_channels_e channel) {
	if (!check_channel(channel)) return false;
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


bool uw_can_reset(uw_can_channels_e channel) {
	if (!check_channel(channel)) return false;
	//clearing the C_CAN bit resets C_CAN hardware
	LPC_SYSCON->PRESETCTRL &= ~(1 << 3);
	//set bit to de-assert the reset
	LPC_SYSCON->PRESETCTRL |= (1 << 3);
	return true;
}


#elif CONFIG_TARGET_LPC178X

bool uw_can_init(uw_can_channels_e channel, unsigned int baudrate, unsigned int fosc,
		uw_can_message_st *tx_buffer, unsigned int tx_buffer_size,
		uw_can_message_st *rx_buffer, unsigned int rx_buffer_size) {

	return true;
}



bool uw_can_step(uw_can_channels_e channel, unsigned int step_ms) {
	if (!check_channel(channel)) return false;

	return true;
}



bool uw_can_config_rx_message(uw_can_channels_e channel,
		unsigned int id,
		unsigned int mask) {
	if (!check_channel(channel)) return false;

	return true;
}



uw_can_errors_e uw_can_send_message(uw_can_channels_e channel, uw_can_message_st* message) {
	if (!check_channel(channel)) return false;

	return true;
}

#else
#error "Controller not defined"
#endif




bool uw_can_add_rx_callback(uw_can_channels_e channel,
		void (*callback_function)(void *user_ptr, uw_can_message_st *msg)) {
	if (!check_channel(channel)) return false;
	this->rx_callback[channel] = callback_function;
	return true;
}
