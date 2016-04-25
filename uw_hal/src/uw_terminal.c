/*
 * hal_debug.c
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */


#include "uw_terminal.h"


#include "uw_reset.h"
#include "uw_canopen.h"
#include <stdio.h>
#include <string.h>
#include "uw_utilities.h"
#include "uw_uart.h"
#include "uw_stdout.h"
#if CONFIG_TARGET_LPC11CXX
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC178X
#include "LPC177x_8x.h"
#endif
#if CONFIG_RTOS
#include "uw_rtos.h"
#endif


// extern declarations for uw_memory module's functions
extern uw_errors_e __uw_save_previous_non_volatile_data();
extern uw_errors_e __uw_load_previous_non_volatile_data();
extern uw_errors_e __uw_clear_previous_non_volatile_data();
extern void uw_enter_ISP_mode(void);


typedef struct {
	const uw_command_st *commands_ptr;
	uint8_t commands_count;
	bool disable_isp;
	uint8_t buffer_index;
	char buffer[CONFIG_TERMINAL_BUFFER_SIZE];
	void (*callback)(void *user_ptr, int cmd, char **args);

} this_st;

static this_st _this;
#define this (&_this)

static const uw_command_st common_cmds[] = {
		{
				.id = CMD_ISP,
				.str = "isp",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Sets the controller to enter ISP \n\r"
						"(In System Programming) mode instantly."
#else
						""
#endif
		},
		{
				.id = CMD_HELP,
				.str = "help",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"List's all available commands"
#else
						""
#endif
		},
		{
				.id = CMD_RESET,
				.str = "reset",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: reset (hard)"
						"Resets the controller instantly.\n\r"
						"All unsave modifications will be lost.\n\r"
						"If 'hard' is given as an argument, the system will use\n\r"
						"watchdog timer to make a hardware reset. Else Cortex-M0 software\n\r"
						"reset is done."
#else
						""
#endif
		},
		{
				.id = CMD_SAVE,
				.str = "save",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Saves application data and settings to non-volatile flash memory."
#else
						""
#endif
		},
		{
				.id = CMD_REVERT,
				.str = "revert",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Reverts all changes to factory defaults.\n\r"
						"The device need's to be restarted for changes to take effect.\n\r"
						"To undo revert, save current values to flash with 'save' command."
#else
						""
#endif
		},
		{
				.id = CMD_SDO,
				.str = "sdo",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: sdo <index> <subindex> <value>\n\r"
						"Used to read or write CANopen SDO object manually.\n\r"
						"All arguments are evaluated as 10-base integer values.\n\r"
						"To define them in 16-base hexadecimals, use '0x' prefix.\n\r"
						"To read an object, specify the index and subindex but leave value\n\r"
						"empty."
#else
						""
#endif
		},
		{
				.id = CMD_PDO_ECHO,
				.str = "pdoecho",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: pdoecho <on/off>\n\r"
						"If on, all sent CANopen PDO messages are echoed into terminal.\n\r"
						"Defaults to 'off'"
#else
						""
#endif
		},
		{
				.id = CMD_STATE,
				.str = "state",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: state <bootup/stopped/preop/op>\n\r"
						"Used to read / write the device's CANopen state machine to\n\r"
						"stopped, pre-operational or operational state."
#else
						""
#endif
		},
		{
				.id = CMD_SET_ISP,
				.str = "setisp",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: isp <on/off>\n\r"
						"Can be used to disable ISP entry when receiving '?' from the terminal."
						"ISP mode is still accessible with 'isp' command."
#else
						""
#endif
		}
};

static void execute_common_cmd(int cmd, char** args);


void uw_terminal_init(const uw_command_st* commands, unsigned int count,
		void (*callback_function)(void* user_ptr, int cmd, char** args)) {
	this->commands_ptr = commands;
	this->commands_count = count;
	this->callback = callback_function;
	this->buffer_index = 0;
	this->buffer[0] = '\0';
}



int uw_terminal_get_commands_count(void) {
	return this->commands_count;
}



