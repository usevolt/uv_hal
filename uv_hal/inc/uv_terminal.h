/*
 * hal_debug.h
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */

#ifndef UW_TERMINAL_H_
#define UW_TERMINAL_H_


#include "uv_hal_config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "uv_stdout.h"
#include "uv_errors.h"

/// @file: A terminal interface which can be used over UART or CAN bus.
/// HAL layer takes care of parsing and redirecting the messages over UART or CAN.
/// User program needs to define all possible commands and register a callback function
/// which executes the received command from the user.
///
/// Some commands common for all CANopen based controllers are already defined in hal_terminal.c
/// These include entering ISP-mode and resetting the mcu.
/// The common commands cannot be disabled, but creating a application level command with the same
/// command name causes terminal to overwrite the common command.
///
/// If CONFIG_TERMINAL_DEDICATED_CALLBACKS is enabled, every command takes their own callback function.
/// This also changes how the arguments are passed to the callbacks. If CONFIG_TERMINAL_DEDICATED_CALLBACKS
/// is 1, the callbacks should be variadic functions which take va_list as a parameter.
/// The given arguments are parsed as integers by default, or as strings if enclosed with " " characters,
/// and pointers to them are given as va_arg arguments to the callback.
/// If CONFIG_TERMINAL_DEDICATED_CALLBACKS is 0, the arguments entered from the command line are not parsed
/// and the arguments are passed to the global callback function as strings.
///
/// NOTE: Note that in order to use UART or CAN as a terminal source, they have to be initialized
/// correctly. It doesn't matter if this module is initialized before them, but without initializing
/// the serial interfaces the terminal will never receive any characters sent to it.


#if !defined(CONFIG_TERMINAL_DEDICATED_CALLBACKS)
#error "CONFIG_TERMINAL_DEDICATED_CALLBACKS not defined. It should be defined as 1 or 0, depending\
 if callback functions should be dedicated individually for every command. Setting this to 0\
 saves memory by having only one callback for all commands, but is inefficient on bigger applications."
#endif
#if !defined(CONFIG_TERMINAL_BUFFER_SIZE)
#error "CONFIG_TERMINAL_BUFFER_SIZE not defined. It should define the maximum buffer size for\
 the terminal in bytes. Note that the buffer has to be big enough to fit all command arguments."
#endif
#if !defined(CONFIG_TERMINAL_ARG_COUNT)
#error "CONFIG_TERMINAL_ARG_COUNT not defined. It should define the maximum argument count for terminal."
#endif
#if !defined(CONFIG_TERMINAL_INSTRUCTIONS)
#error "CONFIG_TERMINAL_INSTRUCTIONS not defined. It should be defined as 1 or 0, depending if instructions\
 for terminal commands are to be included in the build. Instructions are more intuitive but require memory."
#endif


/* ASCII special character values which can be used in terminal printing */
#define CLRL			"\x1B[K"
#define CLRS			"\x1B[2J"
#define ENDL			"\n\r"




/// @brief: Structure which defines a single terminal command
typedef struct {
	/// @brief: Unique enumeration value for this command.
	uint16_t id;
	/// @brief: String which needs to be entered in serial port to execute this command
	char* str;
	/// @brief: A descriptive info from this command. Tells the user how to use command, etc etc.
	char* instructions;
	/// @brief: A callback function which will be called if this command was called
	void (*callback)(void*, unsigned int, unsigned int, ...);
} uv_command_st;


typedef enum {
	CMD_ISP = 0xF0,
	CMD_HELP,
#if CONFIG_TERMINAL_INSTRUCTIONS
	CMD_MAN,
#endif
	CMD_RESET,
#if CONFIG_NON_VOLATILE_MEMORY
	CMD_SAVE,
	CMD_REVERT,
#endif
#if CONFIG_CAN_LOG
	CMD_CAN_LOG,
#endif
#if CONFIG_CANOPEN_LOG
	CMD_CANOPEN_LOG,
#endif
	CMD_SET_ISP
} uv_common_commands_e;



/// @brief: Sets the pointer to an array containing all application commands.
/// This function should be called before any other terminal functions
///
/// @param commands: A pointer to command array containing all application commands.
/// Those commands are appended with uv_common_commands_e common commands.
/// @param buffer: The buffer which will be used to the terminal
/// @param buffer_size: The size of the buffer in bytes.
/// @param count: Indicates how many entries there are in commands array
void uv_terminal_init(const uv_command_st* commands, unsigned int count);


/// @brief: Step function should be called cyclically in the application
uv_errors_e uv_terminal_step();


/// @brief: Returns the number of commands found in command array pointer registered with
/// a hal_terminal_init_commands function call.
/// @pre: hal_terminal_init_commands should have been called to register a command array pointer.
int uv_terminal_get_commands_count(void);


///@brief: Disables or enables ISP mode entry when '?' is received from terminal
void uv_terminal_disable_isp_entry(bool value);


/// @brief: Parses a boolean value and returns it.
/// "on", "true", "1" are evaluated as true, otherwise returns false
bool uv_terminal_parse_bool(char *arg);





#endif /* UW_TERMINAL_H_ */