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


/// @brief: Mask for CAN_ID field's node_id bits
#define CANOPEN_NODE_ID_MASK		0x7F


/// @brief: Returns true if this PDO message was enabled (bit 31 was not set)
#define PDO_IS_ENABLED(x)			(!(x->cob_id & (1 << 31)))

#if CONFIG_CANOPEN_LOG
#define DEBUG_LOG(...)	\
	printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)	//__VA_ARGS__
#endif


#define RXPDO(x)							CAT(RXPDO, INC(x))
#define TXPDO(x)							CAT(TXPDO, INC(x))
#define MAPPING(x)							CAT(_MAPPING, INC(x))

#define PDO_COB_ID(pdo)						(CAT(CONFIG_CANOPEN_, CAT(pdo, _ID)))
#define PDO_TTYPE(pdo)						(CAT(CONFIG_CANOPEN_, CAT(pdo, _TRANSMISSION_TYPE)))
#define PDO_EVENT_TIMER(pdo)				(CAT(CONFIG_CANOPEN_, CAT(pdo, _EVENT_TIMER)))
#define PDO_MAPPING_MINDEX(pdo, mapping)	(CAT(CONFIG_CANOPEN_, CAT(pdo, CAT(mapping, _MAIN_INDEX))))
#define PDO_MAPPING_SINDEX(pdo, mapping)	(CAT(CONFIG_CANOPEN_, CAT(pdo, CAT(mapping, _SUB_INDEX))))
#define PDO_MAPPING_LENGTH(pdo, mapping)	(CAT(CONFIG_CANOPEN_, CAT(pdo, CAT(mapping, _LEN))))



/// @brief: Initializes a pdo mapping.
/// @param pdo: The index number of the pdo to be initialized, starting from zero.
/// @param mapping: The index number of the mapping to be initialized, starting from zero.
#define TXPDO_MAPPING_INIT(mapping, pdo)	\
	me->obj_dict.com_params.txpdo_mappings[pdo][mapping].main_index = \
		PDO_MAPPING_MINDEX(TXPDO(pdo), MAPPING(mapping)); \
	me->obj_dict.com_params.txpdo_mappings[pdo][mapping].sub_index = \
		PDO_MAPPING_SINDEX(TXPDO(pdo), MAPPING(mapping)); \
	me->obj_dict.com_params.txpdo_mappings[pdo][mapping].length = \
		PDO_MAPPING_LENGTH(TXPDO(pdo), MAPPING(mapping));

#define RXPDO_MAPPING_INIT(mapping, pdo)	\
	me->obj_dict.com_params.rxpdo_mappings[pdo][mapping].main_index = \
		PDO_MAPPING_MINDEX(RXPDO(pdo), MAPPING(mapping)); \
	me->obj_dict.com_params.rxpdo_mappings[pdo][mapping].sub_index = \
		PDO_MAPPING_SINDEX(RXPDO(pdo), MAPPING(mapping)); \
	me->obj_dict.com_params.rxpdo_mappings[pdo][mapping].length = \
		PDO_MAPPING_LENGTH(RXPDO(pdo), MAPPING(mapping));


/// @brief: Initializes one TXPDO message. Works as a wrapper when
/// using REPEAT macro from utilities.h. Relies on uv_canopen_st* type variable 'me',
#define TXPDO_INIT(x)	\
		me->obj_dict.com_params.txpdo_coms[x].cob_id = PDO_COB_ID(TXPDO(x)); \
		me->obj_dict.com_params.txpdo_coms[x].transmission_type = PDO_TTYPE(TXPDO(x)); \
		me->obj_dict.com_params.txpdo_coms[x].event_timer = PDO_EVENT_TIMER(TXPDO(x)); \
		REPEAT_ARG2(CONFIG_CANOPEN_PDO_MAPPING_COUNT, TXPDO_MAPPING_INIT, x);

/// @brief: Initializes one RXPDO message. Works as a wrapper when
/// using REPEAT macro from utilities.h. Relies on uw_canopen_st* type variable 'me'.
#define RXPDO_INIT(x) 	\
		me->obj_dict.com_params.rxpdo_coms[x].cob_id = PDO_COB_ID(RXPDO(x)); \
		me->obj_dict.com_params.rxpdo_coms[x].transmission_type = PDO_TTYPE(RXPDO(x)); \
		REPEAT_ARG2(CONFIG_CANOPEN_PDO_MAPPING_COUNT, RXPDO_MAPPING_INIT, x);


