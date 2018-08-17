/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/



/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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

#if CONFIG_CAN_LOG
extern bool can_log;
#endif

typedef struct {
	bool connection;
	unsigned int baudrate;
	// can dev socket
	int soc;
	char dev[32];
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	struct timeval lastrxtime;

} can_st;


static can_st _can = {
		.connection = false,
		.baudrate = CONFIG_CAN0_BAUDRATE,
		.dev = "can0"
};
#define this (&_can)


void _uv_can_hal_send(uv_can_channels_e chn);
static bool cclose(void);
static bool copen(void);



/// @brief: Opens a connection to a SocketCAN device
static bool copen(void) {
	bool ret = true;

	if (this->connection) {
		cclose();
	}

	struct ifreq ifr;
	struct sockaddr_can addr;

	/* open socket */
	this->soc = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(this->soc < 0) {
		printf("Opening the socket failed with error code %i\n", this->soc);
		ret = false;
	}
	else {
		addr.can_family = AF_CAN;
		strcpy(ifr.ifr_name, this->dev);

		if (ioctl(this->soc, SIOCGIFINDEX, &ifr) < 0) {
			printf("ioctl failed, CAN bus not available.\n");
			ret = false;
		}
		else {
			addr.can_ifindex = ifr.ifr_ifindex;

			// no need to set the socket to non-blocking, at least for now...
			// fcntl(this->soc, F_SETFL, O_NONBLOCK);

			if (bind(this->soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				printf("Binding to the CAN socket failed\n");
				ret = false;
			}
			else {
				can_err_mask_t err_mask = ( CAN_ERR_MASK );
				setsockopt(this->soc, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
						&err_mask, sizeof(err_mask));
				printf("CAN connection opened to device %s\n", this->dev);
				this->connection = true;
			}
		}
	}

	return ret;
}


/// @brief: Closes the connection to the SocketCAN device
static bool cclose(void) {
	bool ret = true;

	if (this->connection) {
		close(this->soc);
		this->connection = false;
		printf("Connection closed.\n");
	}

	this->connection = false;
	return ret;
}


#if CONFIG_TARGET_LINUX

bool uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate) {
	bool ret = true;
	if (this->connection) {
		cclose();
	}
	strcpy(this->dev, channel);
	this->baudrate = baudrate;
	// open the connection
	copen();

	return ret;
}

void uv_can_deinit(void) {
	if (this->connection) {
		cclose();
	}
}


struct timeval uv_can_get_rx_time(void) {
	return this->lastrxtime;
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

	return ret;
}



#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif



uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	uv_errors_e ret = ERR_NONE;

	if (this->connection) {
		int retval;
		struct can_frame frame;
		frame.can_id = message->id | ((message->type == CAN_EXT) ? CAN_EFF_FLAG : 0);
		frame.can_dlc = message->data_length;
		memcpy(frame.data, message->data_8bit, message->data_length);

		retval = write(this->soc, &frame, sizeof(struct can_frame));
		if (retval != sizeof(struct can_frame)) {
			printf("Sending a message with ID of 0x%x resulted in a CAN error: %u , %s***\n",
					message->id, errno, strerror(errno));
			if (errno == ENETDOWN) {
				printf("The network is down. Initialize the network with command:\n\n"
						"sudo ip link set CHANNEL type can bitrate BAUDRATE txqueuelen 1000\n\n"
						"And open the network with command:\n\n"
						"sudo ip link set dev CHANNEL up\n\n"
						"After that you can communicate with the device.\n");
				// exit the program to prevent further error messages
				exit(0);
			}
			printf("Failed to send message with id 0f 0x%x to CAN channel %s\n",
					message->id, channel);
			ret = ERR_HARDWARE_NOT_SUPPORTED;
		}
		else {
//			printf("message sent 0x%x 0x%x 0x%x 0%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
//					message->id, message->data_8bit[0], message->data_8bit[1],
//					message->data_8bit[2], message->data_8bit[3], message->data_8bit[4],
//					message->data_8bit[5], message->data_8bit[6], message->data_8bit[7]);
		}
	}

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
	if (this->connection) {
		bool go = true;
		while (go) {
			struct can_frame frame_rd;
			int recvbytes = 0;

			struct timeval timeout = {0, 1000};
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
						if (uv_ring_buffer_push(&this->rx_buffer, &msg) != ERR_NONE) {
							printf("** CAN RX buffer full**\n");
						}

						go = true;

						ioctl(this->soc, SIOCGSTAMP, &this->lastrxtime);
					}
					else if (recvbytes == -1) {
						printf("*** CAN RX error: %u , %s***\n", errno, strerror(errno));
						if (errno == ENETDOWN) {
							printf("The network is down. Initialize the network with command:\n\n"
									"sudo ip link set CHANNEL type can bitrate BAUDRATE\n\n"
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



#endif
