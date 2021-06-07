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
#include "ui/uv_uifont.h"
#include "uv_memory.h"
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <unistd.h>
#include "uv_ui.h"
#include "uv_can.h"

#if CONFIG_UI





ui_font_st ui_fonts[UI_MAX_FONT_COUNT];
static const uint8_t font_sizes[UI_MAX_FONT_COUNT] = {
		14,
		18,
		21,
		23,
		28,
		35,
		47,
		68,
		90
};

static const uv_uistyle_st uistyle  = {
		.bg_c = C(0xFFCCCCCC),
		.fg_c = C(0xFFAAAAAA),
		.window_c = C(0xFFFFFFFF),
		.display_c = C(0xFFFFFFFF),
		.font = &font16,
		.text_color = C(0xFF000000)
};


/// @brief: A linked list member for the uimedia files that are loaded with
/// *uv_uimedia_load* functions
typedef struct {
	char filename[128];
	// pointer to the next uimedia_ll_st
	void *next_ptr;
	cairo_surface_t *surface;
} uimedia_ll_st;



/// @brief: The main uv_ui structure that holds the state of the ui drawn
typedef struct {
	XIC ic;
	cairo_surface_t *surface;
	cairo_t *cairo;
	bool pressed;
	int32_t x;
	int32_t y;
	double scalex;
	double scaley;
	uimedia_ll_st *uimediall;
	uv_ring_buffer_st key_press;
	char key_press_buffer[20];
	uint8_t brightness;

	// configuration window that is shown when setting the settings
	struct {
		bool terminate;
		uv_uidisplay_st display;
		uv_uiobject_st *display_buffer[10];

		uv_uilistbutton_st can_listbutton;
		char *can_listbutton_content[10];

		uv_uidigitedit_st baud_digiedit;

		uv_uibutton_st ok_button;
	} confwindow;
} ui_st;

static ui_st _ui = {
		.surface = NULL,
		.cairo = NULL,
		.uimediall = NULL,
		.brightness = 50
};

#ifdef this
#undef this
#endif
#define this (&_ui)


#define CAIRO_C(color_st)	(((double) (color_st)) / 255.0)





static uv_uiobject_ret_e confwindow_step(void *, uint16_t);



void ui_x11_confwindow_exec(void) {
	uv_ui_init();

	this->confwindow.terminate = false;
	uv_uidisplay_init(&this->confwindow.display, this->confwindow.display_buffer, &uistyle);
	uv_uiwindow_set_stepcallback(&this->confwindow.display, &confwindow_step, NULL);

	uv_uistrlayout_st layout;
	uv_uistrlayout_init(&layout,
			"#3can|#2baud\n"
			"nc\n"
			"nc|nc|nc\n"
			"#4nc|close",
			0, 0, uv_uibb(&this->confwindow.display)->w, uv_uibb(&this->confwindow.display)->h,
			5, 5);
	uv_bounding_box_st bb;

	bb = uv_uistrlayout_find(&layout, "can");
	uint32_t index = 0;
	// load the list of CAN devices
	uv_can_set_baudrate(uv_can_get_dev(), 250000);
	this->confwindow.can_listbutton_content[0] = "NONE";
	for (uint32_t i = 0; i < uv_can_get_device_count(); i++) {
		printf("%s\n", uv_can_get_device_name(i));
		this->confwindow.can_listbutton_content[i] = uv_can_get_device_name(i);
		if (strcmp(this->confwindow.can_listbutton_content[i], uv_can_get_dev()) == 0) {
			index = i;
		}
	}
	uv_uilistbutton_init(&this->confwindow.can_listbutton, this->confwindow.can_listbutton_content,
			MAX(uv_can_get_device_count(), 1),
			index, &uistyle);
	uv_uilistbutton_set_content_type_arrayofpointers(&this->confwindow.can_listbutton);
	uv_uilistbutton_set_title(&this->confwindow.can_listbutton, "CAN dev:");
	uv_uidialog_add(&this->confwindow.display, &this->confwindow.can_listbutton, &bb);

	bb = uv_uistrlayout_find(&layout, "baud");
	uv_uidigitedit_init(&this->confwindow.baud_digiedit,
			uv_can_get_baudrate(uv_can_get_dev()) / 1000,
			&uistyle);
	uv_uidigitedit_set_title(&this->confwindow.baud_digiedit, "CAN baudrate (kBaud)");
	uv_uidigitedit_set_limits(&this->confwindow.baud_digiedit, 0, 1000);
	uv_uidigitedit_set_mode(&this->confwindow.baud_digiedit, UIDIGITEDIT_MODE_INCDEC);
	uv_uidigitedit_set_inc_step(&this->confwindow.baud_digiedit, 50);
	uv_uidisplay_add(&this->confwindow.display, &this->confwindow.baud_digiedit, &bb);

	bb = uv_uistrlayout_find(&layout, "close");
	uv_uibutton_init(&this->confwindow.ok_button, "OK", &uistyle);
	uv_uidisplay_add(&this->confwindow.display, &this->confwindow.ok_button, &bb);

	while (true) {
		uint16_t step_ms = 20;

		uv_uidisplay_step(&this->confwindow.display, step_ms);

		if (this->confwindow.terminate) {
			break;
		}

		usleep(step_ms * 1000);
	}
	uv_ui_destroy();
	printf("Closed the configuration UI\n");
}


