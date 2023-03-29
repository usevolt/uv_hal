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
	uv_rtos_task_delay(2);
	uv_gpio_set(this->oe_gpio, false);

	memset(this->pwm_dc, 0, sizeof(this->pwm_dc));

	uint32_t err = ERR_NONE;

	// write MODE register
	// enable register auto increment and enable normal mode
	{
		uint8_t w[3] = {
				this->address | I2C_WRITE,
				REG_MODE1,
				(1 << 5), // register auto update enabled

		};
		err = uv_i2cm_write(this->i2c, w, sizeof(w));
	}

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
	if (ret == ERR_NONE) {
		// write the pwm frequency
		{
			pwm_freq = 25000000 / (4096 * pwm_freq);
			uint8_t w[3] = {
					this->address | I2C_WRITE,
					REG_PRESCALE,
					pwm_freq
			};
			uv_i2cm_write(this->i2c, w, sizeof(w));
		}

		// set all PWM's to zero
		for (uint16_t i = 0; i < 16; i++) {
			uint8_t w[6] = {
					this->address | I2C_WRITE,
					REG_LED0_ON_L + i * 4,
					0,
					0,
					0,
					0
			};
			uv_i2cm_write(this->i2c, w, sizeof(w));
		}
	}

	return ret;
}


void uv_pca9685_set(void *me, uint32_t chn, uint16_t value) {
	uv_pca9685_st *this = me;

//	printf("%u %u\n", chn, value);
	if (chn < PCA9685_PWM_COUNT) {
		LIMIT_MAX(value, PWM_MAX_VALUE);
		value = value * LED_MAX_VAL / PWM_MAX_VALUE;
		uint8_t w[4] = {
				this->address | I2C_WRITE,
				REG_LED0_OFF_L + chn,
				value & 0xFF,	// OFF_L
				(value >> 8)	// OFF_H
		};
//		if (this->pwm_dc[chn] != value) {
			this->pwm_dc[chn] = value;
			uv_errors_e e = uv_i2cm_write_async(this->i2c, w, sizeof(w));
			if (e != ERR_NONE) {
				uv_terminal_enable(TERMINAL_CAN);
				printf("I2C tx buffer full (%u)\n", e);
			}
//		}
	}
}



uint16_t uv_pca9685_get(void *me, uint32_t chn) {
	uint16_t ret = 0;
	uv_pca9685_st *this = me;
	if (chn < PCA9685_PWM_COUNT) {
		ret = this->pwm_dc[chn];
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
