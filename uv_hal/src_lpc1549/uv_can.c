/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "uv_can.h"

#if CONFIG_CAN

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#include "uv_gpio.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_terminal.h"
#include "uv_memory.h"
#include "uv_uart.h"
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
#elif CONFIG_TARGET_LPC1549
#include "chip.h"
#include "romapi_15xx.h"
#include "rom_can_15xx.h"

/*------------- CAN Controller (CAN) ----------------------------*/
/** @addtogroup LPC15xx_CAN LPC15xx Controller Area Network(CAN)
  @{
*/
typedef struct
{
  __IO uint32_t CNTL;				/* 0x000 */
  __IO uint32_t STAT;
  __I  uint32_t EC;
  __IO uint32_t BT;
  __I  uint32_t INT;
  __IO uint32_t TEST;
  __IO uint32_t BRPE;
       uint32_t RESERVED0;
  __IO uint32_t IF1_CMDREQ;			/* 0x020 */
  __IO uint32_t IF1_CMDMSK;
  __IO uint32_t IF1_MSK1;
  __IO uint32_t IF1_MSK2;
  __IO uint32_t IF1_ARB1;
  __IO uint32_t IF1_ARB2;
  __IO uint32_t IF1_MCTRL;
  __IO uint32_t IF1_DA1;
  __IO uint32_t IF1_DA2;
  __IO uint32_t IF1_DB1;
  __IO uint32_t IF1_DB2;
       uint32_t RESERVED1[13];
  __IO uint32_t IF2_CMDREQ;			/* 0x080 */
  __IO uint32_t IF2_CMDMSK;
  __IO uint32_t IF2_MSK1;
  __IO uint32_t IF2_MSK2;
  __IO uint32_t IF2_ARB1;
  __IO uint32_t IF2_ARB2;
  __IO uint32_t IF2_MCTRL;
  __IO uint32_t IF2_DA1;
  __IO uint32_t IF2_DA2;
  __IO uint32_t IF2_DB1;
  __IO uint32_t IF2_DB2;
       uint32_t RESERVED2[21];
  __I  uint32_t TXREQ1;				/* 0x100 */
  __I  uint32_t TXREQ2;
       uint32_t RESERVED3[6];
  __I  uint32_t ND1;				/* 0x120 */
  __I  uint32_t ND2;
       uint32_t RESERVED4[6];
  __I  uint32_t IR1;				/* 0x140 */
  __I  uint32_t IR2;
       uint32_t RESERVED5[6];
  __I  uint32_t MSGV1;				/* 0x160 */
  __I  uint32_t MSGV2;
       uint32_t RESERVED6[6];
  __IO uint32_t CLKDIV;				/* 0x180 */
} LPC_CAN_TypeDef;
/*@}*/ /* end of group LPC15xx_CAN */


#define LPC_CAN               ((LPC_CAN_TypeDef *)LPC_C_CAN0_BASE)
#endif


#if CONFIG_CAN_LOG
extern bool can_log;
#endif

#include "uv_uart.h"

// all controllers which implement the C_CAN hardware
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549
#define C_CAN		1
#endif


#if C_CAN
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

#if CONFIG_TARGET_LPC11C14

typedef struct {
	uint32_t used_msg_objs;
	// temporary message struct. This can be used instead of local variables
	// to save stack memory.
	uv_can_message_st temp_msg;
	// temporary hal message object to be used when receiving messages
	// defined here to be global instead of local
	hal_can_msg_obj_st temp_obj;
	bool init;
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	int16_t tx_pending;
#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif

} can_st;


#elif CONFIG_TARGET_LPC1785
typedef struct {
	bool init;
	void (*rx_callback[CAN_COUNT])(void *user_ptr);
#if CONFIG_CAN0
	uv_can_message_st rx_buffer_data1[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_can_message_st tx_buffer_data1[CONFIG_CAN0_TX_BUFFER_SIZE];
#endif
#if CONFIG_CAN1
	uv_can_message_st rx_buffer_data2[CONFIG_CAN1_RX_BUFFER_SIZE];
	uv_can_message_st tx_buffer_data2[CONFIG_CAN1_TX_BUFFER_SIZE];
#endif
	uv_ring_buffer_st rx_buffer[CAN_COUNT];
	uv_ring_buffer_st tx_buffer[CAN_COUNT];
	uv_can_message_st temp;


#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif

} can_st;

#elif CONFIG_TARGET_LPC1549

typedef struct {
	uint32_t used_msg_objs;
	// temporary message struct. This can be used instead of local variables
	// to save stack memory.
	uv_can_message_st temp_msg;
	// temporary hal message object to be used when receiving messages
	// defined here to be global instead of local
	hal_can_msg_obj_st temp_obj;
	bool init;
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	int16_t tx_pending;
#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif
	void (*rx_callback[CAN_COUNT])(void *user_ptr);

} can_st;

#elif CONFIG_TARGET_LINUX
typedef struct {
	bool init;
} can_st;
#endif


static can_st _can = {
		.init = false
};
#define this (&_can)


void _uv_can_hal_send(uv_can_channels_e chn);




#if CONFIG_TARGET_LPC11C14

#define TX_MSG_OBJ	1


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
		printf("CAN message received\n   id: 0x%x\n   data length: %u\n   data: ",
				this->temp_msg.id, this->temp_msg.data_length);
		for ( i = 0; i < this->temp_msg.data_length; i++) {
			printf("%02x ", this->temp_msg.data_8bit[i]);
		}
		printf("\n");
	}
