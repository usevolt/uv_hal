/*
 * uv_can.h
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#ifndef UW_CAN_H_
#define UW_CAN_H_

#include "uv_hal_config.h"
#include "uv_errors.h"
#include "uv_gpio.h"
#include <stdint.h>
#include <stdbool.h>
/// @file: CAN implementation on top of the HAL layer
/// Provides the basic CAN interface.
/// Note that this module takes full ownership of the CAN HAL layer interface.
/// For example on CONFIG_TARGET_LPC11C14, this module may use all 32 CAN message objects
/// as it wants.


#if CONFIG_CAN


#if !defined(CONFIG_CAN_LOG)
#error "CONFIG_CAN_LOG not defined"
#endif
#if !CONFIG_CAN_LOG && !defined(CONFIG_CAN_ERROR_LOG)
#error "CONFIG_CAN_ERROR_LOG not defined. When CONFIG_CAN_LOG is set as 0,\
 CONFIG_CAN_ERROR_LOG should be definded as 0 or 1, to disable or enable error logging."
#endif
#if CONFIG_TARGET_LPC11C14
#if CONFIG_CAN2
#error "Hardware doesn't support CAN2 module. Set CONFIG_CAN2 to 0."
#endif
#if CONFIG_CAN1
#if !defined(CONFIG_CAN1_BAUDRATE)
#error "CONFIG_CAN1_BAUDRATE not defined. It should define the baudrate used for CAN1 module."
#endif
#if !defined(CONFIG_CAN1_RX_BUFFER_SIZE)
#error "CONFIG_CAN1_RX_BUFFER_SIZE not defined. It should define the buffer size used for receiving messages."
#endif
#if !defined(CONFIG_CAN1_TX_BUFFER_SIZE)
#error "CONFIG_CAN1_TX_BUFFER_SIZE not defined. It should define the buffer size used for transmit messages."
#endif
#endif
#elif CONFIG_TARGET_LPC1785
#if CONFIG_CAN1
#if !defined(CONFIG_CAN1_BAUDRATE)
#error "CONFIG_CAN1_BAUDRATE not defined. It should define the baudrate used for CAN1 module."
#endif
#if !defined(CONFIG_CAN1_RX_BUFFER_SIZE)
#error "CONFIG_CAN1_RX_BUFFER_SIZE not defined. It should define the buffer size used for receiving messages."
#endif
#if !defined(CONFIG_CAN1_TX_BUFFER_SIZE)
#error "CONFIG_CAN1_TX_BUFFER_SIZE not defined. It should define the buffer size used for transmit messages."
#endif
#if !defined(CONFIG_CAN1_TX_PIN)
#error "CONFIG_CAN1_TX_PIN should define the GPIO pin used as the CAN1 transmit pin"
#endif
#if !defined(CONFIG_CAN1_RX_PIN)
#error "CONFIG_CAN1_RX_PIN should define the GPIO pin used as the CAN1 receive pin"
#endif
#if CONFIG_CAN1_TX_PIN != PIO0_1 && CONFIG_CAN1_TX_PIN != PIO0_22
#error "CONFIG_CAN1_TX_PIN can be PIO0_1 or PIO0_22"
#endif
#if CONFIG_CAN1_RX_PIN != PIO0_0 && CONFIG_CAN1_RX_PIN != PIO0_21
#error "CONFIG_CAN1_RX_PIN can be PIO0_0 or PIO0_21"
#endif
#endif
#if CONFIG_CAN2
#if !defined(CONFIG_CAN2_BAUDRATE)
#error "CONFIG_CAN2_BAUDRATE not defined. It should define the baudrate used for CAN2 module."
#endif
#if !defined(CONFIG_CAN2_RX_BUFFER_SIZE)
#error "CONFIG_CAN2_RX_BUFFER_SIZE not defined. It should define the buffer size used for receiving messages."
#endif
#if !defined(CONFIG_CAN2_TX_BUFFER_SIZE)
#error "CONFIG_CAN2_TX_BUFFER_SIZE not defined. It should define the buffer size used for transmit messages."
#endif
#if !defined(CONFIG_CAN2_TX_PIN)
#error "CONFIG_CAN2_TX_PIN should define the GPIO pin used as the CAN2 transmit pin"
#endif
#if !defined(CONFIG_CAN2_RX_PIN)
#error "CONFIG_CAN2_RX_PIN should define the GPIO pin used as the CAN2 receive pin"
#endif
#if CONFIG_CAN2_TX_PIN != PIO0_5 && CONFIG_CAN2_TX_PIN != PIO2_8
#error "CONFIG_CAN2_TX_PIN can be PIO0_5 or PIO2_8"
#endif
#if CONFIG_CAN2_RX_PIN != PIO0_4 && CONFIG_CAN2_RX_PIN != PIO2_7
#error "CONFIG_CAN2_RX_PIN can be PIO0_4 or PIO2_7"
#endif
#endif
#endif



typedef enum {
	/// @brief: CAN 2.0A messages with 11-bit identifier
	CAN_11_BIT_ID = 0,
	CAN_STD = CAN_11_BIT_ID,
	/// @brief: CAN 2.0B messages with 29-bit identifier
	CAN_29_BIT_ID = 0x20000000UL,
	CAN_EXT = CAN_29_BIT_ID
} uv_can_msg_types_e;


/// @brief: CAN message basic structure
typedef struct {
	/// @brief: maximum of 8 Message data bytes
	union {
		uint8_t data_8bit[8];
		uint16_t data_16bit[4];
		uint32_t data_32bit[2];
		uint64_t data_64bit;
	};
	///@brief: Message id
	uint32_t id;
	/// @brief: Defines how many data bytes this message has
	uint8_t data_length;
	/// @brief: The type of the message. Either 29 or 11 bit (extended or standard)
	uv_can_msg_types_e type;
} uv_can_message_st;



typedef enum {
	CAN_ERROR_ACTIVE = 0,
	CAN_ERROR_HARDWARE_NOT_AVAILABLE,
	CAN_ERROR_HARDWARE_BUSY,
	CAN_ERROR_BUS_OFF,
	CAN_ERROR_PASSIVE,
	CAN_ERROR_WARNING
} uv_can_errors_e;


/// @brief: Describes all the available CAN channels on this hardware
typedef enum {
#if CONFIG_TARGET_LPC11C14
	CAN1 = 0,
	CAN_COUNT
#elif CONFIG_TARGET_LPC1785
#if CONFIG_CAN1
	CAN1,
#endif
#if CONFIG_CAN2
	CAN2,
#endif
	CAN_COUNT
#else
#error "Unknown hardware"
#endif
} uv_can_channels_e;


/// @brief: A enum describing popular CAN masks used when configuring receive messages.
enum {
	/// @brief: Mask where every bit is relevant
	CAN_ID_MASK_DEFAULT = 0xFFFFFFFF
};

/// @brief: Initializes the can module either in synchronous mode or in asynchronous mode.
///
/// @note: The mode is synchronous if tx_buffer and rx_buffer parameters are set to NULL
/// In this mode every message is sent synchronously and the message transmit function
/// will return after the message was sent.
/// In asynchronous mode, call to transmit functions wil cause the message to be queued in
/// the transmit buffer and it will be sent some time later.
uv_errors_e _uv_can_init();



/// @brief: Step function should be called every application step cycle
///
/// @param channel: The CAN channel which is stepped
/// @param step_ms: The step time in milliseconds
uv_errors_e uv_can_step(uv_can_channels_e channel, unsigned int step_ms);



/// @brief: Configures the CAN hardware to receive the messages with the given ID.
///
/// @note: Without a call to this, the CAN hardware doesn't process any received messages.
/// For a receive callback function to be called when the message is received,
/// the wanted message's ID needs to be configured with a call to this function.
///
/// The maximum number of messages which can be registered with this is hardware dependent.
/// If the maximum message count is exceeded, this function returns error from that.
///
/// @param channel: The CAN hardware channel to be configured
/// @param id: The messages ID which is wanted to be received
/// @param mask: The mask for message ID. This can be used to mask off unwanted
/// bits from ID, in order to receive many messages with different ID's.
/// To receive only a single dedicated message, this should be set to 0xFFFFFFFF or
/// CAN_ID_MASK_DEFAULT
/// @param type: The type of the message ID. Either 11-bit or 29-bit identifier is supported.
#if CONFIG_TARGET_LPC11C14
uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type);
#elif CONFIG_TARGET_LPC1785
/// @brief: Configures the CAN hardware to receive the messages with the given ID.
///
/// @note: Without a call to this, the CAN hardware doesn't process any received messages.
/// For a receive callback function to be called when the message is received,
/// the wanted message's ID needs to be configured with a call to this function.
///
/// The maximum number of messages which can be registered with this is hardware dependent.
/// If the maximum message count is exceeded, this function returns error from that.
///
/// @param channel: The CAN hardware channel to be configured
/// @param id: The messages ID which is wanted to be received
/// @param type: The type of the message ID. Either 11-bit or 29-bit identifier is supported.
uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		uv_can_msg_types_e type);

///// @brief: Configures the CAN hardware to receive a range of messages.
///// All messages with the ID between *start_id* and *end_id* are received.
/////
///// @param start_id: The smallest message ID to be received
///// @param end_id: The biggest message ID to be received.
//uv_errors_e uv_can_config_rx_message_range(uv_can_channels_e channel,
//		unsigned int start_id,
//		unsigned int end_id,
//		uv_can_msg_types_e type);
#endif



/// @brief: An alternative way to send a CAN message
///
/// @note: If message sending buffer is enabled by defining a CAN_ENABLE_ASYNCHRONOUS_MODE,
/// Sending messages is asynchronous. Otherwise it's synchronous and this
/// function will return only until the message was sent.
///
/// @return: Enum describing if errors were found while sending the message
///
/// @pre: uv_can_init should be called
uv_can_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message);
static inline uv_can_errors_e uv_can_send(uv_can_channels_e channel, uv_can_message_st *msg) {
	return uv_can_send_message(channel, msg);
}


/// @brief: Pops the lastly received message from the RX buffer and returns it in
/// *message* parameter
///
/// @note: This function pop's the last message out from the RX buffer, after which
/// the message cannot be retrieved anymore.
///
/// @param message: Pointer to the message data structure where the received message will be copied.
uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message);


/// @brief: Returns the CAN 2.0 specification error state. Error active is the normal state.
uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel);






/// @brief: Adds a receive callback function which will be called when a message
/// is received.
///
/// @note: This doesn't receive all messages! CAN interface uses masks to determine
/// which messages will come trough and what will not. To receive a message,
/// register it with a call to uv_can_config_rx_message
///
/// @param callback_function: A function pointer to the callback function. The
/// function takes 2 arguments: A user pointer (refer to uv_utilities.h) and
/// a pointer to the CAN message which was received.
uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		void (*callback_function)(void *user_ptr));


#if CONFIG_TERMINAL_CAN
/// @brief: Gets the next character from the CAN FIFO receive buffer.
/// This is used with terminal CAN redirection, to receive characters to the terminal.
///
/// @param dest: Pointer to the char where received character will be saved.
/// @return: Error if no more characters have been received
uv_errors_e uv_can_get_char(char *dest);
#endif



void _uv_can_hal_step(unsigned int step_ms);



#endif


#endif /* UW_CAN_H_ */