static uv_uiobject_ret_e confwindow_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;


	if (uv_uilistbutton_clicked(&this->confwindow.can_listbutton)) {
		uv_can_set_dev(this->confwindow.can_listbutton_content[
		   uv_uilistbutton_get_current_index(&this->confwindow.can_listbutton)]);
		uv_can_set_baudrate(uv_can_get_dev(),
				uv_uidigitedit_get_value(&this->confwindow.baud_digiedit) * 1000);
	}
	else if (uv_uidigitedit_value_changed(&this->confwindow.baud_digiedit)) {
		uv_can_set_baudrate(uv_can_get_dev(),
				uv_uidigitedit_get_value(&this->confwindow.baud_digiedit) * 1000);
	}
	else if (uv_uibutton_clicked(&this->confwindow.ok_button)) {
		this->confwindow.terminate = true;
	}
	else {

	}

	return ret;
}




void uv_ui_set_backlight(uint8_t percent) {
	this->brightness = percent;
}



uint8_t uv_ui_get_backlight(void) {
	return this->brightness;
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
				printf("%i %i\n", this->x, this->y);
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
		case KeyPress: {
			int count = 0;
			KeySym keysym = 0;
			char buf[20];
			Status status = 0;
			count = Xutf8LookupString(this->ic,
					(XKeyPressedEvent*)&e, buf, 20, &keysym, &status);
			if (status == XBufferOverflow) {
				printf("BufferOverflow\n");
			}
			for (uint32_t i = 0; i < count; i++) {
				uv_ring_buffer_push(&this->key_press, &buf[i]);
			}

			break;
		}
		case KeyRelease:
			break;
		case ResizeRequest: {
			uint32_t ww = e.xresizerequest.width,
			wh = e.xresizerequest.height,
			cw = CONFIG_FT81X_HSIZE,
			ch = CONFIG_FT81X_VSIZE;
			this->scalex = (double) ww / (double) cw,
			this->scaley = (double) wh / (double) ch;
			break;
		}
		case 65:
			// unknown event 65
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


char uv_ui_get_key_press(void) {
	char ret = '\0';
	uv_ring_buffer_pop(&this->key_press, &ret);
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
	cairo_set_source_surface(this->cairo, bitmap->surface_ptr, x, y);
	cairo_paint(this->cairo);
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
		const uint16_t width, const color_t color) {
	color_st c = uv_uic(color);

	cairo_set_source_rgba(this->cairo, CAIRO_C(c.r), CAIRO_C(c.g),
			CAIRO_C(c.b), CAIRO_C(c.a));
	cairo_set_line_width(this->cairo, width);
	cairo_set_line_cap(this->cairo, CAIRO_LINE_CAP_ROUND);
	cairo_move_to(this->cairo, (double) start_x, (double) start_y);
    cairo_line_to(this->cairo, (double) end_x, (double) end_y);
    cairo_stroke(this->cairo);
    cairo_move_to(this->cairo, 0, 0);
}



void uv_ui_draw_linestrip(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
}



void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix) {

}




