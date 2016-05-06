/*
 * hal_debug.h
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */

#ifndef UW_TERMINAL_H_
#define UW_TERMINAL_H_


#include "uw_hal_config.h"

#include <stdint.h>
#include <stdbool.h>
#include "uw_stdout.h"
#include "uw_errors.h"

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
/// Terminal commands should be parsed in the main application loop, not in the ISRs.
/// When using RTOS, this is already applied. Without RTOS the user must call
/// uw_uart_exec or uw_can_exec in the application main loop.
///
/// NOTE: Note that in order to use UART or CAN as a terminal source, they have to be initialized
/// correctly. It doesn't matter if this module is initialized before them, but without initializing
/// the serial interfaces the terminal will never receive any characters sent to it.


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
	CMD_ISP = 0xF0,
	CMD_HELP,
	CMD_RESET,
#if CONFIG_NON_VOLATILE_MEMORY
	CMD_SAVE,
	CMD_REVERT,
#endif
#if CONFIG_CANOPEN
	CMD_SDO,
	CMD_STATE,
#endif
#if CONFIG_CAN_LOG
	CMD_CAN_LOG,
#endif
#if CONFIG_CANOPEN_LOG
	CMD_CANOPEN_LOG,
#endif
	CMD_SET_ISP
} uw_common_commands_e;

/* ------------ PUBLIC FUNCTIONS -----------------*/

/// @brief: Sets the pointer to an array containing all application commands.
/// This function should be called before any other terminal functions
///
/// @param commands: A pointer to command array containing all application commands.
/// Those commands are appended with uw_common_commands_e common commands.
/// @param buffer: The buffer which will be used to the terminal
/// @param buffer_size: The size of the buffer in bytes.
/// @param count: Indicates how many entries there are in commands array
/// @param callback: Callback function which will be called when a command has been received.
/// The callback function should take 2 parameters: command enum value and
/// user's entered arguments.
void uw_terminal_init(const uw_command_st* commands, unsigned int count,
		void (*callback_function)(void* user_ptr, int cmd, char** args));



/// @brief: Step function should be called cyclically in the application
uw_errors_e uw_terminal_step();


/// @brief: Returns the number of commands found in command array pointer registered with
/// a hal_terminal_init_commands function call.
/// @pre: hal_terminal_init_commands should have been called to register a command array pointer.
int uw_terminal_get_commands_count(void);



///@brief: Disables or enables ISP mode entry when '?' is received from terminal
void uw_terminal_disable_isp_entry(bool value);








#endif /* UW_TERMINAL_H_ */
