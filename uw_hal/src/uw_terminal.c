/*
 * hal_debug.c
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */


#include "uw_reset.h"
#include "uw_canopen.h"
#include <stdio.h>
#include <string.h>
#include "uw_memory.h"
#include "uw_terminal.h"
#include "uw_utilities.h"

static const uw_command_st* commands_ptr = 0;
static int commands_count = 0;
static void (*callback)(void* user_ptr, int cmd, char** args) = 0;

/// whole command receive buffer
static char terminal_receive_buffer[TERMINAL_RECEIVE_BUFFER_SIZE];
/// argument pointers
static char* args[TERMINAL_ARG_COUNT];
static uint8_t receive_buffer_index = 0;
static bool disable_isp_entry = false;
static const uw_command_st common_cmds[] = {
		{
				.id = CMD_ISP,
				.str = "isp",
				.instructions = "Sets the controller to enter ISP \n\r\
(In System Programming) mode instantly."
		},
		{
				.id = CMD_HELP,
				.str = "help",
				.instructions = "List's all available commands"
		},
		{
				.id = CMD_RESET,
				.str = "reset",
				.instructions = "Resets the controller instantly.\n\r\
All unsave modifications will be lost.\n\r\
If 'hard' is given as an argument, the system will use\n\r\
watchdog timer to make a hardware reset. Else Cortex-M0 software\n\r\
reset is done."
		},
		{
				.id = CMD_SAVE,
				.str = "save",
				.instructions = "Saves application data and settings to non-volatile memory."
		},
		{
				.id = CMD_REVERT,
				.str = "revert",
				.instructions = "Reverts all changes to factory defaults.\n\r\
The device need's to be restarted for changes to take effect.\n\r\
To undo revert, save current values to flash with 'save' command."
		},
		{
				.id = CMD_SDO,
				.str = "sdo",
				.instructions = "Usage: sdo <index> <subindex> <value>\n\r\
Used to read or write CANopen SDO object manually.\n\r\
All arguments are evaluated as 10-base integer values.\n\r\
To define them in 16-base hexadecimals, use '0x' prefix.\n\r\
To read an object, specify the index and subindex but leave value\n\r\
empty."
		},
		{
				.id = CMD_PDO_ECHO,
				.str = "pdoecho",
				.instructions = "Usage: pdoecho <on/off>\n\r\
If on, all sent CANopen PDO messages are echoed into terminal.\n\r\
Defaults to 'off'"
		},
		{
				.id = CMD_STATE,
				.str = "state",
				.instructions = "Usage: state <bootup/stopped/preop/op>\n\r\
Used to read / write the device's CANopen state machine to\n\r\
stopped, pre-operational or operational state."
		},
		{
				.id = CMD_SET_ISP,
				.str = "setisp",
				.instructions = "Usage: isp <on/off>\n\r\
Can be used to disable ISP entry when receiving '?' from the terminal."
		}
};

static void execute_common_cmd(int cmd, char** args);


void uw_terminal_init(const uw_command_st* commands, unsigned int count,
		void (*callback_function)(void* user_ptr, int cmd, char** args)) {
	commands_ptr = commands;
	commands_count = count;
	callback = callback_function;
	int i;
	for (i = 0; i < TERMINAL_RECEIVE_BUFFER_SIZE; i++) {
		terminal_receive_buffer[i] = '\0';
	}
}




int uw_terminal_get_commands_count(void) {
	return commands_count;
}



