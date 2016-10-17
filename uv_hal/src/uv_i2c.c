/*
 * i2c.c
 *
 *  Created on: Jul 22, 2016
 *      Author: usevolt
 */



#include <uv_i2c.h>

#if CONFIG_I2C

#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif



#define I2C_SEND_START(i2c)			LPC_I2C->CONSET = (1 << 5)
#define I2C_CLEAR_START(i2c)		LPC_I2C->CONCLR = (1 << 5)

#define I2C_SEND_STOP(i2c)			LPC_I2C->CONSET = (1 << 4)

#define I2C_SEND_ACK(i2c)			LPC_I2C->CONSET = (1 << 2)
#define I2C_CLEAR_ACK(i2c)			LPC_I2C->CONCLR = (1 << 2)

#define I2C_CLEAR_INT(i2c)			LPC_I2C->CONCLR = (1 << 3)

#define I2C_STATUS(i2c)				LPC_I2C->STAT
enum {
	IDLE = 0,
	READ,
	WRITE
};
typedef uint8_t states_e;



enum {
	STATUS_BUS_ERROR 					= 0,
	STATUS_IDLE 						= 0xF8,
	STATUS_START_ACK 					= 0x08,
	STATUS_REPEATED_START_ACK 			= 0x10,
	STATUS_SLAW_ACK						= 0x18,
	STATUS_SLAW_NACK					= 0x20,
	STATUS_DATW_ACK						= 0x28,
	STATUS_DATW_NACK					= 0x30,
	STATUS_ARBITRATION_LOST				= 0x38,
	STATUS_SLAR_ACK						= 0x40,
	STATUS_SLAR_NACK					= 0x48,
	STATUS_DATR_ACK						= 0x50,
	STATUS_DATR_NACK					= 0x58
};



typedef struct {
	uint8_t tx_buffer[CONFIG_I2C_TX_BUFFER_LEN];
	uint16_t byte_count;
	int16_t index;
	uint8_t *dest;
	states_e state;
	uint8_t addr;
	uint8_t error;
	uint8_t retry_count;
} i2c_module_st;

typedef struct {
#if CONFIG_TARGET_LPC11C14
	i2c_module_st modules[i2C_COUNT];
#elif CONFIG_TARGET_LPC1785
#warning "I2C not yet implemented"
#endif
} this_st;

static this_st _this;

#define this (&(_this.modules[i2c]))




uv_errors_e _uv_i2c_init(i2c_e i2c) {
	this->state = IDLE;
	this->byte_count = 0;
	this->dest = NULL;
	this->index = 0;
	this->addr = 0;

#if CONFIG_TARGET_LPC11C14
	// configure SCA and SDA pins
	LPC_IOCON->PIO0_4 = 1;
	LPC_IOCON->PIO0_5 = 1;

	// power to the I2C module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 5);
	LPC_SYSCON->PRESETCTRL |= (1 << 1);

	// clock
	SystemCoreClockUpdate();
	LPC_I2C->SCLH = SystemCoreClock / (2 * CONFIG_I2C_BAUDRATE);
	LPC_I2C->SCLL = LPC_I2C->SCLH;

	NVIC_EnableIRQ(I2C_IRQn);

#if CONFIG_I2C_MODE == I2C_MASTER
	LPC_I2C->CONSET = (1 << 6);
#elif CONFIG_I2C_MODE == I2C_SLAVE
#error "I2C slave mode not yet implemented"
#else
#error "I2C mode not recognized. Should be either I2C_MASTER or I2C_SLAVE."
#endif

#elif CONFIG_TARGET_LPC1785

#endif

	return uv_err(ERR_NONE);
}



uv_errors_e uv_i2c_read(i2c_e i2c, uint16_t id, uint16_t data_length, uint8_t *dest) {
	this->error = 0;

	if (this->index) {
		return uv_err(ERR_BUSY | HAL_MODULE_I2C);
	}

	this->addr = (id << 1) + 1;
	this->dest = dest;
	this->byte_count = data_length;
	this->index = 0;

	// if bus is idle, trigger the next byte transfer
	if (this->state == IDLE) {

		uv_i2c_step(i2c, 0);
	}

	return uv_err(ERR_NONE);
}



