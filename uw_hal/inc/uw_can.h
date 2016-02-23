/*
 * uw_can.h
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#ifndef UW_CAN_H_
#define UW_CAN_H_

#include <stdint.h>
#include <stdbool.h>
/// @file: CAN implementation on top of the HAL layer
/// Provides the basic CAN interface.
/// Note that this module takes full ownership of the CAN HAL layer interface.
/// For example on LPC11C14, this module may use all 32 CAN message objects
/// as it wants.


/// @brief: CAN message basic structure
typedef struct {
	///@brief: Message id
	uint32_t id;
	/// @brief: maximum of 8 Message data bytes
	union {
		uint8_t data_8bit[8];
		uint16_t data_16bit[4];
		uint32_t data_32bit[2];
		uint64_t data_64bit;
	};
	/// @brief: Defines how many data bytes this message has
	uint8_t data_length;
} uw_can_message_st;



typedef enum {
	UW_CAN_NO_ERROR,
	UW_CAN_ERROR_HARDWARE_NOT_AVAILABLE,
	UW_CAN_ERROR_HARDWARE_BUSY
} uw_can_errors_e;


/// @brief: Describes all the available CAN channels on this hardware
typedef enum {
#ifdef LPC11C14
	CAN1,
	CAN_COUNT
#elif defined(LPC1785)
	CAN1,
	CAN2,
	CAN_COUNT
#else
#error "Unknown hardware"
#endif
} uw_can_channels_e;


/// @brief: A enum describing popular CAN masks used when configuring receive messages.
enum {
	/// @brief: Mask where every bit is relevant
	UW_CAN_MASK_DEFAULT = 0xFFFFFFFF
};

/// @brief: Initializes the can module either in synchronous mode or in asynchronous mode.
///
/// @note: The mode is synchronous if tx_buffer and rx_buffer parameters are set to NULL
/// In this mode every message is sent synchronously and the message transmit function
/// will return after the message was sent.
/// In asynchronous mode, call to transmit functions wil cause the message to be queued in
/// the transmit buffer and it will be sent some time later.
///
/// @pre: none
///
/// @param channel: The CAN channel to be initialized
/// @param baudrate: The baudrate in Hz of the CAN bus
/// @param fosc: The system oscillator frequency in Hz
bool uw_can_init(uw_can_channels_e channel, unsigned int baudrate, unsigned int fosc,
		uw_can_message_st *tx_buffer, unsigned int tx_buffer_size,
		uw_can_message_st *rx_buffer, unsigned int rx_buffer_size);



/// @brief: Step function should be called every application step cycle
///
/// @param channel: The CAN channel which is stepped
/// @param step_ms: The step time in milliseconds
void uw_can_step(uw_can_channels_e channel, unsigned int step_ms);



/// @brief: Configures the CAN hardware to receive the messages with the given ID.
///
/// @note: Without a call to this, the CAN hardware doesn't process any received messages.
/// For a receive callback function to be called when the message is received,
/// the wanted message's ID needs to be configured with a call to this function.
///
/// @param channel: The CAN hardware channel to be configured
/// @param id: The messages ID which is wanted to be received
/// @param mask: The mask for message ID. This can be used to mask off unwanted
/// bits from ID, in order to receive many messages with different ID's.
/// To receive only a single dedicated message, this should be set to 0xFFFFFFFF or
/// UW_CAN_MASK_DEFAULT
bool uw_can_config_rx_message(uw_can_channels_e channel,
		unsigned int id,
		unsigned int mask);



/// @brief: creates a can message and returns it without sending it.
///
/// @pre: none
uw_can_message_st uw_can_create_message(uint32_t id, uint8_t data_length, uint8_t* data);


/// @brief: An alternative way to send a CAN message
///
/// @note: If message sending buffer is enabled by defining a CAN_ENABLE_ASYNCHRONOUS_MODE,
/// Sending messages is asynchronous. Otherwise it's synchronous and this
/// function will return only until the message was sent.
///
/// @return: Enum describing if errors were found while sending the message
///
/// @pre: uw_can_init should be called
uw_can_errors_e uw_can_send_message(uw_can_channels_e channel, uw_can_message_st* message);



/// @brief: Adds a transmit callback function which will be called when a message
/// is transmitted.
///
/// @param callback_function: A function pointer to the callback function. The
/// function takes 2 arguments: A user pointer (refer to uw_utilities.h) and
/// a pointer to the CAN message which was sent.
bool uw_can_add_tx_callback(uw_can_channels_e channel,
		void (*callback_function)(void *user_ptr, uw_can_message_st *msg));



/// @brief: Adds a receive callback function which will be called when a message
/// is received.
///
/// @note: This doesn't receive all messages! CAN interface uses masks to determine
/// which messages will come trough and what will not. To receive a message,
/// register it with a call to uw_can_config_rx_message
///
/// @param callback_function: A function pointer to the callback function. The
/// function takes 2 arguments: A user pointer (refer to uw_utilities.h) and
/// a pointer to the CAN message which was received.
bool uw_can_add_rx_callback(uw_can_channels_e channel,
		void (*callback_function)(void *user_ptr, uw_can_message_st *msg));


//void uw_can_add_message_callback()


#endif /* UW_CAN_H_ */
