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


#include "uv_lcd.h"
#include "uv_emc.h"
#include <string.h>
#if CONFIG_LCD_TOUCHSCREEN
#include "uv_adc.h"
#include "uv_gpio.h"
#include "uv_filters.h"
#endif

#if CONFIG_LCD




lcd_line_t *lcd = (lcd_line_t *) CONFIG_LCD_BUFFER_ADDRESS;
#if CONFIG_LCD_DOUBLE_BUFFER
static volatile uint8_t double_buffer_swap_req = false;
#endif

#if CONFIG_LCD_TOUCHSCREEN
typedef struct {
	uint16_t x;
	uint16_t y;
} point_st;

#define POINTS_LEN	2
typedef struct {
	point_st points[POINTS_LEN];
	uint8_t index;

} touchscreen_st;

static touchscreen_st ts = {
		.index = 0
};
#endif



uv_errors_e _uv_lcd_init(void) {
	// initialize the GPIO pins
	CONFIG_LCD_PWR_IOCON;
	CONFIG_LCD_DCLK_IOCON;
	CONFIG_LCD_ENAB_M_IOCON;
	CONFIG_LCD_VSYNC_IOCON;
	CONFIG_LCD_HSYNC_IOCON;
	CONFIG_LCD_LE_IOCON;
	CONFIG_LCD_CLKIN_IOCON;
	CONFIG_LCD_VD0_IOCON;
	CONFIG_LCD_VD1_IOCON;
	CONFIG_LCD_VD2_IOCON;
	CONFIG_LCD_VD3_IOCON;
	CONFIG_LCD_VD4_IOCON;
	CONFIG_LCD_VD5_IOCON;
	CONFIG_LCD_VD6_IOCON;
	CONFIG_LCD_VD7_IOCON;
	CONFIG_LCD_VD8_IOCON;
	CONFIG_LCD_VD9_IOCON;
	CONFIG_LCD_VD10_IOCON;
	CONFIG_LCD_VD11_IOCON;
	CONFIG_LCD_VD12_IOCON;
	CONFIG_LCD_VD13_IOCON;
	CONFIG_LCD_VD14_IOCON;
	CONFIG_LCD_VD15_IOCON;
	CONFIG_LCD_VD16_IOCON;
	CONFIG_LCD_VD17_IOCON;
	CONFIG_LCD_VD18_IOCON;
	CONFIG_LCD_VD19_IOCON;
	CONFIG_LCD_VD20_IOCON;
	CONFIG_LCD_VD21_IOCON;
	CONFIG_LCD_VD22_IOCON;
	CONFIG_LCD_VD23_IOCON;

	// set clock frequency
	LPC_SC->LCD_CFG = SystemCoreClock / CONFIG_LCD_FREQ;


	// enable power to the LCD controller
	LPC_SC->PCONP |= (1 << 0);

	// set the panel data adress
	LPC_LCD->UPBASE = CONFIG_LCD_BUFFER_ADDRESS;

	LPC_LCD->TIMH  = ((CONFIG_LCD_H_BACK_PORCH - 1)   << 24) |       // Horizontal back porch
				     ((CONFIG_LCD_H_FRONT_PORCH - 1)   << 16) |       // Horizontal front porch
				     ((CONFIG_LCD_HSYNC_PULSE_WIDTH - 1)    <<  8) |       // Horizontal sync pulse width
				     ((CONFIG_LCD_PIXELS_PER_LINE / 16 - 1)   <<  2) ;       // Pixels-per-line
	LPC_LCD->TIMV  = ((CONFIG_LCD_V_BACK_PORCH)   << 24) |       // Vertical back porch
				     ((CONFIG_LCD_V_FRONT_PORCH)    << 16) |       // Vertical front porch
				     ((CONFIG_LCD_VSYNC_PULSE_WIDTH - 1)    << 10) |       // Vertical sync pulse width
				     ((CONFIG_LCD_LINES_PER_PANEL - 1)  <<  0) ;       // Lines per panel
	LPC_LCD->POL   = (1    << 26) |       // Bypass pixel clock divider
				     ((CONFIG_LCD_PIXELS_PER_LINE - 1)  << 16) |       // Clocks per line: num of LCDCLKs
				     (CONFIG_LCD_INVERT_PANEL_CLOCK    << 13) |       // Invert panel clock
				     (CONFIG_LCD_INVERT_HSYNC    << 12) |       // Invert HSYNC
				     (CONFIG_LCD_INVERT_VSYNC    << 11) ;       // Invert VSYNC
	LPC_LCD->LE    = (0    << 16) |       // LCDLE Enabled: 1, Disabled: 0
				     (9    <<  0) ;       // Line-end delay: LCDCLK clocks - 1
	LPC_LCD->CTRL  = (1    << 11) |       // LCD Power Enable
					 (CONFIG_LCD_RGB_TO_BGR << 8) |	//RGB or BGR color space
				     (1    <<  5) |       // 0 = STN display, 1: TFT display
				     (CONFIG_LCD_BITS_PER_PIXEL <<  1) ;       // Bits per pixel
	volatile uint16_t i;
	for (i = 0; i < 256; i++) {
		LPC_LCD->PAL[i] = 0;                /* Clear palette */
	}

	uv_lcd_draw_rect(0, 0, LCD_W(1.0f), LCD_H(1.0f), C(0));

#if CONFIG_LCD_DOUBLE_BUFFER
	LPC_LCD->INTMSK |= (1 << 2);
	LPC_LCD->INTCLR |= (1 << 2);
	NVIC_EnableIRQ(LCD_IRQn);
	lcd = (lcd_line_t*) CONFIG_LCD_DOUBLE_BUFFER_ADDRESS;
#endif

	LPC_LCD->CTRL |= (1 <<  0);           /* LCD enable */

	// initialize touchscreen ADC channels
#if CONFIG_LCD_TOUCHSCREEN
	_uv_adc_init();
#endif

	return ERR_NONE;
}

