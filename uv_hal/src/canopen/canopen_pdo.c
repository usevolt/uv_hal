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

#include "canopen/canopen_pdo.h"
#include "string.h"
#include "uv_rtos.h"
#include CONFIG_MAIN_H


#if CONFIG_CANOPEN

#define this 								(&_canopen)
#define thisnv								(&CONFIG_NON_VOLATILE_START.canopen_data)

#define NODEID								this->current_node_id

#if !defined(CONFIG_CANOPEN_INITIALIZER)
// PDO's are defined with preprocessor macros in uv_hal_config. The original, but limited,
// uv_hal way

#define RXPDO(x)							CAT(RXPDO, INC(x))
#define TXPDO(x)							CAT(TXPDO, INC(x))
#define MAPPING(x)							CAT(_MAPPING, INC(x))

#define PDO_COB_ID(pdo)						(CAT(CONFIG_CANOPEN_, CAT(pdo, _ID)))
#define PDO_TTYPE(pdo)						(CAT(CONFIG_CANOPEN_, CAT(pdo, _TRANSMISSION_TYPE)))
#define PDO_EVENT_TIMER(pdo)				(CAT(CONFIG_CANOPEN_, CAT(pdo, _EVENT_TIMER)))
#define PDO_INHIBIT_TIME(pdo)				(CAT(CONFIG_CANOPEN_, CAT(pdo, _INHIBIT_TIME)))
#define PDO_MAPPING_MINDEX(pdo, mapping)	(CAT(CONFIG_CANOPEN_, CAT(pdo, CAT(mapping, _MAIN_INDEX))))
#define PDO_MAPPING_SINDEX(pdo, mapping)	(CAT(CONFIG_CANOPEN_, CAT(pdo, CAT(mapping, _SUB_INDEX))))
#define PDO_MAPPING_LENGTH(pdo, mapping)	(CAT(CONFIG_CANOPEN_, CAT(pdo, CAT(mapping, _LEN))))


#define RXPDO_COM_DEF(x) 	\
{ \
	.cob_id = PDO_COB_ID(RXPDO(x)),\
	.transmission_type = PDO_TTYPE(RXPDO(x)), \
},

#define RXPDO_MAP_DEF(mapping, pdo) \
{ \
	.main_index = \
	PDO_MAPPING_MINDEX(RXPDO(pdo), MAPPING(mapping)), \
	.sub_index = \
	PDO_MAPPING_SINDEX(RXPDO(pdo), MAPPING(mapping)), \
	.length = \
	PDO_MAPPING_LENGTH(RXPDO(pdo), MAPPING(mapping)) \
},

#define RXPDO_MAPPINGS_DEF(x)	\
{ \
		.mappings = { \
				REPEAT_ARG2(CONFIG_CANOPEN_PDO_MAPPING_COUNT, RXPDO_MAP_DEF, x) \
		} \
},

#define TXPDO_COM_DEF(x) 	\
{ \
	.cob_id = PDO_COB_ID(TXPDO(x)),\
	.transmission_type = PDO_TTYPE(TXPDO(x)), \
	.event_timer = PDO_EVENT_TIMER(TXPDO(x)), \
	.inhibit_time = PDO_INHIBIT_TIME(TXPDO(x)), \
	._reserved = 0 \
},

#define TXPDO_MAP_DEF(mapping, pdo) \
{ \
	.main_index = \
	PDO_MAPPING_MINDEX(TXPDO(pdo), MAPPING(mapping)), \
	.sub_index = \
	PDO_MAPPING_SINDEX(TXPDO(pdo), MAPPING(mapping)), \
	.length = \
	PDO_MAPPING_LENGTH(TXPDO(pdo), MAPPING(mapping)) \
},

#define TXPDO_MAPPINGS_DEF(x)	\
{ \
		.mappings = { \
				REPEAT_ARG2(CONFIG_CANOPEN_PDO_MAPPING_COUNT, TXPDO_MAP_DEF, x) \
		}\
},

static const canopen_txpdo_com_parameter_st txpdo_com_defs[] = {
		REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_COM_DEF)
};
static const canopen_pdo_mapping_parameter_st txpdo_map_defs[] = {
		REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_MAPPINGS_DEF)
};
static const canopen_rxpdo_com_parameter_st rxpdo_com_defs[] = {
		REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_COM_DEF)
};
static const canopen_pdo_mapping_parameter_st rxpdo_map_defs[] = {
		REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_MAPPINGS_DEF)
};
#endif




/// @brief: Returns true if this PDO message was enabled (bit 31 was not set)
#define IS_ENABLED(pdo_com_ptr)			(!((pdo_com_ptr)->cob_id & (1 << 31)))




