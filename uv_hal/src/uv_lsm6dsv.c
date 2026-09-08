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


#include "uv_lsm6dsv.h"
#include "uv_rtos.h"
#include <string.h>

#if CONFIG_LSM6DSV


/// @brief: The LSM6DSV register addresses used by this module
#define REG_IF_CFG				0x03
#define REG_WHO_AM_I			0x0F
#define REG_CTRL1				0x10
#define REG_CTRL2				0x11
#define REG_CTRL3				0x12
#define REG_CTRL6				0x15
#define REG_CTRL8				0x17
#define REG_STATUS				0x1E
#define REG_OUT_TEMP_L			0x20

/// @brief: The fixed device ID found from WHO_AM_I
#define WHO_AM_I_VALUE			0x70

/// @brief: Set on the address byte to read instead of write
#define READ_BIT				0x80

#define IF_CFG_I2C_I3C_DISABLE	(1 << 0)
#define CTRL3_SW_RESET			(1 << 0)
#define CTRL3_IF_INC			(1 << 2)
#define CTRL3_BDU				(1 << 6)
#define STATUS_XLDA				(1 << 0)
#define STATUS_GDA				(1 << 1)
/// @brief: Bits reserved as zero on STATUS. If any of them is set, the device is
/// not really answering, e.g. the MISO line is floating and reads as all ones.
#define STATUS_RESERVED			0x48

/// @brief: Accelerometer high-performance operating mode, OP_MODE_XL_[2:0] on CTRL1
#define OP_MODE_XL_HIGHPERF		(0 << 4)
/// @brief: Gyroscope high-performance operating mode, OP_MODE_G_[2:0] on CTRL2
#define OP_MODE_G_HIGHPERF		(0 << 4)

/// @brief: The count of bytes read with a single burst read:
/// temperature, gyroscope and accelerometer output registers
#define DATA_LEN				14
/// @brief: The maximum count of bytes read with a single read command
#define MAX_READ_LEN			DATA_LEN

/// @brief: The maximum time in milliseconds waited for the software reset to finish
#define RESET_TIMEOUT_MS		100


/* Configuration defines mapped to the register values and to the sensitivities */

#if (CONFIG_LSM6DSV_ODR_HZ == 15)
#define ODR_BITS				0x3
#elif (CONFIG_LSM6DSV_ODR_HZ == 30)
#define ODR_BITS				0x4
#elif (CONFIG_LSM6DSV_ODR_HZ == 60)
#define ODR_BITS				0x5
#elif (CONFIG_LSM6DSV_ODR_HZ == 120)
#define ODR_BITS				0x6
#elif (CONFIG_LSM6DSV_ODR_HZ == 240)
#define ODR_BITS				0x7
#elif (CONFIG_LSM6DSV_ODR_HZ == 480)
#define ODR_BITS				0x8
#elif (CONFIG_LSM6DSV_ODR_HZ == 960)
#define ODR_BITS				0x9
#else
#error "Unsupported CONFIG_LSM6DSV_ODR_HZ. Should be one of 15, 30, 60, 120, 240, 480 or 960."
#endif

// accelerometer sensitivity in ug / LSB
#if (CONFIG_LSM6DSV_ACC_FS_G == 2)
#define FS_XL_BITS				0x0
#define ACC_SENS_UG				61
#elif (CONFIG_LSM6DSV_ACC_FS_G == 4)
#define FS_XL_BITS				0x1
#define ACC_SENS_UG				122
#elif (CONFIG_LSM6DSV_ACC_FS_G == 8)
#define FS_XL_BITS				0x2
#define ACC_SENS_UG				244
#elif (CONFIG_LSM6DSV_ACC_FS_G == 16)
#define FS_XL_BITS				0x3
#define ACC_SENS_UG				488
#else
#error "Unsupported CONFIG_LSM6DSV_ACC_FS_G. Should be one of 2, 4, 8 or 16."
#endif

// gyroscope sensitivity in mdps / LSB, as a fraction to avoid 64 bit arithmetics
#if (CONFIG_LSM6DSV_GYRO_FS_DPS == 125)
#define FS_G_BITS				0x0
#define GYRO_SENS_NUM			35
#define GYRO_SENS_DEN			8
#elif (CONFIG_LSM6DSV_GYRO_FS_DPS == 250)
#define FS_G_BITS				0x1
#define GYRO_SENS_NUM			35
#define GYRO_SENS_DEN			4
#elif (CONFIG_LSM6DSV_GYRO_FS_DPS == 500)
#define FS_G_BITS				0x2
#define GYRO_SENS_NUM			35
#define GYRO_SENS_DEN			2
#elif (CONFIG_LSM6DSV_GYRO_FS_DPS == 1000)
#define FS_G_BITS				0x3
#define GYRO_SENS_NUM			35
#define GYRO_SENS_DEN			1
#elif (CONFIG_LSM6DSV_GYRO_FS_DPS == 2000)
#define FS_G_BITS				0x4
#define GYRO_SENS_NUM			70
#define GYRO_SENS_DEN			1
#else
#error "Unsupported CONFIG_LSM6DSV_GYRO_FS_DPS. Should be one of 125, 250, 500, 1000 or 2000."
#endif

