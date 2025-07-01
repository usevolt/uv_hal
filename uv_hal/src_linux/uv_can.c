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

#include "uv_can.h"

#if CONFIG_CAN

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#include "uv_gpio.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_terminal.h"
#include "uv_memory.h"
#include "uv_uart.h"
#endif
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#endif
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <linux/if_link.h>


#if !defined(PRINT)
#define PRINT(...) printf(__VA_ARGS__)
#endif

#if CONFIG_CAN_LOG
extern bool can_log;
#endif

#define DEV_COUNT_MAX			30

typedef enum {
	CAN_STATE_INIT = 0,
	CAN_STATE_OPEN,
	CAN_STATE_FAULT
} can_state_e;

typedef struct {
	can_state_e state;
	unsigned int baudrate;
	// can dev socket
	int soc;
	char dev[32];
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	struct timeval lastrxtime;

	// list of available CAN devices
	char devs[DEV_COUNT_MAX][32];
	// count of CAN devs
	int32_t dev_count;

	bool (*rx_callback)(void *user_ptr, uv_can_msg_st *msg);
	bool (*tx_callb)(void *user_Ptr, uv_can_msg_st *msg);

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif

} can_st;


static can_st _can = {
		.state = CAN_STATE_INIT,
		.baudrate = 250000,
		.dev = "can0",
		.dev_count = 0,
		.soc = -1,
		.rx_callback = NULL,
		.tx_callb = NULL
};
#define this (&_can)


void _uv_can_hal_send(uv_can_channels_e chn);
static bool cclose(void);
static bool copen(void);

char *uv_can_get_device_name(int32_t i) {
	if (i < this->dev_count) {
		return this->devs[i];
	}
	else {
		return NULL;
	}
}

/// @brief: Returns the count of found CAN interface devices
int32_t uv_can_get_device_count(void) {
	return this->dev_count;
}


void uv_can_set_dev(uv_can_channels_e can_dev) {
	strcpy(this->dev, can_dev);
}


uv_can_channels_e uv_can_get_dev(void) {
	return (uv_can_channels_e) this->dev;
}



/// @brief: Opens a socket to a SocketCAN device
static bool copen(void) {
	bool ret = true;
	struct ifreq ifr;
	struct sockaddr_can addr;

	if (this->state != CAN_STATE_OPEN) {
		cclose();
	}

	/* open socket */
	this->soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(this->soc < 0) {
		PRINT("Opening the socket failed with error code %i\n", this->soc);
		ret = false;
	}
	else {
		addr.can_family = AF_CAN;
		strcpy(ifr.ifr_name, this->dev);

		if (ioctl(this->soc, SIOCGIFINDEX, &ifr) < 0) {
			PRINT("ioctl failed, CAN bus not available.\n");
			ret = false;
		}
		else {
			addr.can_ifindex = ifr.ifr_ifindex;

			if (bind(this->soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				PRINT("Binding to the CAN socket failed\n");
				ret = false;
			}
			else {
				can_err_mask_t err_mask = ( CAN_ERR_MASK );
				setsockopt(this->soc, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
						&err_mask, sizeof(err_mask));


				PRINT("CAN socket opened to device %s, fd: %i\n",
						this->dev, this->soc);
				this->state = CAN_STATE_OPEN;
			}
		}
	}

	return ret;
}


/// @brief: Closes the socket to the SocketCAN device
static bool cclose(void) {
	bool ret = true;

	if (this->state == CAN_STATE_OPEN) {
		close(this->soc);
		PRINT("Socket closed.\n");
	}

	this->state = CAN_STATE_INIT;
	return ret;
}

void uv_can_close(void) {
	cclose();
}

uv_errors_e uv_can_add_tx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	this->tx_callb = callback_function;
}


#if CONFIG_TARGET_LINUX

