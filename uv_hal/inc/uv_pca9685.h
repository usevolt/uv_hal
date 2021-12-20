/*
 * pca9625.h
 *
 *  Created on: Dec 15, 2021
 *      Author: usevolt
 */

#ifndef HAL_UV_HAL_INC_PCA9685_H_
#define HAL_UV_HAL_INC_PCA9685_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_i2c.h"
#include "uv_gpio.h"



/// @file: PCA9685 is an I2C PWM module with 16 outputs
/// See https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf for more details.



#if CONFIG_I2C



typedef enum {
	PCA9685_PWM0 = 0,
	PCA9685_PWM1,
	PCA9685_PWM2,
	PCA9685_PWM3,
	PCA9685_PWM4,
	PCA9685_PWM5,
	PCA9685_PWM6,
	PCA9685_PWM7,
	PCA9685_PWM8,
	PCA9685_PWM9,
	PCA9685_PWM10,
	PCA9685_PWM11,
	PCA9685_PWM12,
	PCA9685_PWM13,
	PCA9685_PWM14,
	PCA9685_PWM15
} pca9685_chn_e;



/// @brief: The main pca9625 module
typedef struct {
	i2c_e i2c;
	// the address of the device
	uint8_t address;

	uv_gpios_e oe_gpio;

} pca9685_st;



#define PCA9685_PWM_FREQ_MAX			1526


/// @brief: Initializes the PCA9685 module
///
/// @return: ERR_NONE if init succesfully
uv_errors_e pca9685_init(pca9685_st *this, i2c_e i2c_chn, uint8_t address,
		uv_gpios_e oe_gpio, uint32_t pwm_freq);



/// @brief: Sets the output duty cycle to *value*. Value should be 0 ... PWM_MAX_VALUE
///
/// @note: applies to funciton pointer interface on uv_pwm.h
void pca9685_set(void *this, uint32_t chn, uint16_t value);



/// @brief: Gets the output duty cycle from the given channel
///
/// @note: Applies to function pointer interface on uv_pwm.h
uint16_t pca9685_get(void *this, uint32_t chn);


/// @brief: Sets the PWM of the channel. On PCA9685 it will set the global PWM freq to all channels
void pca9685_set_freq(void *this, uint32_t chn, uint32_t freq);


#endif

#endif /* HAL_UV_HAL_INC_PCA9685_H_ */
