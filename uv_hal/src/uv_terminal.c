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


#include "uv_terminal.h"


#if CONFIG_TERMINAL

#include "uv_reset.h"
#include "uv_canopen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "uv_utilities.h"
#include "uv_memory.h"
#include "uv_uart.h"
#include "uv_stdout.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif
#if CONFIG_RTOS
#include "uv_rtos.h"
#endif
#if CONFIG_TERMINAL_USBDVCOM
#include "cdc_vcom.h"
#endif
#include CONFIG_MAIN_H
#include <uv_hal_config.h>



typedef struct {
	const uv_command_st *commands_ptr;
	uint8_t commands_count;
	uint8_t buffer_index;
	char buffer[CONFIG_TERMINAL_BUFFER_SIZE];
	argument_st args[CONFIG_TERMINAL_ARG_COUNT];
	uv_active_terminal_e active_terminal;
} this_st;

this_st _terminal = { };
#define this (&_terminal)



uv_active_terminal_e uv_active_terminal(void) {
	return this->active_terminal;
}

void uv_terminal_enable(uv_active_terminal_e dest) {
	this->active_terminal = dest;
}

void uv_terminal_disable(void) {
	this->active_terminal = TERMINAL_NONE;
}


void uv_terminal_help_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
void uv_terminal_dev_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
void uv_terminal_nodeid_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
void uv_terminal_baud_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
#if CONFIG_TERMINAL_INSTRUCTIONS
void uv_terminal_man_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
#endif
void uv_terminal_reset_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
#if CONFIG_NON_VOLATILE_MEMORY
void uv_terminal_save_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
void uv_terminal_revert_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv);
#endif

const uv_command_st common_cmds[] = {
		{
				.id = CMD_HELP,
				.str = "help",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions = "Lists all available commands" ,
#endif
				.callback = uv_terminal_help_callb
		},
		{
				.id = CMD_DEV,
				.str = "dev",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions = "Logs the device name and build date" ,
#endif
				.callback = uv_terminal_dev_callb
		},
		{
				.id = CMD_NODEID,
				.str = "nodeid",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions = "Returns or sets the device's CANopen Node ID.",
#endif
				.callback = &uv_terminal_nodeid_callb
		},
		{
				.id = CMD_NODEID,
				.str = "baud",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions = "Returns or sets the CAN bus baudrate",
#endif
				.callback = &uv_terminal_baud_callb
		},
#if CONFIG_TERMINAL_INSTRUCTIONS
		{
				.id = CMD_MAN,
				.str = "man",
				.instructions =
						"Gives information from the command. Give the command name as the argument.\n"
						"Usage: man \"<command_name>\"",
				.callback = uv_terminal_man_callb
		},
#endif
#if CONFIG_NON_VOLATILE_MEMORY
		{
				.id = CMD_SAVE,
				.str = "save",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions = "Saves application data and settings to non-volatile flash memory." ,
#endif
				.callback = uv_terminal_save_callb
		},
		{
				.id = CMD_REVERT,
				.str = "revert",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions =
						"Reverts all changes except the device CRC number to factory defaults.\n"
						"To reset the device CRC, set it to 0 with a command 'crc 0'.\n"
						"The device needs to be restarted for changes to take effect.\n"
						"To undo revert, save current values to flash with 'save' command." ,
#endif
				.callback = uv_terminal_revert_callb
		},
		{
				.id = CMD_RESET,
				.str = "reset",
#if CONFIG_TERMINAL_INSTRUCTIONS
				.instructions =
						"Usage: reset"
						"Resets the controller instantly.\n"
						"All unsave modifications will be lost.",
#endif
				.callback = uv_terminal_reset_callb
		}
#endif
};


void uv_terminal_init(const uv_command_st* commands, unsigned int count) {
	this->commands_ptr = commands;
	this->commands_count = count;
	this->buffer_index = 0;
	this->buffer[0] = '\0';
	this->active_terminal = TERMINAL_NONE;

	// print device name and build date
	printf("%s\nBuild on %s\n>", uv_projname, uv_datetime);


}



int uv_terminal_get_commands_count(void) {
	return this->commands_count;
}



