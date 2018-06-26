/*
 * canopen_obj_dict.h
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_


#include <uv_hal_config.h>
#include "canopen/canopen_common.h"

#if CONFIG_CANOPEN


//#define _OBJ(mindex, sindex, typee, permissionss, data_ptrr) \
//,{\
//	.main_index = mindex, \
//	.sub_index = sindex, \
//	.type = typee, \
//	.permissions = permissionss, \
//	.data_ptr = data_ptrr \
//}
//#define _OBJARR(mindex, arr_size, typee, permissionss, data_ptrr) \
//,{\
//	.main_index = mindex, \
//	.array_max_size = arr_size, \
//	.type = typee, \
//	.permissions = permissionss, \
//	.data_ptr = data_ptrr \
//}
//#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
//#define _CONSHEARTBEAT() _OBJARR(CONFIG_CANOPEN_CONSUMER_HEARTBEAT_INDEX, CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT,\
//		CANOPEN_HEARTBEAT_CONSUMER_ST_TYPE, CANOPEN_RW, (_objdict_nonvol->consumer_heartbeats))
//#else
//#define _CONSHEARTBEAT()
//#endif
//#if CONFIG_CANOPEN_TXPDO_COUNT
//#define _TXPDOCOM(x)	_OBJARR(CONFIG_CANOPEN_TXPDO_COM_INDEX + x, CANOPEN_TXPDO_COM_ARRAY_SIZE, \
//		CANOPEN_TXPDO_COM_ARRAY_TYPE, CANOPEN_RW, &(_objdict_nonvol->txpdo_coms[x]))
//#define _TXPDOCOMS()	REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, _TXPDOCOM)
//#else
//#define _TXPDOCOMS()
//#endif
//#if CONFIG_CANOPEN_RXPDO_COUNT
//#define _RXPDOCOM(x)	_OBJARR(CONFIG_CANOPEN_RXPDO_COM_INDEX + x, CANOPEN_RXPDO_COM_ARRAY_SIZE, \
//		CANOPEN_RXPDO_COM_ARRAY_TYPE, CANOPEN_RW, &(_objdict_nonvol->rxpdo_coms[x]))
//#define _RXPDOCOMS()	REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, _RXPDOCOM)
//#else
//#define _RXPDOCOMS()
//#endif
//#if CONFIG_CANOPEN_VARIABLE_PDO_MAPPING
//#define _TXPDOMAP(x)	_OBJARR(CONFIG_CANOPEN_TXPDO_MAP_INDEX + x, CONFIG_CANOPEN_PDO_MAPPING_COUNT, \
//		CANOPEN_PDO_MAPPING_PARAMETER_TYPE, CANOPEN_RW, &(_objdict_nonvol->txpdo_maps[x]))
//#define _TXPDOMAPS()	REPEAT(CONFIG_CANOPEN_PDO_MAPPING_COUNT, _TXPDOMAP)
//#define _RXPDOMAP(x)	_OBJARR(CONFIG_CANOPEN_RXPDO_MAP_INDEX + x, CONFIG_CANOPEN_PDO_MAPPING_COUNT, \
//		CANOPEN_PDO_MAPPING_PARAMETER_TYPE, CANOPEN_RW, &(_objdict_nonvol->rxpdo_maps[x]))
//#define _RXPDOMAPS()	REPEAT(CONFIG_CANOPEN_PDO_MAPPING_COUNT, _RXPDOMAP)
//#else
//#define _TXPDOMAPS()
//#define _RXPDOMAPS()
//#endif
//
//
//#define _objdict_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
//
///// @brief: This macro should be added in the user application object dictionary array
///// to enable CAN communication with the default object dictionary entries, such as
///// load & save parameters, node id, identity field, etc.
//#define CANOPEN_OBJ_DICT() \
//		_OBJ(CONFIG_CANOPEN_NODEID_INDEX, 0, CANOPEN_UNSIGNED8, CANOPEN_RW, &CONFIG_NON_VOLATILE_START.id) \
//		_OBJ(CONFIG_CANOPEN_PRODUCER_HEARTBEAT_INDEX, 0, CANOPEN_UNSIGNED16, CANOPEN_RW, &(_objdict_nonvol->producer_heartbeat_time_ms)) \
//		_CONSHEARTBEAT() \
//		_TXPDOCOMS() \
//		_TXPDOMAPS() \
//		_RXPDOCOMS() \
//		_RXPDOMAPS() \
//		_OBJ(CONFIG_CANOPEN_STORE_PARAMS_INDEX, 1, CANOPEN_UNSIGNED32, CANOPEN_RW, &(_canopen.store_req)) \
//		_OBJ(CONFIG_CANOPEN_RESTORE_PARAMS_INDEX, 1, CANOPEN_UNSIGNED32, CANOPEN_RW, &(_canopen.restore_req))






/// @brief: Returns the CANopen object dictionary object pointed by **main_index**.
/// Subindexes are not supported, since they are assumed to be array indexes. However,
/// subindex can be given to this function, in which case the function checks
/// if the object was an array type and that the object had as many indexes as
/// requested.
///
/// @return: Null if the object couldn't be found.
const canopen_object_st *_uv_canopen_obj_dict_get(uint16_t main_index, uint8_t subindex);




#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_ */
