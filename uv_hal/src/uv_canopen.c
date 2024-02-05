/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "uv_canopen.h"
#if CONFIG_CANOPEN

#include "uv_can.h"
#include "uv_reset.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#include "uv_memory.h"
#include CONFIG_MAIN_H
#include <string.h>
#if CONFIG_CANOPEN_LOG
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#endif






_uv_canopen_st _canopen;

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			this->current_node_id

#if defined(CONFIG_CANOPEN_INITIALIZER)
extern const uv_canopen_non_volatile_st CONFIG_CANOPEN_INITIALIZER;
#endif



void _uv_canopen_init(uint8_t nodeid) {
#if defined(CONFIG_CANOPEN_INITIALIZER)
	// calculate the initializer crc and compare it to ours
	uint16_t crc = uv_memory_calc_crc((void*) &CONFIG_CANOPEN_INITIALIZER,
			sizeof(CONFIG_CANOPEN_INITIALIZER));
	if (crc != this_nonvol->crc) {
		_uv_canopen_reset();
		this_nonvol->crc = crc;
	}

#endif
	this->current_node_id = (nodeid == 0) ? CONFIG_NON_VOLATILE_START.id : nodeid;
	// Greater Node ID than 0x7F is invalid, revert to default
	if (this->current_node_id > 0x7F) {
		this->current_node_id = 0x7F;
	}
	this->can_callback = NULL;
	this->device_type = 'U';
	this->identity.vendor_id = CONFIG_CANOPEN_VENDOR_ID;
	this->identity.product_code = CONFIG_CANOPEN_PRODUCT_CODE;
	this->identity.revision_number = CONFIG_CANOPEN_REVISION_NUMBER;
	this->restore_req[0] = 0x1;
	this->restore_req[1] = 0x1;
	this->restore_req[2] = 0x1;
	this->store_req[0] = 0x1;
	_uv_canopen_nmt_init();
	_uv_canopen_heartbeat_init();
	_uv_canopen_sdo_init();
	_uv_canopen_pdo_init();
	_uv_canopen_emcy_init();

	uv_canopen_config_rx_msgs();

#if CONFIG_UV_BOOTLOADER
	// program started
	this->prog_control = 1;
#endif
}

void _uv_canopen_reset(void) {
#if defined(CONFIG_CANOPEN_INITIALIZER)
	memcpy(this_nonvol, &CONFIG_CANOPEN_INITIALIZER, sizeof(CONFIG_CANOPEN_INITIALIZER));
#endif
	_uv_canopen_nmt_reset();
	_uv_canopen_heartbeat_reset();
	_uv_canopen_sdo_reset();
	_uv_canopen_pdo_reset();
}




void _uv_canopen_step(unsigned int step_ms) {
	_uv_canopen_heartbeat_step(step_ms);
	_uv_canopen_pdo_step(step_ms);
	_uv_canopen_nmt_step(step_ms);
	_uv_canopen_sdo_step(step_ms);
	_uv_canopen_emcy_step(step_ms);

	uv_can_message_st msg;
	uv_errors_e e;
	while (!(e = uv_can_pop_message(CONFIG_CANOPEN_CHANNEL, &msg))) {
		// for every received message, canopen module rx functions are called
		_uv_canopen_nmt_rx(&msg);
		_uv_canopen_heartbeat_rx(&msg);
		_uv_canopen_pdo_rx(&msg);
		_uv_canopen_sdo_rx(&msg);
		_uv_canopen_emcy_rx(&msg);
		if (uv_canopen_get_state() != CANOPEN_STOPPED &&
			this->can_callback) {
			this->can_callback(__uv_get_user_ptr(), &msg);
		}
	}

	// check for restore or store requests
	if (this->restore_req[0] == 0x64616F6C) {
		this->restore_req[0] = 0x1;
		uv_memory_clear(MEMORY_ALL_PARAMS);
	}
	else if (this->restore_req[1] == 0x64616F6C) {
		this->restore_req[1] = 0x1;
		uv_memory_clear(MEMORY_COM_PARAMS);
	}
	else if (this->restore_req[2] == 0x64616F6C) {
		this->restore_req[2] = 0x1;
		uv_memory_clear(MEMORY_APP_PARAMS);
	}
	else {

	}
	if (this->store_req[0] == 0x65766173) {
		this->store_req[0] = 0x1;
		uv_memory_save();
	}
#if CONFIG_UV_BOOTLOADER
	// program control
	// Note: CiA 302 defines requests to this object to be rejected
	// in all other states than PREOPERATIONAL. However, to extend the compatibility,
	// this definition is discarded and the object works just as any other object.
	if (this->prog_control == 2) {
		// reset program
		uv_system_reset();
	}
#endif
}



void uv_canopen_set_state(canopen_node_states_e state) {
	_uv_canopen_nmt_set_state(state);
}

canopen_node_states_e uv_canopen_get_state(void) {
	return _uv_canopen_nmt_get_state();
}

void uv_canopen_set_can_callback(void (*callb)(void *user_ptr, uv_can_message_st *msg)) {
	this->can_callback = callb;
}


uint8_t uv_canopen_sdo_read8(uint8_t node_id, uint16_t mindex,
		uint8_t sindex) {
	uint8_t ret = 0;
	uv_canopen_sdo_read(node_id, mindex, sindex, sizeof(uint8_t), &ret);
	return ret;
}

uint16_t uv_canopen_sdo_read16(uint8_t node_id, uint16_t mindex,
		uint8_t sindex) {
	uint16_t ret = 0;
	uv_canopen_sdo_read(node_id, mindex, sindex, sizeof(uint16_t), &ret);
	return ret;
}

