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


#include "canopen/canopen_obj_dict.h"
#include "uv_canopen.h"
#include "uv_terminal.h"
#include <string.h>
#include CONFIG_MAIN_H
#if CONFIG_W25Q128
#include "uv_w25q128.h"
#endif

#if CONFIG_CANOPEN


/// @brief: Object dictionary's application parameter array,
/// given by the user application.
extern const canopen_object_st CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS [];

/// @brief: The length of object dictionary's application parameter array,
/// given by the user application.
extern uint32_t CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT (void);




#define RXPDO_COM(x)	{	\
	.main_index = CONFIG_CANOPEN_RXPDO_COM_INDEX + x,\
	.array_max_size = CANOPEN_RXPDO_COM_ARRAY_SIZE,\
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.rxpdo_coms[x] \
	}, \

#define RXPDO_MAP(x)	{ \
	.main_index = CONFIG_CANOPEN_RXPDO_MAP_INDEX + x, \
	.array_max_size = CONFIG_CANOPEN_PDO_MAPPING_COUNT, \
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.rxpdo_maps[x] \
	}, \

#define TXPDO_COM(x)	{ \
	.main_index = CONFIG_CANOPEN_TXPDO_COM_INDEX + x, \
	.array_max_size = CANOPEN_TXPDO_COM_ARRAY_SIZE, \
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.txpdo_coms[x] \
	}, \

#define TXPDO_MAP(x)	{ \
	.main_index = CONFIG_CANOPEN_TXPDO_MAP_INDEX + x, \
	.array_max_size = CONFIG_CANOPEN_PDO_MAPPING_COUNT, \
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.txpdo_maps[x] \
	}, \



const canopen_object_st com_params[] = {
		REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_COM)
		REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_MAP)

		REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_COM)
		REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_MAP)
		{
				.main_index = CONFIG_CANOPEN_DEVICE_TYPE_INDEX,
				.sub_index = 0,
				.permissions = CANOPEN_RO,
				.type = CANOPEN_UNSIGNED32,
				.data_ptr = &_canopen.device_type
		},
		{
				.main_index = CONFIG_CANOPEN_NODEID_INDEX,
				.sub_index = 0,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_UNSIGNED8,
				.data_ptr = &CONFIG_NON_VOLATILE_START.id
		},
		{
				.main_index = CONFIG_CANOPEN_STORE_PARAMS_INDEX,
				.array_max_size = sizeof(_canopen.store_req) / sizeof(_canopen.store_req[0]),
				.permissions = CANOPEN_RW,
				.type = CANOPEN_ARRAY32,
				.data_ptr = _canopen.store_req
		},
		{
				.main_index = CONFIG_CANOPEN_RESTORE_PARAMS_INDEX,
				.array_max_size = sizeof(_canopen.restore_req) / sizeof(_canopen.restore_req[0]),
				.permissions = CANOPEN_RW,
				.type = CANOPEN_ARRAY32,
				.data_ptr = _canopen.restore_req
		},
#if CONFIG_UV_BOOTLOADER
		{
				.main_index = CONFIG_CANOPEN_PROGRAM_DATA_INDEX,
				.array_max_size = 1,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_ARRAY8,
				// note: This can be set to null, since canopen stack
				// checks if this object is written and in that case resets
				// to the bootloader.
				// This functionality is implemented in _uv_canopen_sdo_server_rx
				.data_ptr = NULL
		},
		{
				.main_index = CONFIG_CANOPEN_PROGRAM_CONTROL_INDEX,
				.array_max_size = 1,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_ARRAY8,
				.data_ptr = &_canopen.prog_control
		},
		{
				.main_index = CONFIG_CANOPEN_PROGRAM_IDENTIF_INDEX,
				.array_max_size = 1,
				.permissions = CANOPEN_RO,
				.type = CANOPEN_ARRAY32,
				.data_ptr = (void*) &uv_prog_version
		},
#endif
#if CONFIG_TERMINAL
		{
				.main_index = CONFIG_CANOPEN_TERMINAL_INDEX,
				.sub_index = 0,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_UNSIGNED8,
				.data_ptr = (void*) &uv_terminal_enabled
		},