#endif



#if CONFIG_TERMINAL_CAN
	// terminal characters are sent to their specific buffer
	if (this->temp_msg.id == UV_TERMINAL_CAN_RX_ID + uv_get_id() &&
			this->temp_msg.type == CAN_STD &&
			this->temp_msg.data_8bit[0] == 0x22 &&
			this->temp_msg.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
			this->temp_msg.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
			this->temp_msg.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
			this->temp_msg.data_length > 4) {
		uint8_t i;
		for (i = 0; i < this->temp_msg.data_length - 4; i++) {
			uv_ring_buffer_push(&this->char_buffer, (char*) &this->temp_msg.data_8bit[4 + i]);
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
		printf("CAN message sent.\n");
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
	printf("\n");
	if (LPC_CAN->STAT & (1 <<7)) {
		printf("Device is in CAN bus off state\n");
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		printf("Device is in CAN error passive state\n");
	}
#endif
}

void CAN_IRQHandler(void) {
	LPC_CCAN_API->isr();
}


uv_errors_e _uv_can_init() {
	this->init = true;
	this->tx_pending = 0;
	this->used_msg_objs = (1 << TX_MSG_OBJ);
	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			CONFIG_CAN0_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->tx_buffer, this->tx_buffer_data,
			CONFIG_CAN0_TX_BUFFER_SIZE, sizeof(uv_can_message_st));
	SystemCoreClockUpdate();
	uint32_t CanApiClkInitTable[] = {
		//CANCLKDIV register
		0,
		//BTR register: TSEG2 = 2, TSEG1 = 1, SJW = 0 (equals to 0x2300)
		0x2300 + ((SystemCoreClock / (8 * CONFIG_CAN0_BAUDRATE) - 1) & 0x3F)
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
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CAN0, UV_TERMINAL_CAN_RX_ID + uv_get_id(), CAN_STD);
#endif
#endif
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
	for (i = 0; i < 31; i++) {
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



void _uv_can_hal_send(uv_can_channels_e chn) {
	if (chn);

	// if tx object is pending, try to wait for the HAl task to release the pending msg obj
	if (this->tx_pending > 0) {
		return;
	}

	uv_can_message_st msg;
	uv_errors_e e = uv_ring_buffer_pop(&this->tx_buffer, &msg);
	// empty buffer
	if (e) {
		return;
	}

	// the last MSG OBJ is used for transmitting the messages
	hal_can_msg_obj_st obj = {
			.data_length = msg.data_length,
			.msg_id = msg.id | msg.type,
			.msgobj = TX_MSG_OBJ,
	};
	uint8_t i;
	for (i = 0; i < obj.data_length; i++) {
		obj.data[i] = msg.data_8bit[i];
	}

	// send the message
	LPC_CCAN_API->can_transmit(&obj);
	this->tx_pending = PENDING_MSG_OBJ_TIME_LIMIT_MS;
}




uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	if (channel) {};
	uv_can_errors_e e = CAN_ERROR_ACTIVE;

	if (LPC_CAN->STAT & (1 << 7)) {
		e = CAN_ERROR_BUS_OFF;
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		e = CAN_ERROR_PASSIVE;
	}
	else if (LPC_CAN->STAT & (1 << 6)) {
		e = CAN_ERROR_WARNING;
	}
	else {

	}
	return e;
}



uv_errors_e uv_can_reset(uv_can_channels_e channel) {
#if CONFIG_TARGET_LPC11C14
	//clearing the C_CAN bit resets C_CAN hardware
	LPC_SYSCON->PRESETCTRL &= ~(1 << 3);
	//set bit to de-assert the reset
	LPC_SYSCON->PRESETCTRL |= (1 << 3);
#elif CONFIG_TARGET_LPC1549
	LPC_SYSCON->PRESETCTRL[1] |= (1 << 7);
	__NOP();
	LPC_SYSCON->PRESETCTRL[1] &= ~(1 << 7);

#endif
	return uv_err(ERR_NONE);
}








#elif CONFIG_TARGET_LPC1785










// same handler for both CAN channels
void CAN_IRQHandler(void) {
#if CONFIG_CAN0
	// CAN tranceiver is either in bus off or error active state
	if (LPC_CAN1->ICR & (0b11 << 2)) {
		LPC_CAN1->MOD &= ~1;
		return;
	}

	if (LPC_CAN1->GSR & (1 << 2)) {
		_uv_can_hal_send(CAN0);
	}
	while (LPC_CAN1->GSR & 1) {
		// data ready to be received
		this->temp.id = LPC_CAN1->RID;
		this->temp.data_length = ((LPC_CAN1->RFS & (0b1111 << 16)) >> 16);
		if (this->temp.data_length > 8) this->temp.data_length = 8;
		this->temp.type = (LPC_CAN1->RFS & (1 << 31)) ? CAN_EXT : CAN_STD;
		this->temp.data_32bit[0] = LPC_CAN1->RDA;
		this->temp.data_32bit[1] = LPC_CAN1->RDB;

#if CONFIG_TERMINAL_CAN
	// terminal characters are sent to their specific buffer
	if (this->temp.id == UV_TERMINAL_CAN_RX_ID + uv_get_id() &&
			this->temp.type == CAN_STD &&
			this->temp.data_8bit[0] == 0x22 &&
			this->temp.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
			this->temp.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
			this->temp.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
			this->temp.data_length > 4) {
		uint8_t i;
		for (i = 0; i < this->temp.data_length - 4; i++) {
			uv_ring_buffer_push(&this->char_buffer, (char*) &this->temp.data_8bit[4 + i]);
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
#if CONFIG_CAN1
#error "Todo: copy CAN0 message reception to here"
#endif
}


uv_errors_e _uv_can_init() {
	uv_errors_e ret = ERR_NONE;

#if CONFIG_CAN0
	uv_ring_buffer_init(&this->rx_buffer[0], this->rx_buffer_data1,
			CONFIG_CAN0_RX_BUFFER_SIZE, sizeof(uv_can_message_st));

	uv_ring_buffer_init(&this->tx_buffer[0], this->tx_buffer_data1,
			CONFIG_CAN0_TX_BUFFER_SIZE, sizeof(uv_can_message_st));

	this->rx_callback[0] = NULL;

	// enable power to the peripheral
	LPC_SC->PCONP |= (1 << 13);

	// configure the IO pins
#if CONFIG_CAN0_TX_PIN == PIO0_1
	LPC_IOCON->P0_1 = 1;
#elif CONFIG_CAN0_TX_PIN == PIO0_22
	LPC_IOCON->P0_22 = 0b100;
#endif
#if CONFIG_CAN0_RX_PIN == PIO0_0
	LPC_IOCON->P0_0 = 1;
#elif CONFIG_CAN0_RX_PIN == PIO0_21
	LPC_IOCON->P0_21 = 0b100;
#endif
	// enter reset mode
	LPC_CAN1->MOD |= 1;
	// normal mode, transmit priority to CAN IDs, no sleep mode
	LPC_CAN1->MOD &= ~(0b10111110);
	// enable receive interrupts and transmit interrupts
	LPC_CAN1->IER = 1 | (1 << 2) | (1 << 1) | (1 << 9) | (1 << 10);
	NVIC_EnableIRQ(CAN_IRQn);
	// TSEG1, TSEG2
	LPC_CAN1->BTR = (12 << 16) | (1 << 20);
	// for some reason incrementing TSEG by 1 needs CANX_BAUDRATE to be divided by 2
	LPC_CAN1->BTR |= (SystemCoreClock / (CONFIG_CAN0_BAUDRATE * 32) - 1) & 0x3FF;

//	LPC_CAN->BT = (SystemCoreClock / (CONFIG_CAN0_BAUDRATE * 8) & 0x3F)
//				  | (1 << 8) | (2 << 12);

	// set CAN peripheral ON
	LPC_CAN1->MOD &= ~1;

#endif
#if CONFIG_CAN1
	uv_ring_buffer_init(&this->rx_buffer[1], this->rx_buffer_data2,
			CONFIG_CAN1_RX_BUFFER_SIZE, sizeof(uv_can_message_st));

	this->rx_callback[1] = NULL;
#error "Todo: Copy CAN0 initialization here"

	LPC_SC->PCONP |= (1 << 14);

#if CONFIG_CAN1_TX_PIN == PIO0_5
	LPC_IOCON->P0_5 = 0b10;
#elif CONFIG_CAN1_TX_PIN == PIO2_8
	LPC_IOCON->P2_8 = 1;
#endif
#if CONFIG_CAN1_RX_PIN == PIO0_4
	LPC_IOCON->P0_4 = 0b10;
#elif CONFIG_CAN1_RX_PIN == PIO2_7
	LPC_IOCON->P2_7 = 1;
#endif

#endif


	// set acceptance filter to idle mode
	LPC_CANAF->AFMR = 0b10;

	LPC_CANAF->SFF_sa = 0;
	LPC_CANAF->SFF_GRP_sa = 0;
	LPC_CANAF->EFF_sa = 0;
	LPC_CANAF->EFF_GRP_sa = 0;
	LPC_CANAF->ENDofTable = 0;

	// enable acceptance filter
	LPC_CANAF->AFMR = 0;

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data, CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CAN0, UV_TERMINAL_CAN_RX_ID + uv_get_id(), CAN_STD);
#endif

#endif

	this->init = true;


	return ret;
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

#define STD_RANGE_ID_LOW(x)				(((LPC_CANAF_RAM->mask[x]) & (0x7FF << 16)) >> 16)
#define SET_STD_RANGE_ID_LOW(id, x)		LPC_CANAF_RAM->mask[x] &= ~(0x7FF << 16); \
										LPC_CANAF_RAM->mask[x] |= (id << 16)

#define STD_RANGE_ID_HIGH(x)			(((LPC_CANAF_RAM->mask[x]) & (0x7FF)))
#define SET_STD_RANGE_ID_HIGH(id, x)	LPC_CANAF_RAM->mask[x] &= ~(0x7FF); \
										LPC_CANAF_RAM->mask[x] |= id

#define STD_RANGE_CHANNEL(x)			(((LPC_CANAF_RAM->mask[x]) & (0b111 << 13)) >> 13)
#define SET_STD_RANGE_CHANNEL(chn, x)	LPC_CANAF_RAM->mask[x] &= ~((0b111 << 13) | (0b111 << 29));\
										LPC_CANAF_RAM->mask[x] |= ((chn << 13) | (chn << 29))

#define SET_STD_RANGE_DISABLE(value, x)	LPC_CANAF_RAM->mask[x] &= ~((1 << 12) | (1 << 28)); \
										LPC_CANAF_RAM->mask[x] |= ((value << 12) | (value << 28))

#define EXT_ID(x)				(LPC_CANAF_RAM->mask[x] & 0x1FFFFFFF)
#define SET_EXT_ID(x, val)	(LPC_CANAF_RAM->mask[x] &= ~0x1FFFFFFF); \
	(LPC_CANAF_RAM->mask[x] |= (val))

#define EXT_CHANNEL(x) 		((LPC_CANAF_RAM->mask[x] & 0xE0000000) >> 29)
#define SET_EXT_CHANNEL(x, chn) LPC_CANAF_RAM->mask[x] &= ~0xE0000000; \
	LPC_CANAF_RAM->mask[x] |= ((chn) << 29)


/// @brief: Acceptance filter length
#define AC_LEN		2048

uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		uv_can_msg_types_e type) {
	uv_errors_e ret = ERR_NONE;

	// set acceptance filter to idle mode
	LPC_CANAF->AFMR = 0b10;

#if CONFIG_CAN0
		if (channel == CAN0) channel = 0;
#endif
#if CONFIG_CAN1
		if (channel == CAN1) channel = 1;
#endif

	if (type == CAN_STD) {
		uint32_t i = LPC_CANAF->SFF_sa / 4;

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

				LPC_CANAF_RAM->mask[i] = 0;
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
			for (int in = LPC_CANAF->ENDofTable / 4; in >= (int) i; in--) {
				if (in) {
					LPC_CANAF_RAM->mask[in] = LPC_CANAF_RAM->mask[in - 1];
				}
			}
			LPC_CANAF->SFF_GRP_sa += 4;
			LPC_CANAF->EFF_sa += 4;
			LPC_CANAF->EFF_GRP_sa += 4;
			LPC_CANAF->ENDofTable += 4;

			LPC_CANAF_RAM->mask[i] = 0;
			SET_STD_CHANNEL(0, i, 0);
			SET_STD_CHANNEL(1, i, 1);
			SET_STD_DISABLE(channel, i, 0);
			SET_STD_DISABLE(!channel, i, 1);
			SET_STD_ID(0, i, id);
			SET_STD_ID(1, i, id);
		}
	}
	else {
		uint32_t i = LPC_CANAF->EFF_sa / 4;

		// every entry is an individual message

		bool inserted = false;
		// cycle trough all registered messages and find the position where to insert new one
		while (i < LPC_CANAF->EFF_GRP_sa / 4) {
			// continue is this object is not for this channel
			if (EXT_CHANNEL(i) != channel) {
				i++;
				continue;
			}
			// Check if this message's ID is smaller
			if (id < EXT_ID(i)) {
				// copy all messages one word forward
				for (int in = LPC_CANAF->ENDofTable / 4; in >= i; in--) {
					if (in) {
						LPC_CANAF_RAM->mask[in] = LPC_CANAF_RAM->mask[in - 1];
					}
				}
				LPC_CANAF->EFF_GRP_sa += 4;
				LPC_CANAF->ENDofTable += 4;

				LPC_CANAF_RAM->mask[i] = 0;
				SET_EXT_CHANNEL(i, channel);
				SET_EXT_ID(i, id);

				inserted = true;
				break;
			}
			else if (id == EXT_ID(i)) {
				inserted = true;
				break;
			}
			i++;
		}
		if (!inserted) {

			// No smaller ID messages found from the RAM. Add new entry
			// to the end of the EXT RAM
			// copy all messages one word forward
			for (int in = LPC_CANAF->ENDofTable / 4; in >= (int) i; in--) {
				LPC_CANAF_RAM->mask[in] = LPC_CANAF_RAM->mask[in - 1];
			}
			LPC_CANAF->EFF_GRP_sa += 4;
			LPC_CANAF->ENDofTable += 4;

			LPC_CANAF_RAM->mask[i] = 0;
			SET_EXT_CHANNEL(i, channel);
			SET_EXT_ID(i, id);
		}
	}

	// enable acceptance filter
	LPC_CANAF->AFMR = 0;

	return ret;
}




void _uv_can_hal_send(uv_can_channels_e channel) {
#if CONFIG_CAN0
	if (channel == CAN0) {
		uv_can_message_st msg;
		uv_errors_e e = uv_ring_buffer_peek(&this->tx_buffer[0], &msg);
		if (e) {
			return;
		}

		// if any transmit buffer has same ID message to be transmitted, wait until
		// the older message is transmitted.
		while ((LPC_CAN1->TID1 == msg.id && !(LPC_CAN1->SR & (1 << 2))) ||
				(LPC_CAN1->TID2 == msg.id && !(LPC_CAN1->SR & (1 << 10))) ||
				(LPC_CAN1->TID3 == msg.id && !(LPC_CAN1->SR & (1 << 18)))) {
			// CAN tranceiver is in bus off
			if (LPC_CAN1->GSR & (0b1 << 7)) {
				LPC_CAN1->MOD &= ~1;
			}
			return;
		}

		// wait until any one transmit buffer is available for transmitting
		while (!(LPC_CAN1->SR & ((1 << 2) | (1 << 10) | (1 << 18)))) {
			// CAN tranceiver is either in bus off or error passive state
			// and last received error was in ACK slot.
			// This means that the device may be the only one in the CAN bus,
			// and in that case its OK to discard the message transmission
			if (LPC_CAN1->GSR & (0b1 << 7)) {
				LPC_CAN1->MOD &= ~1;
				return;
			}
		}

		if (LPC_CAN1->SR & (1 << 2)) {
			LPC_CAN1->TFI1 = (msg.data_length << 16) |
					((msg.type == CAN_EXT) ? (1 << 31) : 0);
			LPC_CAN1->TID1 = msg.id;
			LPC_CAN1->TDA1 = msg.data_32bit[0];
			LPC_CAN1->TDB1 = msg.data_32bit[1];
			// send message
			LPC_CAN1->CMR = (1 | (1 << 5));
		}
		else if (LPC_CAN1->SR & (1 << 10)) {
			LPC_CAN1->TFI2 = (msg.data_length << 16) |
					((msg.type == CAN_EXT) ? (1 << 31) : 0);
			LPC_CAN1->TID2 = msg.id;
			LPC_CAN1->TDA2 = msg.data_32bit[0];
			LPC_CAN1->TDB2 = msg.data_32bit[1];
			// send message
			LPC_CAN1->CMR = (1 | (1 << 6));
		}
		else if (LPC_CAN1->SR & (1 << 18)) {
			LPC_CAN1->TFI3 = (msg.data_length << 16) |
					((msg.type == CAN_EXT) ? (1 << 31) : 0);
			LPC_CAN1->TID3 = msg.id;
			LPC_CAN1->TDA3 = msg.data_32bit[0];
			LPC_CAN1->TDB3 = msg.data_32bit[1];
			// send message
			LPC_CAN1->CMR = (1 | (1 << 7));
		}
		// message succesfully sent, remove it from the buffer
		uv_ring_buffer_pop(&this->tx_buffer[0], NULL);

		return;
	}
#endif
#if CONFIG_CAN1
	if (channel == CAN1) {
#error "Copy CAN0 message transmission here"
	}
#endif

	return;
}



uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	uv_can_errors_e ret = CAN_ERROR_ACTIVE;
#if CONFIG_CAN0
	if (channel == CAN0) {
		if (LPC_CAN1->GSR & (1 << 7)) {
			ret = CAN_ERROR_BUS_OFF;
		}
		else if (LPC_CAN1->GSR & (1 << 6)) {
			ret = CAN_ERROR_PASSIVE;
		}
		else {
			ret = CAN_ERROR_ACTIVE;
		}
	}
#endif
#if CONFIG_CAN1
#error "Copy CAN0 error state getter here"
#endif
	return ret;
}







uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		void (*callback_function)(void *user_ptr)) {
	uv_errors_e ret = ERR_NONE;

	this->rx_callback[channel] = callback_function;
	return ret;
}





