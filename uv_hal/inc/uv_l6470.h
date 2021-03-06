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


#ifndef UV_HAL_INC_UV_L6470_H_
#define UV_HAL_INC_UV_L6470_H_



/// @file: L6470 Stepper motor driver module


#include <uv_hal_config.h>

#if CONFIG_L6470

#include <uv_utilities.h>
#include <uv_rtos.h>
#include <uv_spi.h>

#if !CONFIG_SPI
#error "L6470 uses SPI to communicate with the mcu. SPI has to be enabled."
#endif


/// @brief: Main module for L6470 stepper motor driver
typedef struct {
	// SPI channel for this node
	spi_e spi;
	// SPI slave select for this node
	spi_slaves_e ssel;
	// GPIO pin to reset L6470. Should be connected to STBY-pin on L6470
	uv_gpios_e reset_pin;
	// busy pin is low when L6470 is performing a command
	uv_gpios_e busy_pin;

	// current position in absolute steps
	int32_t current_pos;
	// true if the position is in known state
	bool pos_known;
} uv_l6470_st;


#define L6470_ACC_MAX					0x7FE
#define L6470_ACC_DEFAULT				0x8A
#define L6470_DEC_MAX					L6470_ACC_MAX
#define L6470_DEC_DEFAULT				0x8A
#define L6470_MAXSPEED_MAX				(0x3FF * 10000)
#define L6470_MAXSPEED_DEFAULT			(0x41 * 10000)
#define L6470_MINSPEED_MAX				0x1FFF
#define L6470_MINSPEED_DEFAULT			0x0

#define L6470_ACC_PWM_DEFAULT_DC		0x29
#define L6470_DEC_PWM_DEFAULT_DC		0x29
#define L6470_RUN_PWM_DEFAULT_DC		0x29
#define L6470_HOLD_PWM_DEFAULT_DC		0x29
#define L6470_ACC_PWM_MAX				0xFF
#define L6470_DEC_PWM_MAX				0xFF
#define L6470_RUN_PWM_MAX				0xFF
#define L6470_HOLD_PWM_MAX				0xFF

#define L6470_OVERCURRENT_DEFAULT_MA	3380
#define L6470_OVERCURRENT_MAX_MA		6000

typedef enum {
	L6470_MICROSTEP_1 = 0,
	L6470_MICROSTEP_2,
	L6470_MICROSTEP_4,
	L6470_MICROSTEP_8,
	L6470_MICROSTEP_16,
	L6470_MICROSTEP_32,
	L6470_MICROSTEP_64,
	L6470_MICROSTEP_128
} l6470_microstep_e;


/// @brief: The direction enumeration. This is used for example *uv_l6470_find_home*,
/// setting the home search direction.
typedef enum {
	L6470_DIR_FORWARD = 0,
	L6470_DIR_BACKWARD
} l6470_dir_e;

/// @brief: Initializes the L6470 module. Should be called in the user application,
/// after the uv_hal library has initialized itself. SPI bus should be preconfigured
/// with right baudrate and other settings.
///
/// @param spi: SPI channel for this device
/// @param ssel: Slave select for the SPI channel
/// @param reset_io: GPIO pin for resetting the L6470
/// @param busy_io: GPIO pin for BUSY flag from the L6470
/// @param microsteps: Enum value for how many microsteps are used to control the motor
void uv_l6470_init(uv_l6470_st *this, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e reset_io, uv_gpios_e busy_io, l6470_microstep_e microsteps);


/// @brief: Sets the acceleration, deceleration, minspeed & maxspeed values
///
/// @param acc: Acceleration. See L6470 datasheet for units. Maximum value is L6470_ACC_MAX.
/// @param dec: Deceleration. See L6470 datasheet for units. Maximum value is L6470_DEC_MAX.
/// @param minspeed: Minimum speed. See L6470 datasheet for units. Maximum value is L6470_MINSPEED_MAX.
/// @param maxspeed: Maximum speed. See L6470 datasheet for units. Maximum value is L6470_MAXSPEED_MAX.
void uv_l6470_set_speeds(uv_l6470_st *this, uint16_t acc, uint16_t dec,
		uint16_t minspeed, uint32_t maxspeed);


/// @brief: Can be used to set the duty cycles for phase driver in acceleration,
/// deceleration, run and hold conditions. Duty cycle is 0...255 and linearly
/// affects the phase current.
void uv_l6470_set_pwm(uv_l6470_st *this, uint8_t acc_duty_cycle, uint8_t dec_duty_cycle,
		uint8_t run_duty_cycle, uint8_t hold_cuty_cycle);

/// @brief: Sets the overcurrent limit in milliamperes
void uv_l6470_set_overcurrent(uv_l6470_st *this, uint16_t value_ma);


/// @brief: L6470 finds the home with the aid of switch connected to L6470 pin 4 (SW).
/// Switch should connect to 0 when home is found. Home is searched in the direction
/// pointed by *dir*.
void uv_l6470_find_home(uv_l6470_st *this, l6470_dir_e dir);


/// @brief: Sets the current position as a home position
void uv_l6470_set_home(uv_l6470_st *this);


/// @brief: Returns the current position
int32_t uv_l6470_get_pos(uv_l6470_st *this);


/// @brief: Goes to absolute position **pos**. The position has to be known before calling this
///
/// @return: ERR_NONE if the position was known, otherwise ERR_ABORTED.
uv_errors_e uv_l6470_goto(uv_l6470_st *this, int32_t pos);



/// @brief: Runs in *direction* with a constant speed *speed*
uv_errors_e uv_l6470_run(uv_l6470_st *this, l6470_dir_e direction, int32_t speed);



/// @brief: Travels **value** steps relative to the current position.
static inline void uv_l6470_travel(uv_l6470_st *this, int32_t value) {
	uv_l6470_goto(this, this->current_pos + value);
}

/// @brief: Forces the L6470 to stop immediately
void uv_l6470_stop(uv_l6470_st *this);


/// @brief: Waits until the last command has been executed
void uv_l6470_wait(uv_l6470_st *this);


/// @brief: Returns true if the L6470 is not busy
static inline bool uv_l6470_is_busy(uv_l6470_st *this) {
	return !uv_gpio_get(this->busy_pin);
}


/// @brief: Puts the motor outputs into high-impedance state lowering current consumption.
/// With this function the current position is lost.
void uv_l6470_release(uv_l6470_st *this);



#endif
#endif /* UV_HAL_INC_UV_L6470_H_ */
