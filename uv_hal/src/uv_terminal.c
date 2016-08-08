/*
 * hal_debug.c
 *
 *  Created on: Nov 13, 2015
 *      Author: usevolt
 */


#include "uv_terminal.h"


#include "uv_reset.h"
#include "uv_canopen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "uv_utilities.h"
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


#if CONFIG_CAN_LOG
// enabled can logging
bool can_log = true;
#endif

#if CONFIG_CANOPEN_LOG
// enables canopen logging
bool canopen_log = true;
#endif


#define ECHO(x...)				x
#define _ARG(x)					ECHO(, argv[x])
#define REPEAT_ARGS()			REPEAT(CONFIG_TERMINAL_ARG_COUNT, _ARG)


// extern declarations for uv_memory module's functions
extern uv_errors_e __uv_save_previous_non_volatile_data();
extern uv_errors_e __uv_load_previous_non_volatile_data();
extern uv_errors_e __uv_clear_previous_non_volatile_data();
extern void uv_enter_ISP_mode(void);


typedef struct {
	const uv_command_st *commands_ptr;
	uint8_t commands_count;
	bool disable_isp;
	uint8_t buffer_index;
	char buffer[CONFIG_TERMINAL_BUFFER_SIZE];
} this_st;

static this_st _this;
#define this (&_this)

void uv_terminal_isp_callb(void *me, unsigned int cmd, unsigned int args, ...);
void uv_terminal_help_callb(void *me, unsigned int cmd, unsigned int args, ...);
#if CONFIG_TERMINAL_INSTRUCTIONS
void uv_terminal_man_callb(void *me, unsigned int cmd, unsigned int args, ...);
#endif
void uv_terminal_reset_callb(void *me, unsigned int cmd, unsigned int args, ...);
#if CONFIG_NON_VOLATILE_MEMORY
void uv_terminal_save_callb(void *me, unsigned int cmd, unsigned int args, ...);
void uv_terminal_revert_callb(void *me, unsigned int cmd, unsigned int args, ...);
#endif
void uv_terminal_ispenable_callb(void *me, unsigned int cmd, unsigned int args, ...);

static const uv_command_st common_cmds[] = {
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
				, .callback = uv_terminal_isp_callb
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
				, .callback = uv_terminal_help_callb
		},
#if CONFIG_TERMINAL_INSTRUCTIONS
		{
				.id = CMD_MAN,
				.str = "man",
				.instructions =
						"Gives information from the command. Give the command name as the argument.\n\r"
						"Usage: man \"<command_name>\""
				, .callback = uv_terminal_man_callb
		},
#endif
		{
				.id = CMD_RESET,
				.str = "reset",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: reset (1/0)"
						"Resets the controller instantly.\n\r"
						"All unsave modifications will be lost.\n\r"
						"If 1 is given as an argument, the system will use\n\r"
						"watchdog timer to make a hardware reset. Else software\n\r"
						"reset is done."
#else
						""
#endif
				, .callback = uv_terminal_reset_callb
		},
#if CONFIG_NON_VOLATILE_MEMORY
		{
				.id = CMD_SAVE,
				.str = "save",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Saves application data and settings to non-volatile flash memory."
#else
						""
#endif
				, .callback = uv_terminal_save_callb
		},
		{
				.id = CMD_REVERT,
				.str = "revert",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Reverts all changes to factory defaults.\n\r"
						"The device needs to be restarted for changes to take effect.\n\r"
						"To undo revert, save current values to flash with 'save' command."
#else
						""
#endif
				, .callback = uv_terminal_revert_callb
		},
#endif
		{
				.id = CMD_SET_ISP,
				.str = "ispenable",
				.instructions =
#if CONFIG_TERMINAL_INSTRUCTIONS
						"Usage: ispenable <on/off>\n\r"
						"Can be used to disable ISP entry when receiving '?' from the terminal."
						"ISP mode is still accessible with 'isp' command."
#else
						""
#endif
				, .callback = uv_terminal_ispenable_callb
		}
};


void uv_terminal_init(const uv_command_st* commands, unsigned int count) {
	this->commands_ptr = commands;
	this->commands_count = count;
	this->buffer_index = 0;
	this->buffer[0] = '\0';
}



int uv_terminal_get_commands_count(void) {
	return this->commands_count;
}



