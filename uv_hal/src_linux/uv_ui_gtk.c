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
#include <pthread.h>
#include <gtk/gtk.h>
#include <math.h>


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
	uv_ring_buffer_st draw_cmds;
	ui_draw_cmd_st draw_cmd_buffer[DRAW_CMD_BUFFER_LEN];
	uv_mutex_st mutex;
	bool pressed;
	int32_t press_x;
	int32_t press_y;
	GtkWidget *window;
} ui_st;

static ui_st _ui = {};
#ifdef this
#undef this
#endif
#define this (&_ui)


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


#define DRAW_CMD_PUSH(element) \
	do { \
		uv_mutex_lock(&this->mutex); \
		if (uv_ring_buffer_push(&this->draw_cmds, &element) != ERR_NONE) {\
			printf("\n**** ERROR ****\nGUI cmd ring buffer full\n\n"); \
		} \
		uv_mutex_unlock(&this->mutex); \
	} while(0);

void uv_ui_clear(color_t c) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_CLEAR;
	cmd.clear.c = c;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_DRAW_BITMAP_EXT;
	cmd.draw_bitmap_ext.bitmap = bitmap;
	cmd.draw_bitmap_ext.c = c;
	cmd.draw_bitmap_ext.h = h;
	cmd.draw_bitmap_ext.w = w;
	cmd.draw_bitmap_ext.wrap = wrap;
	cmd.draw_bitmap_ext.x = x;
	cmd.draw_bitmap_ext.y = y;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_draw_point(int16_t x, int16_t y, color_t color, uint16_t diameter) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_DRAW_POINT;
	cmd.draw_point.c = color;
	cmd.draw_point.diameter = diameter;
	cmd.draw_point.x = x;
	cmd.draw_point.y = y;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_DRAW_RRECT;
	cmd.draw_rrect.c = color;
	cmd.draw_rrect.h = height;
	cmd.draw_rrect.w = width;
	cmd.draw_rrect.x = x;
	cmd.draw_rrect.y = y;
	cmd.draw_rrect.radius = radius;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_DRAW_LINE;
	cmd.draw_line.c = color;
	cmd.draw_line.w = width;
	cmd.draw_line.x_start = start_x;
	cmd.draw_line.x_end = end_x;
	cmd.draw_line.y_start = start_y;
	cmd.draw_line.y_end = end_y;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_draw_linestrip(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_DRAW_LINESTRIP;
	for (uint16_t i = 0; i < point_count; i++) {
		cmd.draw_linestrip.points[i] = (uv_ui_linestrip_point_st) points[i];
	}
	cmd.draw_linestrip.point_count = point_count;
	cmd.draw_linestrip.line_w = line_width;
	cmd.draw_linestrip.c = color;
	cmd.draw_linestrip.type = type;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix) {

}



bool uv_ui_get_touch(int16_t *x, int16_t *y) {
	bool ret;
	uv_mutex_lock(&this->mutex);

	ret = this->pressed;
	*x = (int16_t) this->press_x;
	*y = (int16_t) this->press_y;

	uv_mutex_unlock(&this->mutex);
	return ret;
}



void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_DRAW_STRING;
	cmd.draw_string.align = align;
	cmd.draw_string.c = color;
	cmd.draw_string.font = font;
	strcpy(cmd.draw_string.str, str);
	cmd.draw_string.x = x;
	cmd.draw_string.y = y;
	DRAW_CMD_PUSH(cmd);
}



void uv_ui_set_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	ui_draw_cmd_st cmd;
	cmd.type = CMD_SET_MASK;
	cmd.set_mask.x = x;
	cmd.set_mask.y = y;
	cmd.set_mask.w = width;
	cmd.set_mask.h = height;
	DRAW_CMD_PUSH(cmd);
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
	if (this->window != NULL) {
		printf("queue draw\n");
		gtk_widget_queue_draw(this->window);
	}
}






bool uv_ui_init(void) {

	this->pressed = false;

	return false;
}




// declarations for private GTK thread drawing functions
static void clear(cairo_t *cr, color_t c);
static void draw_bitmap_ext(cairo_t *cr, uv_uimedia_st *bitmap,
		int16_t x, int16_t y, int16_t w, int16_t h,
		uint32_t wrap, color_t c);