#elif CONFIG_TARGET_LPC1549

#define TX_MSG_OBJ	1


/// @brief: Time limit for pending message objects. If this exceeds, all message objects are
/// released from pending state.
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		8


static uint32_t get_msgif_id(const bool ext) {
	uint32_t id;
	if (ext) {
		id = LPC_CAN->IF1_ARB1;
		id += (LPC_CAN->IF1_ARB2 & 0x1FFF) << 16;
	}
	else {
		id = (LPC_CAN->IF1_ARB2 & 0x1FFF) >> 2;
	}
	return id;
}

static void set_msgif_id(const uint32_t id, const bool ext) {
	if (ext) {
		LPC_CAN->IF1_ARB1 = id & 0xFFFF;
		LPC_CAN->IF1_ARB2 &= ~(0x1FFF);
		LPC_CAN->IF1_ARB2 |= ((id >> 16) & 0x1FFF);
	}
	else {
		LPC_CAN->IF1_ARB2 &= ~(0x1FFF);
		LPC_CAN->IF1_ARB2 |= ((id & 0x7FF) << 2);
	}
}

static uint32_t get_msgif_mask(const uv_can_msg_types_e type) {
	uint32_t mask;
	if (type == CAN_EXT) {
		mask = LPC_CAN->IF1_MSK1 + ((LPC_CAN->IF1_MSK2 & 0x1FFF) << 16);
	}
	else {
		mask = (LPC_CAN->IF1_MSK2 & 0x1FFF) >> 2;
	}
	return mask;
}

