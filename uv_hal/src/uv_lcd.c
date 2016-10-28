/*
 * uv_lcd.c
 *
 *  Created on: Aug 18, 2016
 *      Author: usevolt
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
				     (CONFIG_LCD_BITS_PER_PIXEL <<  1) ;       // Bits per pixel: 24bpp
	volatile uint16_t i;
	for (i = 0; i < 256; i++) {
		LPC_LCD->PAL[i] = 0;                /* Clear palette */
	}

	LPC_LCD->CTRL |= (1 <<  0);           /* LCD enable */

	uv_lcd_draw_rect(0, 0, LCD_W(1.0f), LCD_H(1.0f), C(0));

	// initialize touchscreen ADC channels
#if CONFIG_LCD_TOUCHSCREEN
	_uv_adc_init();

#endif

	return uv_err(ERR_NONE);

}

//color_t uv_cdesat(color_t c, float mult) {
//	return c;
//	if (mult < 0.0001 && mult > -0.0001) return c;
//	rgb_st rgb = uv_ctorgb(c);
//	rgb_st r;
//	int32_t intensity = 0.3 * rgb.r + 0.6 * rgb.g + 0.1 * rgb.b;
//	r.r = uv_lerpf(mult, rgb.r, intensity);
//	r.g = uv_lerpf(mult, rgb.g, intensity);
//	r.b = uv_lerpf(mult, rgb.b, intensity);
//	return uv_rgbtoc(r);
//}



color_t uv_cmult(color_t c, float ppt) {
#if CONFIG_LCD_BITS_PER_PIXEL == LCD_24_BPP
	rgb_st t = *((rgb_st*)&c);
	rgb_st tt = {};
	int32_t e;
	e = t.r * ppt;
	if (e > 0xFF) e = 0xFF;
	else if (e < 0) e = 0;
	tt.r = e;
	e = t.g * ppt;
	if (e > 0xFF) e = 0xFF;
	else if (e < 0) e = 0;
	tt.g = e;
	e = t.b * ppt;
	if (e > 0xFF) e = 0xFF;
	else if (e < 0) e = 0;
	tt.b = e;
	return *((color_t*) &tt);


#else
#error "Color darker multiplication needs definition"
#endif
}

#define draw_hline(x, y, length, color)	do{uint32_t hlinei; \
		LCD_PIXEL_TYPE *hlineptr = &lcd[y][x]; \
		for (hlinei = 0; hlinei < length; hlinei++) { \
			*(hlineptr++) = *((int32_t*) &color);\
		} }while(0)\

void uv_lcd_draw_rect(int32_t x, int32_t y, uint32_t width, uint32_t height, color_t color) {
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > LCD_W_PX || y > LCD_H_PX) return;
	if (x + width > LCD_W_PX) width = LCD_W_PX - x;
	if (y + height > LCD_H_PX) height = LCD_H_PX - y;
	uint32_t j;
	for (j = y; j < y + height; j++) {
		draw_hline(x, j, width, color);
	}
}

void uv_lcd_draw_frame(int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t border, color_t color) {
	uv_lcd_draw_rect(x, y, width, border, color);
	uv_lcd_draw_rect(x + width - border, y + border, border, height - border, color);
	uv_lcd_draw_rect(x, y + border, border, height - border, color);
	uv_lcd_draw_rect(x, y + height - border, width, border, color);
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
	// first detect if the display is touched at all
	ADC_INIT(CONFIG_LCD_Y_T_ADC);
	ADC_INIT(CONFIG_LCD_Y_B_ADC);
	ADC_CONF(CONFIG_LCD_Y_T_ADC, ADC_PULL_UP_ENABLED);
	ADC_CONF(CONFIG_LCD_Y_B_ADC, ADC_PULL_UP_ENABLED);
	uv_gpio_init_output(CONFIG_LCD_X_R_GPIO, 0);
	uv_gpio_init_output(CONFIG_LCD_X_L_GPIO, 0);
	if (uv_adc_read_average(CONFIG_LCD_Y_T_ADC, 100) > CONFIG_LCD_TOUCH_THRESHOLD) {
		return false;
	}

	// then get the X coordinate
	ADC_INIT(CONFIG_LCD_Y_T_ADC);
	ADC_INIT(CONFIG_LCD_Y_B_ADC);
	uv_gpio_init_output(CONFIG_LCD_X_R_GPIO, 1);
	uv_gpio_init_output(CONFIG_LCD_X_L_GPIO, 0);
	int x_val = uv_adc_read_average(CONFIG_LCD_Y_T_ADC, 10);

	// then get the Y coordinate
	ADC_INIT(CONFIG_LCD_X_L_ADC);
	ADC_INIT(CONFIG_LCD_X_R_ADC);
	ADC_CONF(CONFIG_LCD_X_L_ADC, ADC_PULL_UP_ENABLED);
	ADC_CONF(CONFIG_LCD_X_R_ADC, ADC_PULL_UP_ENABLED);
	uv_gpio_init_output(CONFIG_LCD_Y_T_GPIO, 0);
	uv_gpio_init_output(CONFIG_LCD_Y_B_GPIO, 1);
	int y_val = uv_adc_read_average(CONFIG_LCD_X_L_ADC, 10);

	*x = x_val;
	*y = y_val;

	// initialize pins to default values for next call
	ADC_INIT(CONFIG_LCD_Y_T_ADC);
	ADC_INIT(CONFIG_LCD_Y_B_ADC);
	ADC_CONF(CONFIG_LCD_Y_T_ADC, ADC_PULL_UP_ENABLED);
	ADC_CONF(CONFIG_LCD_Y_B_ADC, ADC_PULL_UP_ENABLED);
	uv_gpio_init_output(CONFIG_LCD_X_R_GPIO, 0);
	uv_gpio_init_output(CONFIG_LCD_X_L_GPIO, 0);

	return true;
}


#endif

#endif
