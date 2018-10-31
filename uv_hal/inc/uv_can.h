/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549
#if !CONFIG_CAN0
#error "At least one CAN channel should be defined"
#endif
#if CONFIG_CAN1
#error "Hardware doesn't support CAN1 module. Set CONFIG_CAN1 to 0."
#endif
#if CONFIG_CAN0
#if !defined(CONFIG_CAN0_BAUDRATE)
#error "CONFIG_CAN0_BAUDRATE not defined. It should define the default baudrate used for CAN0 module.\
 Note that if can_baudrate is set in uv_memory module via uv_memory_set_can_baudrate, that one will be used\
 instead."
#endif
#if !defined(CONFIG_CAN0_RX_BUFFER_SIZE)
#error "CONFIG_CAN0_RX_BUFFER_SIZE not defined. It should define the buffer size used for receiving messages."
#endif
#if !defined(CONFIG_CAN0_TX_BUFFER_SIZE)
#error "CONFIG_CAN0_TX_BUFFER_SIZE not defined. It should define the buffer size used for transmit messages."
#endif
#if CONFIG_TARGET_LPC1549
#if !defined(CONFIG_CAN0_RX_PIN)
#error "CONFIG_CAN0_RX_PIN should define the pin to be used as CAN RX pin"
#endif
#if !defined(CONFIG_CAN0_TX_PIN)
#error "CONFIG_CAN0_TX_PIN should define the pin to be used as CAN TX pin"
#endif
#endif
#endif
#elif CONFIG_TARGET_LPC1785
#if CONFIG_CAN0
#if !defined(CONFIG_CAN0_BAUDRATE)
#error "CONFIG_CAN0_BAUDRATE not defined. It should define the baudrate used for CAN0 module."
#endif
#if !defined(CONFIG_CAN0_RX_BUFFER_SIZE)
#error "CONFIG_CAN0_RX_BUFFER_SIZE not defined. It should define the buffer size used for receiving messages."
#endif
#if !defined(CONFIG_CAN0_TX_BUFFER_SIZE)
#error "CONFIG_CAN0_TX_BUFFER_SIZE not defined. It should define the buffer size used for transmit messages."
#endif
#if !defined(CONFIG_CAN0_TX_PIN)
#error "CONFIG_CAN0_TX_PIN should define the GPIO pin used as the CAN0 transmit pin"
#endif
#if !defined(CONFIG_CAN0_RX_PIN)
#error "CONFIG_CAN0_RX_PIN should define the GPIO pin used as the CAN0 receive pin"
#endif
#if CONFIG_CAN0_TX_PIN != PIO0_1 && CONFIG_CAN0_TX_PIN != PIO0_22
#error "CONFIG_CAN0_TX_PIN can be PIO0_1 or PIO0_22"
#endif
#if CONFIG_CAN0_RX_PIN != PIO0_0 && CONFIG_CAN0_RX_PIN != PIO0_21
#error "CONFIG_CAN0_RX_PIN can be PIO0_0 or PIO0_21"
#endif
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
#if !defined(CONFIG_CAN1_TX_PIN)
#error "CONFIG_CAN1_TX_PIN should define the GPIO pin used as the CAN1 transmit pin"
#endif
#if !defined(CONFIG_CAN1_RX_PIN)
#error "CONFIG_CAN1_RX_PIN should define the GPIO pin used as the CAN1 receive pin"
#endif
#if CONFIG_CAN1_TX_PIN != PIO0_5 && CONFIG_CAN1_TX_PIN != PIO2_8
#error "CONFIG_CAN1_TX_PIN can be PIO0_5 or PIO2_8"
#endif
#if CONFIG_CAN1_RX_PIN != PIO0_4 && CONFIG_CAN1_RX_PIN != PIO2_7
#error "CONFIG_CAN1_RX_PIN can be PIO0_4 or PIO2_7"
#endif
#endif
#elif CONFIG_TARGET_LINUX
#if !defined(CONFIG_CAN0_BAUDRATE)
#error "CONFIG_CAN0_BAUDRATE should define the default baudrate for CAN."
#endif
#endif



