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
extern unsigned int CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT (void);



#define RXPDO_COM(x)	,{	\
	.main_index = CONFIG_CANOPEN_RXPDO_COM_INDEX + x,\
	.array_max_size = CANOPEN_RXPDO_COM_ARRAY_SIZE,\
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.rxpdo_coms[x] \
	} \

#define RXPDO_MAP(x)	,{ \
	.main_index = CONFIG_CANOPEN_RXPDO_MAP_INDEX + x, \
	.array_max_size = CONFIG_CANOPEN_PDO_MAPPING_COUNT, \
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = CONFIG_NON_VOLATILE_START.canopen_data.rxpdo_maps[x] \
	} \

#define TXPDO_COM(x)	,{ \
	.main_index = CONFIG_CANOPEN_TXPDO_COM_INDEX + x, \
	.array_max_size = CANOPEN_TXPDO_COM_ARRAY_SIZE, \
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.txpdo_coms[x] \
	} \

#define TXPDO_MAP(x)	,{ \
	.main_index = CONFIG_CANOPEN_TXPDO_MAP_INDEX + x, \
	.array_max_size = CONFIG_CANOPEN_PDO_MAPPING_COUNT, \
	.permissions = CANOPEN_RW, \
	.type = CANOPEN_ARRAY32, \
	.data_ptr = CONFIG_NON_VOLATILE_START.canopen_data.txpdo_maps[x] \
	} \




const canopen_object_st com_params[] = {
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
				.data_ptr = &CONFIG_NON_VOLATILE_START.canopen_data.node_id
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
#if CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT
		{
				.main_index = CONFIG_CANOPEN_CONSUMER_HEARTBEAT_INDEX,
				.array_max_size = CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT,
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
		{
				.main_index = CONFIG_CANOPEN_IDENTITY_INDEX,
				.array_max_size = CANOPEN_IDENTITY_OBJECT_ARRAY_SIZE,
				.permissions = CANOPEN_RO,
				.type = CANOPEN_ARRAY32,
				.data_ptr = &_canopen.identity
		}

		REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_COM)
		REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_MAP)

		REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_COM)
		REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_MAP)

};


static inline unsigned int com_params_count() {
	return sizeof(com_params) / sizeof(canopen_object_st);
}


bool _canopen_obj_dict_get(uint16_t main_index, canopen_object_st *dest) {
	for (unsigned int i = 0; i < com_params_count(); i++) {
		if (com_params[i].main_index == main_index) {
			if (dest) {
				memcpy(dest, &com_params[i], sizeof(canopen_object_st));
			}
			return true;
		}
	}
	for (int i = 0; i < CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT(); i++) {
		if (CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS [i].main_index == main_index) {
			if (dest) {
				memcpy(dest, & CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS [i], sizeof(canopen_object_st));
			}
			return true;
		}
	}
	return false;
}


#endif
