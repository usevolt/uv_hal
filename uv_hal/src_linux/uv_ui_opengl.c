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
#include <unistd.h>
#include "uv_ui.h"
#include "uv_can.h"
#include <GLFW/glfw3.h>

#if CONFIG_UI && CONFIG_UI_OPENGL


static void key_callback(GLFWwindow* window,
		int key, int scancode, int action, int mods);
static void cursor_position_callback(GLFWwindow* window,
		double xpos, double ypos);
static void mouse_button_callback(GLFWwindow* window,
		int button, int action, int mods);
static void resize_callback(GLFWwindow* window,
		int width, int height);


ui_font_st ui_fonts[UI_MAX_FONT_COUNT];
static const uint8_t font_sizes[UI_MAX_FONT_COUNT] = {
		13,
		16,
		19,
		21,
		25,
		30,
		40,
		58,
		70
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
	void *surface;
} uimedia_ll_st;



/// @brief: The main uv_ui structure that holds the state of the ui drawn
typedef struct {
	bool pressed;
	int32_t x;
	int32_t y;
	double scalex;
	double scaley;
	double scale;
	int32_t xoffset;
	int32_t yoffset;
	uimedia_ll_st *uimediall;
	uv_ring_buffer_st key_press;
	char key_press_buffer[20];
	uint8_t brightness;

	GLFWwindow* window;

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
		.window = NULL,
		.uimediall = NULL,
		.brightness = 50
};

#ifdef this
#undef this
#endif
#define this (&_ui)






static uv_uiobject_ret_e confwindow_step(void *, uint16_t);



void uv_ui_confwindow_exec(void) {
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
#if CONFIG_CAN
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
#endif

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


#if CONFIG_CAN
	if (uv_uilistbutton_clicked(&this->confwindow.can_listbutton)) {
		uv_can_set_dev(this->confwindow.can_listbutton_content[
		   uv_uilistbutton_get_current_index(&this->confwindow.can_listbutton)]);
		uv_can_set_baudrate(uv_can_get_dev(),
				uv_uidigitedit_get_value(&this->confwindow.baud_digiedit) * 1000);
	}
	if (uv_uidigitedit_value_changed(&this->confwindow.baud_digiedit)) {
		uv_can_set_baudrate(uv_can_get_dev(),
				uv_uidigitedit_get_value(&this->confwindow.baud_digiedit) * 1000);
	}
#endif
	if (uv_uibutton_clicked(&this->confwindow.ok_button)) {
		this->confwindow.terminate = true;
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

	// Poll for and process events
	glfwPollEvents();

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


typedef struct {
	double r;
	double g;
	double b;
	double a;
} glc_st;

static glc_st c_to_glc(color_t color) {
	glc_st ret;
	color_st c = uv_uic(color);
	ret.r = (double) c.r / 255;
	ret.g = (double) c.g / 255;
	ret.b = (double) c.b / 255;
	ret.a = (double) c.a / 255;
	return ret;
}


void uv_ui_clear(color_t col) {
	glc_st c = c_to_glc(col);
    glClearColor(c.r, c.g, c.b, c.a); //clear background screen to black
}



void uv_ui_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {
}




void uv_ui_draw_point(int16_t x, int16_t y, color_t col, uint16_t diameter) {

	color_st c = uv_uic(col);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_TRIANGLE_FAN);

    double r = diameter / 2;
    glVertex2i(x, y); // Center
    for(int i = 0; i <= 360; i += 2)
            glVertex2f(r * cosf(M_PI * i / 180.0) + (double) x,
            		r * sinf(M_PI * i / 180.0) + (double) y);
    glEnd();
}



void uv_ui_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t w, const uint16_t h,
		const uint16_t radius, const color_t col) {
}



void uv_ui_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
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
	if (this->window) {
		if (!glfwWindowShouldClose(this->window)) {

			// Swap front and back buffers
			glfwSwapBuffers(this->window);

		    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		else {
			glfwTerminate();
			this->window = NULL;
		}
	}
}

;



static void key_callback(GLFWwindow* window,
		int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		char c = (char) key;
		uv_ring_buffer_push(&this->key_press, &c);
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	this->x = (int) xpos;
	this->y = (int) ypos;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	this->pressed = (action == GLFW_PRESS);
}


static void resize_callback(GLFWwindow* window, int width, int height) {
	this->scalex = (double) width / CONFIG_FT81X_HSIZE;
	this->scaley  = (double) height / CONFIG_FT81X_VSIZE;
	this->scale = MIN(this->scalex, this->scaley);
	this->xoffset = (width - this->scale * CONFIG_FT81X_HSIZE) / 2;
	this->yoffset = (height - this->scale * CONFIG_FT81X_VSIZE) / 2;


    //Tell OpenGL how to convert from coordinates to pixel values
    glViewport( 0, 0, width, height );
}



bool uv_ui_init(void) {
	bool ret = true;
	this->pressed = false;
	this->x = 0;
	this->y = 0;
	this->scalex = 1.0;
	this->scaley = 1.0;
	this->scale = 1.0;
	this->xoffset = 0;
	this->yoffset = 0;

	// initialize the font sizes
	for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
		ui_fonts[i].char_height = font_sizes[i];
	}
	// initialize the key press buffer
	uv_ring_buffer_init(&this->key_press, this->key_press_buffer,
			sizeof(this->key_press_buffer) / sizeof(this->key_press_buffer[0]),
			sizeof(this->key_press_buffer[0]));

	printf("Initializing GLFW\n");
	fflush(stdout);
	if (!glfwInit()) {
		ret = false;
	}
	else {
	    //Makes 3D drawing work when something is in front of something else
	    glEnable(GL_DEPTH_TEST);

		printf("Creating GLFW window\n");

		// 4x multisampling for anti-aliasing
		glfwWindowHint(GLFW_SAMPLES, 8);
		/* Create a windowed mode window and its OpenGL context */
		this->window = glfwCreateWindow(CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE,
				"Hello World", NULL, NULL);
		if (!this->window) {
			printf("GLFW terminated\n");
			glfwTerminate();
			ret = false;
		}
		else {
			// Make the window's context current
			glfwMakeContextCurrent(this->window);
			// add key listener
			glfwSetKeyCallback(this->window, &key_callback);
			// add cursor position listener
			glfwSetCursorPosCallback(this->window, &cursor_position_callback);
			// add mouse button listener
			glfwSetMouseButtonCallback(this->window, &mouse_button_callback);
			glfwSetWindowSizeCallback(this->window, &resize_callback);
			printf("GLFW running\n");

			//This sets up the viewport so that the coordinates (0, 0) are at the top left of the window
			glViewport(0, 0, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE, 0, -10, 10);

			//Back to the modelview so we can draw stuff
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glEnable(GL_MULTISAMPLE);

			//Clear the screen and depth buffer
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
	}

	return ret;
}



void uv_ui_destroy(void) {
	if (this->uimediall != NULL) {
		uimedia_ll_st *m = this->uimediall;
		while (m != NULL) {
			printf("Freeing bitmap '%s'\n", m->filename);
			uimedia_ll_st *t = m;
			m = m->next_ptr;
			free(t);
		}
	}
	if (this->window) {
		printf("Terminating GLFW window\n");
		glfwTerminate();
	}
}



#endif