typedef enum {
	/// @brief: CAN 2.0A messages with 11-bit identifier
	CAN_11_BIT_ID = 0,
	CAN_STD = CAN_11_BIT_ID,
	/// @brief: CAN 2.0B messages with 29-bit identifier
	CAN_29_BIT_ID = 0x20000000UL,
	CAN_EXT = CAN_29_BIT_ID,
	/// @brief: CAN error frame
	CAN_ERR = 0x40000000UL
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
typedef uv_can_message_st uv_can_msg_st;



typedef enum {
	CAN_ERROR_ACTIVE = 0,
	CAN_ERROR_HARDWARE_NOT_AVAILABLE,
	CAN_ERROR_HARDWARE_BUSY,
	CAN_ERROR_BUS_OFF,
	CAN_ERROR_PASSIVE,
	CAN_ERROR_WARNING
} uv_can_errors_e;


/// @brief: Describes all the available CAN channels on this hardware
#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
/// @brief: On Linux CAN channels are separated by the netdev name
typedef char * uv_can_channels_e;
#else
typedef enum {
#if CONFIG_TARGET_LPC11C14
	CAN0 = 0,
	CAN_COUNT
#elif CONFIG_TARGET_LPC1785
#if CONFIG_CAN0
	CAN0,
#endif
#if CONFIG_CAN1
	CAN1,
#endif
	CAN_COUNT
#elif CONFIG_TARGET_LPC1549
	CAN0 = 0,
	CAN_COUNT
#else
#error "Unknown hardware"
#endif
} uv_can_channels_e;
#endif


/// @brief: A enum describing popular CAN masks used when configuring receive messages.
enum {
	/// @brief: Mask where every bit is relevant
	CAN_ID_MASK_DEFAULT = 0xFFFFFFFF,
};


/// @brief: Initializes the can module
uv_errors_e _uv_can_init();



/// @brief: Configures the CAN hardware to receive the messages with the given ID.
///
/// @note: Without a call to this, the CAN hardware doesn't process any received messages.
/// For a receive callback function to be called when the message is received,
/// the wanted message's ID needs to be configured with a call to this function.
///
/// The maximum number of messages which can be registered with this is hardware dependent.
/// If the maximum message count is exceeded, this function returns error from that.
///
/// On Linux this is just a wrap function, since SocketCAN doens't support mesage filtering.
///
/// @param channel: The CAN hardware channel to be configured
/// @param id: The messages ID which is wanted to be received
/// @param mask: The mask for message ID. This can be used to mask off unwanted
/// bits from ID, in order to receive many messages with different ID's.
/// To receive only a single dedicated message, this should be set to 0xFFFFFFFF or
/// CAN_ID_MASK_DEFAULT
/// @param type: The type of the message ID. Either 11-bit or 29-bit identifier is supported.
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549 || CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
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
uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message);
static inline uv_errors_e uv_can_send(uv_can_channels_e channel, uv_can_message_st *msg) {
	return uv_can_send_message(channel, msg);
}


/// @brief: Sends a CAN message synchronously. Returns when the message has been sent
/// or when an error is received (CAN is in error passive or bus off)
uv_errors_e uv_can_send_sync(uv_can_channels_e channel, uv_can_message_st *msg);


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


uv_errors_e uv_can_reset(uv_can_channels_e channel);

/// @brief: Clears the rx buffer
void uv_can_clear_rx_buffer(uv_can_channels_e channel);

#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
/// @brief: Baudrate setting only possible on Linux systems. Otherwise baudrate is
/// specified via CONFIG_CAN_BAUDRATE symbol.
///
/// @note: Should be called prior to _uv_can_init function.
bool uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate);

struct timeval uv_can_get_rx_time(void);

/// @brief: Returns the name of *i*'th CAN interface device found
char *uv_can_get_device_name(int32_t i);

/// @brief: Returns the count of found CAN interface devices
int32_t uv_can_get_device_count(void);

/// @brief: Returns true if the connection to the CAN dev is open
bool uv_can_is_connected(void);

/// @brief: Sets the CAN dev up and running
void uv_can_set_up(void);

#endif


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
