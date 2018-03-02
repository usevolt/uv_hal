/*
 * canopen_pdo.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_pdo.h"
#include "string.h"
#include "uv_rtos.h"
#include CONFIG_MAIN_H


#if CONFIG_CANOPEN

#define this 								(&_canopen)
#define thisnv								(&CONFIG_NON_VOLATILE_START.canopen_data)

#define NODEID								(CONFIG_NON_VOLATILE_START.id)


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




/// @brief: Configures the HW CAN message object to receive this RXPDO
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549 || CONFIG_TARGET_LINUX
#define RXPDO_CONFIG_MESSAGE_OBJ(x) \
		uv_can_config_rx_message(this->can_channel, \
			this->obj_dict.com_params.rxpdo_coms[x].cob_id, \
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
#elif CONFIG_TARGET_LPC1785
#define RXPDO_CONFIG_MESSAGE_OBJ(x) \
		uv_can_config_rx_message(this->can_channel, \
			this->obj_dict.com_params.rxpdo_coms[x].cob_id\
			, CAN_11_BIT_ID);
#else
#error "Not implemented"
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
			uv_delay_init((uv_delay_st*) &this->txpdo_time[i], com->event_timer);
			this->inhibit_time[i] = 0;
		}
		else {
			// something went wrong
		}
	}
	for (int i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		if ((obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_RXPDO_COM_INDEX + i, 0))) {
#if CONFIG_TARGET_LPC1785
			uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
					((canopen_rxpdo_com_parameter_st*) obj->data_ptr)->cob_id, CAN_STD);
#else
			uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
					((canopen_rxpdo_com_parameter_st*) obj->data_ptr)->cob_id, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
		}
		else {
			// something went wrong, PDO communication parameter couldn't be found
		}
	}

}



void _uv_canopen_pdo_reset() {
	for (int i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
		thisnv->txpdo_coms[i] = txpdo_com_defs[i];
		thisnv->txpdo_maps[i] = txpdo_map_defs[i];
	}
	for (int i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
		thisnv->rxpdo_coms[i] = rxpdo_com_defs[i];
		thisnv->rxpdo_maps[i] = rxpdo_map_defs[i];
	}
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
			if (this->inhibit_time[i] > 0) {
				this->inhibit_time[i] -= step_ms;
			}

			// if PDO is not enabled, skip it
			if (IS_ENABLED(com)) {

				// check if event timer in this PDO triggers and the inhibit time has passed
				// since last transmission
				uv_delay((uv_delay_st*) &this->txpdo_time[i], step_ms);
				if ((uv_delay_has_ended(&this->txpdo_time[i])) &&
						(this->inhibit_time[i] <= 0)) {
					// initialize delay again
					uv_delay_init((uv_delay_st*) &this->txpdo_time[i], com->event_timer);
					this->inhibit_time[i] = com->inhibit_time;

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
							// when mapping with length of 0 is found or too many objects are mapped,
							// stop the mapping
							if (!mapping->length || byte_count + mapping->length > 8) {
								br = true;
							}
							// otherwise map some data to the message bytes
							if (!(obj = _uv_canopen_obj_dict_get(mapping->main_index, mapping->sub_index))) {
								// mapped object not found
								_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, mapping->main_index,
										mapping->sub_index, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
								br = true;
							}
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

							if (br) {
								break;
							}

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
			if (!(obj = _uv_canopen_obj_dict_get(CONFIG_CANOPEN_RXPDO_MAP_INDEX + i, 0))) {
				// something weird happened,
				// mapping parameter not found for this RXPDO object
				_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID,
						CONFIG_CANOPEN_RXPDO_MAP_INDEX + i, 0,
						CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
				valid = false;
			}
		}

		if (valid) {
			canopen_pdo_mapping_parameter_st *mapping_par = obj->data_ptr;

			// matching RXPDO found. Cycle trough it's mapping parameters
			for (uint8_t j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {

				canopen_pdo_mapping_st *mapping = &(*mapping_par).mappings[j];

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
					uv_delay_trigger(&this->txpdo_time[i]);
					break;
				}
			}
		}
	}

}


#endif
