/*
 * uw_can.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#include "uw_can.h"


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

} this_st;
static this_st _this;
#define this (&_this)

#include "uw_can.h"
#include "hal_can.h"

uw_can_message_st uw_can_create_message(uint32_t id, uint8_t data_length, uint8_t* data) {
	uw_can_message_st m;
	int i;
	m.id = id;
	m.data_length = data_length;
	for (i = 0; i < data_length; i++) {
		m.data_8bit[i] = data[i];
	}
	return m;
}

