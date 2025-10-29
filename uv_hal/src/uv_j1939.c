/*
 * uv_j1939.c
 *
 *  Created on: Oct 29, 2025
 *      Author: usevolt
 */

#include "uv_j1939.h"


void uv_j1939_transport_init(uv_j1939_transport_st *this,
						  void *dest, uint16_t dest_max_count,
						  uint16_t pgn) {
	this->byte_count = 0;
	this->byte_index = 0;
	this->dest = dest;
	this->pgn = pgn;
	this->dest_max_count = dest_max_count;
}

bool uv_j1939_transport_rx(uv_j1939_transport_st *this, uv_can_msg_st *msg) {
	if (msg->type == CAN_EXT) {
		switch (msg->id) {
			case J1939_TRANSPORT_CONNECTION_MANAGEMENT:
				if (msg->data_length == 8) {
					uint16_t pgn = msg->data_16bit[3];
					if (!this->pgn) {
						this->pgn = pgn;
					}
					if (pgn == this->pgn) {
						this->byte_count = msg->data_8bit[1] + (msg->data_8bit[2] << 8);
						this->byte_index = 0;
					}
					else {
						this->byte_count = 0;
					}
				}
				break;
			case J1939_TRANSPORT_DATA_TRANSFER:
				for (uint8_t i = 1; i < msg->data_length; i++) {
					if (this->byte_index < this->dest_max_count &&
							this->byte_index < this->byte_count) {
						((uint8_t*) this->dest)[this->byte_index++] = msg->data_8bit[i];
					}
					else {
						break;
					}
				}
				break;
			default:
				break;
		}
	}
	return (this->byte_index == this->byte_count);
}

