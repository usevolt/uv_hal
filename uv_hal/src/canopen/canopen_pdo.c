/*
 * canopen_pdo.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_pdo.h"


#if CONFIG_CANOPEN

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
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549
#define RXPDO_CONFIG_MESSAGE_OBJ(x) \
		uv_can_config_rx_message(this->can_channel, this->obj_dict.com_params.rxpdo_coms[x].cob_id, \
			CAN_ID_MASK_DEFAULT, CAN_11_BIT_ID);
#elif CONFIG_TARGET_LPC1785
#define RXPDO_CONFIG_MESSAGE_OBJ(x) \
		uv_can_config_rx_message(this->can_channel, this->obj_dict.com_params.rxpdo_coms[x].cob_id\
			, CAN_11_BIT_ID);
#else
#error "Not implemented"
#endif


/// @brief: Returns true if this PDO message was enabled (bit 31 was not set)
#define PDO_IS_ENABLED(x)			(!(x->cob_id & (1 << 31)))


#endif