#endif
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
		{
				.main_index = CONFIG_CANOPEN_CONSUMER_HEARTBEAT_INDEX,
				.array_max_size = CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_ARRAY32,
				.data_ptr = CONFIG_NON_VOLATILE_START.canopen_data.consumer_heartbeats
		},
#endif
		{
				.main_index = CONFIG_CANOPEN_PRODUCER_HEARTBEAT_INDEX,
				.sub_index = 0,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_UNSIGNED16,
				.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.producer_heartbeat_time_ms
		},
#if CONFIG_CANOPEN_SDO_SEGMENTED
		{
				.main_index = CONFIG_CANOPEN_DEVNAME_INDEX,
				.sub_index = 0,
				.string_len = sizeof(STRINGIFY(__UV_PROJECT_NAME)),
				.permissions = CANOPEN_RO,
				.type = CANOPEN_STRING,
				.data_ptr = (void*) uv_projname
		},
#endif
#if CONFIG_W25Q128
		{
				.main_index = CONFIG_CANOPEN_EXMEM_DATA_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_DATA_TYPE,
				.string_len = CONFIG_EXMEM_BUFFER_SIZE,
				.permissions = CANOPEN_RW,
				.data_ptr = &exmem_data_buffer
		},
		{
				.main_index = CONFIG_CANOPEN_EXMEM_BLOCKSIZE_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_BLOCKSIZE_TYPE,
				.permissions = CANOPEN_RO,
				.data_ptr = (void*) &exmem_blocksize
		},
		{
				.main_index = CONFIG_CANOPEN_EXMEM_OFFSET_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_OFFSET_TYPE,
				.permissions = CANOPEN_RW,
				.data_ptr = &exmem_data_offset
		},
		{
				.main_index = CONFIG_CANOPEN_EXMEM_FILENAME_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_FILENAME_TYPE,
				.string_len = EXMEM_FILENAME_LEN,
				.permissions = CANOPEN_RW,
				.data_ptr = exmem_filename_buffer
		},
		{
				.main_index = CONFIG_CANOPEN_EXMEM_FILESIZE_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_FILESIZE_TYPE,
				.permissions = CANOPEN_RW,
				.data_ptr = &exmem_file_size
		},
		{
				.main_index = CONFIG_CANOPEN_EXMEM_WRITEREQ_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_WRITEREQ_TYPE,
				.permissions = CANOPEN_RW,
				.data_ptr = &exmem_write_req
		},
		{
				.main_index = CONFIG_CANOPEN_EXMEM_CLEARREQ_INDEX,
				.sub_index = 0,
				.type = CONFIG_CANOPEN_EXMEM_CLEARREQ_TYPE,
				.permissions = CANOPEN_RW,
				.data_ptr = &exmem_clear_req
		},
#endif
		{
				.main_index = CONFIG_CANOPEN_IDENTITY_INDEX,
				.array_max_size = CANOPEN_IDENTITY_OBJECT_ARRAY_SIZE,
				.permissions = CANOPEN_RO,
				.type = CANOPEN_ARRAY32,
				.data_ptr = &_canopen.identity
		}

};


static inline unsigned int com_params_count() {
	return sizeof(com_params) / sizeof(canopen_object_st);
}


bool check(const canopen_object_st *src, uint8_t subindex) {
	bool ret;
	if (uv_canopen_is_array(src)) {
		if (subindex > src->array_max_size) {
			ret = false;
		}
		else {
			ret = true;
		}
	}
	else if (subindex != src->sub_index) {
		ret = false;
	}
	else {
		ret = true;
	}
	return ret;
}



const canopen_object_st *_uv_canopen_obj_dict_get(uint16_t main_index, uint8_t subindex) {
	const canopen_object_st *ret = NULL;
	bool match = false;
	for (uint16_t i = 0; i < com_params_count(); i++) {
		if (com_params[i].main_index == main_index) {
			if (check(&com_params[i], subindex)) {
				ret = &com_params[i];
			}
			match = true;
		}
	}
	if (!match) {
		for (int i = 0; i < CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT(); i++) {
			if (CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS [i].main_index == main_index) {
				if (check(& CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS [i], subindex)) {
					ret = & CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS [i];
				}
			}
		}
	}
	return ret;
}



#endif
