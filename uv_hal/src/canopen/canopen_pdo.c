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
		const canopen_object_st *obj;
		if ((obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_TXPDO_COM_INDEX + i, 0)) ||
				!uv_canopen_is_array(obj)) {
			canopen_txpdo_com_parameter_st *com;
			com = obj->data_ptr;

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

					uint8_t byte_count = 0;
					uv_can_message_st msg;
					memset(msg.data_8bit, 0, 8);

					if (!(obj = _uv_canopen_obj_dict_get(
							CONFIG_CANOPEN_TXPDO_MAP_INDEX + i, 0))) {
						// something weird happened,
						// mapping parameter not found for this TXPDO object
						_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID,
								CONFIG_CANOPEN_TXPDO_MAP_INDEX + i, 0,
								CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
					}
					// TXPDO mapping parameter found
					else {
						canopen_pdo_mapping_parameter_st *mapping_par = obj->data_ptr;

						for (uint8_t j = 0; (j < CONFIG_CANOPEN_PDO_MAPPING_COUNT) && (byte_count != 8); j++) {

							canopen_pdo_mapping_st *mapping = &(*mapping_par).mappings[j];

							bool br = false;
							bool cont = false;
							// when mapping with length of 0 is found or too many objects are mapped,
							// stop the mapping
							if (!mapping->length || byte_count + mapping->length > 8) {
								br = true;
							}
							// if main index was set to zero, skip this mapping and continue
							// on the second one.
							else if (mapping->main_index == 0) {
								cont = true;
							}
							// otherwise map some data to the message bytes
							else if (!(obj = _uv_canopen_obj_dict_get(mapping->main_index, mapping->sub_index))) {
								// mapped object not found
								_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
										mapping->sub_index, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
								br = true;
							}
							else {
								// mapped object has to be readable
								if (!(obj->permissions | CANOPEN_RO)) {
									_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
											mapping->sub_index, CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
									byte_count += mapping->length;
									br = true;
								}
								// mapping length cannot be greater than mapped objects length
								if (mapping->length > uv_canopen_get_object_data_size(obj)) {
									_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
											mapping->sub_index, CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
									br = true;
								}
							}

							if (br) {
								break;
							}

							if (!cont) {
								// for array objects mapping is a little bit complicated
								if (uv_canopen_is_array(obj)) {
									if (!mapping->sub_index) {
										memcpy(&msg.data_8bit[byte_count],
												&obj->array_max_size, sizeof(obj->array_max_size));
									}
									else if (mapping->sub_index > obj->array_max_size) {
										_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
												mapping->sub_index, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
									}
									else {
										memcpy(&msg.data_8bit[byte_count],
												&((uint8_t*) obj->data_ptr)[uv_canopen_get_object_data_size(obj) *
															  (mapping->sub_index - 1)],
												mapping->length);
									}
								}
								// all other types are easy
								else {
									memcpy(&msg.data_8bit[byte_count], obj->data_ptr, mapping->length);
								}
							}
							// increase byte mapping counter
							byte_count += mapping->length;
						}
					}

					// send the txpdo if any data got mapped
					if (byte_count) {
						msg.type = CAN_STD;
						msg.data_length = byte_count;
						msg.id = com->cob_id;
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
					}
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
							obj->def_ptr != NULL &&
							obj->data_ptr != NULL) {
						memcpy(obj->data_ptr, obj->def_ptr, CANOPEN_SIZEOF(obj->type));
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
	if (msg->type != CAN_STD) {
		valid = false;
	}
	if (!valid) { return; }

	/*
	 * RXPDO
	 */
	const canopen_object_st *obj;

	for (uint8_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		valid = true;

		// check trough all RXPDO's and check their cob_id's
		if (!(obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_RXPDO_COM_INDEX + i, 0)) ||
				!uv_canopen_is_array(obj)) {
			// something strange happened, didn't find RXPDO communication parameter
			valid = false;
		}
		canopen_rxpdo_com_parameter_st *com;
		uint8_t byte_count = 0;

		if (valid) {
			com = obj->data_ptr;
			if (com->cob_id != msg->id) {
				valid = false;
			}

		}

		if (valid) {
			// update the def_delay and prevent setting the parameters
			// to their default values
			uv_delay_init(&this->rxpdo[i].def_delay, CONFIG_CANOPEN_RXPDO_TIMEOUT_MS);

			// matching RXPDO found. Cycle trough it's mapping parameters
			for (uint8_t j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {

				canopen_pdo_mapping_st *mapping = &thisnv->rxpdo_maps[i].mappings[j];

				valid = true;

				// prevent over indexing
				if (byte_count + mapping->length > msg->data_length) {
					valid = false;
				}

				// stop copying the data if mapping had zero length
				if (!mapping->length) {
					valid = false;
				}


				// if main index & sub index were zero, jump to next mapping
				if (!(mapping->main_index) && !(mapping->sub_index)) {
					byte_count += mapping->length;
					valid = false;
				}
				// otherwise try to fetch the mapped object
				else if (!(obj = _uv_canopen_obj_dict_get(mapping->main_index, mapping->sub_index))) {
					_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
							mapping->sub_index, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
					byte_count += mapping->length;
					valid = false;
				}
				else {

				}

				if (valid) {

					// cannot write to an object which is not writable
					if (!(obj->permissions | CANOPEN_WO)) {
						_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
								mapping->sub_index, CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
						byte_count += mapping->length;
						valid = false;
					}

					// mapped object found. Prevent over indexing the mapped object
					if (mapping->length > uv_canopen_get_object_data_size(obj)) {
						_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
								mapping->sub_index, CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
						byte_count += mapping->length;
						valid = false;
					}

					if (valid) {
						// copy the actual data if the object was array
						if (uv_canopen_is_array(obj)) {
							if (!mapping->sub_index) {
								// writing to array sub index 0 is illegal since subindex 0
								// determines the array max length
								_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
										mapping->sub_index, CANOPEN_SDO_ERROR_OBJECT_CANNOT_BE_MAPPED_TO_PDO);
							}
							else if (mapping->sub_index > obj->array_max_size) {
								_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
										mapping->sub_index, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
							}
							else {
								memcpy(&((uint8_t*) obj->data_ptr)
										[uv_canopen_get_object_data_size(obj) * (mapping->sub_index - 1)],
										&msg->data_8bit[byte_count], mapping->length);
							}
						}
						// copy the actual data to other types of objects
						else {
							memcpy(obj->data_ptr, &msg->data_8bit[byte_count], mapping->length);
						}

						// increase byte counter
						byte_count += mapping->length;
					}
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
	if (txpdo >= CONFIG_CANOPEN_TXPDO_COUNT) {
		txpdo = CONFIG_CANOPEN_TXPDO_COUNT - 1;
	}
	return &thisnv->txpdo_coms[txpdo];
}


#endif