void _uv_canopen_pdo_init() {
	memset(this->txpdo, 0, sizeof(this->txpdo));
	memset(this->rxpdo, 0, sizeof(this->rxpdo));

	const canopen_object_st *obj;
	for (int i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		if ((obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_TXPDO_COM_INDEX + i, 0))) {
			// PDO communication parameter found
			canopen_txpdo_com_parameter_st *com;
			com = obj->data_ptr;
			uv_delay_init((uv_delay_st*) &this->txpdo[i].time, com->event_timer);
			this->txpdo[i].inhibit_time = 0;
		}
		else {
			// something went wrong
		}
	}
	for (int32_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		uv_delay_init(&this->rxpdo[i].def_delay, CONFIG_CANOPEN_RXPDO_TIMEOUT_MS);
		uv_delay_end(&this->rxpdo[i].def_delay);
	}
}




void _uv_canopen_pdo_reset() {
#if !defined(CONFIG_CANOPEN_INITIALIZER)
	for (int i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		thisnv->txpdo_coms[i] = txpdo_com_defs[i];
		thisnv->txpdo_maps[i] = txpdo_map_defs[i];
	}
	for (int i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		thisnv->rxpdo_coms[i] = rxpdo_com_defs[i];
		thisnv->rxpdo_maps[i] = rxpdo_map_defs[i];
	}
#endif
}



