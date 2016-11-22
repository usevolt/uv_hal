/*
 * uv_can.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#include "uv_can.h"

#if CONFIG_CAN

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#include "uv_utilities.h"
#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_terminal.h"
#include "uv_memory.h"
#endif
#include <stdio.h>
#include <string.h>
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
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

#include "uv_uart.h"


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
	uint32_t used_msg_objs;
	// temporary message struct. This can be used instead of local variables
	// to save stack memory.
	uv_can_message_st temp_msg;
	// temporary hal message object to be used when receiving messages
	// defined here to be global instead of local
	hal_can_msg_obj_st temp_obj;
	bool init;
	uv_can_message_st rx_buffer_data[CONFIG_CAN1_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	int16_t tx_pending;
#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif
	void (*rx_callback[CAN_COUNT])(void *user_ptr);

} can_st;
#elif CONFIG_TARGET_LPC1785
typedef struct {
	bool init;
	void (*rx_callback[CAN_COUNT])(void *user_ptr);
#if CONFIG_CAN1
	uv_can_message_st rx_buffer_data1[CONFIG_CAN1_RX_BUFFER_SIZE];
#endif
#if CONFIG_CAN2
	uv_can_message_st rx_buffer_data2[CONFIG_CAN2_RX_BUFFER_SIZE];
#endif
	uv_ring_buffer_st rx_buffer[CAN_COUNT];
	uv_can_message_st temp;


#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif

} can_st;

#endif


static can_st _can = {
		.init = false
};
#define this (&_can)



#define TX_MSG_OBJ	1

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
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		8

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
	this->temp_msg.id = this->temp_obj.msg_id & ~CAN_EXT;
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



#if CONFIG_TERMINAL_CAN
	// terminal characters are sent to their specific buffer
	if (this->temp_msg.id == UV_TERMINAL_CAN_PREFIX + uv_get_crc()) {
		uint8_t i;
		for (i = 0; i < this->temp_msg.data_length; i++) {
			uv_ring_buffer_push(&this->char_buffer, (char*) &this->temp_msg.data_8bit[i]);
		}
		return;
	}
#endif

	uv_ring_buffer_push(&this->rx_buffer, &this->temp_msg);

}

void CAN_tx(uint8_t msg_obj_num) {
	this->tx_pending = 0;
#if CONFIG_CAN_LOG
	if (can_log) {
		printf("CAN message sent.\n\r");
	}
#endif

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





uv_errors_e _uv_can_init() {
	uint8_t i;
	for (i = 0; i < CAN_COUNT; i++) {
		this->rx_callback[i] = NULL;
	}
	this->init = true;
	this->tx_pending = 0;
	this->used_msg_objs = (1 << TX_MSG_OBJ);
	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			CONFIG_CAN1_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
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

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data, CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
	uv_can_config_rx_message(CAN1, UV_TERMINAL_CAN_PREFIX + uv_get_crc(), CAN_ID_MASK_DEFAULT, CAN_EXT);
#endif
	return uv_err(ERR_NONE);

}



uv_errors_e uv_can_step(uv_can_channels_e channel, unsigned int step_ms) {

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

	// if any message objects are still not in use,
	// config them with settings requested
	// the last message object is reserved for sending messages
	uint8_t i;
	for (i = 0; i < 32; i++) {
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


uv_can_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	if (check_channel(channel)) return check_channel(channel);
	else if (!this->init) {
		return uv_err(ERR_NOT_INITIALIZED | HAL_MODULE_CAN);
	}

	// if tx object is pending, try to wait for the HAl task to release the pending msg obj
	while (this->tx_pending > 0) {
		uv_rtos_task_yield();
	}

	// the last MSG OBJ is used for transmitting the messages
	hal_can_msg_obj_st obj = {
			.data_length = message->data_length,
			.msg_id = message->id | message->type,
			.msgobj = TX_MSG_OBJ,
	};
	uint8_t i;
	for (i = 0; i < obj.data_length; i++) {
		obj.data[i] = message->data_8bit[i];
	}

	// send the message
	LPC_CCAN_API->can_transmit(&obj);
	this->tx_pending = PENDING_MSG_OBJ_TIME_LIMIT_MS;

	return uv_can_get_error_state(channel);
}


uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {
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

#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif


#elif CONFIG_TARGET_LPC1785


// same handler for both CAN channels
void CAN_IRQHandler(void) {
#if CONFIG_CAN1
	printf("CAN interrupt\n\r");
	while (LPC_CAN1->GSR & 1) {
		// data ready to be received
		this->temp.id = LPC_CAN1->RID;
		this->temp.data_length = ((LPC_CAN1->RFS & (0b1111 << 16)) >> 16);
		if (this->temp.data_length > 8) this->temp.data_length = 8;
		this->temp.type = (LPC_CAN1->RFS & (1 << 31)) ? CAN_EXT : CAN_STD;
		this->temp.data_32bit[0] = LPC_CAN1->RDA;
		this->temp.data_32bit[1] = LPC_CAN1->RDB;

		printf("message id %u receveid: ", (unsigned int) this->temp.id);
		for (int i = 0; i < this->temp.data_length; i++) {
			printf( "0x%x ", this->temp.data_8bit[i]);
		}
		printf("\n\r");
#if CONFIG_TERMINAL_CAN
	// terminal characters are sent to their specific buffer
	if (this->temp_msg.id == UV_TERMINAL_CAN_PREFIX + uv_get_crc()) {
		uint8_t i;
		for (i = 0; i < this->temp_msg.data_length; i++) {
			uv_ring_buffer_push(&this->char_buffer, (char*) &this->temp_msg.data_8bit[i]);
		}
		// clear received flag
		LPC_CAN1->CMR |= (1 << 2);
		continue;
	}
#endif
		uv_ring_buffer_push(&this->rx_buffer[0], &this->temp);

		// clear received flag
		LPC_CAN1->CMR = (1 << 2);
	}

#endif
#if CONFIG_CAN2
#error "Todo: copy CAN1 message reception to here"
#endif
}


uv_errors_e _uv_can_init() {

#if CONFIG_CAN1
	uv_ring_buffer_init(&this->rx_buffer[0], this->rx_buffer_data1,
			CONFIG_CAN1_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	this->rx_callback[0] = NULL;

	// enable power to the peripheral
	LPC_SC->PCONP |= (1 << 13);

	// configure the IO pins
#if CONFIG_CAN1_TX_PIN == PIO0_1
	LPC_IOCON->P0_1 = 1;
#elif CONFIG_CAN1_TX_PIN == PIO0_22
	LPC_IOCON->P0_22 = 0b100;
#endif
#if CONFIG_CAN1_RX_PIN == PIO0_0
	LPC_IOCON->P0_0 = 1;
#elif CONFIG_CAN1_RX_PIN == PIO0_21
	LPC_IOCON->P0_21 = 0b100;
#endif
	// enter reset mode
	LPC_CAN1->MOD |= 1;
	// normal mode, transmit priority to CAN IDs, no sleep mode,
	LPC_CAN1->MOD &= ~(0b10111110);
	// enable receive interrupts
	LPC_CAN1->IER = 1;
	// todo: CAN clock settings
//	LPC_CAN1->BTR = ...
	// TSEG1 = 1, TSEG2 = 2, SJW = 1, SAM = 0
	LPC_CAN1->BTR = (1 << 20);
	// CAN baudrate = fosc / ((BRP + 1) * (TSEG1 + TSEG2)
	// ((BRP + 1) * (1 + 2)) * baudrate = fosc
	// (BRP + 1) * 3 = fosc / baudrate
	// BRP = fosc / (3 * baudrate) - 1
	LPC_CAN1->BTR |= (SystemCoreClock / (CONFIG_CAN1_BAUDRATE * 8) - 1) & 0x3FF;
	// set CAN peripheral ON
	LPC_CAN1->MOD &= ~1;

#endif
#if CONFIG_CAN2
	uv_ring_buffer_init(&this->rx_buffer[1], this->rx_buffer_data2,
			CONFIG_CAN2_RX_BUFFER_SIZE, sizeof(uv_can_message_st));

	this->rx_callback[1] = NULL;
#error "Todo: Copy CAN1 initialization here"

	LPC_SC->PCONP |= (1 << 14);

#if CONFIG_CAN2_TX_PIN == PIO0_5
	LPC_IOCON->P0_5 = 0b10;
#elif CONFIG_CAN2_TX_PIN == PIO2_8
	LPC_IOCON->P2_8 = 1;
#endif
#if CONFIG_CAN2_RX_PIN == PIO0_4
	LPC_IOCON->P0_4 = 0b10;
#elif CONFIG_CAN2_RX_PIN == PIO2_7
	LPC_IOCON->P2_7 = 1;
#endif

#endif


#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data, CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
	uv_can_config_rx_message(CAN1, UV_TERMINAL_CAN_PREFIX + uv_get_crc(), CAN_EXT);
#endif

	LPC_CANAF->SFF_sa = 0;
	LPC_CANAF->SFF_GRP_sa = 0;
	LPC_CANAF->EFF_sa = 0;
	LPC_CANAF->EFF_GRP_sa = 0;
	LPC_CANAF->ENDofTable = 0;

	this->init = true;


	return uv_err(ERR_NONE);
}



uv_errors_e uv_can_step(uv_can_channels_e channel, unsigned int step_ms) {
	if (!this->init) return uv_err(ERR_NOT_INITIALIZED | HAL_MODULE_CAN);

	return uv_err(ERR_NONE);
}

#define STD_ID(chn, x)				(((LPC_CANAF_RAM->mask[x]) & (0x7FF << (chn*16))) >> (chn*16))
#define SET_STD_ID(chn, x, val)	(LPC_CANAF_RAM->mask[x]) &= ~(0x7FF << (chn*16)); \
	(LPC_CANAF_RAM->mask[x]) |= ((val & 0x7FF) << (chn*16))

#define STD_DISABLE(chn, x) 		(((LPC_CANAF_RAM->mask[x]) & (1 << (12 + chn*16))) >> (12 + chn*16))
#define SET_STD_DISABLE(chn, x, val) (LPC_CANAF_RAM->mask[x]) &= ~(1 << (12 + chn*16)); \
	(LPC_CANAF_RAM->mask[x]) |= ((val & 1) << (12 + chn*16))

#define STD_CHANNEL(chn, x) 		(((LPC_CANAF_RAM->mask[x]) & (0b111 << (13 + chn*16))) >> (13 + chn*16))
#define SET_STD_CHANNEL(chn, x, val) (LPC_CANAF_RAM->mask[x]) &= ~(0b111 << (13 + chn*16)); \
	(LPC_CANAF_RAM->mask[x]) |= ((val & 0b111) << (13 + chn*16))


/// @brief: Acceptance filter length
#define AC_LEN		2048

uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		uv_can_msg_types_e type) {
	// set acceptance filter to idle mode
	LPC_CANAF->AFMR = 0b10;

#if CONFIG_CAN1
		if (channel == CAN1) channel = 0;
#endif
#if CONFIG_CAN2
		if (channel == CAN2) channel = 1;
#endif

	if (type == CAN_STD) {
		uint32_t i = LPC_CANAF->SFF_sa;

		// since every 32-bit word contains 2 individual messages and it would
		// be hard to parse them all, both of the messages in the same word are
		// written. 1st message is for channel 1 and 2nd message is for channel 2.

		bool inserted = false;
		// cycle trough all registered messages and find the position where to insert new one
		while (i < LPC_CANAF->SFF_GRP_sa / 4) {
			// Check if this message's ID is smaller
			if (id < STD_ID(channel, i)) {
				// copy all messages one word forward
				for (int in = LPC_CANAF->ENDofTable / 4; in >= i; in--) {
					LPC_CANAF_RAM->mask[in] = LPC_CANAF_RAM->mask[in - 1];
				}
				LPC_CANAF->SFF_GRP_sa += 4;
				LPC_CANAF->EFF_sa += 4;
				LPC_CANAF->EFF_GRP_sa += 4;
				LPC_CANAF->ENDofTable += 4;

				SET_STD_CHANNEL(0, i, 0);
				SET_STD_CHANNEL(1, i, 1);
				SET_STD_DISABLE(channel, i, 0);
				SET_STD_DISABLE(!channel, i, 1);
				SET_STD_ID(0, i, id);
				SET_STD_ID(1, i, id);

				inserted = true;
				break;
			}
			else if (id == STD_ID(channel, i)) {
				// existing entry found. Make sure that the disable bit is cleared
				SET_STD_DISABLE(channel, i, 0);
				inserted = true;
				break;
			}
			i++;
		}
		if (!inserted) {

			// No smaller ID messages found from the RAM. Add new entry
			// to the end of the STD RAM
			// copy all messages one word forward
			for (int in = LPC_CANAF->ENDofTable / 4; in >= i; in--) {
				LPC_CANAF_RAM->mask[in] = LPC_CANAF_RAM->mask[in - 1];
			}
			LPC_CANAF->SFF_GRP_sa += 4;
			LPC_CANAF->EFF_sa += 4;
			LPC_CANAF->EFF_GRP_sa += 4;
			LPC_CANAF->ENDofTable += 4;

			SET_STD_CHANNEL(0, i, 0);
			SET_STD_CHANNEL(1, i, 1);
			SET_STD_DISABLE(channel, i, 0);
			SET_STD_DISABLE(!channel, i, 1);
			SET_STD_ID(0, i, id);
			SET_STD_ID(1, i, id);
		}
	}
	else {

	}

	// enable acceptance filter
	LPC_CANAF->AFMR = 0;

	return uv_err(ERR_NONE);
}



uv_errors_e uv_can_config_rx_message_range(uv_can_channels_e channel,
		unsigned int start_id,
		unsigned int end_id,
		uv_can_msg_types_e type) {


	return uv_err(ERR_NONE);
}


uv_can_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* msg) {
#if CONFIG_CAN1
	if (channel == CAN1) {

		// wait until any one transmit buffer is available for transmitting
		while (!(LPC_CAN1->SR & ((1 << 2) | (1 << 10) | (1 << 18))));

		if (LPC_CAN1->SR & (1 << 2)) {
			LPC_CAN1->TFI1 = (msg->data_length << 16) |
					((msg->type == CAN_EXT) ? (1 << 31) : 0);
			LPC_CAN1->TID1 = msg->id;
			LPC_CAN1->TDA1 = msg->data_32bit[0];
			LPC_CAN1->TDB1 = msg->data_32bit[1];
			// send message
			LPC_CAN1->CMR = (1 | (1 << 5));
		}
		else if (LPC_CAN1->SR & (1 << 10)) {
			LPC_CAN1->TFI2 = (msg->data_length << 16) |
					((msg->type == CAN_EXT) ? (1 << 31) : 0);
			LPC_CAN1->TID2 = msg->id;
			LPC_CAN1->TDA2 = msg->data_32bit[0];
			LPC_CAN1->TDB2 = msg->data_32bit[1];
			// send message
			LPC_CAN1->CMR = (1 | (1 << 6));
		}
		else if (LPC_CAN1->SR & (1 << 18)) {
			LPC_CAN1->TFI3 = (msg->data_length << 16) |
					((msg->type == CAN_EXT) ? (1 << 31) : 0);
			LPC_CAN1->TID3 = msg->id;
			LPC_CAN1->TDA3 = msg->data_32bit[0];
			LPC_CAN1->TDB3 = msg->data_32bit[1];
			// send message
			LPC_CAN1->CMR = (1 | (1 << 7));
		}
	}
#endif
#if CONFIG_CAN2
	if (channel == CAN2) {
#error "Copy CAN1 message transmission here"
	}
#endif

	return uv_err(ERR_NONE);
}

uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {

	return uv_ring_buffer_pop(&this->rx_buffer[channel], message);
}


uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
#if CONFIG_CAN1
	if (channel == CAN1) {
		if (LPC_CAN1->GSR & (1 << 7)) {
			return CAN_ERROR_BUS_OFF;
		}
		else if (LPC_CAN1->GSR & (1 << 6)) {
			return CAN_ERROR_PASSIVE;
		}
		else return CAN_ERROR_ACTIVE;
	}
#endif
#if CONFIG_CAN2
#error "Copy CAN1 error state getter here"
#endif
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


/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {
#if CONFIG_TARGET_LPC11C14
	if (this->tx_pending > 0)
		this->tx_pending -= step_ms;
	else this->tx_pending = 0;
#endif
}




#endif