#if CONFIG_LCD_DOUBLE_BUFFER
void LCD_IRQHandler(void) {
	// check if base address update interrupt is active
	if (LPC_LCD->INTSTAT & (1 << 2)) {
		// change the buffer address if application has requested it
		if (double_buffer_swap_req) {
			double_buffer_swap_req = false;
			if (lcd == (lcd_line_t*) CONFIG_LCD_BUFFER_ADDRESS) {
				lcd = (lcd_line_t*) CONFIG_LCD_DOUBLE_BUFFER_ADDRESS;
				LPC_LCD->UPBASE = CONFIG_LCD_BUFFER_ADDRESS;
			}
			else {
				lcd = (lcd_line_t*) CONFIG_LCD_BUFFER_ADDRESS;
				LPC_LCD->UPBASE = CONFIG_LCD_DOUBLE_BUFFER_ADDRESS;
			}
			// lastly update both buffer addresses to contain the same data
			memcpy(lcd, (void*) LPC_LCD->UPBASE,
					CONFIG_LCD_PIXELS_PER_LINE *
					CONFIG_LCD_LINES_PER_PANEL *
					sizeof(LCD_PIXEL_TYPE));
		}
	}

	// clear all active interrupts
	LPC_LCD->INTCLR = LPC_LCD->INTSTAT;
}
#endif



#define draw_hline(x, y, length, color)	\
	do { int32_t hlinei; \
		LCD_PIXEL_TYPE *hlineptr = &lcd[y][x]; \
		for (hlinei = 0; hlinei < length; hlinei++) { \
			*(hlineptr++) = *((int32_t*) &color); \
		} \
	} while(0)


void uv_lcd_draw_mrect(int32_t x, int32_t y, int32_t width, int32_t height, const color_t c,
		const uv_bounding_box_st *maskbb) {

	if (x < maskbb->x) {
		width -= maskbb->x - x;
		x = maskbb->x;
	}
	if (y < maskbb->y) {
		height -= maskbb->y - y;
		y = maskbb->y;
	}

	if (x > LCD_W_PX || y > LCD_H_PX) { return; }

	if ((x + width) > (maskbb->x + maskbb->width)) { width = maskbb->x + maskbb->width - x; }
	if ((y + height) > (maskbb->y + maskbb->height)) { height = maskbb->y + maskbb->height - y; }
	if (x + width > LCD_W_PX) { width = LCD_W_PX - x; }
	if (y + height > LCD_H_PX) { height = LCD_H_PX - y; }
	if ((width < 0) || (height < 0)) {
		return;
	}
	uint32_t j;
	for (j = y; j < y + height; j++) {
		draw_hline(x, j, width, c);
	}
}



void uv_lcd_draw_mframe(int32_t x, int32_t y, int32_t width, int32_t height,
		const int32_t border, const color_t color, const uv_bounding_box_st *maskbb) {
	uv_lcd_draw_mrect(x, y, width, border, color, maskbb);
	uv_lcd_draw_mrect(x + width - border, y + border, border, height - border, color,
			maskbb);
	uv_lcd_draw_mrect(x, y + border, border, height - border, color,
			maskbb);
	uv_lcd_draw_mrect(x, y + height - border, width, border, color,
			maskbb);
}



