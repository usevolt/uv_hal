/*
 * uv_canopen.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */


#include "uv_canopen.h"
#if CONFIG_CANOPEN

#include "uv_can.h"
#include "uv_reset.h"
#include "uv_utilities.h"
#include "uv_memory.h"
#include <string.h>
#if CONFIG_CANOPEN_LOG
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#endif
extern uv_errors_e __uv_save_previous_non_volatile_data();
extern uv_errors_e __uv_clear_previous_non_volatile_data();







_uv_canopen_st _canopen;

#define this (&_canopen)




void _uv_canopen_init(void) {


}

void _uv_canopen_reset(void) {

}




void _uv_canopen_step(unsigned int step_ms) {

}



void uv_canopen_set_state(canopen_node_states_e state) {

}

canopen_node_states_e uv_canopen_get_state(void) {

}

uv_errors_e uv_canopen_send_sdo(uv_canopen_sdo_message_st *sdo, uint8_t node_id) {
}


uv_errors_e uv_canopen_sdo_write(uv_canopen_sdo_commands_e sdoreq, uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data) {

}



#endif