char *uv_can_set_up(bool force_set_up) {
	char *ret = NULL;
	char cmd[128];
	int current_baud = this->baudrate;

	// get the net dev baudrate. If dev was not available, baudrate will be 0.
	sprintf(cmd, "ip -det link show %s | grep bitrate | awk '{print $2}'", this->dev);
	FILE *fp = popen(cmd, "r");
	if (fgets(cmd, sizeof(cmd), fp)) {
		current_baud = strtol(cmd, NULL, 0);
	}
	pclose(fp);
	// If the net dev is not UP, force it to be set up by setting baudrate to incorrect value.
	sprintf(cmd, "ip link show %s | grep state | awk '{print $9}'", this->dev);
	fp = popen(cmd, "r");
	if (fgets(cmd, sizeof(cmd), fp)) {
		if (strstr(cmd, "UP") != NULL) {
			current_baud = this->baudrate;
		}
		else if (strstr(cmd, "UNKNOWN") != NULL) {
			// virtual CAN bus can be in UNKNOWN state. Baudrate settings don't apply
			// on virtual busses, thus keep the netdev open
			this->baudrate = current_baud;
		}
		else {
			// Mark the baudrate to be set by closing and reopening the netdev connection
			current_baud = -1;
		}
	}
	pclose(fp);

	if (this->baudrate != current_baud ||
			force_set_up) {
		if (this->state == CAN_STATE_OPEN) {
			// close the socket if one is open
			cclose();
		}
		// since baudrate is not what we want, we need to set network dev down
		sprintf(cmd, "sudo ip link set dev %s down", this->dev);
		PRINT("%s\n", cmd);
		if (system(cmd));

		// set the net dev up and configure all parameters
		sprintf(cmd, "sudo ip link set %s type can bitrate %u", this->dev, this->baudrate);
		PRINT("%s\n", cmd);
		if (system(cmd));
		sprintf(cmd, "sudo ip link set %s txqueuelen 1000", this->dev);
		PRINT("%s\n", cmd);
		if (system(cmd));
		sprintf(cmd, "sudo ip link set dev %s up", this->dev);
		PRINT("%s\n", cmd);
		if (system(cmd));
	}

	/* open socket */
	if (!copen()) {
		ret = "Couldn't open the connection to the CAN network.";
		PRINT("%s\n", ret);
		this->state = CAN_STATE_FAULT;
	}

	return ret;
}


bool uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate) {
	bool ret = true;
	this->baudrate = baudrate;
	strcpy(this->dev, channel);

	// find out the names of network interfaces
	this->dev_count = 0;
	struct if_nameindex *ind, *indd = 0;
	ind = indd = if_nameindex();
	while(ind != NULL && ind->if_name != NULL) {
		strcpy(this->devs[this->dev_count++], ind->if_name);
		ind++;
	}
	if_freenameindex(indd);

	return ret;
}


unsigned int uv_can_get_baudrate(uv_can_channels_e channel) {
	return this->baudrate;
}


struct timeval uv_can_get_rx_time(void) {
	return this->lastrxtime;
}

bool uv_can_is_connected(void) {
	return (this->state == CAN_STATE_OPEN);
}


#endif


uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {

	// SocketCAN doesn't support message filtering, this function does nothing.

	return ERR_NONE;
}


uv_errors_e _uv_can_init() {
	uv_errors_e ret = ERR_NONE;

	// CAN is already initialized. Connection will be opened when baudrate is set.
	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			sizeof(this->rx_buffer_data) / sizeof(this->rx_buffer_data[0]),
			sizeof(this->rx_buffer_data[0]));

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data,
			CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CAN0, UV_TERMINAL_CAN_RX_ID + uv_get_id(),
			CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
#endif
	this->rx_callback = NULL;

	if (this->baudrate == 0) {
		// if the baudrate was not yet set, set it. This causes the SocketCAN to be opened
		uv_can_set_baudrate(this->dev, this->baudrate);
	}

	return ret;
}



#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif


uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	this->rx_callback = callback_function;

	return ERR_NONE;
}



static uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	uv_errors_e ret = ERR_NONE;

	if (this->state == CAN_STATE_INIT) {
		uv_can_set_up(false);
	}

	if (this->state == CAN_STATE_OPEN) {
		int retval;
		struct can_frame frame;
		frame.can_id = message->id | ((message->type == CAN_EXT) ? CAN_EFF_FLAG : 0);
		frame.can_dlc = message->data_length;
		memcpy(frame.data, message->data_8bit, message->data_length);

		retval = write(this->soc, &frame, sizeof(struct can_frame));
		if (retval != sizeof(struct can_frame) && errno == ENETDOWN) {
			// try to set the net dev up
			uv_can_set_up(false);
			retval = write(this->soc, &frame, sizeof(struct can_frame));
		}
		else if (errno &&
				errno != EINTR &&
				errno != EAGAIN &&
				errno != ENOENT) {
			PRINT("Sending a message with ID of 0x%x resulted in a CAN error: %u , %s***\n",
					message->id, errno, strerror(errno));
			ret = ERR_HARDWARE_NOT_SUPPORTED;
		}
		else {

		}
	}

	return ret;
}