/// @brief: Temperature sensitivity is 256 LSB / celsius and 0 LSB equals to 25 celsius
#define TEMP_SENS_LSB			256
#define TEMP_OFFSET_DC			250


/// @brief: On the simulated targets there is no SPI bus available. The module
/// reports a device laying level on a table, keeping the application code
/// free of target specific compile time switches.
#if (CONFIG_TARGET_LPC15XX || CONFIG_TARGET_LPC40XX)
#define LSM6DSV_SIMULATED		0
#else
#define LSM6DSV_SIMULATED		1
#endif



#if !LSM6DSV_SIMULATED

/// @brief: Reads *len* bytes starting from the register *reg*. The device
/// increments the register address automatically (CTRL3 IF_INC).
///
/// @return: True if the data was read successfully
static bool read_regs(uv_lsm6dsv_st *this, uint8_t reg, uint8_t *dest, uint8_t len) {
	bool ret = false;
	spi_data_t write[MAX_READ_LEN + 1];
	spi_data_t read[MAX_READ_LEN + 1];

	if (len <= MAX_READ_LEN) {
		memset(write, 0, sizeof(write));
		write[0] = reg | READ_BIT;

		if (uv_spi_readwrite_sync(this->spi, this->ssel,
				write, read, 8, len + 1) != 0) {
			for (uint8_t i = 0; i < len; i++) {
				// the first received byte is clocked out while the address is
				// still being sent, thus the data starts from the second byte
				dest[i] = (uint8_t) read[i + 1];
			}
			ret = true;
		}
		else {
		}
	}
	else {
	}

	return ret;
}


/// @brief: Writes a single register
///
/// @return: True if the data was written successfully
static bool write_reg(uv_lsm6dsv_st *this, uint8_t reg, uint8_t value) {
	spi_data_t write[2];
	write[0] = reg;
	write[1] = value;

	return (uv_spi_write_sync(this->spi, this->ssel, write, 8, 2) != 0);
}


/// @brief: Resets the device and writes the configuration registers
static uv_errors_e configure(uv_lsm6dsv_st *this) {
	uv_errors_e ret = ERR_NONE;
	uint8_t d = 0;

	if (!write_reg(this, REG_CTRL3, CTRL3_SW_RESET)) {
		ret = ERR_NOT_RESPONDING;
	}
	else {
		// the reset bit clears itself once the device is ready
		uint16_t i = 0;
		bool reset = false;
		while ((i < RESET_TIMEOUT_MS) && !reset) {
			uv_rtos_task_delay(1);
			if (read_regs(this, REG_CTRL3, &d, 1) &&
					!(d & CTRL3_SW_RESET)) {
				reset = true;
			}
			else {
				i++;
			}
		}

		if (!reset) {
			ret = ERR_NOT_RESPONDING;
		}
		else if (!read_regs(this, REG_WHO_AM_I, &d, 1)) {
			ret = ERR_NOT_RESPONDING;
		}
		else if (d != WHO_AM_I_VALUE) {
			ret = ERR_HARDWARE_NOT_SUPPORTED;
		}
		else {
			// note: IF_CFG is not affected by the software reset
			if (write_reg(this, REG_IF_CFG, IF_CFG_I2C_I3C_DISABLE) &&
					// block data update keeps the LSB and MSB of a single
					// measurement from being updated in the middle of a read
					write_reg(this, REG_CTRL3, CTRL3_BDU | CTRL3_IF_INC) &&
					write_reg(this, REG_CTRL8, FS_XL_BITS) &&
					write_reg(this, REG_CTRL6, FS_G_BITS) &&
					// writing a nonzero ODR starts the measurements
					write_reg(this, REG_CTRL1, OP_MODE_XL_HIGHPERF | ODR_BITS) &&
					write_reg(this, REG_CTRL2, OP_MODE_G_HIGHPERF | ODR_BITS)) {

			}
			else {
				ret = ERR_NOT_RESPONDING;
			}
		}
	}

	return ret;
}



#endif



#if CONFIG_LSM6DSV_TILT

