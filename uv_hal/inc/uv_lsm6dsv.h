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

#ifndef UV_HAL_INC_UV_LSM6DSV_H_
#define UV_HAL_INC_UV_LSM6DSV_H_


/// @file: ST LSM6DSV 6-axis IMU (3-axis accelerometer + 3-axis gyroscope) module.
/// The device is accessed over a 4-wire SPI bus. Optionally the module calculates
/// the device orientation (roll & pitch) from the measured data, see CONFIG_LSM6DSV_TILT.


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_errors.h"
#include "uv_spi.h"

#if CONFIG_LSM6DSV

#if !CONFIG_SPI
#error "LSM6DSV uses SPI to communicate with the mcu. Define CONFIG_SPI as 1."
#endif
#if !defined(CONFIG_LSM6DSV_ACC_FS_G)
#error "CONFIG_LSM6DSV_ACC_FS_G should define the accelerometer full scale in g's. \
Should be one of 2, 4, 8 or 16."
#endif
#if !defined(CONFIG_LSM6DSV_GYRO_FS_DPS)
#error "CONFIG_LSM6DSV_GYRO_FS_DPS should define the gyroscope full scale in degrees \
per second. Should be one of 125, 250, 500, 1000 or 2000."
#endif
#if !defined(CONFIG_LSM6DSV_ODR_HZ)
#error "CONFIG_LSM6DSV_ODR_HZ should define the output data rate of the sensor in Hz. \
Should be one of 15, 30, 60, 120, 240, 480 or 960."
#endif
#if !defined(CONFIG_LSM6DSV_TIMEOUT_MS)
/// @brief: The time in milliseconds without any new data from the sensor after which
/// the module reports a fault and tries to reinitialize the device
#define CONFIG_LSM6DSV_TIMEOUT_MS			500
#endif
#if !defined(CONFIG_LSM6DSV_TILT)
#error "CONFIG_LSM6DSV_TILT should be defined as 1 or 0, depending if the orientation \
(roll & pitch) calculation is compiled into the module. When disabled, only the raw \
accelerometer, gyroscope and temperature data are available."
#endif
#if CONFIG_LSM6DSV_TILT
#if !defined(CONFIG_LSM6DSV_TILT_TAU_MS)
#error "CONFIG_LSM6DSV_TILT_TAU_MS should define the time constant of the complementary \
filter in milliseconds. Below the time constant the orientation follows the gyroscope, \
above it the accelerometer. Bigger values reject vibration better but drift more."
#endif
#endif


/// @brief: A single 3-axis measurement result
typedef struct {
	int32_t x;
	int32_t y;
	int32_t z;
} uv_lsm6dsv_axes_st;


/// @brief: The state of the LSM6DSV module
typedef enum {
	/// @brief: uv_lsm6dsv_init has not been called, or it failed
	LSM6DSV_STATE_UNINITIALIZED = 0,
	/// @brief: The device is responding and the measurement data is valid
	LSM6DSV_STATE_OK,
	/// @brief: The device stopped responding. The module tries to reinitialize
	/// the device automatically.
	LSM6DSV_STATE_FAULT
} uv_lsm6dsv_state_e;


/// @brief: Main data structure for the LSM6DSV module
typedef struct {
	/// @brief: SPI channel where the device is connected
	spi_e spi;
	/// @brief: SPI slave select for the device
	spi_slaves_e ssel;
	/// @brief: The state of the module, see uv_lsm6dsv_state_e
	uint8_t state;
	/// @brief: The time in milliseconds since the last valid measurement
	uint16_t timeout_ms;

	/// @brief: The measured acceleration in mg (1000 == 1 g)
	uv_lsm6dsv_axes_st acc;
	/// @brief: The measured angular rate in mdps (1000 == 1 degree per second)
	uv_lsm6dsv_axes_st gyro;
	/// @brief: The measured device temperature in 0.1 degrees of celsius
	int16_t temp;

#if CONFIG_LSM6DSV_TILT
	/// @brief: The filtered orientation in 0.001 degrees, without the zero offset
	int32_t rollval;
	int32_t pitchval;
	/// @brief: The zero offset set with uv_lsm6dsv_set_zero, in 0.001 degrees
	int32_t rolloffset;
	int32_t pitchoffset;
	/// @brief: The output orientation in 0.1 degrees, zero offset applied
	int16_t roll;
	int16_t pitch;
	/// @brief: True when the orientation has been initialized from the accelerometer
	bool tiltinit;
#endif
} uv_lsm6dsv_st;