static void set_msgif_mask(const uint32_t mask, const uv_can_msg_types_e type) {
	if (type == CAN_EXT) {
		LPC_CAN->IF1_MSK1 = mask & 0xFFFF;
		LPC_CAN->IF1_MSK2 &= ~(0x1FFF);
		LPC_CAN->IF1_MSK2 |= ((mask >> 16) & 0x1FFF);
	}
	else {
		LPC_CAN->IF1_MSK1 = 0xFFFF;
		LPC_CAN->IF1_MSK2 &= ~(0x1FFF);
		LPC_CAN->IF1_MSK2 |= ((mask & 0x7FF) << 2);
	}
}

/// @return: 1 if extended, 0 if standard
static inline uint8_t get_msgif_type(void) {
	return ((LPC_CAN->IF1_ARB2 & (1 << 14)) >> 14);
}

static void read_msg_obj(uint8_t msg_obj) {
	// load the whole message
	LPC_CAN->IF1_CMDMSK = 0x7F;
	while (LPC_CAN->IF1_CMDREQ & (1 << 15));
	LPC_CAN->IF1_CMDREQ = msg_obj;
	while (LPC_CAN->IF1_CMDREQ & (1 << 15));
}

static void write_msg_obj(uint8_t msg_obj, bool txrqst) {
	LPC_CAN->IF1_CMDMSK = 0xFB | ((txrqst ? 1 : 0) << 2);
	while(LPC_CAN->IF1_CMDREQ & (1 << 15));
	LPC_CAN->IF1_CMDREQ = msg_obj;
	while(LPC_CAN->IF1_CMDREQ & (1 << 15));
}

