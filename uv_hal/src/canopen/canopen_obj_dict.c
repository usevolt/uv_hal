/*
 * canopen_obj_dict.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_obj_dict.h"
#include "uv_canopen.h"
#include <string.h>
#include CONFIG_MAIN_H

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
				.sub_index = 1,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_UNSIGNED32,
				.data_ptr = &_canopen.store_req
		},
		{
				.main_index = CONFIG_CANOPEN_RESTORE_PARAMS_INDEX,
				.sub_index = 1,
				.permissions = CANOPEN_RW,
				.type = CANOPEN_UNSIGNED32,
				.data_ptr = &_canopen.restore_req
		},
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