/// @brief: Initializes the LSM6DSV module. Resets the device, verifies that it
/// answers correctly and configures the full scales and the output data rate
/// with the CONFIG_LSM6DSV_* defines. The SPI bus should be initialized with
/// clock mode 3 (CONFIG_SPIx_CLOCK_POL and CONFIG_SPIx_CLOCK_PHASE both as 1)
/// and with a maximum of 10 MHz baudrate.
///
/// @return: ERR_NONE if the device was found and configured,
/// ERR_HARDWARE_NOT_SUPPORTED if the device answered with a wrong ID, or
/// ERR_NOT_RESPONDING if the device didn't answer at all.
///
/// @param spi: SPI channel where the device is connected
/// @param ssel: SPI slave select for the device
uv_errors_e uv_lsm6dsv_init(uv_lsm6dsv_st *this, spi_e spi, spi_slaves_e ssel);


/// @brief: Step function. Reads the new measurement data from the device and,
/// when CONFIG_LSM6DSV_TILT is enabled, updates the orientation.
/// Should be called cyclically with a step time smaller than
/// the sensor output data rate (CONFIG_LSM6DSV_ODR_HZ).
void uv_lsm6dsv_step(uv_lsm6dsv_st *this, uint16_t step_ms);


/// @brief: Returns the state of the module. The measurement data is valid
/// only when this returns LSM6DSV_STATE_OK.
static inline uv_lsm6dsv_state_e uv_lsm6dsv_get_state(uv_lsm6dsv_st *this) {
	return (uv_lsm6dsv_state_e) this->state;
}


/// @brief: Returns the measured acceleration in mg (1000 == 1 g)
static inline uv_lsm6dsv_axes_st uv_lsm6dsv_get_acc(uv_lsm6dsv_st *this) {
	return this->acc;
}


/// @brief: Returns the measured angular rate in mdps (1000 == 1 degree per second)
static inline uv_lsm6dsv_axes_st uv_lsm6dsv_get_gyro(uv_lsm6dsv_st *this) {
	return this->gyro;
}


/// @brief: Returns the device temperature in 0.1 degrees of celsius
static inline int16_t uv_lsm6dsv_get_temp(uv_lsm6dsv_st *this) {
	return this->temp;
}


#if CONFIG_LSM6DSV_TILT

/// @brief: Returns the device roll angle, i.e. the rotation around the X axis,
/// in 0.1 degrees. The value is in the range of -1800 ... 1800.
///
/// @note: The angle is calculated from the direction of the gravity. The rotation
/// around the gravity vector (yaw) cannot be measured without a magnetometer and
/// thus it is not provided.
static inline int16_t uv_lsm6dsv_get_roll(uv_lsm6dsv_st *this) {
	return this->roll;
}


/// @brief: Returns the device pitch angle, i.e. the rotation around the Y axis,
/// in 0.1 degrees. The value is in the range of -900 ... 900.
static inline int16_t uv_lsm6dsv_get_pitch(uv_lsm6dsv_st *this) {
	return this->pitch;
}


/// @brief: Stores the current orientation as the zero point. Can be used to
/// compensate the mounting angle of the circuit board.
void uv_lsm6dsv_set_zero(uv_lsm6dsv_st *this);


/// @brief: Clears the zero point set with uv_lsm6dsv_set_zero
void uv_lsm6dsv_clear_zero(uv_lsm6dsv_st *this);

#endif


#endif

#endif /* UV_HAL_INC_UV_LSM6DSV_H_ */
