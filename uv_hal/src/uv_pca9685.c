/*
 * pca9625.c
 *
 *  Created on: Dec 15, 2021
 *      Author: usevolt
 */


#include <uv_pca9685.h>
#include "uv_pwm.h"
#include "uv_terminal.h"
#include "uv_rtos.h"


#if CONFIG_I2C

enum {
	REG_MODE1 = 0,
	REG_MODE2,
	REG_SUBADDR1,
	REG_SUBADDR2,
	REG_SUBADDR3,
	REG_ALLCALLADR,
	REG_LED0_ON_L,
	REG_LED0_ON_H,
	REG_LED0_OFF_L,
	REG_LED0_OFF_H,
	REG_LED1_ON_L,
	REG_LED1_ON_H,
	REG_LED1_OFF_L,
	REG_LED1_OFF_H,
	REG_LED2_ON_L,
	REG_LED2_ON_H,
	REG_LED2_OFF_L,
	REG_LED2_OFF_H,
	REG_LED3_ON_L,
	REG_LED3_ON_H,
	REG_LED3_OFF_L,
	REG_LED3_OFF_H,
	REG_LED4_ON_L,
	REG_LED4_ON_H,
	REG_LED4_OFF_L,
	REG_LED4_OFF_H,
	REG_LED5_ON_L,
	REG_LED5_ON_H,
	REG_LED5_OFF_L,
	REG_LED5_OFF_H,
	REG_LED6_ON_L,
	REG_LED6_ON_H,
	REG_LED6_OFF_L,
	REG_LED6_OFF_H,
	REG_LED7_ON_L,
	REG_LED7_ON_H,
	REG_LED7_OFF_L,
	REG_LED7_OFF_H,
	REG_LED8_ON_L,
	REG_LED8_ON_H,
	REG_LED8_OFF_L,
	REG_LED8_OFF_H,
	REG_LED9_ON_L,
	REG_LED9_ON_H,
	REG_LED9_OFF_L,
	REG_LED9_OFF_H,
	REG_LED10_ON_L,
	REG_LED10_ON_H,
	REG_LED10_OFF_L,
	REG_LED10_OFF_H,
	REG_LED11_ON_L,
	REG_LED11_ON_H,
	REG_LED11_OFF_L,
	REG_LED11_OFF_H,
	REG_LED12_ON_L,
	REG_LED12_ON_H,
	REG_LED12_OFF_L,
	REG_LED12_OFF_H,
	REG_LED13_ON_L,
	REG_LED13_ON_H,
	REG_LED13_OFF_L,
	REG_LED13_OFF_H,
	REG_LED14_ON_L,
	REG_LED14_ON_H,
	REG_LED14_OFF_L,
	REG_LED14_OFF_H,
	REG_LED15_ON_L,
	REG_LED15_ON_H,
	REG_LED15_OFF_L,
	REG_LED15_OFF_H,
	REG_ALL_LED_ON_L = 0xFA,
	REG_ALL_LED_ON_H,
	REG_ALL_LED_OFF_L,
	REG_ALL_LED_OFF_H,
	REG_PRESCALE,
	REG_TESTMODE
};


#define LED_COUNT		16

#define LED_MAX_VAL		(0x1000 - 1)





uv_errors_e uv_pca9685_init(uv_pca9685_st *this, i2c_e i2c_chn, uint8_t address,
		uv_gpios_e oe_gpio, uint32_t pwm_freq) {
	uv_errors_e ret = ERR_NONE;

	this->i2c = i2c_chn;
	// check that the given address had the fixed 1 bit set
	address |= (1 << 6);
	// MSB is fied as 1, LSB is read/write flag
	this->address = (address << 1) | (1 << 7);
	this->oe_gpio = oe_gpio;
	// reset and enable the OE pin
	uv_gpio_init_output(this->oe_gpio, true);
	this->tx.addrbyte = this->address | I2C_WRITE;
	this->tx.regbyte = REG_LED0_ON_L;
	memset(this->tx.pwm, 0, sizeof(this->tx.pwm));

	uint32_t err = ERR_NONE;
	// Reset PCA9685
	{
		uint8_t w[2] = {
				0,
				0x6

		};
		err = uv_i2cm_write(this->i2c, w, sizeof(w));
	}

	uv_rtos_task_delay(2);

	// write the pwm frequency
	{
		int32_t prescale = 25000000 / (4096 * pwm_freq);
		LIMITS(prescale, 3, 0xFF);
		uint8_t w[3] = {
				this->address | I2C_WRITE,
				REG_PRESCALE,
				prescale
		};
		err = uv_i2cm_write(this->i2c, w, sizeof(w));
	}

	// write MODE register
	// enable register auto increment and enable normal mode
	{
		uint8_t w[3] = {
				this->address | I2C_WRITE,
				REG_MODE1,
				(1 << 5), // register auto update enabled

		};
		err |= uv_i2cm_write(this->i2c, w, sizeof(w));
	}
	{
		uint8_t w[3] = {
				this->address | I2C_WRITE,
				REG_MODE2,
				(1 << 2) |// push-pull outputs
				(1 << 3) // update on ACK
		};
		err |= uv_i2cm_write(this->i2c, w, sizeof(w));
	}

	uv_rtos_task_delay(1);
	// read MODE register
	{
		uint8_t w[2] = {
				this->address | I2C_READ,
				REG_MODE1
		};
		uint8_t r[2] = { w[0] };
		err |= uv_i2cm_read(this->i2c, w, sizeof(w), r, sizeof(r));
		if (r[1] != (1 << 5) ||
				(err != ERR_NONE)) {
			uv_terminal_enable(TERMINAL_CAN);
			printf("*** PCA9685 INIT ERROR 0x%x 0x%x***\n", r[0], r[1]);
			ret = ERR_NOT_RESPONDING;
		}
	}
	// set all PWM's to zero
	uv_pca9685_update(this);

	uv_gpio_set(this->oe_gpio, false);

	return ret;
}


uv_errors_e uv_pca9685_update(uv_pca9685_st *this) {
	uv_errors_e ret = uv_i2cm_write_async(this->i2c,
		(uint8_t*) &this->tx, sizeof(this->tx));

	return ret;
}



void uv_pca9685_set(void *me, uint32_t chn, uint16_t value) {
	uv_pca9685_st *this = me;

	if (chn < PCA9685_PWM_COUNT) {
		LIMIT_MAX(value, PWM_MAX_VALUE);
		value = (value * LED_MAX_VAL + (PWM_MAX_VALUE / 2)) / PWM_MAX_VALUE;
		this->tx.pwm[chn].dc = value;
	}
}



uint16_t uv_pca9685_get(void *me, uint32_t chn) {
	uint16_t ret = 0;
	uv_pca9685_st *this = me;
	if (chn < PCA9685_PWM_COUNT) {
		ret = this->tx.pwm[chn].dc;
		ret = ((int32_t) ret * PWM_MAX_VALUE + (LED_MAX_VAL / 2)) / LED_MAX_VAL;
	}

	return ret;
}


void uv_pca9685_set_freq(void *me, uint32_t chn, uint32_t freq) {
	uv_pca9685_st *this = me;
	// write the pwm frequency
	uint32_t pwm_freq = 25000000 / (4096 * freq);
	uint8_t w[3] = {
			this->address | I2C_WRITE,
			REG_PRESCALE,
			pwm_freq
	};
	uv_i2cm_write(this->i2c, w, sizeof(w));
}


#endif
