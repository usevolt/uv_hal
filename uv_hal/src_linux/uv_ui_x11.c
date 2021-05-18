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


#include "uv_ui_common.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#include "uv_uifont.h"
#include "uv_memory.h"
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>

#if CONFIG_UI



typedef enum {
	CMD_DLSWAP = 0,
	CMD_CLEAR,
	CMD_DRAW_BITMAP_EXT,
	CMD_DRAW_POINT,
	CMD_DRAW_RRECT,
	CMD_DRAW_LINE,
	CMD_DRAW_LINESTRIP,
	CMD_DRAW_STRING,
	CMD_SET_MASK,
	CMD_COUNT
} draw_cmd_e;

/// @brief: Defines a struct that is used to pass the drawing command to the DTK
/// thread. When calling the *uv_ui_common* drawing functions, they append these
/// *ui_draw_cmd_st* structs to the drawing queue, and after call to *uv_ui_dlswap*,
/// the GTK thread actually draws these.
typedef struct {
	// the type of this draw command
	draw_cmd_e type;
	struct {

	} dlswap;
	struct {
		color_t c;
	} clear;
	struct {
		uv_uimedia_st *bitmap;
		int32_t x;
		int32_t y;
		int16_t w;
		int16_t h;
		uint32_t wrap;
		color_t c;
	} draw_bitmap_ext;
	struct {
		int32_t x;
		int32_t y;
		color_t c;
		uint16_t diameter;
	} draw_point;
	struct {
		int32_t x;
		int32_t y;
		uint16_t w;
		uint16_t h;
		uint16_t radius;
		color_t c;
	} draw_rrect;
	struct {
		int32_t x_start;
		int32_t y_start;
		int32_t x_end;
		int32_t y_end;
		uint16_t w;
		color_t c;
	} draw_line;
	struct {
		uv_ui_linestrip_point_st points[64];
		uint16_t point_count;
		uint16_t line_w;
		color_t c;
		uv_ui_strip_type_e type;
	} draw_linestrip;
	struct {
		int32_t x;
		int32_t y;
		color_t c;
		alignment_e align;
		char str[1024];
		uv_font_st *font;
	} draw_string;
	struct {
		int32_t x;
		int32_t y;
		int32_t w;
		int32_t h;
	} set_mask;
} ui_draw_cmd_st;

#define DRAW_CMD_BUFFER_LEN			500


/// @brief: The main uv_ui structure that holds the state of the ui drawn
typedef struct {
	cairo_surface_t *surface;
	cairo_t *cairo;
	bool pressed;
	int32_t x;
	int32_t y;
} ui_st;

static ui_st _ui = {
		.surface = NULL,
		.cairo = NULL
};
#ifdef this
#undef this
#endif
#define this (&_ui)


#define CAIRO_C(color_st)	(((double) (color_st)) / 255.0)



ui_font_st ui_fonts[UI_MAX_FONT_COUNT];
static const uint8_t font_sizes[UI_MAX_FONT_COUNT] = {
		16,
		20,
		25,
		28,
		36,
		49,
		63,
		83,
		108
};




void uv_ui_set_backlight(uint8_t percent) {

}



uint8_t uv_ui_get_backlight(void) {
	return 100;
}




bool uv_ui_get_touch(int16_t *x, int16_t *y) {
	bool ret;

	while (XPending(cairo_xlib_surface_get_display(this->surface))) {
		XEvent e;
		XNextEvent(cairo_xlib_surface_get_display(this->surface), &e);

		switch (e.type) {
		case ButtonPress:
			if (e.xbutton.button == 1) {
				this->pressed = true;
				this->x = (int16_t) e.xbutton.x;
				this->y = (int16_t) e.xbutton.y;
			}
			break;
		case ButtonRelease:
			if (e.xbutton.button == 1) {
				this->pressed = false;
				this->x = (int16_t) e.xbutton.x;
				this->y = (int16_t) e.xbutton.y;
			}
			break;
		case MotionNotify:
			this->pressed = true;
			this->x = (int16_t) e.xmotion.x;
			this->y = (int16_t) e.xmotion.y;
			break;
		default:
			fprintf(stderr, "Dropping unhandled XEevent.type = %d.\n", e.type);
			break;
		}
	}

	ret = this->pressed;
	*x = this->x;
	*y = this->y;

	return ret;
}



