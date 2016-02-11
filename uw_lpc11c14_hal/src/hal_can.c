/*
 * uw_can_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#include <hal_terminal.h>
#include "hal_can_controller.h"
#include "hal_stdout.h"
#include "LPC11xx.h"
#include "stdlib.h"
#include <stdint.h>
#include <stdio.h>

// for debugging can connection
#include "hal_uart_controller.h"
#include "hal_reset_controller.h"


/// @brief: Time limit for pending message objects. If this exceedes, all message objects are
/// released from pending state.
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		50




void CAN_rx(uint8_t msg_obj_num);
void CAN_tx(uint8_t msg_obj_num);
void CAN_error(uint32_t error_info);



/************ C_CAN hardware configuration ************************/
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

// LPC11Cxx C_CAN driver struct. Do not change!
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
	hal_can_msg_objs_t msgobj_rx;
	hal_can_msg_objs_t msgobj_tx;
	uint8_t isr_handled;
	uint32_t od_const_num;
	hal_canopen_obj_dict_const_entry_st *od_const_table;
	uint32_t od_num;
	hal_canopen_obj_dict_entry_st *od_table;

} CCAN_CANOPENCFG_T;

// LPC11Cxx C_CAN driver struct. Do not change!
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



/// @brief: global message object for CAN receive callback function parameter
static hal_can_msg_obj_st msg_obj;

/// @brief: variable remebering all message objects which wait for message to be sent
static uint32_t pending_msg_objs = 0;

/// @brief: Timer for pending message objects. If no message objects are set or released from pending
/// in a certain time limit, all pending objects are cleared. This needs to be done since the LPC11Cxx
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

	// call application callback if assigned
	if (rx_callback != NULL) {
		rx_callback(&msg_obj);
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


inline void hal_can_add_rx_callback(
		void (*callback_function) (hal_can_msg_obj_st*)) {
	rx_callback = callback_function;
}


inline void hal_can_add_error_callback(void (*callback_function)(uint32_t)) {
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