void _uv_canopen_pdo_step(uint16_t step_ms) {

	// PDO's are active only if the device is in operational state
	if (_uv_canopen_nmt_get_state() != CANOPEN_OPERATIONAL) {
		return;
	}

	/*
	 * TXPDO
	 */
	for (int i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		const canopen_txpdo_com_parameter_st *com = this->txpdo[i].com_ptr;

		// check inhibit time
		if (this->txpdo[i].inhibit_time > 0) {
			this->txpdo[i].inhibit_time -= step_ms;
		}

		// if PDO is not enabled, skip it
		if (IS_ENABLED(com)) {
			// check if event timer in this PDO triggers and the inhibit time has passed
			// since last transmission
			uv_delay((uv_delay_st*) &this->txpdo[i].time, step_ms);
			if ((uv_delay_has_ended(&this->txpdo[i].time)) &&
					(this->txpdo[i].inhibit_time <= 0)) {
				// initialize delay again
				uv_delay_init((uv_delay_st*) &this->txpdo[i].time, com->event_timer);
				this->txpdo[i].inhibit_time = com->inhibit_time;

				uv_can_message_st msg = {};
				uint8_t next_mapped_byte = 0;

				for (uint8_t j = 0; (j < CONFIG_CANOPEN_PDO_MAPPING_COUNT); j++) {
					uint8_t *ptr = this->txpdo[i].mapping_ptr[j];
					if (ptr) {
						msg.data_8bit[j] = *ptr;
						next_mapped_byte = j + 1;
					}
				}
				if (next_mapped_byte) {
					// send the txpdo if any data got mapped
					msg.type = CAN_STD;
					msg.data_length = next_mapped_byte;
					msg.id = com->cob_id;
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
					// send all PDO's locally in case if this device is mapped
					// to receive it's own messages
					uv_can_send_local(CONFIG_CANOPEN_CHANNEL, &msg);
				}
			}
		}
	}

	/*
	 * RXPDO
	 */
	for (int32_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		if (uv_delay(&this->rxpdo[i].def_delay, step_ms)) {
			// the delay has passed, reset the mapped object dictionary parameters
			// to their default values

			for (uint8_t j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {
				canopen_pdo_mapping_st *map = &thisnv->rxpdo_maps[i].mappings[j];

				if (map->length != 0 &&
						map->main_index != 0) {
					// if mapping was found, try to find the object that it was pointing
					// and reset it to defaults
					const canopen_object_st *obj =
							_uv_canopen_obj_dict_get(map->main_index, map->sub_index);
					if (obj != NULL &&
							obj->data_ptr != NULL) {
						if (obj->def_ptr != NULL) {
							// if default value pointer is assigned,
							// copy the default value to the parameter
							memcpy(obj->data_ptr, obj->def_ptr, CANOPEN_SIZEOF(obj->type));
						}
						else {
							// default value pointer not assigned
							// initialize the data to zeroes
							memset(obj->data_ptr, 0, CANOPEN_SIZEOF(obj->type));
						}
					}
				}
			}
		}
	}
}



void _uv_canopen_pdo_rx(const uv_can_message_st *msg) {

	// PDO's are active only if the device is in operational state
	bool valid = true;
	if (_uv_canopen_nmt_get_state() != CANOPEN_OPERATIONAL) {
		valid = false;
	}
	if (!valid) {
		return;
	}

	/*
	 * RXPDO
	 */
	for (uint8_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		valid = true;
		const canopen_rxpdo_com_parameter_st *com = this->rxpdo[i].com_ptr;
		if (com) {
			if (com->cob_id & CANOPEN_PDO_DISABLED) {
				valid = false;
			}
			// CAN frame type check
			else if (!!(com->cob_id & CANOPEN_PDO_EXT) == (msg->type == CAN_STD)) {
				valid = false;
			}
			// CAN ID check
			else if ((com->cob_id & 0x1FFFFFFF) != msg->id) {
				valid = false;
			}
			else {

			}

		}

		if (valid) {
			// update the def_delay and prevent setting the parameters
			// to their default values
			uv_delay_init(&this->rxpdo[i].def_delay, CONFIG_CANOPEN_RXPDO_TIMEOUT_MS);

			// matching RXPDO found. copy data to it's pointers
			for (uint8_t j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {
				uint8_t *dest = this->rxpdo[i].mapping_ptr[j];
				if (dest) {
					memcpy(dest, &msg->data_8bit[j], sizeof(uint8_t));
				}
			}
		}
	}
}


void uv_canopen_pdo_mapping_update(uint16_t main_index, uint8_t subindex) {

	// check for PDO mappings and trigger that PDO
	for (uint16_t i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		const canopen_object_st *obj;
		if ((obj = _uv_canopen_obj_dict_get(
				CONFIG_CANOPEN_TXPDO_MAP_INDEX + i, 0))) {
			canopen_pdo_mapping_parameter_st *mapping_par = obj->data_ptr;
			for (uint8_t j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {
				if ((mapping_par->mappings[j].main_index == main_index) &&
						(mapping_par->mappings[j].sub_index == subindex)) {
					uv_delay_trigger(&this->txpdo[i].time);
					break;
				}
			}
		}
	}
}


canopen_pdo_mapping_parameter_st *uv_canopen_rxpdo_get_mapping(uint16_t rxpdo) {
	LIMIT_MAX(rxpdo, CONFIG_CANOPEN_RXPDO_COUNT - 1);
	return &thisnv->rxpdo_maps[rxpdo];
}


canopen_pdo_mapping_parameter_st *uv_canopen_txpdo_get_mapping(uint16_t txpdo) {
	LIMIT_MAX(txpdo, CONFIG_CANOPEN_TXPDO_COUNT - 1);
	return &thisnv->txpdo_maps[txpdo];
}



canopen_rxpdo_com_parameter_st *uv_canopen_rxpdo_get_com(uint16_t rxpdo) {
	if (rxpdo >= CONFIG_CANOPEN_RXPDO_COUNT) {
		rxpdo = CONFIG_CANOPEN_RXPDO_COUNT - 1;
	}
	return &thisnv->rxpdo_coms[rxpdo];
}

canopen_txpdo_com_parameter_st *uv_canopen_txpdo_get_com(uint16_t txpdo) {
	canopen_txpdo_com_parameter_st *ret = NULL;
	if (txpdo < CONFIG_CANOPEN_TXPDO_COUNT) {
		ret = &thisnv->txpdo_coms[txpdo];
	}
	return ret;
}


void _uv_canopen_pdo_mapping_ptr_conf(canopen_pdo_mapping_parameter_st *mapping_param,
		uint8_t **ptr_bfr, canopen_permissions_e permissions) {
	// initialize all mappings to NULL
	memset(ptr_bfr, 0, sizeof(uint8_t*) * CONFIG_CANOPEN_PDO_MAPPING_COUNT);
	uint8_t byte_count = 0;
	for (uint8_t i = 0; i < CONFIG_CANOPEN_PDO_MAPPING_COUNT; i++) {
		canopen_pdo_mapping_st *map = &mapping_param->mappings[i];
		// length 0 means we stop mapping here
		if (map->length == 0 ||
				byte_count >= CONFIG_CANOPEN_PDO_MAPPING_COUNT) {
			break;
		}
		const canopen_object_st *obj =
				_uv_canopen_obj_dict_get(map->main_index, map->sub_index);

		if (obj != NULL) {
			if ((obj->permissions & permissions)) {
				uint8_t *ptr = NULL;
				if (uv_canopen_is_array(obj)) {
					if (((map->sub_index != 0) &&
							(map->sub_index - 1) + map->length) <=
							(obj->array_max_size + CANOPEN_SIZEOF(obj->type))) {
						ptr = (uint8_t*) obj->data_ptr + (map->sub_index - 1) *
								uv_canopen_get_object_data_size(obj);
					}
				}
				else if (uv_canopen_is_string(obj)) {
					if (map->sub_index + map->length <= obj->string_len) {
						ptr = (uint8_t*) obj->data_ptr + (map->sub_index - 1);
					}
				}
				else {
					// expedited transfer
					if (map->length <= CANOPEN_SIZEOF(obj->type)) {
						ptr = obj->data_ptr;
					}
				}
				// copy data to destination
				if (ptr) {
					for (uint8_t j = 0;
							(j < map->length) &&
							(byte_count + j < CONFIG_CANOPEN_PDO_MAPPING_COUNT);
							j++) {
						ptr_bfr[byte_count + j] = ptr + j;
					}
				}
			}
		}

		byte_count += map->length;
	}
}


#endif