static void msg_obj_enable(uint8_t msg_obj) {
	read_msg_obj(msg_obj);
	LPC_CAN->IF1_ARB2 |= (1 << 15);
	write_msg_obj(msg_obj, false);
}

static void msg_obj_disable(uint8_t msg_obj) {
	read_msg_obj(msg_obj);
	LPC_CAN->IF1_ARB2 &= ~(1 << 15);
	write_msg_obj(msg_obj, false);
}



void CAN_IRQHandler(void) {
	volatile uint32_t p = LPC_CAN->INT;

	if (p <= 0x20) {
		uint32_t int_pend = LPC_CAN->IR1 | (LPC_CAN->IR2 << 16);
		while (int_pend) {
			// find out the pending message object
			uint8_t msg_obj = 0;
			for (msg_obj = 0; msg_obj < 32; msg_obj++) {
				if (int_pend & (1 << msg_obj)) {

					__disable_irq();

					// read the pending object
					read_msg_obj(msg_obj + 1);

					if ((msg_obj + 1) != TX_MSG_OBJ) {
						// copy the message data and push it into rx ring buffer
						uv_can_msg_st msg;
						msg.type = (LPC_CAN->IF1_ARB2 & (1 << 14)) ? CAN_EXT : CAN_STD;
						msg.data_length = LPC_CAN->IF1_MCTRL & 0b1111;
						msg.id = get_msgif_id(msg.type == CAN_EXT);
						msg.data_16bit[0] = LPC_CAN->IF1_DA1;
						msg.data_16bit[1] = LPC_CAN->IF1_DA2;
						msg.data_16bit[2] = LPC_CAN->IF1_DB1;
						msg.data_16bit[3] = LPC_CAN->IF1_DB2;

#if CONFIG_TERMINAL_CAN
						// terminal characters are sent to their specific buffer
						if (msg.id == UV_TERMINAL_CAN_RX_ID + uv_get_id() &&
								msg.type == CAN_STD &&
								msg.data_8bit[0] == 0x22 &&
								msg.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
								msg.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
								msg.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
								msg.data_length > 4) {
							uint8_t i;
							for (i = 0; i < msg.data_length - 4; i++) {
								uv_ring_buffer_push(&this->char_buffer, (char*) &msg.data_8bit[4 + i]);
							}
						}
						else {
#endif
							uv_ring_buffer_push(&this->rx_buffer, &msg);
#if CONFIG_TERMINAL_CAN
						}
#endif
					}
					int_pend &= ~(1 << msg_obj);

					__enable_irq();

					if ((msg_obj + 1) == TX_MSG_OBJ) {
						_uv_can_hal_send(CAN0);
					}
				}
			}

		}
	}
	else {
		// detect bus off and fix it
		if (LPC_CAN->STAT & (1 << 7)) {
			LPC_CAN->CNTL &= ~(1);
		}
	}

	// clear status interrupts by reading the status register
	// clear succesfull rx & tx
	LPC_CAN->STAT &= ~(0b11 << 3);
}


