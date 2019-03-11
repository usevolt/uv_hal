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



#include "uv_l6470.h"
#include <string.h>

#if CONFIG_L6470


#define CMD_NOP					0x0
#define CMD_SETPARAM			0x0
#define CMD_GETPARAM			0b00100000
#define CMD_GOTODIR				0b01101000
#define CMD_GOTODIR_LEN			4
#define CMD_RESETPOS			0b11011000
#define CMD_RELEASESW			0b10010010
#define CMD_SOFTSTOP			0b10110000
#define CMD_SOFTHIZ				0b10100000
#define CMD_GETSTATUS			0b11010000


#define FORWARD					1
#define REVERSE					0


#define REG_ABS_POS			0x01
#define REG_ABS_POS_LEN		3
#define REG_EL_POS			0x02
#define REG_EL_POS_LEN		2
#define REG_MARK			0x03
#define REG_MARK_LEN		3
#define REG_SPEED			0x04
#define REG_SPEED_LEN		3
#define REG_ACC				0x05
#define REG_ACC_LEN			2
#define REG_DEC				0x06
#define REG_DEC_LEN			2
#define REG_MAX_SPEED		0x07
#define REG_MAX_SPEED_LEN	2
#define REG_MIN_SPEED		0x08
#define REG_MIN_SPEED_LEN	2
#define REG_FS_SPD			0x15
#define REG_FS_SPD_LEN		2
#define REG_KVAL_HOLD		0x09
#define REG_KVAL_HOLD_LEN	1
#define REG_KVAL_RUN		0x0A
#define REG_KVAL_RUN_LEN	1
#define REG_KVAL_ACC		0x0B
#define REG_KVAL_ACC_LEN	1
#define REG_KVAL_DEC		0x0C
#define REG_KVAL_DEC_LEN	1
#define REG_INT_SPEED		0x0D
#define REG_INT_SPEED_LEN	2
#define REG_ST_SLP			0x0E
#define REG_ST_SLP_LEN		1
#define REG_FN_SLP_ACC		0x0F
#define REG_FN_SLP_ACC_LEN	1
#define REG_FN_SLP_DEC		0x10
#define REG_FN_SLP_DEC_LEN	1
#define REG_K_THERM			0x11
#define REG_K_THERM_LEN		1
#define REG_ADC_OUT			0x12
#define REG_ADC_OUT_LEN		1
#define REG_OCD_TH			0x13
#define REG_OCD_TH_LEN		1
#define REG_STALL_TH		0x14
#define REG_STALL_TH_LEN	1
#define REG_STEP_MODE		0x16
#define REG_STEP_MODE_LEN	1
#define REG_ALARM_EN		0x17
#define REG_ALARM_EN_LEN	1
#define REG_CONFIG			0x18
#define REG_CONFIG_LEN		2
#define REG_STATUS			0x19
#define REG_STATUS_LEN		2


/// @brief: can be used to communicate with the device. Wraps the uv_spi
/// calls into one function.
static void readwrite(uv_l6470_st *this, uint8_t *write_ptr, uint8_t *read_ptr, uint8_t data_len);

void uv_l6470_init(uv_l6470_st *this, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e reset_io, uv_gpios_e busy_io, l6470_microstep_e microsteps) {
	this->reset_pin = reset_io;
	this->spi = spi;
	this->ssel = ssel;
	this->busy_pin = busy_io;
	this->current_pos = 0;
	this->pos_known = false;

	uv_gpio_init_input(this->busy_pin, PULL_UP_ENABLED);
	uv_gpio_init_output(this->reset_pin, false);
	uv_rtos_task_delay(10);
	uv_gpio_set(this->reset_pin, true);
	uv_rtos_task_delay(1);

	// L6470 should now be operational, but in high impedance mode
	// set the microstep count
	uint8_t write[REG_STEP_MODE_LEN + 1];
	write[0] = CMD_SETPARAM | REG_STEP_MODE;
	write[1] = microsteps;
	readwrite(this, write, NULL, sizeof(write));

	uv_l6470_stop(this);
}


void uv_l6470_set_speeds(uv_l6470_st *this, uint16_t acc, uint16_t dec,
		uint16_t minspeed, uint32_t maxspeed) {
	uv_l6470_wait(this);
	if (acc > L6470_ACC_MAX) {
		acc = L6470_ACC_MAX;
	}
	if (dec > L6470_DEC_MAX) {
		dec = L6470_DEC_MAX;
	}
	if (minspeed > L6470_MINSPEED_MAX) {
		minspeed = L6470_MINSPEED_MAX;
	}
	if (maxspeed > L6470_MAXSPEED_MAX) {
		maxspeed = L6470_MAXSPEED_MAX;
	}
	maxspeed /= 10000;

	if (this->pos_known) {
		uv_l6470_release(this);
		uv_l6470_wait(this);
	}
	uint8_t write[REG_ACC_LEN + 1];
	// acc
	write[0] = CMD_SETPARAM | REG_ACC;
	write[1] = (acc >> 8) & 0xFF;
	write[2] = acc & 0xFF;
	readwrite(this, write, NULL, sizeof(write));
	uv_l6470_wait(this);
	// dec
	write[0] = CMD_SETPARAM | REG_DEC;
	write[1] = (dec >> 8) & 0xFF;
	write[2] = dec & 0xFF;
	readwrite(this, write, NULL, sizeof(write));
	uv_l6470_wait(this);
	// minspeed
	write[0] = CMD_SETPARAM | REG_MIN_SPEED;
	write[1] = ((minspeed >> 8) & 0xFF);
	write[2] = minspeed & 0xFF;
	readwrite(this, write, NULL, sizeof(write));
	uv_l6470_wait(this);
	// maxspeed
	write[0] = CMD_SETPARAM | REG_MAX_SPEED;
	write[1] = (maxspeed >> 8) & 0xFF;
	write[2] = maxspeed & 0xFF;
	readwrite(this, write, NULL, sizeof(write));
	uv_l6470_wait(this);

}