void uv_lcd_double_buffer_swap(void) {
#if CONFIG_LCD_DOUBLE_BUFFER
	double_buffer_swap_req = true;
#endif
}


uint8_t *uv_lcd_get_active_buffer_ptr(void) {
	return (uint8_t*) lcd;
}



#if CONFIG_LCD_TOUCHSCREEN

void uv_lcd_touch_calib(uint16_t x, uint16_t y) {
	if (ts.index < POINTS_LEN - 1) {
		ts.points[ts.index].x = x;
		ts.points[ts.index++].y = y;
	}
}



void uv_lcd_touch_calib_clear(void) {
	ts.index = 0;
}



bool uv_lcd_touch_get(int16_t *x, int16_t *y) {
	bool ret = true;

	// first detect if the display is touched at all
	ADC_INIT(CONFIG_LCD_Y_T_ADC);
	ADC_INIT(CONFIG_LCD_Y_B_ADC);
	ADC_CONF(CONFIG_LCD_Y_T_ADC, ADC_PULL_UP_ENABLED);
	ADC_CONF(CONFIG_LCD_Y_B_ADC, ADC_PULL_UP_ENABLED);
	UV_GPIO_INIT_OUTPUT(CONFIG_LCD_X_R_GPIO, 0);
	UV_GPIO_INIT_OUTPUT(CONFIG_LCD_X_L_GPIO, 0);
	if (uv_adc_read_average(CONFIG_LCD_Y_T_ADC, 4) > CONFIG_LCD_TOUCH_THRESHOLD) {
		ret = false;
	}

	if (ret) {
		// then get the X coordinate
		ADC_INIT(CONFIG_LCD_Y_T_ADC);
		ADC_INIT(CONFIG_LCD_Y_B_ADC);
		UV_GPIO_INIT_OUTPUT(CONFIG_LCD_X_R_GPIO, 1);
		UV_GPIO_INIT_OUTPUT(CONFIG_LCD_X_L_GPIO, 0);
		int x_val = uv_adc_read_average(CONFIG_LCD_Y_T_ADC, 10);
		if (x_val < CONFIG_LCD_X_MIN) x_val = CONFIG_LCD_X_MIN;
		else if (x_val > CONFIG_LCD_X_MAX) x_val = CONFIG_LCD_X_MAX;
		int x_px = uv_reli(x_val, CONFIG_LCD_X_MIN, CONFIG_LCD_X_MAX) *
				CONFIG_LCD_PIXELS_PER_LINE / 1000;

		// then get the Y coordinate
		ADC_INIT(CONFIG_LCD_X_L_ADC);
		ADC_INIT(CONFIG_LCD_X_R_ADC);
		ADC_CONF(CONFIG_LCD_X_L_ADC, ADC_PULL_UP_ENABLED);
		ADC_CONF(CONFIG_LCD_X_R_ADC, ADC_PULL_UP_ENABLED);
		UV_GPIO_INIT_OUTPUT(CONFIG_LCD_Y_T_GPIO, 0);
		UV_GPIO_INIT_OUTPUT(CONFIG_LCD_Y_B_GPIO, 1);
		int y_val = uv_adc_read_average(CONFIG_LCD_X_L_ADC, 10);
		if (y_val < CONFIG_LCD_Y_MIN) y_val = CONFIG_LCD_Y_MIN;
		else if (y_val > CONFIG_LCD_Y_MAX) y_val = CONFIG_LCD_Y_MAX;
		int y_px = uv_reli(y_val, CONFIG_LCD_Y_MIN, CONFIG_LCD_Y_MAX) *
				CONFIG_LCD_LINES_PER_PANEL / 1000;

		if (x) { *x = x_px; }
		if (y) { *y = y_px; }

		// initialize pins to default values for next call
		ADC_INIT(CONFIG_LCD_Y_T_ADC);
		ADC_INIT(CONFIG_LCD_Y_B_ADC);
		ADC_CONF(CONFIG_LCD_Y_T_ADC, ADC_PULL_UP_ENABLED);
		ADC_CONF(CONFIG_LCD_Y_B_ADC, ADC_PULL_UP_ENABLED);
		UV_GPIO_INIT_OUTPUT(CONFIG_LCD_X_R_GPIO, 0);
		UV_GPIO_INIT_OUTPUT(CONFIG_LCD_X_L_GPIO, 0);
	}

	return ret;
}

#endif

#endif