static void draw_point(cairo_t *cr, int16_t x, int16_t y,
		color_t color, uint16_t diameter);
static void draw_rrect(cairo_t *cr, int16_t x, int16_t y,
		uint16_t width, uint16_t height,
		uint16_t radius, color_t color);
static void draw_line(cairo_t *cr, int16_t start_x, int16_t start_y,
		int16_t end_x, int16_t end_y,
		uint16_t width, color_t color);
static void draw_linestrip(cairo_t *cr, uv_ui_linestrip_point_st *points,
		uint16_t point_count, uint16_t line_width, color_t color,
		uv_ui_strip_type_e type);
static void draw_string(cairo_t *cr, char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color);
static void set_mask(cairo_t *cr, int16_t x, int16_t y, int16_t width, int16_t height);
static void window_closed(GtkApplication *application,
		GtkWindow *window, gpointer user_data);
static void activate (GtkApplication* app, gpointer user_data);
static void *start_scheduler(void *ptr);

static cairo_surface_t *surface = NULL;

#define CAIRO_CR(color)		(((double) ((color_st*) &color)->r) / 255.0)
#define CAIRO_CG(color)		(((double) ((color_st*) &color)->g) / 255.0)
#define CAIRO_CB(color)		(((double) ((color_st*) &color)->b) / 255.0)
#define CAIRO_CA(color)		(((double) ((color_st*) &color)->a) / 255.0)


static void clear(cairo_t *cr, color_t c) {
	printf("clear\n");
	cairo_set_source_rgba(cr,
			CAIRO_CR(c), CAIRO_CG(c), CAIRO_CB(c), CAIRO_CA(c));
	cairo_paint(cr);
}



static void draw_bitmap_ext(cairo_t *cr, uv_uimedia_st *bitmap,
		int16_t x, int16_t y, int16_t w, int16_t h,
		uint32_t wrap, color_t c) {

}



static void draw_point(cairo_t *cr, int16_t x, int16_t y,
		color_t c, uint16_t diameter) {
	printf("Drawing point %i %i %i\n", x, y, diameter);
	cairo_set_source_rgba(cr,
			CAIRO_CR(c), CAIRO_CG(c), CAIRO_CB(c), CAIRO_CA(c));
	cairo_arc(cr, (double) x, (double) y, diameter / 2, 0, 2 * M_PI);
	cairo_fill(cr);
}



static void draw_rrect(cairo_t *cr, int16_t x, int16_t y,
		uint16_t w, uint16_t h,
		uint16_t radius, color_t c) {
	printf("drawing rrect %i %i %i %i\n", x, y, w, h);

	/* a custom shape that could be wrapped in a function */
	double dx         = (double) x,        /* parameters like cairo_rectangle */
	       dy         = (double) y,
	       width         = (double) w,
	       height        = (double) h,
	       aspect        = 1.0,     /* aspect ratio */
	       corner_radius = height / 10.0;   /* and corner curvature radius */

	double r = corner_radius / aspect;
	double degrees = M_PI / 180.0;

	cairo_new_sub_path (cr);
	cairo_arc (cr, dx + width - r, dy + r, r, -90 * degrees, 0 * degrees);
	cairo_arc (cr, dx + width - r, dy + height - r, r, 0 * degrees, 90 * degrees);
	cairo_arc (cr, dx + r, dy + height - r, r, 90 * degrees, 180 * degrees);
	cairo_arc (cr, dx + r, dy + r, r, 180 * degrees, 270 * degrees);
	cairo_close_path (cr);

	cairo_set_source_rgba(cr,
			CAIRO_CR(c), CAIRO_CG(c), CAIRO_CB(c), CAIRO_CA(c));
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, radius);
	cairo_stroke (cr);
}



static void draw_line(cairo_t *cr, int16_t start_x, int16_t start_y,
		int16_t end_x, int16_t end_y,
		uint16_t width, color_t color) {

}



static void draw_linestrip(cairo_t *cr, uv_ui_linestrip_point_st *points,
		uint16_t point_count, uint16_t line_width, color_t color,
		uv_ui_strip_type_e type) {

}