void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {

	color_st c = uv_uic(color);

	cairo_set_font_size(this->cairo, font->char_height);
	cairo_set_source_rgba(this->cairo, CAIRO_C(c.r), CAIRO_C(c.g),
			CAIRO_C(c.b), CAIRO_C(c.a));
	char *s = malloc(strlen(str) + 2);
	strcpy(s, str);
	char *last_s = s;
	// calculate the number of lines
	uint16_t line_count = 0;
	for (uint32_t i = 0; i < strlen(str) + 1; i++) {
		if (str[i] == '\n' || str[i] == '\0') {
			line_count++;
		}
	}

//	// Debug drawing
//	cairo_set_source_rgba(this->cairo,
//			CAIRO_C(0xFF), CAIRO_C(0), CAIRO_C(0), CAIRO_C(c.a / 2));
//	cairo_rectangle(this->cairo, (double) x, (double) y, 2, 2);
//	cairo_set_line_width(this->cairo, 0.5);
//	cairo_fill(this->cairo);

	cairo_font_extents_t fontex;
	cairo_font_extents(this->cairo, &fontex);
	if (align & VALIGN_CENTER) {
		// reduce the y by the number of line counts
		y -= line_count * fontex.height / 2;
	}
	for (uint32_t i = 0; i < strlen(str) + 1; i++) {
		if (s[i] == '\r' || s[i] == '\n' || s[i] == '\0') {
			s[i] = '\0';
			int16_t lx = x, ly = y;

			// draw one line of text
			cairo_text_extents_t ex;
			cairo_text_extents(this->cairo, last_s, &ex);

//			// Debug drawing
//			cairo_set_source_rgba(this->cairo,
//					CAIRO_C(c.r), CAIRO_C(c.g), CAIRO_C(c.b), CAIRO_C(c.a / 2));
//			cairo_rectangle(this->cairo, (double) x, (double) y, ex.width, fontex.height);
//			cairo_set_line_width(this->cairo, 0.5);
//			cairo_fill(this->cairo);

			if (align & HALIGN_CENTER) {
				lx -= ex.width / 2;
			}
			else if (align & HALIGN_RIGHT) {
				lx -= ex.width;
			}
			else {

			}
			ly += ex.height + ((fontex.height - ex.height) / 2);
			cairo_move_to(this->cairo, (double) lx, (double) ly);
			if (strlen(last_s)) {
				cairo_show_text(this->cairo, last_s);

				y += fontex.height;
			}
			last_s = &s[i + 1];
		}
	}
	free(s);
	cairo_move_to(this->cairo, 0.0, 0.0);
}



void uv_ui_set_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	cairo_reset_clip(this->cairo);
	cairo_rectangle(this->cairo, (double) x, (double) y, (double) width, (double) height);
	cairo_clip(this->cairo);
}




int16_t uv_ui_get_string_width(char *str, ui_font_st *font) {
	fflush(stdout);
	int16_t ret = 0;
	cairo_set_font_size(this->cairo, font->char_height);

	char *string = malloc(strlen(str) + 1);
	char *s = string;
	char *last_s = s;
	strcpy(s, str);
	while (*s != '\0') {
		if (*s == '\n') {
			*s = '\0';
			cairo_text_extents_t ex;
			cairo_text_extents(this->cairo, last_s, &ex);
			ret = MAX(ret, (unsigned int) ex.width);
			last_s = s + 1;
		}
		s++;
	}
	free(string);

	return ret;
}