void uv_ui_clear(color_t col) {
	color_st c = uv_uic(col);
	cairo_set_source_rgba(this->cairo,
			CAIRO_C(c.r), CAIRO_C(c.g), CAIRO_C(c.b), CAIRO_C(c.a));
	cairo_paint(this->cairo);
}



void uv_ui_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {
}



void uv_ui_draw_point(int16_t x, int16_t y, color_t col, uint16_t diameter) {
	color_st c = uv_uic(col);
	cairo_set_source_rgba(this->cairo,
			CAIRO_C(c.r), CAIRO_C(c.g), CAIRO_C(c.b), CAIRO_C(c.a));
	cairo_arc(this->cairo, (double) x, (double) y, diameter / 2, 0, 2 * M_PI);
	cairo_fill(this->cairo);
}



void uv_ui_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t w, const uint16_t h,
		const uint16_t radius, const color_t col) {
	color_st c = uv_uic(col);

	/* a custom shape that could be wrapped in a function */
	double dx         = (double) x,        /* parameters like cairo_rectangle */
	       dy         = (double) y,
	       width         = (double) w,
	       height        = (double) h,
	       aspect        = 1.0,     /* aspect ratio */
	       corner_radius = height / 10.0;   /* and corner curvature radius */

	double r = corner_radius / aspect;
	double degrees = M_PI / 180.0;

	cairo_new_sub_path (this->cairo);
	cairo_arc (this->cairo, dx + width - r, dy + r, r, -90 * degrees, 0 * degrees);
	cairo_arc (this->cairo, dx + width - r, dy + height - r, r, 0 * degrees, 90 * degrees);
	cairo_arc (this->cairo, dx + r, dy + height - r, r, 90 * degrees, 180 * degrees);
	cairo_arc (this->cairo, dx + r, dy + r, r, 180 * degrees, 270 * degrees);
	cairo_close_path (this->cairo);

	cairo_set_source_rgba(this->cairo,
			CAIRO_C(c.r), CAIRO_C(c.g), CAIRO_C(c.b), CAIRO_C(c.a));
	cairo_fill_preserve (this->cairo);
	cairo_set_line_width (this->cairo, radius);
	cairo_stroke (this->cairo);
}



void uv_ui_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t col) {
	color_st c = uv_uic(col);
}



void uv_ui_draw_linestrip(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
}



void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix) {

}




void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
}



void uv_ui_set_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	cairo_reset_clip(this->cairo);
	cairo_rectangle(this->cairo, (double) x, (double) y, (double) width, (double) height);
	cairo_clip(this->cairo);
}



int16_t uv_ui_get_string_height(char *str, ui_font_st *font) {
	int16_t ret = 0;

	return ret;
}



int16_t uv_ui_get_string_width(char *str, ui_font_st *font) {
	int16_t ret = 0;

	return ret;
}


uint32_t uv_uimedia_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename) {
	uint32_t ret = 0;


	return ret;
}


void uv_ui_touchscreen_set_transform_matrix(ui_transfmat_st *transform_matrix) {

}


void uv_ui_dlswap(void) {

}







bool uv_ui_init(void) {

	printf("X11 Started\n");
	Display *dsp;
	Drawable da;
	int screen;

	this->pressed = false;
	this->x = 0;
	this->y = 0;

	if ((dsp = XOpenDisplay(NULL)) == NULL) {
		printf("Failed to open X11 display\n");
		exit(1);
	}
	screen = DefaultScreen(dsp);
	da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0,
			CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE, 0, 0, 0);
	XSelectInput(dsp, da, ButtonPressMask | ButtonReleaseMask | Button1MotionMask);
	XStoreName(dsp, da, uv_projname);
	XMapWindow(dsp, da);

	this->surface = cairo_xlib_surface_create(dsp, da, DefaultVisual(dsp, screen),
			CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
	cairo_xlib_surface_set_size(this->surface, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
	this->cairo = cairo_create(this->surface);

	return false;
}



void uv_ui_destroy(void) {
	if (this->surface) {
		Display *dsp = cairo_xlib_surface_get_display(this->surface);

		cairo_destroy(this->cairo);
		cairo_surface_destroy(this->surface);
		XCloseDisplay(dsp);

		printf("X11 closed\n");
	}
}



#endif