static void draw_string(cairo_t *cr, char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {

}



static void set_mask(cairo_t *cr, int16_t x, int16_t y, int16_t width, int16_t height) {
	printf("setting mask to %i %i %i %i\n", x, y, width, height);
	cairo_reset_clip(cr);
	cairo_rectangle(cr, (double) x, (double) y, (double) width, (double) height);
	cairo_clip(cr);
}


static bool first = true;
static gboolean draw_cb(GtkWidget *widget, cairo_t *cr, gpointer data) {
	gboolean ret = FALSE;

	if (first) {
		// start the FreeRTOS scheduler in a different thread, because GTK
		// has to be run in this main thread
		// For some reason GTK calls this draw function twice at start up.
		// To make sure that the UI get's drawn correctly, this thread is created here
		// in the draw_callback.
		pthread_t scheduler;
		pthread_create(&scheduler, NULL,  &start_scheduler, NULL);
		first = false;
	}

	printf("draw callback\n");

	uv_mutex_lock(&this->mutex);

	ui_draw_cmd_st cmd;
	uint16_t i = 0;
	while (uv_ring_buffer_pop(&this->draw_cmds, &cmd) == ERR_NONE) {
		i++;
		switch(cmd.type) {
		case CMD_CLEAR:
			clear(cr, cmd.clear.c);
			break;
		case CMD_DRAW_BITMAP_EXT:
			draw_bitmap_ext(cr, cmd.draw_bitmap_ext.bitmap,
					cmd.draw_bitmap_ext.x, cmd.draw_bitmap_ext.y,
					cmd.draw_bitmap_ext.w, cmd.draw_bitmap_ext.h,
					cmd.draw_bitmap_ext.wrap, cmd.draw_bitmap_ext.c);
			break;
		case CMD_DRAW_POINT:
			draw_point(cr, cmd.draw_point.x, cmd.draw_point.y,
					cmd.draw_point.c, cmd.draw_point.diameter);
			break;
		case CMD_DRAW_RRECT:
			draw_rrect(cr, cmd.draw_rrect.x, cmd.draw_rrect.y,
					cmd.draw_rrect.w, cmd.draw_rrect.h,
					cmd.draw_rrect.radius, cmd.draw_rrect.c);
			break;
		case CMD_DRAW_LINE:
			draw_line(cr, cmd.draw_line.x_start, cmd.draw_line.y_start,
					cmd.draw_line.x_end, cmd.draw_line.y_end, cmd.draw_line.w,
					cmd.draw_line.c);
			break;
		case CMD_DRAW_LINESTRIP:
			draw_linestrip(cr, cmd.draw_linestrip.points, cmd.draw_linestrip.point_count,
					cmd.draw_linestrip.line_w, cmd.draw_linestrip.c, cmd.draw_linestrip.type);
			break;
		case CMD_DRAW_STRING:
			draw_string(cr, cmd.draw_string.str, cmd.draw_string.font,
					cmd.draw_string.x, cmd.draw_string.y,
					cmd.draw_string.align, cmd.draw_string.c);
			break;
		case CMD_SET_MASK:
			set_mask(cr, cmd.set_mask.x, cmd.set_mask.y, cmd.set_mask.w, cmd.set_mask.h);
			break;
		default:
			printf("Unknown draw type %u\n", cmd.type);
			break;
		}

	}
	printf("Drew %u objects\n", i);

	uv_mutex_unlock(&this->mutex);

	return ret;
}



/* Create a new surface of the appropriate size to store our scribbles */
static gboolean configure_event_cb(
		GtkWidget *widget, GdkEventConfigure *event, gpointer data) {
	gboolean ret = TRUE;

	if (surface) {
		cairo_surface_destroy(surface);
	}

	surface = gdk_window_create_similar_surface(gtk_widget_get_window (widget),
			CAIRO_CONTENT_COLOR,
			gtk_widget_get_allocated_width(widget),
			gtk_widget_get_allocated_height(widget));

	cairo_t *cr;
	cr = cairo_create(surface);

	cairo_reset_clip(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_paint(cr);

	cairo_destroy(cr);

	/* We've handled the configure event, no need for further processing. */
	return ret;
}



static gboolean button_press_callb(GtkWidget *widget,
		GdkEvent *event, gpointer data) {
	gboolean ret = TRUE;
	if (event->button.button == GDK_BUTTON_PRIMARY) {
		printf("pressed %i %i\n", (int) event->button.x, (int) event->button.y);
		uv_mutex_lock(&this->mutex);
		this->pressed = true;
		this->press_x = (int32_t) event->button.x;
		this->press_y = (int32_t) event->button.y;
		uv_mutex_unlock(&this->mutex);
	}
	/* We've handled the event, stop processing */
	return ret;
}



static gboolean button_release_callb(GtkWidget *widget,
		GdkEvent *event, gpointer data) {
	gboolean ret = TRUE;

	if (event->button.button == GDK_BUTTON_PRIMARY) {
		printf("released %i %i\n", (int) event->button.x, (int) event->button.y);
		uv_mutex_lock(&this->mutex);
		this->pressed = false;
		uv_mutex_unlock(&this->mutex);
	}

	/* We've handled the event, stop processing */
	return ret;
}



static gboolean motion_callb(GtkWidget *widget,
		GdkEventMotion *event, gpointer data) {

	if (event->state & GDK_BUTTON1_MASK) {
		printf("Drag %i %i\n", (int) event->x, (int) event->y);
		uv_mutex_lock(&this->mutex);
		this->press_x = (int32_t) event->x;
		this->press_y = (int32_t) event->y;
		uv_mutex_unlock(&this->mutex);
	}
	/* We've handled it, stop processing */
	return TRUE;
}



static void activate (GtkApplication* app, gpointer user_data) {
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *drawing_area;

	window = gtk_application_window_new(app);
	g_signal_connect (window, "destroy", G_CALLBACK(window_closed), NULL);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(window), frame);

	drawing_area = gtk_drawing_area_new();
	/* set a minimum size */
	gtk_widget_set_size_request(drawing_area,
			CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);

	gtk_container_add(GTK_CONTAINER(frame), drawing_area);

	/* Signals used to handle the backing surface */
	g_signal_connect(drawing_area, "draw",
					G_CALLBACK(draw_cb), NULL);
	g_signal_connect(drawing_area,"configure-event",
					G_CALLBACK(configure_event_cb), NULL);

	/* Event signals */
	g_signal_connect(drawing_area, "motion-notify-event",
					G_CALLBACK(motion_callb), NULL);
	g_signal_connect(drawing_area, "button-press-event",
					G_CALLBACK(button_press_callb), NULL);
	g_signal_connect(drawing_area, "button-release-event",
					G_CALLBACK(button_release_callb), NULL);

	/* Ask to receive events the drawing area doesn't normally
	* subscribe to. In particular, we need to ask for the
	* button press and motion notify events that want to handle.
	*/
	gtk_widget_set_events(drawing_area, gtk_widget_get_events(drawing_area)
									 | GDK_BUTTON_PRESS_MASK
									 | GDK_POINTER_MOTION_MASK
									 | GDK_BUTTON_RELEASE_MASK);


	// add a reference to the drawing area
	this->window = drawing_area;
	uv_ring_buffer_init(&this->draw_cmds, this->draw_cmd_buffer,
			DRAW_CMD_BUFFER_LEN, sizeof(this->draw_cmd_buffer[0]));
	uv_mutex_init(&this->mutex);
	uv_mutex_unlock(&this->mutex);


	gtk_widget_show_all(window);
}



static void window_closed(GtkApplication *application, GtkWindow *window, gpointer user_data) {
	if (surface) {
		cairo_surface_destroy(surface);
	}
	uv_deinit();
}



static void *start_scheduler(void *ptr) {
	uv_rtos_start_scheduler();

	return NULL;
}



/// @brief: Starts the scheduler with the GTK UI in the main thread.
void uv_ui_rtos_start_scheduler(void) {

	printf("GTK starting\n");

	GtkApplication *app;
	app = gtk_application_new("org.gtk.uv0d", G_APPLICATION_FLAGS_NONE);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	g_application_run(G_APPLICATION(app), 0, NULL);
	g_object_unref(app);

	printf("GTK finished. Terminating.\n");
	exit(0);
}



#endif
