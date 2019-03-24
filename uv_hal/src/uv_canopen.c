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


#include "uv_canopen.h"
#if CONFIG_CANOPEN

#include "uv_can.h"
#include "uv_reset.h"
#include "uv_utilities.h"
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



void _uv_canopen_init(void) {
#if defined(CONFIG_CANOPEN_INITIALIZER)
	// calculate the initializer crc and compare it to ours
	uint16_t crc = uv_memory_calc_crc((void*) &CONFIG_CANOPEN_INITIALIZER,
			sizeof(CONFIG_CANOPEN_INITIALIZER));
	if (crc != this_nonvol->crc) {
		_uv_canopen_reset();
		this_nonvol->crc = crc;
	}

#endif
	this->current_node_id = (CONFIG_NON_VOLATILE_START.id);
	// Greater Node ID than 0x7F is invalid, revert to default
	if (this->current_node_id > 0x7F) {
		this->current_node_id = 0x7F;
	}
	this->can_callback = NULL;
	this->device_type = 'U';
	this->identity.vendor_id = CONFIG_CANOPEN_VENDOR_ID;
	this->identity.product_code = 0;
	this->identity.revision_number = 0;
	this->restore_req[0] = 0x1;
	this->restore_req[1] = 0x1;
	this->restore_req[2] = 0x1;
	this->store_req[0] = 0x1;
	_uv_canopen_nmt_init();
	_uv_canopen_heartbeat_init();
	_uv_canopen_sdo_init();
	_uv_canopen_pdo_init();
	_uv_canopen_emcy_init();
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

	uv_can_message_st msg;
	uv_errors_e e;
	while (!(e = uv_can_pop_message(CONFIG_CANOPEN_CHANNEL, &msg))) {
		// for every received message, canopen module rx functions are called
		_uv_canopen_nmt_rx(&msg);
		_uv_canopen_heartbeat_rx(&msg);
		_uv_canopen_pdo_rx(&msg);
		_uv_canopen_sdo_rx(&msg);
		_uv_canopen_emcy_rx(&msg);
		if (this->can_callback) {
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
	CONFIG_NON_VOLATILE_START.id = nodeid;
}



#endif
