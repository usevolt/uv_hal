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



/// @brief: Returns the CANopen object dictionary object pointed by **main_index**.
///
/// @return: False if the object couldn't be found.
bool _canopen_obj_dict_get(uint16_t main_index, canopen_object_st *dest);


#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_ */