/// @brief: The count of cordic iterations. Iterations after this would
/// rotate the vector less than the 0.01 degree resolution.
#define CORDIC_ITERATIONS		13
/// @brief: The input values are shifted left by this before iterating. Without
/// the headroom the shifted-away bits would stall the iteration.
#define CORDIC_PRESCALE			12
/// @brief: The cordic algorithm scales the vector length by this constant
#define CORDIC_GAIN_X10000		16468
/// @brief: The half circle in the 0.01 degree units the cordic works with
#define CORDIC_HALF_CIRCLE		18000
/// @brief: The full circle in the 0.001 degree units the orientation is filtered
/// with. The unit has to be this small for the integer division of the
/// complementary filter not to truncate the smallest corrections to zero.
#define CIRCLE					360000
#define HALF_CIRCLE				(CIRCLE / 2)
/// @brief: Scales the cordic output to the units used by the filter
#define CORDIC_TO_ANGLE			10
/// @brief: Scales the filtered angle to the 0.1 degrees given out
#define ANGLE_TO_OUT			100

/// @brief: arctan(2^-i) in 0.01 degrees
static const int16_t cordic_atan[CORDIC_ITERATIONS] = {
	4500, 2657, 1404, 713, 358, 179, 90, 45, 22, 11, 6, 3, 1
};

/// @brief: The limits in mg for the measured acceleration to be accepted as
/// the direction of the gravity. When the device is accelerated harder than
/// this, the orientation follows the gyroscope only.
#define ACC_1G_MIN				800
#define ACC_1G_MAX				1200


/// @brief: Cordic vectoring mode. Calculates the angle of the vector (*x*, *y*)
/// and optionally the length of it.
///
/// @return: The angle of the vector in 0.01 degrees, -18000 ... 18000
///
/// @param mag: If not NULL, the length of the vector is written here in the
/// same units as the parameters.
static int32_t cordic(int32_t y, int32_t x, int32_t *mag) {
	int32_t z = 0;

	x <<= CORDIC_PRESCALE;
	y <<= CORDIC_PRESCALE;

	// the iteration converges only on the right half plane, rotate the
	// vector by 180 degrees and compensate it on the result
	if (x < 0) {
		z = (y >= 0) ? CORDIC_HALF_CIRCLE : -CORDIC_HALF_CIRCLE;
		x = -x;
		y = -y;
	}
	else {
	}

	for (uint8_t i = 0; i < CORDIC_ITERATIONS; i++) {
		int32_t xn;
		int32_t yn;
		// note: every iteration has to be run for the cordic gain to stay constant
		if (y >= 0) {
			xn = x + (y >> i);
			yn = y - (x >> i);
			z += cordic_atan[i];
		}
		else {
			xn = x - (y >> i);
			yn = y + (x >> i);
			z -= cordic_atan[i];
		}
		x = xn;
		y = yn;
	}

	if (mag != NULL) {
		int64_t div = (int64_t) CORDIC_GAIN_X10000 << CORDIC_PRESCALE;
		*mag = (int32_t) (((int64_t) x * 10000 + div / 2) / div);
	}
	else {
	}

	return z;
}


/// @brief: Wraps the angle *value* to -18000 ... 18000 (0.01 degrees)
static int32_t wrap(int32_t value) {
	int32_t ret = value;

	while (ret > HALF_CIRCLE) {
		ret -= CIRCLE;
	}
	while (ret < -HALF_CIRCLE) {
		ret += CIRCLE;
	}

	return ret;
}


/// @brief: Updates the orientation with a complementary filter. The gyroscope
/// gives the short term movement and the direction of the gravity corrects the
/// long term drift of it.
static void tilt_step(uv_lsm6dsv_st *this, uint16_t step_ms) {
	int32_t magyz;
	int32_t magxyz;
	uint16_t d = step_ms;
	// limit the step time to keep the gyroscope integration from overflowing
	LIMITS(d, 1, 100);

	// roll is the angle of the gravity on the YZ plane. The vector length from
	// the same calculation gives the tilt compensation for the pitch, as well as
	// the total acceleration when combined with the X axis.
	int32_t accroll = cordic(this->acc.y, this->acc.z, &magyz) * CORDIC_TO_ANGLE;
	int32_t accpitch = cordic(-this->acc.x, magyz, &magxyz) * CORDIC_TO_ANGLE;

	if (!this->tiltinit) {
		// start from the measured orientation instead of waiting for the filter
		// to converge from zero
		this->rollval = accroll;
		this->pitchval = accpitch;
		this->tiltinit = true;
	}
	else {
		// mdps * ms equals to 0.001 degrees when divided by 1000
		this->rollval += (this->gyro.x * (int32_t) d) / 1000;
		this->pitchval += (this->gyro.y * (int32_t) d) / 1000;

		if ((magxyz > ACC_1G_MIN) && (magxyz < ACC_1G_MAX)) {
			// pull the integrated angle towards the measured gravity direction.
			// The error is wrapped to take the shortest way over -180 / 180 degrees.
			int32_t err = wrap(accroll - this->rollval);
			this->rollval += (err * (int32_t) d) /
					(CONFIG_LSM6DSV_TILT_TAU_MS + (int32_t) d);

			err = wrap(accpitch - this->pitchval);
			this->pitchval += (err * (int32_t) d) /
					(CONFIG_LSM6DSV_TILT_TAU_MS + (int32_t) d);
		}
		else {
		}
	}

	this->rollval = wrap(this->rollval);
	this->pitchval = wrap(this->pitchval);

	this->roll = (int16_t) (wrap(this->rollval - this->rolloffset) / ANGLE_TO_OUT);
	this->pitch = (int16_t) (wrap(this->pitchval - this->pitchoffset) / ANGLE_TO_OUT);
}


