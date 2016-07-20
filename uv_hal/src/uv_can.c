/*
 * uv_can.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#include "uv_can.h"
#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#include "uv_utilities.h"
#include <stdio.h>
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#warning "Implementation needs verification for CONFIG_TARGET_LPC1785"
#endif
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
enum {
	MSG_OBJ_COUNT = 32
};
#endif
#if CONFIG_CAN_LOG
extern bool can_log;
#endif


#if CONFIG_TARGET_LPC11C14
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
#endif


typedef struct {
#ifdef CAN_ENABLE_ASYNCHRONOUS_MODE
#ifdef CAN_TX_BUFFER_SIZE
	uv_can_message_st tx_buffer[CAN_TX_BUFFER_SIZE];
#else
	uv_can_message_st tx_buffer[CAN_ENABLE_ASYNCHRONOUS_MODE];
#endif
#ifdef CAN_RX_BUFFER_SIZE
	uv_can_message_st tx_buffer[CAN_RX_BUFFER_SIZE];
#else
	uv_can_message_st tx_buffer[CAN_ENABLE_ASYNCHRONOUS_MODE];
#endif
#else

#endif
#if CONFIG_TARGET_LPC11C14
	uint32_t pending_msg_objs;
	uint32_t used_msg_objs;
	int8_t pending_msg_obj_time_limit;
	// temporary message struct. This can be used instead of local variables
	// to save stack memory.
	uv_can_message_st temp_msg;
	// temporary hal message object to be used when receiving messages
	// defined here to be global instead of local
	hal_can_msg_obj_st temp_obj;
#elif CONFIG_TARGET_LPC1785

#endif
	uv_can_message_st rx_buffer_data[CONFIG_CAN1_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN1_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	void (*rx_callback[CAN_COUNT])(void *user_ptr);
} this_st;
static this_st _this;
#define this (&_this)



#define TX_MSG_OBJ	31

static uv_errors_e check_channel(uv_can_channels_e channel) {
	if (channel < CAN_COUNT) {
		return uv_err(ERR_NONE);
	}
	else {
		__uv_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_CAN);
	}
}



// to keep this file clear, first all function are defined for CONFIG_TARGET_LPC11C14 and
// after that to CONFIG_TARGET_LPC1785
#if CONFIG_TARGET_LPC11C14

/// @brief: Time limit for pending message objects. If this exceedes, all message objects are
/// released from pending state.
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		20

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


static uv_errors_e send_next_msg( void );

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

// CONFIG_TARGET_LPC11C14 C_CAN driver struct. Do not change!
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

// CONFIG_TARGET_LPC11C14 C_CAN driver struct. Do not change!
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
	/* Determine which CAN message has been received */
	this->temp_obj.msgobj = msg_obj_num;
	/* Now load up the msg_obj structure with the CAN message */
	LPC_CCAN_API->can_receive(&this->temp_obj);
	this->temp_msg.data_length = this->temp_obj.data_length;
	this->temp_msg.id = this->temp_obj.msg_id;
	uint8_t i;
	for (i = 0; i < this->temp_msg.data_length; i++) {
		this->temp_msg.data_8bit[i] = this->temp_obj.data[i];
	}
#if CONFIG_CAN_LOG
	if (can_log) {
		// log debug info
		printf("CAN message received\n\r   id: 0x%x\n\r   data length: %u\n\r   data: ",
				this->temp_msg.id, this->temp_msg.data_length);
		for ( i = 0; i < this->temp_msg.data_length; i++) {
			printf("%02x ", this->temp_msg.data_8bit[i]);
		}
		printf("\n\r");
	}
#endif

	uv_ring_buffer_push(&this->rx_buffer, &this->temp_msg);

}

void CAN_tx(uint8_t msg_obj_num) {
	// clear pending msg object
	this->pending_msg_objs &= ~(1 << msg_obj_num);
	this->pending_msg_obj_time_limit = 0;

#if CONFIG_CAN_LOG
	if (can_log) {
		printf("CAN message sent.\n\r");
	}
#endif

	// send the next message if there is one in the message buffer
	send_next_msg();
}

void CAN_error(uint32_t error_info) {
#if CONFIG_CAN_LOG || CONFIG_CAN_ERROR_LOG
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
#endif
}

void CAN_IRQHandler(void) {
	LPC_CCAN_API->isr();
}



uv_errors_e uv_can_init(uv_can_channels_e channel) {
	if (check_channel(channel)) return check_channel(channel);
	uint8_t i;
	for (i = 0; i < CAN_COUNT; i++) {
		this->rx_callback[i] = NULL;
	}
	this->pending_msg_obj_time_limit = 0;
	this->pending_msg_objs = 0;
	this->used_msg_objs = 0;
	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			CONFIG_CAN1_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->tx_buffer, this->tx_buffer_data,
			CONFIG_CAN1_TX_BUFFER_SIZE, sizeof(uv_can_message_st));
	SystemCoreClockUpdate();
	uint32_t CanApiClkInitTable[] = {
		//CANCLKDIV register
		0,
		//BTR register: TSEG2 = 2, TSEG1 = 1, SJW = 0 (equals to 0x2300)
		0x2300 + ((SystemCoreClock / (8 * CONFIG_CAN1_BAUDRATE) - 1) & 0x3F)
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
	return uv_err(ERR_NONE);

}