uint32_t uv_uimedia_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename) {
	uint32_t ret = 0;

	bool match = false;
	uimedia_ll_st *m = this->uimediall;
	uimedia_ll_st *last = NULL;
	// try to find the uimedia file if it's already loaded
	while (m != NULL) {
		if (strcmp(filename, m->filename) == 0) {
			match = true;
			ret = 1;
			break;
		}
		last = m;
		m = m->next_ptr;
	}
	if (match) {
		// match already found
		printf("Bitmap '%s' already existed\n", filename);
		bitmap->surface_ptr = m->surface;
		bitmap->height = cairo_image_surface_get_height(bitmap->surface_ptr);
		bitmap->width = cairo_image_surface_get_width(bitmap->surface_ptr);
		bitmap->type = UV_UIMEDIA_IMAGE;
		ret = 1;
	}
	else {
		m = malloc(sizeof(uimedia_ll_st));
		strcpy(m->filename, filename);
		m->next_ptr = NULL;
		// create link to the new uimedia linked list entry
		if (last) {
			last->next_ptr = m;
		}
		else {
			this->uimediall = m;
		}
		// load new file
		m->surface = cairo_image_surface_create_from_png(filename);
		if (m->surface != NULL) {
			bitmap->height = cairo_image_surface_get_height(m->surface);
			bitmap->width = cairo_image_surface_get_width(m->surface);
			bitmap->surface_ptr = m->surface;
			bitmap->type = UV_UIMEDIA_IMAGE;
			ret = 1;
			printf("Bitmap '%s' loaded\n", filename);
		}
		else {
			fprintf(stderr, "Could not create surface from png file '%'s\n", filename);
			ret = 0;
		}
	}


	return ret;
}


void uv_ui_touchscreen_set_transform_matrix(ui_transfmat_st *transform_matrix) {

}


void uv_ui_dlswap(void) {
	if (this->cairo) {
		cairo_pop_group_to_source(this->cairo);
		cairo_paint(this->cairo);
		cairo_xlib_surface_set_size(this->surface,
				(int) ((double) CONFIG_FT81X_HSIZE * this->scalex),
				(int) ((double) CONFIG_FT81X_VSIZE * this->scaley));
		cairo_push_group(this->cairo);
		// apply the window resize scale. This has to be set right after creating the group,
		// so that all drawing is affected
		cairo_scale(this->cairo, this->scalex, this->scaley);
	}
}







bool uv_ui_init(void) {

	Display *dsp;
	Drawable da;
	int screen;

	this->pressed = false;
	this->x = 0;
	this->y = 0;
	this->scalex = 1.0;
	this->scaley = 1.0;

	if ((dsp = XOpenDisplay(NULL)) == NULL) {
		printf("Failed to open X11 display\n");
		exit(1);
	}
	printf("X11 Started\n");
	screen = DefaultScreen(dsp);
	da = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0,
			CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE, 0, 0, 0);
	XSelectInput(dsp, da, ButtonPressMask | ButtonReleaseMask |
			Button1MotionMask | KeyPressMask | ResizeRedirectMask);
	XStoreName(dsp, da, uv_projname);
	XMapWindow(dsp, da);

    XIM im;
    im = XOpenIM(dsp, NULL, NULL, NULL);
	if (im == NULL) {
		fputs("Could not open input method\n", stdout);
		exit(1);
	}

	this->ic = XCreateIC(im, XNInputStyle,
    		XIMPreeditNothing | XIMStatusNothing, XNClientWindow, da, NULL);
    if (this->ic == NULL) {
        printf("Could not open IC\n");
        exit(1);
    }

    XSetICFocus(this->ic);

	this->surface = cairo_xlib_surface_create(dsp, da, DefaultVisual(dsp, screen),
			CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
	cairo_xlib_surface_set_size(this->surface, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
	this->cairo = cairo_create(this->surface);
	// create new group that will be popped in dlswap function
	cairo_push_group(this->cairo);

	// initialize the font sizes
	for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
		ui_fonts[i].char_height = font_sizes[i];
	}
	// initialize the key press buffer
	uv_ring_buffer_init(&this->key_press, this->key_press_buffer,
			sizeof(this->key_press_buffer) / sizeof(this->key_press_buffer[0]),
			sizeof(this->key_press_buffer[0]));

	return false;
}



void uv_ui_destroy(void) {
	if (this->surface) {
		Display *dsp = cairo_xlib_surface_get_display(this->surface);

		cairo_destroy(this->cairo);
		cairo_surface_destroy(this->surface);
		XDestroyIC(this->ic);
		XCloseDisplay(dsp);

		printf("X11 closed\n");
	}
	if (this->uimediall != NULL) {
		uimedia_ll_st *m = this->uimediall;
		while (m != NULL) {
			printf("Freeing bitmap '%s'\n", m->filename);
			uimedia_ll_st *t = m;
			m = m->next_ptr;
			cairo_surface_destroy(t->surface);
			free(t);
		}
	}
}



#endif