uv_errors_e uv_terminal_step() {
	// if pointer to commands array is null, terminal is not initialized, so we do nothing
	if (!this->commands_ptr) {
		__uv_err_throw(ERR_NOT_INITIALIZED | HAL_MODULE_TERMINAL);
	}

	// cycle trough all received characters
	char data;
	// arguments can be either integers or strings
	void *argv[CONFIG_TERMINAL_ARG_COUNT + 1];

	uv_errors_e e;
	while (true) {

		// redirect printf to the source where message was received
		if (!(e = uv_uart_get_char(UART0, &data))) {
			uv_stdout_set_source(STDOUT_UART0);
		}
		else {
//			uv_stdout_set_source(STDOUT_CAN);
			//todo: CAN message retrieval here
		}

		// getting an error means that no more data is available
		if (e) {
			break;
		}

		// check for buffer overflows
		if (this->buffer_index >= CONFIG_TERMINAL_BUFFER_SIZE) {
			this->buffer_index = 0;
			__uv_err_throw(ERR_BUFFER_OVERFLOW | HAL_MODULE_TERMINAL);
		}

		// echo back received characters
		uv_stdout_send(&data, 1);

		// escape clears the terminal
		if (data == 0x1B) {
			uv_stdout_send("\033[2K\r>", 6);
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
		if (data == 0x0D || data == 0x0A) {
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

			bool string_arg = false;
			for (i = 0; i < this->buffer_index; i++) {
				// '"' mark means a string argument (multi-space argument if dedicated callback are not used)
				if (i) {
					if (this->buffer[i] == '"') {
						if (!string_arg) {
							// multi-space argument start, edit arg starting with \0
							if (this->buffer[i - 1] == '\0') {
								this->buffer[i] = '\0';
								argv[p - 1] = &this->buffer[i + 1];
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
						argv[p] = (void*) atoi(&this->buffer[i + 1]);
						p++;
					}
				}
			}
			this->buffer_index = 0;

			// find out which command was received by looping trough them
			int q;
			bool match = false;

			for (q = 0; q < uv_terminal_get_commands_count(); q++) {
				if (strcmp(this->buffer, this->commands_ptr[q].str) == 0) {
					if (this->commands_ptr[q].callback) {
						this->commands_ptr[q].callback(__uv_get_user_ptr(),
								this->commands_ptr[q].id, p REPEAT_ARGS());
					}
					match = true;
					break;
				}
			}
			// if no match was found, search common commands
			if (!match) {
				for (q = 0; q < sizeof(common_cmds) / sizeof(uv_command_st); q++) {
					if (strcmp(this->buffer, common_cmds[q].str) == 0) {
						match = true;
						common_cmds[q].callback(__uv_get_user_ptr(), common_cmds[q].id, p REPEAT_ARGS());
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

			continue;
		}

		//if questionmark was received, enter isp mode
		if (data == '?' && !this->disable_isp) {
			uv_enter_ISP_mode();
		}

		// add character to buffer
		this->buffer[this->buffer_index++] = data;
	}
	return uv_err(ERR_NONE);
}


void uv_terminal_disable_isp_entry(bool value) {
	this->disable_isp = value;
}

void uv_terminal_isp_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	uv_enter_ISP_mode();
}
void uv_terminal_help_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	printf("\n\r");
	int p;
	for (p = 0; p < sizeof(common_cmds) / sizeof(uv_command_st); p++) {
		printf("\"%s\"\n\r", common_cmds[p].str);
	}
	for (p = 0; p < uv_terminal_get_commands_count(); p++) {
		printf("\"%s\"\n\r", this->commands_ptr[p].str);
	}
}
#if CONFIG_TERMINAL_INSTRUCTIONS
void uv_terminal_man_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	va_list l;
	va_start(l, args);
	char *str = va_arg(l, char*);
	va_end(l);
	int i;
	for (i = 0; i < this->commands_count; i++) {
		if (strcmp(str, this->commands_ptr[i].str) == 0) {
			printf("\n\r%s:\n\r%s\n\r\n\r", this->commands_ptr[i].str, this->commands_ptr[i].instructions);
			return;
		}
	}
	for (i = 0; i < sizeof(common_cmds) / sizeof(uv_command_st); i++) {
		if (strcmp(str, common_cmds[i].str) == 0) {
			printf("\n\r%s:\n\r%s\n\r\n\r", common_cmds[i].str, common_cmds[i].instructions);
			return;
		}
	}
	printf("Give a command name as a string argument\n\r");
}
#endif
void uv_terminal_reset_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	if (!args) {
		uv_system_reset(false);
	}
	va_list l;
	va_start(l, args);
	if (va_arg(l, int)) {
		uv_system_reset(true);
	}
	else {
		uv_system_reset(false);
	}
	va_end(l);

}
#if CONFIG_NON_VOLATILE_MEMORY
void uv_terminal_save_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	if (!__uv_save_previous_non_volatile_data()) {
		printf("saved\n\r");
	}
}
void uv_terminal_revert_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	if (!__uv_load_previous_non_volatile_data()) {
		printf("reverted\n\r");
	}
}
#endif
void uv_terminal_ispenable_callb(void *me, unsigned int cmd, unsigned int args, ...) {
	if (!args) {
		printf("isp: %u\n\r", !this->disable_isp);
	}
	else {
		va_list(l);
		va_start(l, args);
		int i = va_arg(l, int);
		printf("%x\n\r", i);
		this->disable_isp = !(i);
		va_end(l);
	}
}



bool uv_terminal_parse_bool(char *arg) {
	if (strcmp(arg, "on") == 0 || strcmp(arg, "true") == 0 || strcmp(arg, "1") == 0) {
		return true;
	}
	return false;
}
