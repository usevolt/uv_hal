/*
 * hal_debug.h
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */

#ifndef HAL_TERMINAL_H_
#define HAL_TERMINAL_H_


#include <stdint.h>
#include <stdbool.h>
#include "hal_stdout.h"

/// @file: A terminal interface which can be used over UART or CAN bus.
/// HAL layer takes care of parsing and redirecting the messages over UART or CAN.
/// User program needs to define all possible commands and register a callback function
/// which executes the received command from the user.
///
/// Some commands common for all CANopen based controllers are already defined in hal_terminal.c
/// These include entering ISP-mode, resetting the mcu and interfacing with CANopen object dictionary.
/// The common commands cannot be disabled, but creating a application level command with the same
/// command name causes terminal to overwrite the common command.
///
/// @note: Note that in order to use UART or CAN as a terminal source, they have to be initialized
/// correctly. It doesn't matter if this module is initialized before them, but without initializing
/// the serial interfaces the terminal will never receive any characters sent to it.


// terminal command max character length with arguments included
#define TERMINAL_RECEIVE_BUFFER_SIZE		30
// terminal command max argument count
#define TERMINAL_ARG_COUNT					4


/// @brief: Structure which defines a single terminal command
typedef struct {
	/// @brief: Unique enumeration value for this command.
	uint16_t id;
	/// @brief: String which needs to be entered in serial port to execute this command
	char* str;
	/// @brief: A descriptive info from this command. Tells the user how to use command, etc etc.
	char* instructions;
} uw_command_st;


typedef enum {
	CMD_ENTER_ISP,
	CMD_RESET,
	CMD_SAVE,
	CMD_REVERT,
	CMD_SET_SDO,
	CMD_GET_SDO,
	CMD_PDO_ECHO,
	CMD_SET_STATE
} uw_common_commands_e;

/* ------------ PUBLIC FUNCTIONS -----------------*/

/// @brief: Sets the pointer to an array containing all application commands.
/// This function should be called before any other terminal functions
/// @param commands: A pointer to command array containing all commands
/// @param count: Indicates how many entries there are in commands array
void hal_terminal_init(const uw_command_st* commands, unsigned int count);

/// @brief: Registers a callback function which is responsible for executing application's
/// terminal commands. The callback function should take 2 parameters: command enum value and
/// user's entered arguments.
/// @note: If unknown command is received, all available commands are listed and this
/// callback is not called.
void hal_terminal_set_callback( void (*callback_function)(int cmd, char** args) );

/// @brief: Returns the number of commands found in command array pointer registered with
/// a hal_terminal_init_commands function call.
/// @pre: hal_terminal_init_commands should have been called to register a command array pointer.
int hal_terminal_get_commands_count(void);

///@brief: Disables or enables ISP mode entry when '?' is received from terminal
void hal_terminal_disable_isp_entry(bool value);





/* ---------- PRIVATE FUNCTIONS ---------------*/


/// @brief: Processes rx messages. Parses it and calls the user callback function which
/// in turn is responsible for executing the command.
/// @note: This function shouldn't be called by the user application. It is intended for HAL
/// library's internal use. Also, as the messages coming to this function can
/// come from multiple serial protocol sources, this function redirects the stdio's printf
/// to print into the location where it got the message. This causes printf to print into that
/// source in the future, until message from different source has been received.
/// @param data: A string of incoming data. Doesn't need to be null-terminated since null
/// can be included in the data stream.
/// @param data_length: The length of data in characters
/// @param source: The serial protocol source where the data is coming from.
void __hal_terminal_process_rx_msg(char* data, uint8_t data_length, hal_stdout_sources_e source);




#endif /* HAL_TERMINAL_H_ */