uv_errors_e uv_can_send_flags(uv_can_channels_e chn, uv_can_msg_st *msg,
		can_send_flags_e flags) {
	uv_errors_e ret = ERR_NONE;
	uv_disable_int();
	if (flags & CAN_SEND_LOCAL) {
		ret = uv_ring_buffer_push(&this->rx_buffer, msg);
	}
	if ((flags & CAN_SEND_FLAGS_LOCAL) ||
			(flags & CAN_SEND_FLAGS_SYNC)) {
		ret = uv_can_send_message(chn, msg);
	}
	if (ret == ERR_NONE &&
			!(flags & CAN_SEND_FLAGS_NO_TX_CALLB)) {
		this->tx_callb(__uv_get_user_ptr(), msg);
	}

	uv_enable_int();
	return ret;
}


uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {
	uv_errors_e ret = uv_ring_buffer_pop(&this->rx_buffer, message);
	return ret;
}



uv_errors_e uv_can_reset(uv_can_channels_e channel) {
	return ERR_NONE;
}



/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {
	if (this->state == CAN_STATE_OPEN) {
		bool go = true;
		while (go) {
			struct can_frame frame_rd;
			int recvbytes = 0;

			struct timeval timeout = {0, 0};
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(this->soc, &readSet);

			go = false;

			if (select((this->soc + 1), &readSet, NULL, NULL, &timeout) >= 0) {
				if (FD_ISSET(this->soc, &readSet)) {
					recvbytes = read(this->soc, &frame_rd, sizeof(struct can_frame));

					if(recvbytes > 0) {
						uv_can_msg_st msg;
						if (frame_rd.can_id & CAN_ERR_FLAG) {
							// error frame
							msg.id = frame_rd.can_id & CAN_ERR_MASK;
							msg.type = CAN_ERR;
						}
						else if (frame_rd.can_id & CAN_EFF_FLAG) {
							// extended frame
							msg.id = frame_rd.can_id & CAN_EFF_MASK;
							msg.type = CAN_EXT;
						}
						else {
							// standard frame
							msg.id = frame_rd.can_id & CAN_SFF_MASK;
							msg.type = CAN_STD;
						}
						msg.data_length = frame_rd.can_dlc;
						memcpy(msg.data_8bit, frame_rd.data, msg.data_length);

						if (this->rx_callback != NULL &&
								!this->rx_callback(__uv_get_user_ptr(), &msg)) {

						}
						else {
#if CONFIG_TERMINAL_CAN
							// terminal characters are sent to their specific buffer
							if (msg.id == UV_TERMINAL_CAN_RX_ID + uv_canopen_get_our_nodeid() &&
									msg.type == CAN_STD &&
									msg.data_8bit[0] == 0x22 &&
									msg.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
									msg.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
									msg.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
									msg.data_length > 4) {
								uint8_t i;
								for (i = 0; i < msg.data_length - 4; i++) {
									uv_ring_buffer_push(&this->char_buffer,
											(char*) &msg.data_8bit[4 + i]);
								}
							}
							else {
#endif
								if (uv_ring_buffer_push(&this->rx_buffer, &msg) != ERR_NONE) {
									PRINT("** CAN RX buffer full**\n");
									fflush(stdout);
								}
#if CONFIG_TERMINAL_CAN
							}
#endif
						}


						go = true;
#if defined(SIOCGSTAMP)
						ioctl(this->soc, SIOCGSTAMP, &this->lastrxtime);
#else
						ioctl(this->soc, SIOCGSTAMP_OLD, &this->lastrxtime);
#endif
					}
					else if (recvbytes == -1) {
						PRINT("*** CAN RX error: %u , %s***\n",
								errno, strerror(errno));
						if (errno == ENETDOWN) {
							PRINT("The network is down. Initialize the network with command:\n\n"
									"sudo ip link set CHANNEL type can bitrate BAUDRATE txqueuelen 1000\n\n"
									"And open the network with command:\n\n"
									"sudo ip link set dev CHANNEL up\n\n"
									"After that you can communicate with the device.\n");
							// exit the program to prevent further error messages
							exit(0);
						}
					}
				}
			}
		}
	}
}


void uv_can_clear_rx_buffer(uv_can_channels_e channel) {
	uv_ring_buffer_clear(&this->rx_buffer);
}



uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	if (channel) {};
	uv_can_errors_e e = CAN_ERROR_ACTIVE;

	if (this->state == CAN_STATE_INIT ||
			this->state == CAN_STATE_FAULT) {
		e = CAN_ERROR_BUS_OFF;
	}
	return e;
}


void uv_can_clear_rx_messages(uv_can_chn_e chn) {
	if (chn);
}



#endif
