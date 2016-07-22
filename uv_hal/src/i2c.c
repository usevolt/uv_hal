/*
 * i2c.c
 *
 *  Created on: Jul 22, 2016
 *      Author: usevolt
 */



#include "i2c.h"

#if I2C

#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif



typedef struct {
	uv_ring_buffer_st rx;
	uv_ring_buffer_st tx;
	uint8_t rx_buffer[CONFIG_I2C_RX_BUFFER_LEN];
	uint8_t tx_buffer[CONFIG_I2C_TX_BUFFER_LEN];
} i2c_module_st;

typedef struct {
#if CONFIG_TARGET_LPC11C14
	i2c_module_st i2c1;
#elif CONFIG_TARGET_LPC1785
#warning "I2C not yet implemented"
#endif
} this_st;

static this_st _this;

#define this (&_this)


uv_errors_e i2c_init(i2c_e i2c) {

}


uv_errors_e i2c_send(i2c_e i2c, uint16_t data) {

}


uv_errors_e i2c_read(i2c_e i2c, uint8_t *data) {

}


bool i2c_ready(i2c_e i2c) {

}

#endif