uint32_t uv_canopen_sdo_read32(uint8_t node_id, uint16_t mindex,
		uint8_t sindex) {
	uint32_t ret = 0;
	uv_canopen_sdo_read(node_id, mindex, sindex, sizeof(uint32_t), &ret);
	return ret;
}


uv_errors_e uv_canopen_sdo_write8(uint8_t node_id, uint16_t mindex, uint8_t sindex, uint8_t data) {
	return uv_canopen_sdo_write(node_id, mindex, sindex, sizeof(uint8_t), &data);
}

uv_errors_e uv_canopen_sdo_write16(uint8_t node_id, uint16_t mindex, uint8_t sindex, uint16_t data) {
	return uv_canopen_sdo_write(node_id, mindex, sindex, sizeof(uint16_t), &data);
}

uv_errors_e uv_canopen_sdo_write32(uint8_t node_id, uint16_t mindex, uint8_t sindex, uint32_t data) {
	return uv_canopen_sdo_write(node_id, mindex, sindex, sizeof(uint32_t), &data);
}



void uv_canopen_config_rx_msgs(void) {
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	// Heartbeat consumer
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_HEARTBEAT_ID, ~0x7F, CAN_STD);
#endif

#if !CONFIG_CANOPEN_RXCONFIG_DISABLE
	// NMT
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_NMT_ID, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif

	// RXPDO
	const canopen_object_st *obj;
	for (int i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		if ((obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_RXPDO_COM_INDEX + i, 0))) {
			canopen_rxpdo_com_parameter_st* com = obj->data_ptr;
			if (uv_canopen_is_array(obj)) {
				this->rxpdo[i].com_ptr = com;
				if (!(com->cob_id & CANOPEN_PDO_DISABLED)) {
					// only config the receive msg object if the pdo is enabled
					uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
							// clear possible EXTENDED ID flag bit
							com->cob_id & ~CANOPEN_PDO_EXT,
							CAN_ID_MASK_DEFAULT, (com->cob_id & CANOPEN_PDO_EXT) ?
									CAN_EXT : CAN_STD);
				}
			}
			// configure mapping pointers
			_uv_canopen_pdo_mapping_ptr_conf(&this_nonvol->rxpdo_maps[i],
					this->rxpdo[i].mapping_ptr, CANOPEN_WO);
		}
		else {
			// something went wrong, PDO communication parameter couldn't be found
		}
	}
	// TXPDO
	for (int i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		if ((obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_TXPDO_COM_INDEX + i, 0))) {
			canopen_txpdo_com_parameter_st *com = obj->data_ptr;
			if (uv_canopen_is_array(obj)) {
				this->txpdo[i].com_ptr = com;
			}
		}
		// configure mapping pointers
		_uv_canopen_pdo_mapping_ptr_conf(&this_nonvol->txpdo_maps[i],
				this->txpdo[i].mapping_ptr, CANOPEN_RO);
	}


	// SDO Client
	// configure to receive all SDO response messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID, ~0x7F, CAN_STD);

	// SDO Server
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_REQUEST_ID + NODEID, CAN_ID_MASK_DEFAULT, CAN_STD);

	// EMCY
	// all emcy messages should be received
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_EMCY_ID, ~0x7F, CAN_STD);

}


uv_errors_e uv_canopen_sdo_restore_params(uint8_t node_id, memory_scope_e_ param_scope) {
	uint32_t d = 0x64616F6C;
	uint8_t subindex;
	switch(param_scope) {
	case MEMORY_COM_PARAMS:
		subindex = 2;
		break;
	case MEMORY_APP_PARAMS:
		subindex = 3;
		break;
	default:
		subindex = 1;
		break;
	}
	return uv_canopen_sdo_write(node_id, CONFIG_CANOPEN_RESTORE_PARAMS_INDEX, subindex,
			CANOPEN_SIZEOF(CANOPEN_UNSIGNED32), &d);
}

uv_errors_e uv_canopen_sdo_store_params(uint8_t node_id, memory_scope_e_ param_scope) {
	uint32_t d = 0x65766173;
	uint8_t subindex;
	switch(param_scope) {
	case MEMORY_COM_PARAMS:
		subindex = 2;
		break;
	case MEMORY_APP_PARAMS:
		subindex = 3;
		break;
	default:
		subindex = 1;
		break;
	}
	return uv_canopen_sdo_write(node_id, CONFIG_CANOPEN_STORE_PARAMS_INDEX, subindex,
			CANOPEN_SIZEOF(CANOPEN_UNSIGNED32), &d);
}


void uv_canopen_set_our_nodeid(uint8_t nodeid) {
	uint8_t last_nodeid = uv_canopen_get_our_nodeid();
	CONFIG_NON_VOLATILE_START.id = nodeid;
	// update all PDO's which where mapped for the previous node id
	uv_enter_critical();
	for (uint8_t i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		canopen_txpdo_com_parameter_st *com = &dev.data_start.canopen_data.txpdo_coms[i];
		if ((com->cob_id & 0x7F) == last_nodeid) {
			com->cob_id &= ~(0x7F);
			com->cob_id |= nodeid;
		}
	}
	for (uint8_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		canopen_rxpdo_com_parameter_st *com = &dev.data_start.canopen_data.rxpdo_coms[i];
		if ((com->cob_id & 0x7F) == last_nodeid) {
			com->cob_id &= ~(0x7F);
			com->cob_id |= nodeid;
		}
	}
	uv_exit_critical();
}


bool uv_canopen_rxpdo_is_received(uint16_t rxpdo_i) {
	bool ret = false;
	if (rxpdo_i < CONFIG_CANOPEN_RXPDO_COUNT) {
		ret = !uv_delay_has_ended(&this->rxpdo[rxpdo_i].def_delay);
	}
	return ret;
}



#endif
