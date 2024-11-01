/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
/// The common commands cannot be disabled, but creating a application level command with the same
/// command name causes terminal to overwrite the common command.
///
/// NOTE: Note that in order to use UART or CAN as a terminal source, they have to be initialized
/// correctly. It doesn't matter if this module is initialized before them, but without initializing
/// the serial interfaces the terminal will never receive any characters sent to it.


/* ASCII special character values which can be used in terminal printing */
#define CLRL			"\x1B[K"
#define CLRS			"\x1B[2J"
#define ENDL			"\n"


/// @brief: Prefix for CAN message ID's used for command line interface.
/// The actual CAN ID consists of this and a CANopen node id
#define UV_TERMINAL_CAN_ID			0x580
#define UV_TERMINAL_CAN_RX_ID		0x600
#define UV_TERMINAL_CAN_INDEX		0x5FFF
#define UV_TERMINAL_CAN_SUBINDEX	0


#if CONFIG_TERMINAL

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
#if !CONFIG_TERMINAL_UART && !CONFIG_TERMINAL_CAN
#error "either CONFIG_TERMINAL_UART or CONFIG_TERMINAL_CAN has to be defined as 1, to redirect printf to\
 corresponding peripherals."
#endif
#if CONFIG_TERMINAL_CAN
#if CAN_COUNT > 1
#if !defined(CONFIG_TERMINAL_CAN_CHN)
#error "CONFIG_TERMINAL_CAN_CHN has to define the CAN channel to be used"
#endif
#else
#if !defined(CONFIG_TERMINAL_CAN_CHN)
#define CONFIG_TERMINAL_CAN_CHN		0
#endif
#endif
#endif



/// @brief: Defines the type of the argument
enum {
	ARG_UNDEFINED = 0,
	ARG_INTEGER,
	ARG_STRING
};
typedef uint8_t type_t;

/// @brief: A single argument struct
typedef struct {
	/// @brief: Union which contains the actual argument
	union {
		/// @brief: Generic way for returning the "value". Depends on the type of this argument
		/// if this is a pointer to a string or an integer.
		void *value;
		char *str;
		int32_t number;
	};
	type_t type;
} argument_st;


/// @brief: Structure which defines a single terminal command
typedef struct {
	/// @brief: Unique enumeration value for this command.
	uint16_t id;
	/// @brief: String which needs to be entered in serial port to execute this command
	char* str;
#if CONFIG_TERMINAL_INSTRUCTIONS
	/// @brief: A descriptive info from this command. Tells the user how to use command, etc etc.
	char* instructions;
#endif
	/// @brief: A callback function which will be called if this command was called
	void (*callback)(void*, unsigned int, unsigned int, argument_st *);
} uv_command_st;


typedef enum {
	CMD_HELP,
	CMD_DEV,
	CMD_NODEID,
#if CONFIG_TERMINAL_INSTRUCTIONS
	CMD_MAN,
#endif
#if CONFIG_NON_VOLATILE_MEMORY
	CMD_SAVE,
	CMD_REVERT,
#endif
	CMD_RESET
} uv_common_commands_e;


/// @brief: Tells the active terminal, where the output is directed.
/// The active terminal is determined from the source of the last received character
typedef enum {
	TERMINAL_NONE = 0,
	TERMINAL_UART,
	TERMINAL_CAN,
	TERMINAL_USB
} uv_active_terminal_e;

/// @brief: Returns the currently active terminal
uv_active_terminal_e uv_active_terminal(void);

void uv_terminal_enable(uv_active_terminal_e dest);

void uv_terminal_disable(void);

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


/// @brief: Parses a boolean value and returns it.
/// "on", "true", "1" are evaluated as true, otherwise returns false
bool uv_terminal_parse_bool(char *arg);



/// @brief: Adds a character to the RX buffer
void _uv_terminal_add_rx_buffer(char c);


#endif

#endif /* UW_TERMINAL_H_ */