uv_errors_e uv_terminal_step() {
	uv_errors_e ret = ERR_NONE;

	// if pointer to commands array is null, terminal is not initialized, so we do nothing
	if (!this->commands_ptr) {
		ret = ERR_NOT_INITIALIZED;
	}
	else {

		// cycle trough all received characters
		while (true) {

			uv_errors_e e = ERR_NONE;
			char data = '\0';

#if CONFIG_TERMINAL_UART
			// redirect printf to the source where message was received
			e = uv_uart_get_char(UART0, &data);
			if (data != '\0') {
				uv_terminal_enable(TERMINAL_UART);
			}
#endif

			// No more data is available on uart. Try CAN
#if CONFIG_TERMINAL_CAN
			if (!data) {
				e = uv_can_get_char(&data);
				if (data != '\0') {
					uv_terminal_enable(TERMINAL_CAN);
				}
			}
#endif
			// no more data is available on can. Try USB VCOM
#if CONFIG_TERMINAL_USBDVCOM
			if (!data) {
				e = (vcom_bread((uint8_t*) &data, 1) != 0) ? ERR_NONE : ERR_BUFFER_EMPTY;
				if (data != '\0') {
					uv_terminal_enable(TERMINAL_USB);
				}
			}
#endif
			if (e != ERR_NONE) {
				break;
			}

			// check for buffer overflows
			if (this->buffer_index >= CONFIG_TERMINAL_BUFFER_SIZE) {
				this->buffer_index = 0;
				ret = ERR_BUFFER_OVERFLOW;
				break;
			}
			else {
				// echo back received characters
				uv_stdout_send(&data, 1);

				// escape clears the terminal
				if (data == 0x1B) {
					uv_stdout_send("\033[2K\r>", 6);
					this->buffer_index = 0;
				}
				// if backspace was received, delete last saved character
				else if (data == 0x08) {
					if (this->buffer_index > 0) {
						this->buffer_index--;
					}
				}
				//if carriage return was received, read command and clear buffer
				else if (data == 0x0D || data == 0x0A) {
					int i;
					int p = 0;
					// do nothing if receive buffer was empty
					if (this->buffer_index == 0) {
						printf(">");
#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
						printf("\n");
						fflush(stdout);
#endif
					}
					else {
						//replace carriage return with a terminating mark
						this->buffer[this->buffer_index] = '\0';

						bool string_arg = false;
						for (i = 0; i < this->buffer_index; i++) {
							// '"' mark means a string argument
							// (multi-space argument if dedicated callback are not used)
							if (i) {
								if (this->buffer[i] == '"') {
									if (!string_arg) {
										// multi-space argument start, edit arg starting with \0
										if (this->buffer[i - 1] == '\0') {
											this->buffer[i] = '\0';
											this->args[p - 1].type = ARG_STRING;
											this->args[p - 1].value = (char*) &this->buffer[i + 1];
											string_arg = true;
										}
									}
									else {
										// multi-space argument end
										string_arg = false;
										this->buffer[i] = '\0';
									}
								}
							}
							// search for space (argument start)
							if (!string_arg && this->buffer[i] == ' ') {
								this->buffer[i] = '\0';
								if (p < CONFIG_TERMINAL_ARG_COUNT) {
									// integer argument found
									this->args[p].type = ARG_INTEGER;
									this->args[p].number = strtol((char*) &this->buffer[i + 1], NULL, 0);
									p++;
								}
							}
						}
						this->buffer_index = 0;

						// find out which command was received by looping trough them
						int q;
						bool match = false;

						for (q = 0; q < uv_terminal_get_commands_count(); q++) {
							if (strcmp((const char*) this->buffer, (const char*) this->commands_ptr[q].str) == 0) {
								if (this->commands_ptr[q].callback) {
									this->commands_ptr[q].callback(__uv_get_user_ptr(),
											(int) this->commands_ptr[q].id, p, (argument_st*) this->args);
								}
								match = true;
								break;
							}
						}
						// if no match was found, search common commands
						if (!match) {
							for (q = 0; q < sizeof(common_cmds) / sizeof(uv_command_st); q++) {
								if (strcmp((char*) this->buffer, (char*) common_cmds[q].str) == 0) {
									match = true;
									common_cmds[q].callback(__uv_get_user_ptr(), (int) common_cmds[q].id, p,
											(argument_st*) this->args);
									break;
								}
							}
							if (!match) {
								printf("Command '%s' not found\n",
										this->buffer);
							}
						}
						printf(">");
#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
						printf("\n");
						fflush(stdout);
#endif
					}
				}
				else {
					// add character to buffer
					this->buffer[this->buffer_index++] = data;
				}
			}
		}
	}
	return ret;
}