uw_errors_e uw_terminal_step() {
	// if pointer to commands array is null, terminal is not initialized, so we do nothing
	if (!this->commands_ptr) {
		__uw_err_throw(ERR_NOT_INITIALIZED | HAL_MODULE_TERMINAL);
	}

	// cycle trough all received characters
	char data;
	char *args[CONFIG_TERMINAL_ARG_COUNT + 1];

	uw_errors_e e;
	while (true) {

		// redirect printf to the source where message was received
		if (!(e = uw_uart_get_char(UART0, &data))) {
			uw_stdout_set_source(STDOUT_UART0);
		}
		else {
//			uw_stdout_set_source(STDOUT_CAN);
			//todo: CAN message retrieval here
		}

		// getting an error means that no more data is available
		if (e) {
			break;
		}

		// check for buffer overflows
		if (this->buffer_index >= CONFIG_TERMINAL_BUFFER_SIZE) {
			this->buffer_index = 0;
			__uw_err_throw(ERR_BUFFER_OVERFLOW | HAL_MODULE_TERMINAL);
		}

		// echo back received characters
		uw_stdout_send(&data, 1);

		// escape clears the terminal
		if (data == 0x1B) {
			uw_stdout_send("\033[2K\r>", 6);
			this->buffer_index = 0;
			continue;
		}

		// if backspace was received, delete last saved character
		if (data == 0x08) {
			if (this->buffer_index > 0) {
				this->buffer_index--;
			}
			continue;
		}
		//if carriage return was received, read command and clear buffer
		if (data == 0x0D) {
			int i;
			int p = 0;
			//change line
			printf("\n\r");
			// do nothing if receive buffer was empty
			if (this->buffer_index == 0) {
				printf(">");
				continue;
			}
			//replace carriage return with a terminating mark
			this->buffer[this->buffer_index] = '\0';

			bool multi_arg = false;
			for (i = 0; i < this->buffer_index; i++) {
				// '"' mark means a multi-space argument

				// gargarg arg "arg arg"
				if (i) {
					if (this->buffer[i] == '"') {
						if (!multi_arg) {
							// multi-space argument start, edit arg starting with \0
							if (this->buffer[i - 1] == '\0') {
								this->buffer[i] = '\0';
								args[p - 1] = &this->buffer[i + 1];
								multi_arg = true;
							}
						}
						else {
							// multi-space argument end
							multi_arg = false;
							this->buffer[i] = '\0';
						}
					}
				}
				// search for space (argument start)
				if (!multi_arg && this->buffer[i] == ' ') {
					this->buffer[i] = '\0';
					if (p < CONFIG_TERMINAL_ARG_COUNT) {
						args[p++] = &this->buffer[i + 1];
					}
				}
			}
			while (p < CONFIG_TERMINAL_ARG_COUNT + 1) {
				args[p++] = &this->buffer[this->buffer_index];
			}
			this->buffer_index = 0;

			if (this->callback) {
				// find out which command was received by looping trough them
				int p;
				bool match = false;

				for (p = 0; p < uw_terminal_get_commands_count(); p++) {
					if (strcmp(this->buffer, this->commands_ptr[p].str) == 0) {
						this->callback(__uw_get_user_ptr(), this->commands_ptr[p].id, args);
						match = true;
						break;
					}
				}
				// if no match was found, search common commands
				if (!match) {
					for (p = 0; p < sizeof(common_cmds) / sizeof(uw_command_st); p++) {
						if (strcmp(this->buffer, common_cmds[p].str) == 0) {
							match = true;
							execute_common_cmd(common_cmds[p].id, args);
							break;
						}
					}
					if (!match) {
						printf("Command '%s' not found\n\r",
								this->buffer);
					}
				}
				// printf command prompt character after callback function has been executed
				printf(">");
			}
			continue;
		}

		//if questionmark was received, enter isp mode
		if (data == '?' && !this->disable_isp) {
			uw_enter_ISP_mode();
		}

		// add character to buffer
		this->buffer[this->buffer_index++] = data;
	}
	return uw_err(ERR_NONE);
}


void uw_terminal_disable_isp_entry(bool value) {
	this->disable_isp = value;
}

static void execute_common_cmd(int cmd, char** args) {
	uw_canopen_node_states_e state;
	uint8_t p;

	switch (cmd) {
	case CMD_ISP:
		uw_enter_ISP_mode();
		break;
	case CMD_HELP:
		printf("\n\r");
		for (p = 0; p < sizeof(common_cmds) / sizeof(uw_command_st); p++) {
#if CONFIG_TERMINAL_INSTRUCTIONS
			printf("\"%s\"\n\r%s\n\r\n\r", common_cmds[p].str,
					common_cmds[p].instructions);
#else
			printf("\"%s\"\n\r", common_cmds[p].str);
#endif
		}
		for (p = 0; p < uw_terminal_get_commands_count(); p++) {
#if CONFIG_TERMINAL_INSTRUCTIONS
			printf("\"%s\"\n\r%s\n\r\n\r", this->commands_ptr[p].str,
					this->commands_ptr[p].instructions);
#else
			printf("\"%s\"\n\r", this->commands_ptr[p].str);
#endif
		}
		printf("\n\r");
		break;
	case CMD_SDO:
		printf("Command not yet implemented in HAL.\n\r");
		break;
	case CMD_PDO_ECHO:
		printf("Command not yet implemented in HAL.\n\r");
		break;
	case CMD_RESET:
		if (strcmp(args[0], "hard") == 0) {
			uw_system_reset(true);
		}
		else {
			uw_system_reset(false);
		}
		break;
	case CMD_REVERT:
		__uw_clear_previous_non_volatile_data();
		printf("OK\n\r");
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
			switch (uw_canopen_get_state()) {
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
		uw_canopen_set_state(state);
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
			printf("isp %s\n\r", this->disable_isp ? "off" : "on");
		}
		break;
	default:
		break;
	}
}