uv_errors_e uv_i2c_write(i2c_e i2c, uint8_t id, uint16_t data_length, uint8_t *data) {
	this->error = 0;
	if (this->index) {
		return uv_err(ERR_BUSY | HAL_MODULE_I2C);
	}
	if (data_length > CONFIG_I2C_TX_BUFFER_LEN - 1) {
		return uv_err(ERR_BUFFER_OVERFLOW | HAL_MODULE_I2C);
	}

	this->addr = id << 1;

	int i;
	for (i = 0; i < data_length; i++) {
		this->tx_buffer[i] = data[data_length - i - 1];
	}
	this->byte_count = this->index = data_length - 1;

	this->retry_count = 0;


	// if bus is idle, trigger the next byte transfer
	if (this->state == IDLE) {
		uv_i2c_step(i2c, 0);
	}

	return uv_err(ERR_NONE);
}



uv_errors_e uv_i2c_step(i2c_e i2c, uint16_t step_ms) {

	if (this->state == IDLE) {
		// starting to write
		if (this->index) {
			this->state = WRITE;
			I2C_SEND_START(i2c);
		}
		// starting to read
		else if (this->dest) {
			this->state = READ;
			I2C_SEND_START(i2c);
		}
	}
	return uv_err(ERR_NONE);
}



#if CONFIG_TARGET_LPC11C14
/// @brief: Interrupt callback
void I2C_IRQHandler (void) {
	uint8_t i2c = 0;

	uint8_t status = LPC_I2C->STAT;

	// clear START bit just to make sure we dont forget it
	I2C_CLEAR_START(i2c);
	I2C_CLEAR_ACK(i2c);

	uint8_t data;
	switch (status) {
	case STATUS_START_ACK:
		LPC_I2C->DAT = this->addr;
		break;
	case STATUS_REPEATED_START_ACK:
		break;
	case STATUS_SLAW_ACK:
	case STATUS_DATW_ACK:
		if (this->index < 0) {
			// no more data to be sent
			I2C_SEND_STOP(i2c);
			this->state = IDLE;
			this->index = 0;
			this->byte_count = 0;
		}
		else {
			LPC_I2C->DAT = this->tx_buffer[this->index--];
		}
		break;
	case STATUS_SLAR_ACK:
		I2C_SEND_ACK(i2c);
		break;
	case STATUS_DATR_ACK:
		data = LPC_I2C->DAT;

		this->dest[this->index++] = data;
		if (this->index >= this->byte_count) {
			I2C_SEND_STOP(i2c);
			this->state = IDLE;
			this->dest = NULL;
			this->index = 0;
			this->byte_count = 0;
		}
		else {
			I2C_SEND_ACK(i2c);
		}
		break;
	case STATUS_IDLE:
		this->state = IDLE;
		this->dest = NULL;
		this->index = 0;
		this->byte_count = 0;
		break;
	default:
		// error received, send a STOP byte
		I2C_SEND_STOP(i2c);

		// increase the retry count and try to send the same data again
		if (this->retry_count++ < CONFIG_I2C_RETRY_COUNT && this->state == WRITE) {
			// retry sending the same data
			this->index = this->byte_count;
			I2C_SEND_START(i2c);
		}
		else {
			this->error = status;
			this->state = IDLE;
			this->dest = NULL;
			this->index = 0;
			this->byte_count = 0;
			this->retry_count = 0;
		}

		break;
	}
	// clear pending interrupt
	I2C_CLEAR_INT(i2c);

}
#elif CONFIG_TARGET_LPC1785

#endif


uv_errors_e uv_i2c_busy(i2c_e i2c) {
	if (this->state == IDLE && !this->error) {
		return uv_err(ERR_NONE);
	}
	else if (this->error) {
		this->error = 0;
		return uv_err(ERR_NACK | HAL_MODULE_I2C);
	}
	else {
		return uv_err(ERR_BUSY | HAL_MODULE_I2C);
	}
}



#endif