void uv_l6470_set_pwm(uv_l6470_st *this, uint8_t acc_duty_cycle, uint8_t dec_duty_cycle,
		uint8_t run_duty_cycle, uint8_t hold_duty_cycle) {
	uv_l6470_wait(this);
	uint8_t write[REG_KVAL_ACC_LEN + 1];
	// acceleration kval
	write[0] = CMD_SETPARAM | REG_KVAL_ACC;
	write[1] = acc_duty_cycle;
	readwrite(this, write, NULL, sizeof(write));
	// deceleration kval
	write[0] = CMD_SETPARAM | REG_KVAL_DEC;
	write[1] = dec_duty_cycle;
	readwrite(this, write, NULL, sizeof(write));
	// run kval
	write[0] = CMD_SETPARAM | REG_KVAL_RUN;
	write[1] = run_duty_cycle;
	readwrite(this, write, NULL, sizeof(write));
	// stop kval
	write[0] = CMD_SETPARAM | REG_KVAL_HOLD;
	write[1] = hold_duty_cycle;
	readwrite(this, write, NULL, sizeof(write));

}


void uv_l6470_set_overcurrent(uv_l6470_st *this, uint16_t value_ma) {
	uv_l6470_wait(this);
	uint8_t write[REG_OCD_TH_LEN + 1];
	uint8_t value = (value_ma / 375);
	if (value > 0) {
		value--;
	}
	write[0] = CMD_SETPARAM | REG_OCD_TH;
	write[1] = value;
	readwrite(this, write, NULL, sizeof(write));
}



void uv_l6470_find_home(uv_l6470_st *this, l6470_dir_e dir) {
	uv_l6470_wait(this);
	uint8_t write[4] = {};
	uint8_t read[4] = {};

	write[0] = CMD_RELEASESW | (dir == L6470_DIR_FORWARD);
	readwrite(this, write, NULL, 1);
	this->pos_known = true;
	this->current_pos = 0;

	uv_l6470_set_home(this);
	uv_l6470_wait(this);
	write[0] = CMD_GETSTATUS;
	readwrite(this, write, read, 4);
	uv_l6470_wait(this);
}


void uv_l6470_set_home(uv_l6470_st *this) {
	uv_l6470_wait(this);
	uint8_t write = CMD_RESETPOS;
	readwrite(this, &write, NULL, 1);
	this->current_pos = 0;
}


int32_t uv_l6470_get_pos(uv_l6470_st *this) {
	uv_l6470_wait(this);
	// read absolute position register value
	uint8_t write[REG_ABS_POS_LEN + 1] = {};
	uint8_t read[sizeof(write)];

	// read the status to clear possible interrupts
	// This can cause hazard if the L6470 is already at the home position and
	// it is set to move in the wrong directions
	write[0] = CMD_GETSTATUS;
	readwrite(this, write, read, 3);
	uv_l6470_wait(this);

	write[0] = CMD_GETPARAM | REG_ABS_POS;
	readwrite(this, write, read, sizeof(write));
	int32_t result = (read[1] << 16) | (read[2] << 8) | read[3];
//	printf("result: 0x%x 0x%x 0x%x 0x%x\n", read[0], read[1], read[2], read[3]);
	// apply 2's complement sign
	result = (int32_t) (result << (32 - 22));
	result /= (1 << (32 - 22));
	this->current_pos = result;
	return result;
}



void uv_l6470_goto(uv_l6470_st *this, int32_t pos) {
	uv_l6470_wait(this);
	if (!this->pos_known) {
		uv_l6470_get_pos(this);
		this->pos_known = true;
	}
	uint8_t dir = (this->current_pos < pos) ? FORWARD : REVERSE;
	uint8_t write[CMD_GOTODIR_LEN] = {};
	uint32_t value = ((int32_t) (pos * (1 << (32 - 22))) >> (32 - 22));
	write[0] = CMD_GOTODIR | dir;
	write[1] = (value >> 16) & 0xFF;
	write[2] = (value >> 8) & 0xFF;
	write[3] = value & 0xFF;
	readwrite(this, write, NULL, sizeof(write));
	this->current_pos = pos;
	// small delay so that L6470 has enough time to process the command
	uv_rtos_task_delay(1);
}



void uv_l6470_stop(uv_l6470_st *this) {
//	printf("stop\n");
	uint8_t write = CMD_SOFTSTOP;
	readwrite(this, &write, NULL, sizeof(write));
	uv_rtos_task_delay(1);
	uv_l6470_wait(this);
	uv_l6470_get_pos(this);
	this->pos_known = true;
}


void uv_l6470_release(uv_l6470_st *this) {
	uv_l6470_wait(this);
	this->pos_known = false;
	uint8_t write = CMD_SOFTHIZ;
	readwrite(this, &write, NULL, sizeof(write));
}




void uv_l6470_wait(uv_l6470_st *this) {
	while (!uv_gpio_get(this->busy_pin)) {
		uv_rtos_task_yield();
	};
}



void readwrite(uv_l6470_st *this, uint8_t *write_ptr, uint8_t *read_ptr, uint8_t data_len) {
	for (uint8_t i = 0; i < data_len; i++) {
		uint16_t w = write_ptr[i];
		uint16_t r = 0;
		uv_spi_readwrite_sync(this->spi, this->ssel, &w, &r, 8, 1);
		if (read_ptr) {
			read_ptr[i] = (uint8_t) r;
		}
		uv_rtos_task_delay(1);
	}
}


#endif
