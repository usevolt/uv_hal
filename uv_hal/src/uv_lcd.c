/*
 * uv_lcd.c
 *
 *  Created on: Aug 18, 2016
 *      Author: usevolt
 */


#include "uv_lcd.h"
#include "uv_emc.h"
#include <string.h>

#if CONFIG_LCD


uv_errors_e uv_lcd_tft_init(void) {
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

	uv_lcd_draw_rect(0, 0, LCD_W(1.0f), LCD_H(1.0f), 0);

	return uv_err(ERR_NONE);

}


void uv_lcd_draw_pixel(uint32_t x, uint32_t y, color_t value) {
	*(((LCD_PIXEL_TYPE *)(CONFIG_LCD_BUFFER_ADDRESS)) + y * CONFIG_LCD_PIXELS_PER_LINE + x) = value;
}


#define draw_hline(x, y, length, color)	do{uint32_t hlinei; \
		LCD_PIXEL_TYPE *hlineptr = (LCD_PIXEL_TYPE*)(CONFIG_LCD_BUFFER_ADDRESS) + y * LCD_W_PX + x; \
		for (hlinei = 0; hlinei < length; hlinei++) { \
			*(hlineptr++) = color;\
		} }while(0)\

void uv_lcd_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, color_t color) {
	uint32_t j;
	for (j = y; j < y + height; j++) {
		draw_hline(x, j, width, color);
	}
}

void uv_lcd_draw_frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t border, color_t color) {
	uv_lcd_draw_rect(x, y, width, border, color);
	uv_lcd_draw_rect(x + width - border, y + border, border, height - border, color);
	uv_lcd_draw_rect(x, y + border, border, height - border, color);
	uv_lcd_draw_rect(x, y + height - border, width, border, color);
}


void uv_lcd_draw_v_gradient(uint32_t x, uint32_t y, uint32_t width,
		uint32_t height, color_t t_color, color_t b_color) {
	uint32_t i;
	color_t c;
	for (i = 0; i < height; i++) {
		float rel = i / height;
		c = t_color + (t_color - b_color) * rel;
		draw_hline(x, y + i, width, c);
	}
}


#endif
