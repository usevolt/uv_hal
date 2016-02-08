/*
 * hal_debug.c
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */


#include "hal_terminal.h"
#include "hal_iap.h"
#include <stdio.h>
#include <string.h>

static const uw_command_st* commands_ptr = 0;
static int commands_count = 0;
static void (*callback)(int cmd, char** args) = 0;

/// whole command receive buffer
static char terminal_receive_buffer[TERMINAL_RECEIVE_BUFFER_SIZE];
/// argument pointers
static char* args[TERMINAL_ARG_COUNT];
static uint8_t receive_buffer_index = 0;
static bool disable_isp_entry = false;
static const uw_command_st common_cmds[] = {
		{
				CMD_ENTER_ISP,
				"enterisp",
				"Sets the controller to enter ISP \n\r\
(in system programming) mode instantly."
		},
		{
				CMD_RESET,
				"reset",
				"Resets the controller instantly.\n\r\
All unsave modifications will be lost."
		},
		{
				CMD_SAVE,
				"save",
				"Saves application data and settings to non-volatile memory."
		},
		{
				CMD_REVERT,
				"revert",
				"Reverts the device to it's default settings.\n\r\
To undo this command, reset the controller.\n\r\
To keep the changes, save them to non-volatile memory with 'save'"
		},
		{
				CMD_SET_SDO,
				"setsdo",
				"Usage: setsdo <index> <subindex> <value>\n\r\
Used to write CANopen SDO object manually.\n\r\
All arguments are evaluated as 10-base integer values.\n\r\
To define them in 16-base hexadecimals, use '0x' prefix."
		},
		{
				CMD_GET_SDO,
				"getsdo",
				"Usage: getsdo <index> <subindex>\n\r\
Used to read CANopen SDO object manually.\n\r\
Both arguments are evaluated as 10-base integer values.\n\r\
To define them in 16-base hexadecimals, use '0x' prefix."
		},
		{
				CMD_PDO_ECHO,
				"pdoecho",
				"Usage: pdoecho <on/off>\n\r\
If on, all sent CANopen PDO messages are echoed into terminal.\n\r\
Defaults to 'off'"
		},
		{
				CMD_SET_STATE,
				"setstate",
				"Usage: setstate <stopped/preop/op>\n\r\
Used to set the device's CANopen state machine to\n\r\
stopped, pre-operational or operational state."
		}
};

static void execute_common_cmd(int cmd, char** args);


void hal_terminal_init(const uw_command_st* commands, unsigned int count) {
	commands_ptr = commands;
	commands_count = count;
	int i;
	for (i = 0; i < TERMINAL_RECEIVE_BUFFER_SIZE; i++) {
		terminal_receive_buffer[i] = '\0';
	}
}



void hal_terminal_set_callback( void (*callback_function)(int cmd, char** args) ) {
	callback = callback_function;
}



int hal_terminal_get_commands_count(void) {
	return commands_count;
}



void __hal_terminal_process_rx_msg(char* data, uint8_t data_length, hal_stdout_sources_e source) {
	// if pointer to commands array is null, terminal is not initialized, so we do nothing
	if (!commands_ptr) {
		return;
	}
	// redirect printf to the source where message was received
	hal_stdout_set_source(source);

	//check for buffer overflow
	if (receive_buffer_index + data_length >= TERMINAL_RECEIVE_BUFFER_SIZE) {
		int i;
		for (i = 0; i < TERMINAL_RECEIVE_BUFFER_SIZE; i++) {
			terminal_receive_buffer[i] = '\0';
		}
		receive_buffer_index = 0;
		printf("\n\r\n\r**** Warning: Terminal buffer overflow ****\n\r\n\r");
	}

	//echo back received characters
	hal_stdout_send(data, data_length);

	// cycle trough received characters
	unsigned int i;
	for (i = 0; i < data_length; i++) {

		// if backspace was received, delete last saved character
		if (data[i] == 0x08) {
			if (receive_buffer_index > 0) {
				receive_buffer_index--;
			}
			terminal_receive_buffer[receive_buffer_index] = '\0';
			continue;
		}
		//if carriage return was received, read command, and clear buffer
		if (data[i] == 0x0D) {
			int i;
			int p = 0;
			//change line
			printf("\n");
			receive_buffer_index = 0;
			for (i = 0; i < TERMINAL_RECEIVE_BUFFER_SIZE - 1; i++) {
				// on '\0' command has ended
				if (terminal_receive_buffer[i] == '\0') {
					break;
				}
				// search for space (argument start)
				else if (terminal_receive_buffer[i] == ' ') {
					terminal_receive_buffer[i] = '\0';
					if (p < TERMINAL_ARG_COUNT) {
						args[p++] = &terminal_receive_buffer[i + 1];
					}
				}
			}

			if (callback) {
				// find out which command was received by looping trough them
				int p;
				bool match = false;
				for (p = 0; p < hal_terminal_get_commands_count(); p++) {
					if (strcmp(terminal_receive_buffer, commands_ptr[p].str) == 0) {
						callback(commands_ptr[p].id, args);
						match = true;
						break;
					}
				}
				// if no match was found, search common commands
				if (!match) {
					for (p = 0; p < sizeof(common_cmds) / sizeof(uw_command_st); p++) {
						if (strcmp(terminal_receive_buffer, common_cmds[p].str) == 0) {
							match = true;
							execute_common_cmd(common_cmds[p].id, args);
							break;
						}
					}
					if (!match) {
						printf("Command '%s' not found. Available commands:\n\r\n\r",
								terminal_receive_buffer);
						for (p = 0; p < sizeof(common_cmds) / sizeof(uw_command_st); p++) {
							printf("\"%s\"\n\r'%s'\n\r\n\r", common_cmds[p].str,
									common_cmds[p].instructions);
						}
						for (p = 0; p < hal_terminal_get_commands_count(); p++) {
							printf("\"%s\"\n\r'%s'\n\r\n\r", commands_ptr[p].str,
									commands_ptr[p].instructions);
						}
					}
				}
				// printf command prompt character after callback function has been executed
				printf(">");
			}

			for (i = 0; i < TERMINAL_RECEIVE_BUFFER_SIZE; i++) {
				terminal_receive_buffer[i] = '\0';
			}
			for (i = 0; i < TERMINAL_ARG_COUNT; i++) {
				args[i] = &terminal_receive_buffer[TERMINAL_RECEIVE_BUFFER_SIZE - 1];
			}
			continue;
		}

		//if questionmark was received, enter isp mode
		if (data[i] == '?' && !disable_isp_entry) {
			hal_enter_ISP_mode();
		}

		// add character to buffer
		terminal_receive_buffer[receive_buffer_index++] = data[i];
	}
}


void hal_terminal_disable_isp_entry(bool value) {
	disable_isp_entry = value;
}

static void execute_common_cmd(int cmd, char** args) {
	printf("Common command received\n\r");
}