void __uw_terminal_process_rx_msg(char* data, uint8_t data_length, uw_stdout_sources_e source) {
	// if pointer to commands array is null, terminal is not initialized, so we do nothing
	if (!commands_ptr) {
		return;
	}
	// redirect printf to the source where message was received
	uw_stdout_set_source(source);

	//check for buffer overflow
	if (receive_buffer_index + data_length >= TERMINAL_RECEIVE_BUFFER_SIZE) {
		int i;
		for (i = 0; i < TERMINAL_RECEIVE_BUFFER_SIZE; i++) {
			terminal_receive_buffer[i] = '\0';
		}
		receive_buffer_index = 0;
		printf("\n\r\n\r**** Warning: Terminal buffer overflow ****\n\r\n\r");
	}

	// cycle trough received characters
	unsigned int i;
	for (i = 0; i < data_length; i++) {

		// echo back received characters
		uw_stdout_send(&data[i], 1);

		// escape clears the terminal
		if (data[i] == 0x1B) {
			uw_stdout_send("\033[2K\r>", 6);
			receive_buffer_index = 0;
			continue;
		}

		// if backspace was received, delete last saved character
		if (data[i] == 0x08) {
			if (receive_buffer_index > 0) {
				receive_buffer_index--;
			}
			continue;
		}
		//if carriage return was received, read command and clear buffer
		if (data[i] == 0x0D) {
			int i;
			int p = 0;
			//change line
			printf("\n\r");
			// do nothing if receive buffer was empty
			if (receive_buffer_index == 0) {
				printf(">");
				return;
			}
			//replace carriage return with a terminating pointer
			terminal_receive_buffer[receive_buffer_index] = '\0';
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
				for (p = 0; p < uw_terminal_get_commands_count(); p++) {
					if (strcmp(terminal_receive_buffer, commands_ptr[p].str) == 0) {
						callback(__uw_get_user_ptr(), commands_ptr[p].id, args);
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
						printf("Command '%s' not found\n\r",
								terminal_receive_buffer);
					}
				}
				// printf command prompt character after callback function has been executed
				printf(">");
			}

			for (i = 0; i < TERMINAL_ARG_COUNT; i++) {
				args[i] = &terminal_receive_buffer[TERMINAL_RECEIVE_BUFFER_SIZE - 1];
			}
			continue;
		}

		//if questionmark was received, enter isp mode
		if (data[i] == '?' && !disable_isp_entry) {
			uw_enter_ISP_mode();
		}

		// add character to buffer
		terminal_receive_buffer[receive_buffer_index++] = data[i];
	}
}


void uw_terminal_disable_isp_entry(bool value) {
	disable_isp_entry = value;
}

static void execute_common_cmd(int cmd, char** args) {
	uw_canopen_node_states_e state;
	uint8_t p;

	switch (cmd) {
	case CMD_ISP:
		uw_enter_ISP_mode();
		break;
	case CMD_HELP:
		for (p = 0; p < sizeof(common_cmds) / sizeof(uw_command_st); p++) {
			printf("\"%s\"\n\r%s\n\r\n\r", common_cmds[p].str,
					common_cmds[p].instructions);
		}
		for (p = 0; p < uw_terminal_get_commands_count(); p++) {
			printf("\"%s\"\n\r%s\n\r\n\r", commands_ptr[p].str,
					commands_ptr[p].instructions);
		}
		break;
	case CMD_SDO:
		printf("Command not yet implemented in HAL.\n\r");
		break;
	case CMD_PDO_ECHO:
		printf("Command not yet implemented in HAL.\n\r");
		break;
	case CMD_RESET:
		if (strcmp(args[0], "hard") == 0) {
			hal_system_reset(true);
		}
		else {
			hal_system_reset(false);
		}
		break;
	case CMD_REVERT:
		printf("%u\n\r", __uw_clear_previous_non_volatile_data());
		break;
	case CMD_SAVE:
		if (__uw_save_previous_non_volatile_data()) {
			printf("Saved.\n\r");
		}
		break;
	case CMD_STATE:
		if (strcmp(args[0], "stopped") == 0) {
			state = STATE_STOPPED;
		}
		else if (strcmp(args[0], "bootup") == 0) {
			state = STATE_BOOT_UP;
		}
		else if (strcmp(args[0], "preop") == 0) {
			state = STATE_PREOPERATIONAL;
		}
		else if (strcmp(args[0], "op") == 0) {
			state = STATE_OPERATIONAL;
		}
		else {
			char * str;
			switch (__uw_canopen_get_state()) {
			case STATE_BOOT_UP:
				str = "bootup";
				break;
			case STATE_OPERATIONAL:
				str = "operational";
				break;
			case STATE_PREOPERATIONAL:
				str = "preoperational";
				break;
			case STATE_STOPPED:
				str = "stopped";
				break;
			default:
				str = "unknown";
				break;
			}
			printf("%s\n\r", str);
			break;
		}
		__uw_canopen_set_state(state);
		printf("State: %s.\n\r", args[0]);
		break;
	case CMD_SET_ISP:
		if (strcmp(args[0], "on") == 0) {
			uw_terminal_disable_isp_entry(false);
			printf("isp on\n\r");
		}
		else if (strcmp(args[0], "off") == 0) {
			uw_terminal_disable_isp_entry(true);
			printf("isp off\n\r");
		}
		else {
			printf("isp %s\n\r", disable_isp_entry ? "off" : "on");
		}
		break;
	default:
		break;
	}
}

