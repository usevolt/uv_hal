
#ifndef UW_MESSAGES_H_
#define UW_MESSAGES_H_

#include "hal_can_controller.h"
#include "hal_reset_controller.h"


// ---------------------------       CANopen definitions       ---------------------------

// CANopen node ID
//	 should be unique integer between 1 - 127 in connected CAN-bus

#define KEYPAD_DEFAULT_NODE_ID			3
#define MSB_DEFAULT_NODE_ID				6
#define CSB_DEFAULT_NODE_ID				7
#define UW180S_MB_DEFAULT_NODE_ID		0xD


// ---------------------------       common messages       ---------------------------

// message indexes 0x1000 - 0x10FF reserved for common use

// tells the device status
#define UW_DEVICE_STATUS_INDEX			0x1000
#define UW_DEVICE_STATUS_SUB_INDEX		0
#define UW_DEVICE_STATUS_LENGTH			1
#define UW_DEVICE_STATUS_TYPE			OD_EXP_RO

// CANopen node ID
#define UW_NODE_ID_INDEX				0x1001
#define UW_NODE_ID_SUB_INDEX			0
#define UW_NODE_ID_LENGTH				1
#define UW_NODE_ID_TYPE					OD_EXP_WO

// holds all device errors
#define UW_RESET_SOURCE_INDEX			0x1002
#define UW_RESET_SOURCE_SUB_INDEX		1
#define UW_RESET_SOURCE_LENGTH			1
#define UW_RESET_SOURCE_TYPE			OD_EXP_RO

// tells device manufacturer (constant)
#define UW_MANUFACTURER_INDEX			0x1003
#define UW_MANUFACTURER_SUB_INDEX		0
#define UW_MANUFACTURER_LENGTH			2
#define UW_MANUFACTURER_VALUE			((int) 'U' + ((int) 'W' << 8))

// tells device's current version (constant)
#define UW_CURRENT_VERSION_INDEX		0x1004
#define UW_CURRENT_VERSION_SUB_INDEX	0
#define UW_CURRENT_VERSION_LENGTH		2

// tells device's minimum compatible version (constant)
#define UW_MINIMUM_VERSION_INDEX		0x1005
#define UW_MINIMUM_VERSION_SUB_INDEX	0
#define UW_MINIMUM_VERSION_LENGTH		2

// revert to default CANopen node ID
#define UW_RESET_NODE_ID_INDEX			0x1006
#define UW_RESET_NODE_ID_SUB_INDEX		0
#define UW_RESET_NODE_ID_LENGTH			1
#define UW_RESET_NODE_ID_TYPE			OD_EXP_WO

// revert to factory settings
#define UW_RESET_SETTINGS_INDEX			0x1007
#define UW_RESET_SETTINGS_SUB_INDEX		0
#define UW_RESET_SETTINGS_LENGTH		1
#define UW_RESET_SETTINGS_TYPE			OD_EXP_WO

// save all settings to flash
#define UW_SAVE_SETTINGS_INDEX			0x1008
#define UW_SAVE_SETTINGS_SUB_INDEX		0
#define UW_SAVE_SETTINGS_LENGTH			1
#define UW_SAVE_SETTINGS_TYPE			OD_EXP_WO


#endif /* UW_MESSAGES_H_ */
