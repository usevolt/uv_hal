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
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <linux/if_link.h>

#if CONFIG_CAN_LOG
extern bool can_log;
#endif

#define DEV_COUNT_MAX			30

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

	// list of available CAN devices
	char devs[DEV_COUNT_MAX][32];
	// count of CAN devs
	int32_t dev_count;

} can_st;


static can_st _can = {
		.connection = false,
		.baudrate = CONFIG_CAN0_BAUDRATE,
		.dev = "can0",
		.dev_count = 0,
		.soc = -1
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


/// @brief: Opens a socket to a SocketCAN device
static bool copen(void) {
	bool ret = true;
	struct ifreq ifr;
	struct sockaddr_can addr;

	if (this->connection) {
		cclose();
	}

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

			if (bind(this->soc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
				printf("Binding to the CAN socket failed\n");
				ret = false;
			}
			else {
				can_err_mask_t err_mask = ( CAN_ERR_MASK );
				setsockopt(this->soc, SOL_CAN_RAW, CAN_RAW_ERR_FILTER,
						&err_mask, sizeof(err_mask));


				printf("CAN socket opened to device %s, fd: %i\n", this->dev, this->soc);
				this->connection = true;
			}
		}
	}

	return ret;
}


/// @brief: Closes the socket to the SocketCAN device
static bool cclose(void) {
	bool ret = true;

	if (this->connection) {
		close(this->soc);
		printf("Socket closed.\n");
	}

	this->connection = false;
	return ret;
}

void uv_can_close(void) {
	cclose();
}


#if CONFIG_TARGET_LINUX

char *uv_can_set_up(void) {
	char *ret = NULL;
	char cmd[128];
	int current_baud = 0;

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
		if (!strstr(cmd, "UP")) {
			current_baud = -1;
		}
	}
	pclose(fp);

	if (this->baudrate != current_baud) {
		if (this->connection) {
			// close the socket if one is open
			cclose();
		}
		// since baudrate is not what we want, we need to set network dev down
		sprintf(cmd, "sudo ip link set dev %s down", this->dev);
		printf("%s\n", cmd);
		if (system(cmd));

		// set the net dev up and configure all parameters
		sprintf(cmd, "sudo ip link set %s type can bitrate %u", this->dev, this->baudrate);
		printf("%s\n", cmd);
		if (system(cmd));
		sprintf(cmd, "sudo ip link set %s txqueuelen 1000", this->dev);
		printf("%s\n", cmd);
		if (system(cmd));
		sprintf(cmd, "sudo ip link set dev %s up", this->dev);
		printf("%s\n", cmd);
		if (system(cmd));
	}

	/* open socket */
	copen();

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


struct timeval uv_can_get_rx_time(void) {
	return this->lastrxtime;
}

bool uv_can_is_connected(void) {
	return this->connection;
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
		if (retval != sizeof(struct can_frame) && errno == ENETDOWN) {
			// try to set the net dev up
			uv_can_set_up();
			retval = write(this->soc, &frame, sizeof(struct can_frame));
		}
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
		}
	}
	else {
		uv_can_set_up();
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



#endif