void uv_lsm6dsv_set_zero(uv_lsm6dsv_st *this) {
	this->rolloffset = this->rollval;
	this->pitchoffset = this->pitchval;
}


void uv_lsm6dsv_clear_zero(uv_lsm6dsv_st *this) {
	this->rolloffset = 0;
	this->pitchoffset = 0;
}

#endif



uv_errors_e uv_lsm6dsv_init(uv_lsm6dsv_st *this, spi_e spi, spi_slaves_e ssel) {
	uv_errors_e ret = ERR_NONE;

	this->spi = spi;
	this->ssel = ssel;
	this->state = LSM6DSV_STATE_UNINITIALIZED;
	this->timeout_ms = 0;
	memset(&this->acc, 0, sizeof(this->acc));
	memset(&this->gyro, 0, sizeof(this->gyro));
	this->temp = TEMP_OFFSET_DC;
#if CONFIG_LSM6DSV_TILT
	this->rollval = 0;
	this->pitchval = 0;
	this->rolloffset = 0;
	this->pitchoffset = 0;
	this->roll = 0;
	this->pitch = 0;
	this->tiltinit = false;
#endif

#if LSM6DSV_SIMULATED
	this->acc.z = 1000;
	this->state = LSM6DSV_STATE_OK;
#else
	ret = configure(this);
	if (ret == ERR_NONE) {
		this->state = LSM6DSV_STATE_OK;
	}
	else {
	}
#endif

	return ret;
}


void uv_lsm6dsv_step(uv_lsm6dsv_st *this, uint16_t step_ms) {
#if !LSM6DSV_SIMULATED
	uint8_t status = 0;

	if (read_regs(this, REG_STATUS, &status, 1) &&
			!(status & STATUS_RESERVED) &&
			(status & (STATUS_XLDA | STATUS_GDA))) {
		uint8_t data[DATA_LEN];

		if (read_regs(this, REG_OUT_TEMP_L, data, DATA_LEN)) {
			int16_t raw[DATA_LEN / 2];

			for (uint8_t i = 0; i < (DATA_LEN / 2); i++) {
				raw[i] = (int16_t) ((uint16_t) data[i * 2] |
						((uint16_t) data[(i * 2) + 1] << 8));
			}

			this->temp = (int16_t) (TEMP_OFFSET_DC +
					(((int32_t) raw[0] * 10) / TEMP_SENS_LSB));

			this->gyro.x = ((int32_t) raw[1] * GYRO_SENS_NUM) / GYRO_SENS_DEN;
			this->gyro.y = ((int32_t) raw[2] * GYRO_SENS_NUM) / GYRO_SENS_DEN;
			this->gyro.z = ((int32_t) raw[3] * GYRO_SENS_NUM) / GYRO_SENS_DEN;

			this->acc.x = ((int32_t) raw[4] * ACC_SENS_UG) / 1000;
			this->acc.y = ((int32_t) raw[5] * ACC_SENS_UG) / 1000;
			this->acc.z = ((int32_t) raw[6] * ACC_SENS_UG) / 1000;

			this->timeout_ms = 0;
			this->state = LSM6DSV_STATE_OK;
		}
		else {
		}
	}
	else {
		// the device is expected to provide new data on every step cycle.
		// If it doesn't, it has either lost its configuration or the bus is broken.
		if (this->timeout_ms < CONFIG_LSM6DSV_TIMEOUT_MS) {
			this->timeout_ms += step_ms;
		}
		else {
			this->state = LSM6DSV_STATE_FAULT;
			this->timeout_ms = 0;
			if (configure(this) == ERR_NONE) {
				this->state = LSM6DSV_STATE_OK;
			}
			else {
			}
		}
	}
#endif

#if CONFIG_LSM6DSV_TILT
	if (this->state == LSM6DSV_STATE_OK) {
		tilt_step(this, step_ms);
	}
	else {
	}
#endif
}


#endif
