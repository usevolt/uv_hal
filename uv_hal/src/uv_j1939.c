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

	// config rx objects
	// multipackage protocol start msg
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
							 0x18ECFF00,
							 CAN_ID_MASK_DEFAULT,
							 CAN_EXT);
	// multipacket protocol data
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
							 0x18EBFF00,
							 CAN_ID_MASK_DEFAULT,
							 CAN_EXT);
}

bool uv_j1939_transport_rx(uv_j1939_transport_st *this, uv_can_msg_st *msg) {
	if (msg->type == CAN_EXT) {
		switch (msg->id) {
			case J1939_TRANSPORT_CONNECTION_MANAGEMENT:
				if (msg->data_length == 8) {
					uint16_t pgn = msg->data_8bit[5] | (msg->data_16bit[3] << 8);
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


uv_errors_e uv_j1939_dm1_get_dtc(j1939_dtc_st *dest,
								 uint8_t *raw_data,
								 uint16_t data_len,
								 uint8_t dtc_index) {
	uv_errors_e ret = ERR_NONE;
	if (dest != NULL &&
			data_len >= dtc_index * 4 + 2) {
		uint16_t i = 2 + dtc_index * 4;
		dest->spn = raw_data[i] +
				(raw_data[i + 1] << 8) +
				((raw_data[i + 2] >> 5) & 0b111);
		dest->fmi = (raw_data[i + 2] & 0x1F);
		dest->cm = (raw_data[i + 3] >> 7);
		dest->oc = raw_data[i + 3] & 0x7F;
	}
	else {
		ret = ERR_BUFFER_OVERFLOW;
	}
	return ret;
}