uv_errors_e _uv_can_init() {
	uint8_t i;
	for (i = 0; i < CAN_COUNT; i++) {
		this->rx_callback[i] = NULL;
	}
	this->init = true;
	this->used_msg_objs = (1 << (TX_MSG_OBJ - 1));

	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			CONFIG_CAN0_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->tx_buffer, this->tx_buffer_data,
			CONFIG_CAN0_TX_BUFFER_SIZE, sizeof(uv_can_message_st));

	SystemCoreClockUpdate();

	/* Enable clocking for CAN and reset the controller */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CAN);
	Chip_SYSCTL_PeriphReset(RESET_CAN);

	// enable error interrupts & automatic retransmission
	LPC_CAN->CNTL = 0b11 | (1 << 3) | (1 << 6);

	// baudrate prescaler
	// BTR register: TSEG1 = 4, TSEG2 = 3, SJW = 0
	LPC_CAN->BT = ((SystemCoreClock / (CONFIG_CAN0_BAUDRATE * 8) - 1) & 0x3F)
				  | (3 << 8) | (2 << 12);
	LPC_CAN->CLKDIV = 0;

	// init all message objects
	for (int i = 0; i < 32; i++) {
		msg_obj_disable(i + 1);
	}

	/* Enable the CAN Interrupt */
	NVIC_EnableIRQ(CAN_IRQn);

	Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_CAN0_TX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_TX_PIN), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_CAN0_RX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_RX_PIN), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_SWM_MovablePortPinAssign(SWM_CAN_TD1_O, UV_GPIO_PORT(CONFIG_CAN0_TX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_TX_PIN));
	Chip_SWM_MovablePortPinAssign(SWM_CAN_RD1_I,  UV_GPIO_PORT(CONFIG_CAN0_RX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_RX_PIN));