/// @brief: Configures the HW CAN message object to receive this RXPDO
#if CONFIG_TARGET_LPC11C14
#define RXPDO_CONFIG_MESSAGE_OBJ(x) \
		uv_can_config_rx_message(this->can_channel, this->obj_dict.com_params.rxpdo_coms[x].cob_id, \
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
#elif CONFIG_TARGET_LPC1785
#define RXPDO_CONFIG_MESSAGE_OBJ(x) \
		uv_can_config_rx_message(this->can_channel, this->obj_dict.com_params.rxpdo_coms[x].cob_id\
			, CAN_11_BIT_ID);
#endif

/// @brief: Clear's the x'th predefined error to zero. Useful for initializing the error register
#define PREDEFINED_ERROR_CLEAR(x)	\
		this->obj_dict.com_params.predefined_errors[x] = 0;\



// A C++ style this-pointer definition
#define this 	((uv_canopen_st*)me)


/// @brief: Executes the sdo command to either read or write the object dictionary.
/// Modifies the object dictionary as necessary, sends the answer message and executes
/// the callback function from object dictionary entry.
/// Possible errors are handled as well and the request message struct error field is updated
/// correspondly.
static void canopen_parse_sdo(uv_canopen_st *me, uv_can_message_st* req);

/// @brief: Executes RXPDO command.
/// Check's which objects are mapped to the specific PDO and assigns their values accordingly.
static inline void parse_rxpdo(uv_canopen_st *me, uv_can_message_st* msg);

/// @brief: Parses the received NMT message and does the actions necessary
static inline void parse_nmt_message(uv_canopen_st *me, uv_can_message_st* msg);

#define NODE_ID					(this->obj_dict.com_params.node_id)
#define IS_ARRAY(x)				(x & CANOPEN_ARRAY_MASK)
//#define ARRAY_ELEMENT_SIZE(x)	(x & (~CANOPEN_ARRAY_MASK))
#define GET_OBJECT_SIZE(obj_ptr)	(CANOPEN_NUMBER_MASK & obj_ptr->type)

#define MSG_NODE_ID(msg)		((msg)->id & CANOPEN_NODE_ID_MASK)
#define MSG_TYPE(msg)			((msg)->id & (~CANOPEN_NODE_ID_MASK))



/// @brief: Searches the user application portion of the object dictionary and
/// returns a pointer to found object. If object couldn't be found, returns NULL.
static const uv_canopen_object_st *find_object(uv_canopen_st *me, uint16_t index, uint8_t sub_index) {
	unsigned int i;
	for (i = 0; i < this->obj_dict.app_parameters_length; i++) {
		if (this->obj_dict.app_parameters[i].main_index == index) {
			// object is of array type and sub index is dont_care
			if (IS_ARRAY(this->obj_dict.app_parameters[i].type)) {
				return &this->obj_dict.app_parameters[i];
			}
			else {
				if (this->obj_dict.app_parameters[i].sub_index == sub_index) {
					return &this->obj_dict.app_parameters[i];
				}
			}
		}
	}
	return NULL;
}

static void array_write(const uv_canopen_object_st *obj, uint16_t index, unsigned int value) {
	if (index > obj->array_max_size) {
#if CONFIG_CANOPEN_LOG
	if (!obj) {
		printf("Tried to over index CANopen array object\n\r");
	}
#endif
		return;
	}
	memcpy(obj->data_ptr + GET_OBJECT_SIZE(obj) * index, &value, GET_OBJECT_SIZE(obj));
}

/// @brief: Can be used to index object which is of type array
static unsigned int array_read(const uv_canopen_object_st *obj, uint16_t index) {
	if (index > obj->array_max_size) {
#if CONFIG_CANOPEN_LOG
	if (!obj) {
		printf("Tried to over-index CAnopen array object %x. Max index %u, indexed with %u.\n\r",
				obj->main_index, obj->array_max_size, index);
	}
#endif
		return 0;
	}
	unsigned int value;
	memcpy(&value, obj->data_ptr + GET_OBJECT_SIZE(obj) * index, GET_OBJECT_SIZE(obj));
	return value;
}

static unsigned int get_object_value(const uv_canopen_object_st* obj) {
	unsigned int value;
	memcpy(&value, obj->data_ptr, CANOPEN_NUMBER_MASK & obj->type);
	return value;
}




uv_errors_e uv_canopen_init(uv_canopen_st *me,
		const uv_canopen_object_st *obj_dict,
		uint16_t obj_dict_length,
		uv_can_channels_e can_channel,
		int *heartbeat_delay,
		void (*sdo_write_callback)(uv_canopen_object_st* obj_dict_entry),
		void (*emcy_callback)(void *user_ptr, uv_canopen_emcy_msg_st *msg)) {

	this->obj_dict.app_parameters = obj_dict;
	this->obj_dict.app_parameters_length = obj_dict_length;
	this->state = CANOPEN_PREOPERATIONAL;
	this->sdo_write_callback = sdo_write_callback;
	this->emcy_callback = emcy_callback;
	this->can_channel = can_channel;
	this->heartbeat_delay = heartbeat_delay;
	uv_set_id((uint16_t) this->obj_dict.com_params.node_id);

	// configure receive messages for NMT, SDO and PDO messages for this node id and broadcast
#if CONFIG_TARGET_LPC11C14
	// NMT broadcasting
	uv_can_config_rx_message(this->can_channel, CANOPEN_NMT_ID,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// NMT node id
	uv_can_config_rx_message(this->can_channel, CANOPEN_NMT_ID + NODE_ID,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// SDO request broadcasting
	uv_can_config_rx_message(this->can_channel, CANOPEN_SDO_REQUEST_ID,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// SDO request node id
	uv_can_config_rx_message(this->can_channel, CANOPEN_SDO_REQUEST_ID + NODE_ID,
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
	// all other nodes' EMCY messages
	if (emcy_callback) {
		uv_can_config_rx_message(this->can_channel, CANOPEN_EMCY_ID, 0xFFFFFF00, CAN_11_BIT_ID);
	}
#elif CONFIG_TARGET_LPC1785
	// NMT broadcasting
	uv_can_config_rx_message(this->can_channel, CANOPEN_NMT_ID, CAN_11_BIT_ID);
	// NMT node id
	uv_can_config_rx_message(this->can_channel, CANOPEN_NMT_ID + NODE_ID, CAN_11_BIT_ID);
	// SDO request broadcasting
	uv_can_config_rx_message(this->can_channel, CANOPEN_SDO_REQUEST_ID, CAN_11_BIT_ID);
	// SDO request node id
	uv_can_config_rx_message(this->can_channel, CANOPEN_SDO_REQUEST_ID + NODE_ID, CAN_11_BIT_ID);
	// all other nodes' EMCY messages
	if (emcy_callback) {
		uv_can_config_rx_message_range(this->can_channel, CANOPEN_EMCY_ID, CANOPEN_EMCY_ID + 0xFF, CAN_11_BIT_ID);
	}
#endif
	// RXPDOs
	REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_CONFIG_MESSAGE_OBJ);

#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX

	uv_ring_buffer_init(&this->errors, this->obj_dict.com_params.predefined_errors,
			CONFIG_CANOPEN_PREDEFINED_ERROR_SIZE, sizeof(this->obj_dict.com_params.predefined_errors[1]));
	// clear the predefined error register
	REPEAT(CONFIG_CANOPEN_PREDEFINED_ERROR_SIZE,
			PREDEFINED_ERROR_CLEAR);
#endif

#if CONFIG_CANOPEN_ERROR_REGISTER_INDEX
	this->obj_dict.com_params.error_register = 0;
#endif

	DEBUG_LOG("canopen initialized with node id %x\n\r", NODE_ID);

#if CONFIG_CANOPEN_HEARTBEAT_INDEX
	uv_delay_init(this->obj_dict.com_params.heartbeat_time, this->heartbeat_delay);
#endif

	// send boot up message
	uv_can_message_st msg = {
			.id = CANOPEN_BOOTUP_ID + NODE_ID,
			.data_length = 1,
			.data_8bit[0] = (uint8_t) CANOPEN_BOOT_UP
	};
	uv_can_send_message(this->can_channel, &msg);

	return uv_err(ERR_NONE);
}

uv_errors_e uv_canopen_restore_defaults(uv_canopen_st *me,
		const uv_canopen_object_st *obj_dict,
		uint16_t obj_dict_length,
		uv_can_channels_e can_channel,
		int *heartbeat_delay,
		void (*sdo_write_callback)(uv_canopen_object_st* obj_dict_entry),
		void (*emcy_callback)(void *user_ptr, uv_canopen_emcy_msg_st *msg)) {
#if CONFIG_CANOPEN_DEVICE_TYPE_INDEX
	this->obj_dict.com_params.device_type = CONFIG_CANOPEN_DEVICE_TYPE;
#endif
#if CONFIG_CANOPEN_HEARTBEAT_INDEX
	this->obj_dict.com_params.heartbeat_time = CONFIG_CANOPEN_DEFAULT_HEARTBEAT_TIME;
#endif
#if CONFIG_CANOPEN_IDENTITY_INDEX
	this->obj_dict.com_params.identity.vendor_id = CONFIG_CANOPEN_VENDOR_ID;
	this->obj_dict.com_params.identity.revision_number = CONFIG_CANOPEN_REVISION_CODE;
	this->obj_dict.com_params.identity.product_code = CONFIG_CANOPEN_PRODUCT_CODE;
	// todo: Does reading serial corrupt stack?
//		uv_get_device_serial(this->obj_dict.com_params.identity.serial_number);

#endif
	this->obj_dict.com_params.node_id = CONFIG_CANOPEN_DEFAULT_NODE_ID;
#if CONFIG_CANOPEN_RESTORE_PARAMS_INDEX
	this->obj_dict.com_params.restore_params = 0;
#endif
#if CONFIG_CANOPEN_STORE_PARAMS_INDEX
	this->obj_dict.com_params.store_params = 0;
#endif

	// configure TXPDO's to their default values
	REPEAT(CONFIG_CANOPEN_TXPDO_COUNT, TXPDO_INIT);

	// configure RXPDO's to their default values
	REPEAT(CONFIG_CANOPEN_RXPDO_COUNT, RXPDO_INIT);


	// lastly initialize all volatile data
	uv_canopen_init(this, obj_dict, obj_dict_length,
			can_channel, heartbeat_delay, sdo_write_callback, emcy_callback);
	return ERR_NONE;
}




uv_errors_e uv_canopen_step(uv_canopen_st *me, unsigned int step_ms) {
	// in stopped state we do nothing
	if (this->state == CANOPEN_STOPPED) {
		return uv_err(ERR_NONE);
	}
	if (this->can_channel >= CAN_COUNT) {
		return uv_err(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_CANOPEN);
	}

	// RX message parsing
	uv_can_message_st msg;
	// returning an error means that the can rx buffer was empty and no messages have been received
	while (!uv_can_pop_message(this->can_channel, &msg)) {
		// process messages which are addressed to this node OR broadcasted
		if (NODE_ID == MSG_NODE_ID(&msg) || MSG_NODE_ID(&msg) == 0) {

			switch (MSG_TYPE(&msg)) {
			case CANOPEN_SDO_REQUEST_ID:
				// SDO msgs are always 8 bytes long
				if (msg.data_length < 8) {
					break;
				}
				// in stopped state SDO is disabled
				if (this->state == CANOPEN_STOPPED) {
					break;
				}
				canopen_parse_sdo(this, &msg);
				break;
			case CANOPEN_NMT_ID:
				parse_nmt_message(this, &msg);
				break;
			case CANOPEN_EMCY_ID:
				// in stopped state EMCY msg handling is disabled
				if (this->state == CANOPEN_STOPPED) {
					break;
				}
				if (this->emcy_callback) {
					// EMCY msg's length should always be 8 bits
					if (msg.data_length == 8) {
						this->temp_emcy.node_id = msg.id & CANOPEN_NODE_ID_MASK;
						this->temp_emcy.error_code = msg.data_16bit[0];
						this->temp_emcy.data_as_16_bit[0] = msg.data_16bit[1];
						this->temp_emcy.data_as_16_bit[1] = msg.data_16bit[2];
						this->temp_emcy.data_as_16_bit[2] = msg.data_16bit[3];
						this->emcy_callback(__uv_get_user_ptr(), &this->temp_emcy);
					}
				}
				break;
			default:
				break;
			}
		}

		// regardless of what kind of msg was received, check if it was mapped to any RXPDO
		// (RXPDO's can be mapped with any kind of msg)
		if (this->state == CANOPEN_OPERATIONAL) {
			parse_rxpdo(this, &msg);
		}
	}



#if CONFIG_CANOPEN_HEARTBEAT_INDEX
	// send heartbeat message
	if (uv_delay(step_ms, this->heartbeat_delay)) {
		// send heartbeat msg
		msg = (uv_can_message_st) {
				.id = CANOPEN_HEARTBEAT_ID + NODE_ID,
				.data_length = 1,
				.data_8bit[0] = this->state
		};

		if (uv_can_get_error_state(this->can_channel) == CAN_ERROR_ACTIVE) {
			uv_can_send_message(this->can_channel, &msg);
		}

		// start the delay again
		uv_delay_init(this->obj_dict.com_params.heartbeat_time, this->heartbeat_delay);
	}
#endif

	// from this point forward is executed only in operational state
	if (this->state != CANOPEN_OPERATIONAL) {
		return uv_err(ERR_NONE);
	}

	// TXPDO handling
	uint8_t i;
	for (i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {

		uv_txpdo_com_parameter_st* com = &this->obj_dict.com_params.txpdo_coms[i];

		// if this PDO is enabled, fetch the data from mapping object and send the message
		if (PDO_IS_ENABLED(com)) {
			/* PDO communication */

			// fetch the message COB-ID
			msg.id = com->cob_id;

			// check the transmission type
			// Currently only asynchronous transmission is implemented
			if (com->transmission_type != CANOPEN_PDO_TRANSMISSION_ASYNC) {

				DEBUG_LOG("TXPDO transmission type was not asynchronous. Currently only async transmissions"
						  " are supported.\n\r");
				continue;
			}

			// for asynchronous transmissions check the time delay
			// reserved variable is used as the time counter variable
			com->_reserved += step_ms;
			if (com->_reserved >= com->event_timer) {
				com->_reserved = 0;
			}
			else {
				continue;
			}


			/* PDO mappings */
			uint8_t j;
			uint8_t byte_count = 0;
			const uv_canopen_object_st *map_obj;
			// cycle trough all mapping parameters and fetch the PDO data
			for (j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {

				uv_pdo_mapping_parameter_st *map = &this->obj_dict.com_params.txpdo_mappings[i][j];
				// if all are zeroes, all data has been mapped to the PDO
				if (map->main_index == 0 && map->sub_index == 0) {
					break;
				}
				// otherwise map some data to the message
				map_obj = find_object(this, map->main_index, map->sub_index);
				if (!map_obj) {
					DEBUG_LOG("TXPDO mapping parameter 0x%x points to an object 0x%x 0x%x which doens't exist\n\r",
								CONFIG_CANOPEN_TXPDO_MAP_INDEX + i, map->main_index, map->sub_index);
					return uv_err(ERR_CANOPEN_MAPPED_OBJECT_NOT_FOUND);
				}
				unsigned int value;
				if (IS_ARRAY(map_obj->type)) {
					value = array_read(map_obj, map->sub_index - 1);
				}
				else {
					value = get_object_value(map_obj);
				}
				uint8_t k;
				// 8 bytes is the maximum length
				if (byte_count + map->length > 8) {
					DEBUG_LOG("Mapped %u bytes to TXPDO %u, maximum allowed is 8.\n\r",
								byte_count + map->length, i);
					map->length = 0;
				}
				for (k = 0; k < map->length; k++) {
					msg.data_8bit[byte_count++] = ((uint8_t*)(&value))[k];
				}

			}
			msg.data_length = byte_count;
			uv_can_send_message(this->can_channel, &msg);
		}
	}


	return uv_err(ERR_NONE);
}





uv_errors_e uv_canopen_emcy_send(uv_canopen_st *me, uv_canopen_emcy_msg_st *msg) {
	if (this->state == CANOPEN_STOPPED) {
		return uv_err(ERR_CANOPEN_STACK_IN_STOPPED_STATE | HAL_MODULE_CANOPEN);
	}
	uv_can_message_st canmsg = {
			.data_length = 8,
			.id = CANOPEN_EMCY_ID + msg->node_id,
			.data_16bit[0] = msg->error_code,
			.data_16bit[1] = msg->data_as_16_bit[0],
			.data_16bit[2] = msg->data_as_16_bit[1],
			.data_16bit[3] = msg->data_as_16_bit[2]
	};
	uv_errors_e err = ERR_NONE;

	uv_err_check(uv_can_send_message(this->can_channel, &canmsg)) {
		DEBUG_LOG("EMCY message sending failed. CAN TX buffer full?\n\r");
		err = ERR_BUFFER_OVERFLOW | HAL_MODULE_CANOPEN;
	}
#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
	uv_err_check (uv_ring_buffer_push(&this->errors, &msg->error_code)) {
		// pushing failed, array was full. Discard the oldest error and then put new one
		uv_ring_buffer_pop(&this->errors, NULL);
		uv_ring_buffer_push(&this->errors, &msg->error_code);
		err = ERR_BUFFER_OVERFLOW | HAL_MODULE_CANOPEN;
	}
#endif
	return uv_err(err);
}


#define SDO_MINDEX(msg)			(msg->data_8bit[1] + (msg->data_8bit[2] << 8))
#define SDO_SINDEX(msg)			(msg->data_8bit[3])
#define SDO_CMD(msg)			(msg->data_8bit[0])
#define SDO_DATA(msg, index)	(msg->data_8bit[4 + index])

/// @brief: Sends a SDO error response
static void sdo_send_error(uv_canopen_st *me, uv_can_message_st *msg, uv_sdo_error_codes_e error) {
	uv_can_message_st reply = {
			.id = CANOPEN_SDO_ERROR_ID + NODE_ID,
			.data_length = 8,
			.data_8bit[0] = CANOPEN_SDO_CMD_ERROR,
			.data_8bit[1] =	SDO_MINDEX(msg),
			.data_8bit[2] = SDO_MINDEX(msg) >> 8,
			.data_8bit[3] = SDO_SINDEX(msg),
			.data_8bit[4] = (uint8_t) error,
			.data_8bit[5] = (uint8_t) (error >> 8),
			.data_8bit[6] = (uint8_t) (error >> 16),
			.data_8bit[7] = (uint8_t) (error >> 24)
	};
	DEBUG_LOG("SDO returned an error: %x %x %x %x %x %x %x %x\n\r",
			reply.data_8bit[0], reply.data_8bit[1], reply.data_8bit[2], reply.data_8bit[3],
			reply.data_8bit[4], reply.data_8bit[5], reply.data_8bit[6], reply.data_8bit[7]);
	uv_can_send_message(this->can_channel, &reply);
}

static void sdo_send_response(uv_canopen_st *me, uv_can_message_st *msg, uv_canopen_sdo_commands_e cmd,
		const uint8_t *data, uint8_t data_length) {
	uv_can_message_st reply = {
			.id = CANOPEN_SDO_RESPONSE_ID + NODE_ID,
			.data_length = 8,
			.data_8bit[0] = cmd,
			.data_8bit[1] =	SDO_MINDEX(msg),
			.data_8bit[2] = SDO_MINDEX(msg) >> 8,
			.data_8bit[3] = SDO_SINDEX(msg)
	};
	uint8_t i = 0;
	if (data) {
		for (i = 0; i < data_length; i++) {
			reply.data_8bit[4 + i] = data[i];
		}
	}
	while (i < 4) {
		reply.data_8bit[4 + i] = 0;
		i++;
	}

	DEBUG_LOG("Sent response with data: %x %x %x %x %x %x %x %x\n\r", reply.data_8bit[0],
			reply.data_8bit[1], reply.data_8bit[2], reply.data_8bit[3],
			reply.data_8bit[4], reply.data_8bit[5], reply.data_8bit[6],
			reply.data_8bit[7]);

	uv_can_send_message(this->can_channel, &reply);
}


static void canopen_parse_sdo(uv_canopen_st *me, uv_can_message_st *msg) {
	const uv_canopen_object_st *obj = find_object(this, SDO_MINDEX(msg), SDO_SINDEX(msg));
	DEBUG_LOG("SDO received: %x, %x\n\r", SDO_MINDEX(msg), SDO_SINDEX(msg));

	if (!obj) {
		DEBUG_LOG("SDO object doesn't exist: %x, %x\n\r", SDO_MINDEX(msg), SDO_SINDEX(msg));
		sdo_send_error(this, msg, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
		return;
	}

	// SDO READ requests...
	if (SDO_CMD(msg) == CANOPEN_SDO_CMD_READ) {
		DEBUG_LOG("SDO read object %x command received\n\r", SDO_MINDEX(msg));

		// error with read permissions
		if (!(obj->permissions & CANOPEN_RO)) {
			DEBUG_LOG("SDO error: object %x is write only\n\r", SDO_MINDEX(msg));
			sdo_send_error(this, msg, CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
		}
		// for array type objects
		else if (IS_ARRAY(obj->type)) {
			// indexing the array
			if (SDO_SINDEX(msg)) {
				if (SDO_SINDEX(msg) <= obj->array_max_size) {
					uint32_t value = array_read(obj, SDO_SINDEX(msg) - 1);
					sdo_send_response(this, msg, CANOPEN_SDO_CMD_READ_RESPONSE_BYTES, (uint8_t *) &value, 4);
				}
				else {
					DEBUG_LOG("SDO error: over indexing object %x with sub-index %x\n\r",
								SDO_MINDEX(msg), SDO_SINDEX(msg));
					sdo_send_error(this, msg, CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
				}
			}
			// index 0 is the array max size
			else {
				sdo_send_response(this, msg, CANOPEN_SDO_CMD_READ_RESPONSE_BYTES,
						&obj->array_max_size, 1);
			}
		}
		// normal data
		else {
			// obj->type should straight contain the number of bytes
			sdo_send_response(this, msg, CANOPEN_SDO_CMD_READ_RESPONSE_BYTES, obj->data_ptr, obj->type);
		}
	}

	/*
	 * all SDO write requests...
	 */
	else if (SDO_CMD(msg) == CANOPEN_SDO_CMD_WRITE_1_BYTE ||
			SDO_CMD(msg) == CANOPEN_SDO_CMD_WRITE_2_BYTES ||
			SDO_CMD(msg) == CANOPEN_SDO_CMD_WRITE_4_BYTES ||
			SDO_CMD(msg) == CANOPEN_SDO_CMD_WRITE_BYTES) {

		if (obj->permissions & CANOPEN_WO) {
			DEBUG_LOG("SDO write to object %x\n\r", SDO_MINDEX(msg));
			// Predefined error register clearing
#if CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX
			if (SDO_MINDEX(msg) == CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX &&
					SDO_SINDEX(msg) == 0) {
				// clear all errors
				uint8_t i;
				uv_ring_buffer_clear(&this->errors);
				for (i = 0; i < obj->array_max_size * GET_OBJECT_SIZE(obj); i++) {
					obj->data_ptr[i] = 0;
				}
				DEBUG_LOG("SDO: cleared errors from predefined error field %x\n\r",
							CONFIG_CANOPEN_PREDEFINED_ERROR_FIELD_INDEX);
				sdo_send_response(this, msg, CANOPEN_SDO_CMD_WRITE_RESPONSE, &i, 1);
				return;

			}
#endif
			// writing to arrays
			if (IS_ARRAY(obj->type)) {
				// writing to array sub index 0 as well as over indexing is not permitted
				if (SDO_SINDEX(msg) == 0 || SDO_SINDEX(msg) - 1 >= obj->array_max_size) {
					sdo_send_error(this, msg, CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					DEBUG_LOG("SDO error: Unsupported access to array type object %x\n\r", SDO_MINDEX(msg));
					return;
				}
				array_write(obj, SDO_SINDEX(msg) - 1, msg->data_32bit[1]);
			}
			else {
				// writing to all other objects
				uint8_t i;
				for (i = 0; i < obj->type; i++) {
					obj->data_ptr[i] = msg->data_8bit[4 + i];
				}
				i = 0;
			}
			sdo_send_response(this, msg, CANOPEN_SDO_CMD_WRITE_RESPONSE, NULL, 0);

			// parameter saving and loading
#if CONFIG_CANOPEN_STORE_PARAMS_INDEX
			if (SDO_MINDEX(msg) == CONFIG_CANOPEN_STORE_PARAMS_INDEX &&
					SDO_SINDEX(msg) == 1) {
				if (obj->data_ptr[0] == 's' &&
						obj->data_ptr[1] == 'a' &&
						obj->data_ptr[2] == 'v' &&
						obj->data_ptr[3] == 'e') {
					DEBUG_LOG("Saving settings to flash memory\n\r");
					*((unsigned int*)obj->data_ptr) = 0;
#if !defined(CONFIG_NON_VOLATILE_MEMORY)
#warning "CANopen store parameters object needs CONFIG_MEMORY defined as 1"
#else
					__uv_save_previous_non_volatile_data();
#endif

				}
			}
#endif
#if CONFIG_CANOPEN_RESTORE_PARAMS_INDEX
			if (SDO_MINDEX(msg) == CONFIG_CANOPEN_RESTORE_PARAMS_INDEX &&
					SDO_SINDEX(msg) == 1) {
				if (obj->data_ptr[0] == 'l' &&
						obj->data_ptr[1] == 'o' &&
						obj->data_ptr[2] == 'a' &&
						obj->data_ptr[3] == 'd') {
					DEBUG_LOG("Loading factory settings from flash memory\n\r");
#if !defined(CONFIG_NON_VOLATILE_MEMORY)
#warning "CANopen restore parameters object needs CONFIG_MEMORY defined as 1"
#else
					*((unsigned int*)obj->data_ptr) = 0;
					__uv_clear_previous_non_volatile_data();
#endif

				}

			}
#endif
		}
		else {
			DEBUG_LOG("SDO error: object %x is write only\n\r", SDO_MINDEX(msg));
			sdo_send_error(this, msg, CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
		}
	}
	else {
		DEBUG_LOG("SDO error: invalid command %x\n\r", SDO_CMD(msg));
		sdo_send_error(this, msg, CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND);
	}

}

static inline void parse_rxpdo(uv_canopen_st *me, uv_can_message_st* msg) {
	uint8_t i;
	// cycle trough all RXPDO communication parameters
	for (i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {

		if (this->obj_dict.com_params.rxpdo_coms[i].cob_id == msg->id) {
			uint8_t j;
			uint8_t bytes = 0;
			// cycle trough all mapping entries used
			for (j = 0; j < CONFIG_CANOPEN_PDO_MAPPING_COUNT; j++) {
				if (this->obj_dict.com_params.rxpdo_mappings[i][j].main_index) {
					// object index which will be written
					uint16_t index = this->obj_dict.com_params.rxpdo_mappings[i][j].main_index;
					// object sub-index which will be written
					uint8_t subindex = this->obj_dict.com_params.rxpdo_mappings[i][j].sub_index;
					// the length of the write operation in bytes
					uint8_t byte_length = this->obj_dict.com_params.rxpdo_mappings[i][j].length;
					const uv_canopen_object_st *trg = find_object(this, index, subindex);
					if (!trg) {
						DEBUG_LOG("RXPDO error: object with index %x and subindex %x not found\n\r",
									index, subindex);
						break;
					}
					if (bytes + byte_length > 8) {
						DEBUG_LOG("RXPDO error: PDO mapping length exceeds 8 bytes\n\r");
					}

					if (msg->data_length < bytes + byte_length) {
						DEBUG_LOG("RXPDO error: Message length was shorter than mapped length\n\r");
						break;
					}
					// write bits to the object
					memcpy(trg->data_ptr, &msg->data_8bit[bytes], byte_length);
					bytes += byte_length;

				}
				else {
					// if main index was zero, PDO mappings have been ended and we return.
					break;
				}
			}
			DEBUG_LOG("RXPDO with id %x written\n\r", msg->id);
			continue;
		}
	}

}


#define nmt_command(msg)		(msg->data_8bit[0])
#define nmt_target_node(msg)	(msg->data_8bit[1])
static inline void parse_nmt_message(uv_canopen_st *me, uv_can_message_st* msg) {
	// check if the NMT message was broadcasted or dedicated to this node
	if (nmt_target_node(msg) == NODE_ID || !nmt_target_node(msg)) {
		switch (nmt_command(msg)) {
		case CANOPEN_NMT_START_NODE:
			DEBUG_LOG("NMT start\n\r");
			uv_canopen_set_state(this, CANOPEN_OPERATIONAL);
			break;
		case CANOPEN_NMT_STOP_NODE:
			DEBUG_LOG("NMT stop\n\r");
			uv_canopen_set_state(this, CANOPEN_STOPPED);
			break;
		case CANOPEN_NMT_SET_PREOPERATIONAL:
			DEBUG_LOG("NMT preoperational\n\r");
			uv_canopen_set_state(this, CANOPEN_PREOPERATIONAL);
			break;
		case CANOPEN_NMT_RESET_NODE:
			DEBUG_LOG("NMT reset\n\r");
			uv_system_reset(false);
			break;
		case CANOPEN_NMT_RESET_COM:
			DEBUG_LOG("NMT reset communication\n\r");
			uv_canopen_init(this, this->obj_dict.app_parameters, this->obj_dict.app_parameters_length,
					this->can_channel, this->heartbeat_delay, this->sdo_write_callback, this->emcy_callback);
			break;
		default:
			break;
		}
	}
}

uv_errors_e uv_canopen_send_sdo(uv_canopen_st *me,
		uv_canopen_sdo_message_st *sdo,
		uint8_t node_id) {

	// to CAN bus bytes are sent as 0, 1, 2, 3 (although CAN should be big-endian)
	// in CANopen data is little-endian, so for example main-index of 0x1017 is sent as 17 10 bytes
	uv_can_message_st msg;
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	msg.data_8bit[0] = sdo->request;
	msg.data_8bit[1] = (uint8_t) sdo->main_index;
	msg.data_8bit[2] = (uint8_t) (sdo->main_index >> 8);
	msg.data_8bit[3] = sdo->sub_index;
	msg.data_32bit[1] = sdo->data_32bit;
	msg.data_length = 8;

	uv_can_send_message(this->can_channel, &msg);

	return uv_err(ERR_NONE);
}



#endif