uv_errors_e uv_can_step(uv_can_channels_e channel, unsigned int step_ms) {
	if (check_channel(channel)) return check_channel(channel);
	//check if the pending time limit has been exceeded.
	// If so, clear the pending message object.

	if (this->pending_msg_objs & (1 << TX_MSG_OBJ)) {
		if (this->pending_msg_obj_time_limit < PENDING_MSG_OBJ_TIME_LIMIT_MS) {
			this->pending_msg_obj_time_limit += step_ms;
		}
		else {
			this->pending_msg_objs &= ~(1 << TX_MSG_OBJ);
			// send the next message
			send_next_msg();
		}
	}

	if (this->rx_callback[CAN1] != NULL) {
		while (true) {
			// call application callback if assigned
			if (uv_ring_buffer_empty(&this->rx_buffer)) {
				break;
			}
			else {
				this->rx_callback[CAN1](__uv_get_user_ptr());
			}
		}
	}


	return uv_err(ERR_NONE);
}



uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {
	if (check_channel(channel)) return check_channel(channel);

	// if any message objects are still not in use,
	// config them with settings requested
	// the last message object is reserved for sending messages
	uint8_t i;
	for (i = 0; i < TX_MSG_OBJ; i++) {
		// search for a unused message object to be used for receiving the messages
		if (!GET_MASKED(this->used_msg_objs, (1 << i))) {
			hal_can_msg_obj_st obj = {
					.msg_id = id | type,
					.msgobj = i,
					.mask = mask,
			};
			LPC_CCAN_API->config_rxmsgobj(&obj);
			this->used_msg_objs |= (1 << i);
			return uv_err(ERR_NONE);
		}
	}
	__uv_err_throw(ERR_CAN_RX_MESSAGE_COUNT_FULL | HAL_MODULE_CAN);
}

static uv_errors_e send_next_msg( void ) {
	uv_can_message_st msg;
	uv_ring_buffer_pop(&this->tx_buffer, &msg);
	if (uv_get_error == ERR_BUFFER_EMPTY) {
		return uv_err(ERR_NONE);
	}

	// the last MSG OBJ is used for transmitting the messages
	hal_can_msg_obj_st obj = {
			.data_length = msg.data_length,
			.msg_id = msg.id,
			.msgobj = TX_MSG_OBJ
	};
	uint8_t i;
	for (i = 0; i < obj.data_length; i++) {
		obj.data[i] = msg.data_8bit[i];
	}
	// mark message object as pending
	this->pending_msg_objs |= (1 << obj.msgobj);
	this->pending_msg_obj_time_limit = 0;
#if CONFIG_CAN_LOG
	if (can_log) {
		printf("Sending CAN message\n\r   id: 0x%x\n\r   data:", msg.id);
		for (i = 0; i < msg.data_length; i++) {
			printf("%02x ", msg.data_8bit[i]);
		}
		printf("\n\r");
	}
#endif

	// send the message
	LPC_CCAN_API->can_transmit(&obj);

	return uv_err(ERR_NONE);

}

uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	if (check_channel(channel)) return check_channel(channel);

	// put the message into TX buffer
	uv_errors_e e = uv_ring_buffer_push(&this->tx_buffer, message);

	// if the TX msg obj is ready, send the next message
	if (!(this->pending_msg_objs & (1 << TX_MSG_OBJ))) {
		return send_next_msg();
	}

	__uv_err_throw(e);
}


uv_errors_e uv_can_fetch_message(uv_can_channels_e channel, uv_can_message_st *message) {
	if (check_channel(channel)) return check_channel(channel);

	return uv_ring_buffer_pop(&this->rx_buffer, message);
}


uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	if (check_channel(channel)) return check_channel(channel);
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


uv_errors_e uv_can_reset(uv_can_channels_e channel) {
	if (check_channel(channel)) return check_channel(channel);
	//clearing the C_CAN bit resets C_CAN hardware
	LPC_SYSCON->PRESETCTRL &= ~(1 << 3);
	//set bit to de-assert the reset
	LPC_SYSCON->PRESETCTRL |= (1 << 3);
	return uv_err(ERR_NONE);
}


#elif CONFIG_TARGET_LPC1785

uv_errors_e uv_can_init(uv_can_channels_e channel) {
	if (check_channel(channel)) return check_channel(channel);

	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			CONFIG_CAN1_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->tx_buffer, this->tx_buffer_data,
			CONFIG_CAN1_TX_BUFFER_SIZE, sizeof(uv_can_message_st));

	return uv_err(ERR_NONE);
}



uv_errors_e uv_can_step(uv_can_channels_e channel, unsigned int step_ms) {
	if (check_channel(channel)) return check_channel(channel);

	return uv_err(ERR_NONE);
}



uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {
	if (check_channel(channel)) return check_channel(channel);

	return uv_err(ERR_NONE);
}


uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	if (check_channel(channel)) return check_channel(channel);

	return uv_err(ERR_NONE);
}

uv_errors_e uv_can_fetch_message(uv_can_channels_e channel, uv_can_message_st *message) {

	return uv_ring_buffer_pop(&this->rx_buffer, message);
}


uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {

	return uv_err(ERR_NONE);
}



#else
#error "Controller not defined"
#endif




uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		void (*callback_function)(void *user_ptr)) {

	if (check_channel(channel)) return false;
	this->rx_callback[channel] = callback_function;
	return uv_err(ERR_NONE);
}