#if CAN_LOOP_BACK_MODE
	// enable loop-back mode for testing
	LPC_CAN->CNTL |= 1 << 7;
	LPC_CAN->TEST |= 1 << 4;
#endif

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data,
			CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CAN0, UV_TERMINAL_CAN_RX_ID + uv_get_id(),
			CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
#endif


	// disable access to bit timing register and start CAN controller operation
	LPC_CAN->CNTL &= ~((1 << 0) | (1 << 6));

	return ERR_NONE;

}




uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {

	uv_errors_e ret = ERR_NONE;

	__disable_irq();

	// check if any message objects are configured to receive this type of data already
	bool match = false;
	for (uint8_t i = 0; i < 32; i++) {
		if (GET_MASKED(this->used_msg_objs, (1 << i))) {

			read_msg_obj(i + 1);
			if ((id == get_msgif_id((type == CAN_EXT))) &&
					(mask == get_msgif_mask(CAN_EXT)) &&
					(get_msgif_type() == (type == CAN_EXT))) {

				match = true;
				break;
			}
		}
	}

	if (!match) {
		// this message is not yet configured.
		// If any message objects are still not in use,
		// config them with settings requested
		// the last message object is reserved for sending messages
		for (uint8_t i = 0; i < 32; i++) {
			// search for a unused message object to be used for receiving the messages
			if (!GET_MASKED(this->used_msg_objs, (1 << i))) {

				msg_obj_disable(i + 1);

				read_msg_obj(i + 1);
				// write receive message data to msg obj
				set_msgif_mask(mask, type);
				LPC_CAN->IF1_MSK2 |= (0b11 << 14);		// use msg type and dir for filtering
				LPC_CAN->IF1_ARB2 = 0;	// msg dir receive, also init ARB2 register
				set_msgif_id(id, (type == CAN_EXT));
				if (type == CAN_EXT) {
					LPC_CAN->IF1_ARB2 |= (1 << 14);
				}
				else {
					LPC_CAN->IF1_ARB2 &= ~(1 << 14);
				}
				LPC_CAN->IF1_MCTRL = (1 << 7) |		// single message (end of buffer)
						(1 << 10) |					// receive interrupt enabled
						(1 << 12);					// use mask for filtering

				write_msg_obj(i + 1, false);
				msg_obj_enable(i + 1);

				this->used_msg_objs |= (1 << i);
				match = true;

				printf("configured msg to be received with msg obj %u\n", i);
				break;
			}
		}
	}

	if (!match) {
		ret = ERR_CAN_RX_MESSAGE_COUNT_FULL;
	}

	__enable_irq();

	return ret;
}



void _uv_can_hal_send(uv_can_channels_e chn) {
	if (chn);

	// if tx object is pending, try to wait for the HAl task to release the pending msg obj
	if ((LPC_CAN->TXREQ1 & 1) || !this->init) {
		return;
	}

	uv_can_message_st msg;
	uv_errors_e e = uv_ring_buffer_pop(&this->tx_buffer, &msg);


	if (e == ERR_NONE) {

		__disable_irq();

		msg_obj_disable(TX_MSG_OBJ);

		set_msgif_id(msg.id, (msg.type == CAN_EXT));

		LPC_CAN->IF1_ARB2 |= (1 << 13) |					// transmit msg
				(((msg.type == CAN_EXT) ? 1 : 0) << 14) |	// frame type
				(1 << 15);									// message valid

		LPC_CAN->IF1_MCTRL = (msg.data_length << 0) |		// data length
				(1 << 7) |									// single message (end of buffer)
				(1 << 8) |									// transmit request
				(1 << 11) |									// transmit int enabled
				(1 << 15);									// new data written by CPU
		LPC_CAN->IF1_DA1 = msg.data_16bit[0];
		LPC_CAN->IF1_DA2 = msg.data_16bit[1];
		LPC_CAN->IF1_DB1 = msg.data_16bit[2];
		LPC_CAN->IF1_DB2 = msg.data_16bit[3];

		// load the message into msg obj
		write_msg_obj(TX_MSG_OBJ, true);

		msg_obj_enable(TX_MSG_OBJ);

		__enable_irq();
	}
}



uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	if (channel) {};
	uv_can_errors_e e = CAN_ERROR_ACTIVE;

	if (LPC_CAN->STAT & (1 << 7)) {
		e = CAN_ERROR_BUS_OFF;
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		e = CAN_ERROR_PASSIVE;
	}
	else if (LPC_CAN->STAT & (1 << 6)) {
		e = CAN_ERROR_WARNING;
	}
	else {

	}
	return e;
}


uv_errors_e uv_can_send_sync(uv_can_channels_e channel, uv_can_message_st *msg) {
	uv_errors_e ret = ERR_NONE;

	if (this->init) {

		// wait until tx msg obj is free
		while (LPC_CAN->TXREQ1 & (1 << (TX_MSG_OBJ - 1))) {
			// if CAN bus is in error state, stop and return
			uv_can_errors_e e = uv_can_get_error_state(channel);
			if (e != CAN_ERROR_ACTIVE) {
				ret = ERR_CAN_BUS_OFF;
				break;
			}
			uv_rtos_task_yield();
		}

		if (ret == ERR_NONE) {

		uv_disable_int();

			msg_obj_disable(TX_MSG_OBJ);

			set_msgif_id(msg->id, (msg->type == CAN_EXT));

			LPC_CAN->IF1_ARB2 |= (1 << 13) |					// transmit msg
					(((msg->type == CAN_EXT) ? 1 : 0) << 14) |	// frame type
					(1 << 15);									// message valid

			LPC_CAN->IF1_MCTRL = (msg->data_length << 0) |		// data length
					(1 << 7) |									// single message (end of buffer)
					(1 << 8) |									// transmit request
					(1 << 11) |									// transmit int enabled
					(1 << 15);									// new data written by CPU
			LPC_CAN->IF1_DA1 = msg->data_16bit[0];
			LPC_CAN->IF1_DA2 = msg->data_16bit[1];
			LPC_CAN->IF1_DB1 = msg->data_16bit[2];
			LPC_CAN->IF1_DB2 = msg->data_16bit[3];

			// load the message into msg obj
			write_msg_obj(TX_MSG_OBJ, true);

			msg_obj_enable(TX_MSG_OBJ);


			uv_enable_int();

			// wait until message is transferred or CAN status changes
			bool br = false;
			while (true) {
				if (!(LPC_CAN->TXREQ1 & 1)) {
					br = true;
				}
				else if (LPC_CAN->STAT & (0b111 << 5)) {
					br = true;
					ret = ERR_CAN_BUS_OFF;
				}
				else {

				}
				if (br) {
					break;
				}
			}
		}
	}
	else {
		ret = ERR_NOT_INITIALIZED;
	}

	return ret;
}



#elif CONFIG_TARGET_LINUX
uv_errors_e _uv_can_init() {
	uv_errors_e ret = ERR_NONE;

	return ret;
}

#else
#error "Controller not defined"
#endif











/*
 * GLOBAL FUNCTIONS ACROSS TARGET MCU's
 */


#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif



uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	uv_disable_int();
	uv_errors_e ret = ERR_NONE;
#if (CONFIG_TARGET_LPC1549 || CONFIG_TARGET_LPC11C14)
	ret = uv_ring_buffer_push(&this->tx_buffer, message);
#elif CONFIG_TARGET_LPC1785
	ret = uv_ring_buffer_push(&this->tx_buffer[channel], message);
#endif
	uv_enable_int();
	return ret;
}



uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {
	uv_errors_e ret = ERR_NONE;
	uv_disable_int();
#if (CONFIG_TARGET_LPC1549 || CONFIG_TARGET_LPC11C14)
	ret = uv_ring_buffer_pop(&this->rx_buffer, message);
#elif CONFIG_TARGET_LPC1785
	ret = uv_ring_buffer_pop(&this->rx_buffer[channel], message);
#endif
	uv_enable_int();
	return ret;
}



uv_errors_e uv_can_reset(uv_can_channels_e channel) {
#if CONFIG_TARGET_LPC11C14
	//clearing the C_CAN bit resets C_CAN hardware
	LPC_SYSCON->PRESETCTRL &= ~(1 << 3);
	//set bit to de-assert the reset
	LPC_SYSCON->PRESETCTRL |= (1 << 3);
#elif CONFIG_TARGET_LPC1549
	Chip_SYSCTL_PeriphReset(RESET_CAN);

#endif
	return ERR_NONE;
}



/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {

#if C_CAN
	if (this->tx_pending > 0) {
		this->tx_pending -= step_ms;
	}
	else if (this->tx_pending == 0) {
		this->tx_pending = -1;
	}
#endif

	// send the next message from the buffer
#if CONFIG_CAN0
	_uv_can_hal_send(CAN0);
#endif
#if CONFIG_CAN1
	_uv_can_hal_send(CAN1);
#endif
}


#endif
