/*
 * uw_can.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

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


uw_can_errors_e uw_can_send(uint32_t id, uint8_t data_length, uint8_t* data) {

}

uw_can_errors_e uw_can_send_message(uw_can_message_st* message) {

}