void uv_terminal_help_callb(void *me, unsigned int cmd, unsigned int args, argument_st *argv) {
	printf("\n");
	int p;
	for (p = 0; p < sizeof(common_cmds) / sizeof(uv_command_st); p++) {
		printf("\"%s\"\n", common_cmds[p].str);
	}
	for (p = 0; p < uv_terminal_get_commands_count(); p++) {
		printf("\"%s\"\n", this->commands_ptr[p].str);
	}
}
void uv_terminal_dev_callb(void *me, unsigned int cmd, unsigned int args, argument_st *argv) {
	printf("%s, if revision %u, Build on %s\n", uv_projname, CONFIG_INTERFACE_REVISION, uv_datetime);
}

void uv_terminal_nodeid_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv) {
	if (args && argv[0].type == ARG_INTEGER) {
		if (argv[0].number == 0 ||
				argv[0].number > 0x7F) {
			printf("Invalid node id given: 0x%x\n", (unsigned int) argv[0].number);
		}
		else {
			uv_canopen_set_our_nodeid(argv[0].number);
			printf("Node ID set to 0x%x. Save and reset required.\n", (unsigned int) argv[0].number);
		}
	}
	else {
		printf("Node ID: 0x%x\n", uv_canopen_get_our_nodeid());
	}
}


void uv_terminal_baud_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv) {
	if (args && argv[0].type == ARG_INTEGER) {
		if (argv[0].number == 125000 ||
				argv[0].number == 250000 ||
				argv[0].number == 500000 ||
				argv[0].number == 1000000) {
			CONFIG_NON_VOLATILE_START.can_baudrate = argv[0].number;
			printf("CAN baudrate set to %u. Save and reset to apply changes.\n",
					argv[0].number);
		}
		else {
			printf("Supported baudrates are 125000, 250000, 500000 or 1000000\n");
		}

		printf("CAN baudrate: %u baud/s\n", CONFIG_NON_VOLATILE_START.can_baudrate);
	}
}



#if CONFIG_TERMINAL_INSTRUCTIONS
void uv_terminal_man_callb(void *me, unsigned int cmd, unsigned int args, argument_st *argv) {
	int i;
	for (i = 0; i < this->commands_count; i++) {
		if (strcmp(argv[0].str, this->commands_ptr[i].str) == 0) {
			printf("\n%s:\n%s\n\n", this->commands_ptr[i].str, this->commands_ptr[i].instructions);
			return;
		}
	}
	for (i = 0; i < sizeof(common_cmds) / sizeof(uv_command_st); i++) {
		if (strcmp(argv[0].str, common_cmds[i].str) == 0) {
			printf("\n%s:\n%s\n\n", common_cmds[i].str, common_cmds[i].instructions);
			return;
		}
	}
	printf("Give a command name as a string argument\n");
}
#endif
void uv_terminal_reset_callb(void *me, unsigned int cmd, unsigned int args, argument_st *argv) {
	uv_system_reset();
}
#if CONFIG_NON_VOLATILE_MEMORY
void uv_terminal_save_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv) {
	if (!uv_memory_save()) {
		printf("saved\n");
	}
}
void uv_terminal_revert_callb(void *me, unsigned int cmd, unsigned int args, argument_st * argv) {
	if (!uv_memory_clear(MEMORY_ALL_PARAMS)) {
		printf("reverted\n");
	}
}
#endif



bool uv_terminal_parse_bool(char *arg) {
	bool ret = false;
	if (strcmp(arg, "on") == 0 || strcmp(arg, "true") == 0 || strcmp(arg, "1") == 0) {
		ret = true;
	}
	return ret;
}


#endif

