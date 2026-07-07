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

#if CONFIG_UI && CONFIG_UI_OPENGL

#include "uv_utilities.h"
#include "uv_rtos.h"
#include "ui/uv_uifont.h"
#include "uv_memory.h"
#include "uv_ui.h"
#include "uv_can.h"
#include "lodepng.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H
// font glyph data compiled into the binary (used when no font file is found)
#include "ui/embedded_font.h"
#include "ui/embedded_mono_font.h"


#if !defined(CONFIG_UI_NAME)
#define CONFIG_UI_NAME		"Window"
#endif

#define DEFAULT_FONT	"LiberationSans-Regular.ttf"
#define MONO_FONT		"LiberationMono-Regular.ttf"


static void key_callback(GLFWwindow* window,
		int key, int scancode, int action, int mods);
static void char_callback(GLFWwindow* window, unsigned int codepoint);
static void cursor_position_callback(GLFWwindow* window,
		double xpos, double ypos);
static void mouse_button_callback(GLFWwindow* window,
		int button, int action, int mods);
static void scroll_callback(GLFWwindow* window,
		double xoffset, double yoffset);
static void resize_callback(GLFWwindow* window,
		int width, int height);
static void load_fonts(void);
static void free_fonts(void);
static GLint get_uniform(GLuint shader_program, const char *name);
static GLint get_attrib(GLuint program, const char *name);

ui_font_st ui_fonts[UI_MAX_FONT_COUNT] = {};
ui_font_st ui_mono_fonts[UI_MAX_FONT_COUNT] = {};
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
		.bg_c = C(0xFF373a44),
		.fg_c = C(0xFF945dd2),
		.window_c = C(0xFF040404),
		.display_c = C(0xFF151516),
		.font = &font16,
		.text_color = C(0xFFFFFFFF)
};


/// @brief: A linked list member for the uimedia files that are loaded with
/// *uv_uimedia_load* functions
typedef struct {
	char filename[128];
	unsigned int texture;
	GLuint vao;
	GLuint vbo;
	GLuint ebo;
	uint8_t *data;
	uint32_t data_len;
	uint32_t width;
	uint32_t height;
	// pointer to the next uimedia_ll_st
	void *next_ptr;
} uimedia_ll_st;

/// @brief: Creates new uimedia_ll_st module and adds it into the linked list
uimedia_ll_st *uimedia_ll_st_create(const char *filename, uint8_t* data,
		uint32_t width, uint32_t height, uint32_t data_len);

/// @brief: Finds the already existing uimedia_ll struct and returns it.
/// Returns NULL if not found.
uimedia_ll_st *uimedia_ll_st_find(const char *filename);

typedef struct {
    uint32_t TextureID;  // ID handle of the glyph texture
    uint16_t size_x;
    uint16_t size_y;
    uint16_t bearing_x;
    uint16_t bearing_y;
    uint16_t advance;
} ft_char_st;



/// @brief: The main uv_ui structure that holds the state of the ui drawn
typedef struct {
	bool refresh;
	bool resized;
	bool pressed;
	int32_t x;
	int32_t y;
	// accumulated mouse-wheel notches since the last uv_ui_get_scroll() (positive
	// = scrolled up / away from the user)
	int32_t scroll;
	double scalex;
	double scaley;
	double scale;
	int32_t width;
	int32_t height;
	int32_t xoffset;
	int32_t yoffset;
	uimedia_ll_st *uimediall;
	uv_ring_buffer_st key_press;
	char key_press_buffer[20];
	uint8_t brightness;

	GLFWwindow* window;
	unsigned int text_shader_program;
	unsigned int text_vbo;

	unsigned int bitmap_shader_program;

	// configuration window that is shown when setting the settings
	struct {
		bool terminate;
		uv_uidisplay_st display;
		uv_uiobject_st *display_buffer[10];

		uv_uilistbutton_st can_listbutton;
		char *can_listbutton_content[10];

		uv_uilistbutton_st baud_listbutton;

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



uimedia_ll_st *uimedia_ll_st_create(const char *filename, uint8_t* data,
		uint32_t width, uint32_t height, uint32_t data_len) {
	uimedia_ll_st *ret = this->uimediall;
	uimedia_ll_st *parent = NULL;
	while (ret != NULL) {
		parent = ret;
		ret = ret->next_ptr;
	}
	ret = malloc(sizeof(uimedia_ll_st));
	if (parent != NULL) {
		parent->next_ptr = ret;
	}
	else {
		this->uimediall = ret;
	}

	strcpy(ret->filename, filename);
	ret->data = data;
	ret->data_len = data_len;
	ret->width = width;
	ret->height = height;
	ret->next_ptr = NULL;

	// generate texture
	unsigned int texture;
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// set texture options
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// upload bitmap
	glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA,
			ret->width,
			ret->height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			ret->data);
	ret->texture = texture;

	// the image data can be freed since the texture has been created
	free(ret->data);

	glGenVertexArrays(1, &ret->vao);
	glGenBuffers(1, &ret->vbo);
	glGenBuffers(1, &ret->ebo);

	return ret;
}

uimedia_ll_st *uimedia_ll_st_find(const char *filename) {
	uimedia_ll_st *ret;
	if (filename == NULL ||
			strlen(filename) == 0) {
		ret = NULL;
	}
	else {
		ret = this->uimediall;
	}

	while (ret != NULL) {
		if (strcmp(ret->filename, filename) == 0) {
			break;
		}
		else {
			ret = ret->next_ptr;
		}
	}
	return ret;
}



// selectable CAN baudrates shown in the baud listbutton, and their values in Hz
static char *baud_listbutton_content[] = {
		"125k", "250k", "500k", "1M"
};
static const unsigned int baud_listbutton_values[] = {
		125000, 250000, 500000, 1000000
};
#define BAUD_LISTBUTTON_COUNT \
	(sizeof(baud_listbutton_values) / sizeof(baud_listbutton_values[0]))


void uv_ui_confwindow_exec(const uv_uistyle_st *style) {
	// fall back to this backend's built-in style when the caller gives none
	if (style == NULL) {
		style = &uistyle;
	}

	uv_ui_init();

	this->confwindow.terminate = false;
	uv_uidisplay_init(&this->confwindow.display, this->confwindow.display_buffer, style);
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
	// load the list of CAN devices
#if CONFIG_CAN
	uint32_t index = 0;
	uint32_t count = 0;
	uv_can_set_baudrate(uv_can_get_dev(), 250000);
	this->confwindow.can_listbutton_content[0] = "NONE";
	// list the CAN devices ordered: physical "can*" first, then virtual "vcan*",
	// then any others, so the most relevant interfaces appear at the top
	for (uint8_t pass = 0; pass < 3; pass++) {
		for (uint32_t i = 0; i < uv_can_get_device_count(); i++) {
			char *name = uv_can_get_device_name(i);
			bool is_can = (strncmp(name, "can", 3) == 0);
			bool is_vcan = (strncmp(name, "vcan", 4) == 0);
			bool match = (pass == 0) ? is_can :
						 (pass == 1) ? is_vcan :
						 (!is_can && !is_vcan);
			if (match) {
				this->confwindow.can_listbutton_content[count] = name;
				if (strcmp(name, uv_can_get_dev()) == 0) {
					index = count;
				}
				count++;
			}
		}
	}
	uv_uilistbutton_init(&this->confwindow.can_listbutton, this->confwindow.can_listbutton_content,
			MAX(uv_can_get_device_count(), 1),
			index, style);
	uv_uilistbutton_set_content_type_arrayofpointers(&this->confwindow.can_listbutton);
	uv_uilistbutton_set_title(&this->confwindow.can_listbutton, "CAN dev:");
	uv_uidialog_add(&this->confwindow.display, &this->confwindow.can_listbutton, &bb);

	bb = uv_uistrlayout_find(&layout, "baud");
	// select the listbutton entry matching the current baudrate, defaulting to the first
	uint8_t baud_index = 0;
	unsigned int cur_baud = uv_can_get_baudrate(uv_can_get_dev());
	for (uint8_t i = 0; i < BAUD_LISTBUTTON_COUNT; i++) {
		if (baud_listbutton_values[i] == cur_baud) {
			baud_index = i;
		}
	}
	uv_uilistbutton_init(&this->confwindow.baud_listbutton, baud_listbutton_content,
			BAUD_LISTBUTTON_COUNT, baud_index, style);
	uv_uilistbutton_set_content_type_arrayofpointers(&this->confwindow.baud_listbutton);
	uv_uilistbutton_set_title(&this->confwindow.baud_listbutton, "CAN baudrate:");
	uv_uidisplay_add(&this->confwindow.display, &this->confwindow.baud_listbutton, &bb);
#endif

	bb = uv_uistrlayout_find(&layout, "close");
	uv_uibutton_init(&this->confwindow.ok_button, "OK", style);
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
				baud_listbutton_values[
				uv_uilistbutton_get_current_index(&this->confwindow.baud_listbutton)]);
	}
	if (uv_uilistbutton_clicked(&this->confwindow.baud_listbutton)) {
		uv_can_set_baudrate(uv_can_get_dev(),
				baud_listbutton_values[
				uv_uilistbutton_get_current_index(&this->confwindow.baud_listbutton)]);
	}
#endif
	if (uv_uibutton_clicked(&this->confwindow.ok_button)) {
#if CONFIG_CAN
		// Apply the currently shown CAN dev and baudrate even if the listbuttons
		// were never pressed, so closing the window uses the shown value (the
		// first list entry by default) instead of the initial built-in default.
		uv_can_set_dev(this->confwindow.can_listbutton_content[
		   uv_uilistbutton_get_current_index(&this->confwindow.can_listbutton)]);
		uv_can_set_baudrate(uv_can_get_dev(),
				baud_listbutton_values[
				uv_uilistbutton_get_current_index(&this->confwindow.baud_listbutton)]);
#endif
		this->confwindow.terminate = true;
	}

	return ret;
}



bool uv_ui_get_refresh_request(void) {
	if (glfwWindowShouldClose(this->window)) {
		this->refresh = true;
	}

	return this->refresh;
}


void uv_ui_set_backlight(uint8_t percent) {
	this->brightness = percent;
}



uint8_t uv_ui_get_backlight(void) {
	return this->brightness;
}




bool uv_ui_get_touch_impl(int16_t *x, int16_t *y) {
	bool ret;

	// Poll for and process events
	glfwPollEvents();

	ret = this->pressed;
	*x = this->x;
	*y = this->y;


	return ret;
}


int16_t uv_ui_get_scroll(void) {
	// return and clear the accumulated wheel notches (filled by scroll_callback)
	int16_t ret = (int16_t) this->scroll;
	this->scroll = 0;
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


void uv_ui_clear_impl(color_t col) {
	glc_st c = c_to_glc(col);
    // disable any active scissor box so the whole framebuffer is cleared, not
    // just the last mask rectangle left over from the previous frame
    glDisable(GL_SCISSOR_TEST);
    glClearColor(c.r, c.g, c.b, c.a);
    // Clear the colour buffer immediately, before any widgets are drawn, so the
    // background colour is correct from the very first frame. (The clear used to
    // be deferred to uv_ui_dlswap_impl() after the buffer swap, so the first frame was
    // drawn over the default black init clear and looked dark until a later
    // refresh.) Only the colour buffer is cleared here: the depth buffer is left
    // to uv_ui_dlswap_impl() as before, since the 2D widgets rely on that existing
    // depth-clear timing for correct draw ordering (clearing depth here breaks
    // text rendering).
    glClear(GL_COLOR_BUFFER_BIT);
}



void uv_ui_draw_bitmap_ext_impl(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {
	if (bitmap->visible) {
	uimedia_ll_st *media = uimedia_ll_st_find(bitmap->filename);

	if (media == NULL) {
		printf("ERROR: load bitmap with uv_ui_media_loadbitmapexmem before drawing it.\n");
	}
	else {
		glUseProgram(this->bitmap_shader_program);

		// blend colour: the texture's RGBA is multiplied by this ARGB value,
		// matching the FT81X BITMAP blend behaviour (white = unchanged).
		color_st blendcol = uv_uic(c);
		GLfloat blend[4] = { blendcol.r / 255.0f, blendcol.g / 255.0f,
				blendcol.b / 255.0f, blendcol.a / 255.0f };
		glUniform4fv(get_uniform(this->bitmap_shader_program, "blendColor"),
				1, blend);

		glBindTexture(GL_TEXTURE_2D, media->texture);

		float sx = 2.0f / (this->width / this->scalex);
		float sy = 2.0f / (this->height / this->scaley);
		float x2 = x * sx - 1.0f;
		float y2 = -y * sy + 1.0f - bitmap->height * sy;
		float w2 = w * sx;
		float h2 = h * sy;
		// configure VAO/VBO
		float vertices[] = {
			// positions                // colors           // texture coords
			x2 + w2,  y2 + h2, 0.0f,    1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // bottom right
			x2 + w2, y2, 0.0f,          0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // top right
			x2, y2, 0.0f,               0.0f, 0.0f, 1.0f,   0.0f, 1.0f, // top left
			x2,  y2 + h2, 0.0f,         1.0f, 1.0f, 0.0f,   0.0f, 0.0f  // bottom left
		};
		unsigned int indices[] = {
			0, 1, 3, // first triangle
			1, 2, 3  // second triangle
		};

		glBindVertexArray(media->vao);

		glBindBuffer(GL_ARRAY_BUFFER, media->vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, media->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		// texture coord attribute
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(media->vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


		glUseProgram(0);
	}
	}
}




void uv_ui_draw_point_impl(int16_t x, int16_t y, color_t col, uint16_t diameter) {

	color_st c = uv_uic(col);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_TRIANGLE_FAN);

    double r = diameter / 2;
    glVertex2i(x, y); // Center
	int resolution = MAX(this->height / 50 * diameter / 100, 30);
    for(int i = 0; i <= 360; i += MAX(360 / resolution, 1))
            glVertex2f(r * cosf(M_PI * i / 180.0) + (double) x,
            		r * sinf(M_PI * i / 180.0) + (double) y);
    glEnd();
}






void uv_ui_draw_rrect_impl(const int16_t x, const int16_t y,
		const uint16_t w, const uint16_t h,
		const uint16_t radius, const color_t col) {
	color_st c = uv_uic(col);
	glColor4ub(c.r, c.g, c.b, c.a);

	if (radius == 0) {
		glBegin(GL_TRIANGLE_FAN);
		glVertex2i(x, y);
		glVertex2i(x + w, y);
		glVertex2i(x + w, y + h);
		glVertex2i(x, y + h);
		glEnd();
	}
	else {
		glBegin(GL_TRIANGLE_FAN);
		glVertex2i(x + w, y + radius);
		int resolution = MAX(this->height / 50, 3);
		for (int i = 0; i < 90; i += MAX(90 / resolution, 1)) {
			glVertex2f((float) x + w - radius + radius * cosf(M_PI * i / 180),
					(float) y + radius - radius * sinf(M_PI * i / 180));
		}
		for (int i = 0; i < 90; i += MAX(90 / resolution, 1)) {
			glVertex2f((float) x + radius + radius * cosf(M_PI / 2 + M_PI * i / 180),
					(float) y + radius - radius * sinf(M_PI / 2 + M_PI * i / 180));
		}
		for (int i = 0; i < 90; i += MAX(90 / resolution, 1)) {
			glVertex2f((float) x + radius + radius * cosf(M_PI + M_PI * i / 180),
					(float) y + h - radius - radius * sinf(M_PI + M_PI * i / 180));
		}
		for (int i = 0; i < 90; i += MAX(90 / resolution, 1)) {
			glVertex2f((float) x + w - radius + radius * cosf(M_PI / 2 * 3 + M_PI * i / 180),
					(float) y + h - radius - radius * sinf(M_PI / 2  *3 + M_PI * i / 180));
		}
		glEnd();
	}
}



void uv_ui_draw_line_impl(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
	color_st c = uv_uic(color);
	glColor4ub(c.r, c.g, c.b, c.a);
	glLineWidth(width);
	glBegin(GL_LINES);
	glVertex2i(start_x, start_y);
	glVertex2i(end_x, end_y);
	glEnd();
}



void uv_ui_draw_linestrip_impl(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
	if (point_count) {
		color_st c = uv_uic(color);
		glColor4ub(c.r, c.g, c.b, c.a);
		glLineWidth(line_width);

		if (type == UI_STRIP_TYPE_LINE) {
			glBegin(GL_LINE_STRIP);
			for (uint16_t i = 0; i < point_count; i++) {
				glVertex2i(points[i].x, points[i].y);
			}
			glEnd();
		}
		else {
			// Edge strips fill the area between the polyline and one screen
			// edge. Emulate the FT81X EDGE_STRIP primitives with a quad strip
			// pairing every point with its projection onto that edge. Unlike a
			// single GL_POLYGON (which only fills convex shapes correctly), a
			// quad strip fills correctly even when the polyline is concave -
			// e.g. a pie sector whose apex dips back to the centre.
			glBegin(GL_QUAD_STRIP);
			for (uint16_t i = 0; i < point_count; i++) {
				glVertex2i(points[i].x, points[i].y);
				if (type == UI_STRIP_TYPE_ABOVE) {
					glVertex2i(points[i].x, 0);
				}
				else if (type == UI_STRIP_TYPE_BELOW) {
					glVertex2i(points[i].x, CONFIG_FT81X_VSIZE);
				}
				else if (type == UI_STRIP_TYPE_LEFT) {
					glVertex2i(0, points[i].y);
				}
				else if (type == UI_STRIP_TYPE_RIGHT) {
					glVertex2i(CONFIG_FT81X_HSIZE, points[i].y);
				}
				else {
					glVertex2i(points[i].x, points[i].y);
				}
			}
			glEnd();
		}
	}
}



void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix) {

}



// Appends the triangle vertices for one line of *str* to *verts* (4 floats per
// vertex: clip-space x, y and atlas texture s, t), starting from pen position
// (x, y) in application space. Advances *vcount* by the number of vertices added
// (6 per drawn glyph). All glyphs come from the font's single atlas texture, so
// the whole batch is later drawn in one glDrawArrays call (see uv_ui_draw_string_impl).
static void append_line_verts(char *str, ui_font_st *font,
		int16_t x, int16_t y, GLfloat *verts, size_t *vcount) {
	// coordinates are in application space, and since uv_ui uses fixed window size,
	// we resize these accordingly
	float fx = x * this->scalex;
	float fy = y * this->scaley;

	float sx = 2.0f / this->width;
	float sy = 2.0f / this->height;
	float aw = (font->atlas_w > 0) ? (float) font->atlas_w : 1.0f;
	float ah = (font->atlas_h > 0) ? (float) font->atlas_h : 1.0f;

	/* Translate the UTF-8 line into single-byte font glyph slots so the Nordic
	 * letters map to their loaded glyphs (see uv_ui_str_to_glyphs). */
	// the simulator always loads the Nordic glyphs from FreeType, so they are
	// available (nordic_glyphs = true)
	char glyphs[strlen(str) + 1];
	uv_ui_str_to_glyphs(str, glyphs, sizeof(glyphs), true);

	for (char *p = glyphs; *p; p++) {
		unsigned char g = (unsigned char) *p;

		/* Calculate the vertex and texture coordinates */
		float x2 = (fx + font->ft_char[g].bearing_x) * sx - 1.0f;
		float y2 = (fy - font->ft_char[g].bearing_y) * sy - 1.0f +
				(font->ft_char['H'].size_y * sy);
		float w = (font->ft_char[g].size_x) * sx;
		float h = (font->ft_char[g].size_y) * sy;

		// texture coordinates of this glyph's rectangle within the atlas
		float u0 = font->ft_char[g].atlas_x / aw;
		float v0 = font->ft_char[g].atlas_y / ah;
		float u1 = (font->ft_char[g].atlas_x + font->ft_char[g].size_x) / aw;
		float v1 = (font->ft_char[g].atlas_y + font->ft_char[g].size_y) / ah;

		// skip zero-area glyphs (e.g. space) so they add no geometry
		if ((font->ft_char[g].size_x > 0) && (font->ft_char[g].size_y > 0)) {
			// two triangles (TL,TR,BL) and (TR,BR,BL)
			const GLfloat quad[6][4] = {
					{ x2,     -y2,     u0, v0 },
					{ x2 + w, -y2,     u1, v0 },
					{ x2,     -y2 - h, u0, v1 },
					{ x2 + w, -y2,     u1, v0 },
					{ x2 + w, -y2 - h, u1, v1 },
					{ x2,     -y2 - h, u0, v1 },
			};
			GLfloat *dst = &verts[*vcount * 4];
			memcpy(dst, quad, sizeof(quad));
			*vcount += 6;
		}

		/* Advance the cursor to the start of the next character */
		fx += (font->ft_char[g].advance / 64);
	}
}

void uv_ui_draw_string_impl(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
	if ((str == NULL) || (font == NULL)) {
		return;
	}
	size_t len = strlen(str);
	if (len == 0) {
		return;
	}

	// The text shader program is created once at start-up and never rebuilt, so
	// its attribute/uniform locations are stable; look them up only on the first
	// call rather than for every string (the -2 sentinel means "not yet queried").
	static GLint uniform_tex = -2;
	static GLint uniform_color = -1;
	static GLint attribute_coord = -1;
	if (uniform_tex == -2) {
		uniform_tex = get_uniform(this->text_shader_program, "tex");
		uniform_color = get_uniform(this->text_shader_program, "color");
		attribute_coord = get_attrib(this->text_shader_program, "coord");
		if (uniform_tex == -1 || uniform_color == -1 || attribute_coord == -1) {
			printf("ERROR loading OPENGL text shader program\n");
		}
	}

	// working copy so each line can be null-terminated in place
	char *s = malloc(len + 2);
	if (s == NULL) {
		return;
	}
	memcpy(s, str, len + 1);

	// count the lines once (the original re-ran strlen() every loop iteration,
	// which is O(n^2) over the string length)
	uint16_t line_count = 1;
	for (size_t i = 0; i < len; i++) {
		if (str[i] == '\n') {
			line_count++;
		}
	}
	if (align & VALIGN_CENTER) {
		// reduce the y by the number of line counts
		y -= (line_count * font->char_height / 2);
	}

	// Accumulate every glyph of the whole (possibly multi-line) string into a
	// single vertex batch, then submit it with one texture bind and one draw call.
	// Upper bound: 6 vertices per byte, 4 floats each (glyph count <= byte count).
	GLfloat *verts = malloc((size_t) len * 6 * 4 * sizeof(GLfloat));
	if (verts == NULL) {
		free(s);
		return;
	}
	size_t vcount = 0;

	char *last_s = s;
	for (size_t i = 0; i <= len; i++) {
		if (s[i] == '\r' || s[i] == '\n' || s[i] == '\0') {
			char sep = s[i];
			s[i] = '\0';

			// lay out one line at a time (kept identical to the original: empty
			// lines are skipped and do not advance the baseline)
			if (last_s[0] != '\0') {
				int16_t lx = x;
				int32_t str_width = uv_ui_get_string_width(last_s, font);
				if (align & HALIGN_CENTER) {
					lx -= str_width / 2;
				}
				else if (align & HALIGN_RIGHT) {
					lx -= str_width;
				}
				else {
					// left-aligned: the pen starts at x
				}
				append_line_verts(last_s, font, lx, y, verts, &vcount);
				y += font->char_height;
			}
			last_s = &s[i + 1];
			if (sep == '\0') {
				break;
			}
		}
	}

	if (vcount > 0) {
		glUseProgram(this->text_shader_program);

		color_st col = uv_uic(color);
		GLfloat c[4] = { col.r / 255.0f, col.g / 255.0f,
				col.b / 255.0f, col.a / 255.0f };
		glUniform4fv(uniform_color, 1, c);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, font->atlas_tex);
		glUniform1i(uniform_tex, 0);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glEnableVertexAttribArray(attribute_coord);
		glBindBuffer(GL_ARRAY_BUFFER, this->text_vbo);
		glBufferData(GL_ARRAY_BUFFER, vcount * 4 * sizeof(GLfloat), verts,
				GL_DYNAMIC_DRAW);
		glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei) vcount);

		glDisableVertexAttribArray(attribute_coord);
		glUseProgram(0);
	}

	free(verts);
	free(s);
}


void uv_ui_draw_polygon_impl(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const color_t color) {
	if (point_count >= 3) {
		color_st c = uv_uic(color);
		glColor4ub(c.r, c.g, c.b, c.a);
		// a triangle fan fills a convex polygon (apex = first point) exactly,
		// with true straight edges between the perimeter points
		glBegin(GL_TRIANGLE_FAN);
		for (uint16_t i = 0; i < point_count; i++) {
			glVertex2i(points[i].x, points[i].y);
		}
		glEnd();
	}
}



void uv_ui_set_mask_impl(int16_t x, int16_t y, int16_t width, int16_t height) {
	// Rectangular clip via the GL scissor box. This mirrors the FT81X SCISSOR
	// display-list commands. The earlier stencil-based implementation was
	// commented out because it was slow; glScissor is essentially free.
	//
	// Our draw coordinates are FT81X pixels with a top-left origin, scaled to
	// the window by scalex/scaley. glScissor expects window framebuffer pixels
	// with a bottom-left origin, so scale and flip Y here.
	int32_t sx = (int32_t) (x * this->scalex);
	int32_t sw = (int32_t) (width * this->scalex);
	int32_t sh = (int32_t) (height * this->scaley);
	int32_t sy = (int32_t) (this->height - (int32_t)(y + height) * this->scaley);
	glEnable(GL_SCISSOR_TEST);
	glScissor(sx, sy, sw, sh);
}



void uv_ui_force_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	// stencil test is used for masking objects
	glEnable(GL_STENCIL_TEST);
	glClearStencil(0);
	glStencilMask(0xFF);
	// clear the old stencil to zero
	glClear(GL_STENCIL_BUFFER_BIT);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	glBegin(GL_POLYGON);
	glVertex2i(x, y);
	glVertex2i(x + width, y);
	glVertex2i(x + width, y + height);
	glVertex2i(x, y + height);
	glEnd();

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilFunc(GL_EQUAL, 1, 0xFF);

}




int16_t uv_ui_get_string_width(char *str, ui_font_st *font) {
	int16_t ret = 0;
	int line_width = 0;
	if (str) {
		const char *p = str;
		while (*p != '\0') {
			uint32_t cp = uv_ui_utf8_next(&p);
			if (cp == '\n' ||
					cp == '\r') {
				line_width = 0;
			}
			else {
				uint8_t glyph = uv_ui_codepoint_glyph(cp, true);
				line_width += font->ft_char[glyph].advance / 64;
				if (line_width > ret) {
					ret = line_width;
				}
			}
		}
	}

	return (ret / this->scalex);
}


uint32_t uv_uimedia_newbitmapexmem(uv_uimedia_st *bitmap,
		uv_w25q128_st *exmem, const char *filename) {
	uint32_t ret = 0;
	uimedia_ll_st *media = uimedia_ll_st_find(filename);
	memset(bitmap, 0, sizeof(*bitmap));
	bitmap->filename = "";
	bitmap->visible = true;
	bitmap->type = UV_UIMEDIA_IMAGE;

	if (media == NULL &&
			filename != NULL &&
			strlen(filename) != 0) {
		// load the pixel data
		unsigned error;
		unsigned char* image = NULL;
		unsigned width, height;
		printf("Loading bitmap '%s'\n", filename);
		error = lodepng_decode32_file(&image, &width, &height, filename);
		if (error) {
			printf("Loading bitmap '%s' failed\n", filename);
		}
		else {
			media = uimedia_ll_st_create(filename, image,
					width, height, width * height * 4);
		}
	}
	if (media != NULL) {
		bitmap->filename = media->filename;
		bitmap->width = media->width;
		bitmap->height = media->height;
		bitmap->size = media->data_len;
		ret = media->data_len;
	}

	return ret;
}


uint32_t uv_uimedia_newbitmapexmem_mem(uv_uimedia_st *bitmap,
		const char *name, const uint8_t *data, uint32_t datalen) {
	uint32_t ret = 0;
	// the decoded bitmap is cached (and later found at draw time) by *name*,
	// exactly like the file variant caches by filename
	uimedia_ll_st *media = uimedia_ll_st_find(name);
	memset(bitmap, 0, sizeof(*bitmap));
	bitmap->filename = "";
	bitmap->visible = true;
	bitmap->type = UV_UIMEDIA_IMAGE;

	if (media == NULL &&
			name != NULL && strlen(name) != 0 &&
			data != NULL && datalen != 0) {
		// decode the PNG straight from the memory buffer
		unsigned error;
		unsigned char* image = NULL;
		unsigned width, height;
		printf("Loading embedded bitmap '%s'\n", name);
		error = lodepng_decode32(&image, &width, &height, data, datalen);
		if (error) {
			printf("Loading embedded bitmap '%s' failed\n", name);
		}
		else {
			media = uimedia_ll_st_create(name, image,
					width, height, width * height * 4);
		}
	}
	if (media != NULL) {
		bitmap->filename = media->filename;
		bitmap->width = media->width;
		bitmap->height = media->height;
		bitmap->size = media->data_len;
		ret = media->data_len;
	}

	return ret;
}


void uv_uimedia_free(uv_uimedia_st *bitmap) {
	memset(bitmap, 0, sizeof(*bitmap));
}


void uv_ui_touchscreen_set_transform_matrix(ui_transfmat_st *transform_matrix) {

}



void uv_ui_dlswap_impl(void) {
	if (this->window) {
		if (!glfwWindowShouldClose(this->window)) {


		    double time1 = glfwGetTime();
			// Swap front and back buffers
			glfwSwapBuffers(this->window);
		    double time2 = glfwGetTime();
		    double fps = (time2 - time1);
		    if (fps != 0) {
		    	fps = 1 / fps;
		    }
//		    printf("fps %f\n", fps);

			glClearStencil(1);
		    // clear the full back buffer, not just the last mask rectangle
		    glDisable(GL_SCISSOR_TEST);
		    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		    this->refresh = false;

		    if (this->resized) {
		    	load_fonts();
		    	this->refresh = true;
		    	this->resized = false;
		    }
		}
		else {
			// The user closed the window (X button / Alt-F4). Terminate the whole
			// application. This is routed through the same path as Ctrl-C — raise
			// SIGINT so the installed handler runs the application's cleanup and
			// _exit()s — rather than exit(): calling exit() from this render thread
			// flushes the captured stdout/stderr back through the log-capture pipe
			// and runs atexit handlers, which can deadlock and leave the process
			// running (the window "won't close").
			//
			// Block SIGINT on THIS (render) thread first so the kernel delivers it
			// to a different thread. Otherwise the handler could run right here and,
			// if its cleanup blocks (e.g. writing to a full log-capture pipe, or a
			// malloc/stdio lock held by the interrupted context), it would never
			// return and the _exit() fallback below would never be reached — the
			// process would hang with the window closed. With SIGINT masked here we
			// always fall through to _exit(), guaranteeing the process dies even if
			// the handler stalls on another thread.
			//
			// POSIX-only: Windows has no pthread_sigmask/kill/SIGINT-based cleanup
			// path, so there we skip straight to the _exit(0) fallback below.
#ifndef _WIN32
			sigset_t sigint_set;
			sigemptyset(&sigint_set);
			sigaddset(&sigint_set, SIGINT);
			pthread_sigmask(SIG_BLOCK, &sigint_set, NULL);
			kill(getpid(), SIGINT);
			for (int i = 0; i < 200; i++) {
				usleep(5000);	// up to ~1s for the SIGINT handler to _exit()
			}
#endif
			_exit(0);
		}
	}
}



static void key_callback(GLFWwindow* window,
		int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		char c;
		switch (key) {
		case GLFW_KEY_BACKSPACE:
			c = '\b';
			break;
		case GLFW_KEY_ENTER:
		case GLFW_KEY_KP_ENTER:
			c = '\n';
			break;
		case GLFW_KEY_ESCAPE:
			c = 0x1b;
			break;
		case GLFW_KEY_DELETE:
			c = 0x7f;
			break;
		case GLFW_KEY_TAB:
			c = '\t';
			break;
		default:
			// printable characters arrive via char_callback so that
			// shift/caps/layout are applied correctly by GLFW.
			return;
		}
		taskENTER_CRITICAL();
		uv_ring_buffer_push(&this->key_press, &c);
		taskEXIT_CRITICAL();
	}
}

static void char_callback(GLFWwindow* window, unsigned int codepoint) {
	if (codepoint < 0x80) {
		char c = (char) codepoint;
		taskENTER_CRITICAL();
		uv_ring_buffer_push(&this->key_press, &c);
		taskEXIT_CRITICAL();
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	this->x = (int) xpos / this->scalex;
	this->y = (int) ypos / this->scaley;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	this->pressed = (action == GLFW_PRESS);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	// accumulate vertical wheel notches; drained by uv_ui_get_scroll()
	this->scroll += (int32_t) yoffset;
}


static void resize_callback(GLFWwindow* window, int width, int height) {
    taskENTER_CRITICAL();
	this->scalex = (double) width / CONFIG_FT81X_HSIZE;
	this->scaley  = (double) height / CONFIG_FT81X_VSIZE;
	this->scale = MIN(this->scalex, this->scaley);
	this->xoffset = (width - this->scale * CONFIG_FT81X_HSIZE) / 2;
	this->yoffset = (height - this->scale * CONFIG_FT81X_VSIZE) / 2;
	this->width = width;
	this->height = height;

    //Tell OpenGL how to convert from coordinates to pixel values
    glViewport( 0, 0, width, height );

    this->refresh = true;
    this->resized = true;
    taskEXIT_CRITICAL();

}



// The glyph slots to bake into each font: the 128 ASCII slots (slot == codepoint)
// followed by the non-ASCII glyphs remapped into their low UV_UI_GLYPH_* slots so
// UTF-8 source strings render once uv_ui_str_to_glyphs() has translated them.
#define FONT_ATLAS_PAD		2
#define FONT_ATLAS_WIDTH	1024

static const struct {
	uint8_t slot;
	uint32_t codepoint;
} font_extra_glyphs[] = {
		{ UV_UI_GLYPH_a_UML, 0x00E4 },
		{ UV_UI_GLYPH_o_UML, 0x00F6 },
		{ UV_UI_GLYPH_a_RING, 0x00E5 },
		{ UV_UI_GLYPH_A_UML, 0x00C4 },
		{ UV_UI_GLYPH_O_UML, 0x00D6 },
		{ UV_UI_GLYPH_A_RING, 0x00C5 },
};
#define FONT_EXTRA_COUNT	(sizeof(font_extra_glyphs) / sizeof(font_extra_glyphs[0]))
#define FONT_GLYPH_COUNT	(128 + FONT_EXTRA_COUNT)


// Bakes every glyph of the currently-sized *face* into a single atlas texture for
// *font*: the glyphs are shelf-packed left-to-right, wrapping rows, and the whole
// atlas is uploaded with one glTexImage2D. ft_char[] then records each glyph's
// pixel rectangle within the atlas (plus its metrics), so uv_ui_draw_string_impl() can
// render an entire string with one texture bind and one draw call rather than one
// per glyph. Every glyph is rendered twice (once to measure/pack, once to blit);
// this only runs at start-up and on window resize.
static void build_font_atlas(ui_font_st *font, FT_Face face) {
	// the codepoint to load for slot i, and which ft_char slot it lands in
	uint32_t cp[FONT_GLYPH_COUNT];
	uint8_t slot[FONT_GLYPH_COUNT];
	for (uint32_t c = 0; c < 128; c++) {
		cp[c] = c;
		slot[c] = c;
	}
	for (uint32_t j = 0; j < FONT_EXTRA_COUNT; j++) {
		cp[128 + j] = font_extra_glyphs[j].codepoint;
		slot[128 + j] = font_extra_glyphs[j].slot;
	}

	// pass 1: load each glyph, record metrics, and shelf-pack to assign positions
	int cur_x = FONT_ATLAS_PAD, cur_y = FONT_ATLAS_PAD, row_h = 0;
	for (uint32_t i = 0; i < FONT_GLYPH_COUNT; i++) {
		if (FT_Load_Char(face, cp[i], FT_LOAD_RENDER)) {
			memset(&font->ft_char[slot[i]], 0, sizeof(font->ft_char[slot[i]]));
			continue;
		}
		FT_GlyphSlot g = face->glyph;
		int w = g->bitmap.width, h = g->bitmap.rows;
		if (cur_x + w + FONT_ATLAS_PAD > FONT_ATLAS_WIDTH) {
			// wrap to the next shelf
			cur_x = FONT_ATLAS_PAD;
			cur_y += row_h + FONT_ATLAS_PAD;
			row_h = 0;
		}
		font->ft_char[slot[i]].atlas_x = cur_x;
		font->ft_char[slot[i]].atlas_y = cur_y;
		font->ft_char[slot[i]].size_x = w;
		font->ft_char[slot[i]].size_y = h;
		font->ft_char[slot[i]].bearing_x = g->bitmap_left;
		font->ft_char[slot[i]].bearing_y = g->bitmap_top;
		font->ft_char[slot[i]].advance = g->advance.x;
		cur_x += w + FONT_ATLAS_PAD;
		if (h > row_h) {
			row_h = h;
		}
	}
	int atlas_h = cur_y + row_h + FONT_ATLAS_PAD;

	// pass 2: render each glyph again and blit it into the CPU atlas buffer
	uint8_t *buf = calloc((size_t) FONT_ATLAS_WIDTH * atlas_h, 1);
	if (buf != NULL) {
		for (uint32_t i = 0; i < FONT_GLYPH_COUNT; i++) {
			if (FT_Load_Char(face, cp[i], FT_LOAD_RENDER)) {
				continue;
			}
			FT_GlyphSlot g = face->glyph;
			int ax = font->ft_char[slot[i]].atlas_x;
			int ay = font->ft_char[slot[i]].atlas_y;
			for (unsigned int row = 0; row < g->bitmap.rows; row++) {
				memcpy(buf + (size_t) (ay + row) * FONT_ATLAS_WIDTH + ax,
						g->bitmap.buffer + (size_t) row * g->bitmap.pitch,
						g->bitmap.width);
			}
		}

		// upload the whole atlas as a single 8-bit alpha texture
		glDeleteTextures(1, &font->atlas_tex);
		unsigned int texture;
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FONT_ATLAS_WIDTH, atlas_h, 0,
				GL_ALPHA, GL_UNSIGNED_BYTE, buf);
		font->atlas_tex = texture;
		font->atlas_w = FONT_ATLAS_WIDTH;
		font->atlas_h = atlas_h;
		free(buf);
	}
}


// Load all UI_MAX_FONT_COUNT sizes from the TTF at *path* into *fonts*. If the
// file cannot be opened and *mem* is given, falls back to that in-memory font so
// the UI still renders text regardless of the working directory.
static void load_font_face(FT_Library ft, const char *path, ui_font_st *fonts,
		const unsigned char *mem, unsigned int mem_len) {
	FT_Face face;
	FT_Error fterr = FT_New_Face(ft, path, 0, &face);
	bool from_file = (fterr == 0);
	if (fterr && (mem != NULL)) {
		fterr = FT_New_Memory_Face(ft, mem, mem_len, 0, &face);
	}
	if (fterr) {
		printf("Could not load font '%s'\n", path);
	}
	else {
		// Report the real source: reading "Loaded font '<path>'" while the file was
		// actually missing (and the embedded fallback used) previously masked a
		// wrong-fallback bug where the monospace view silently rendered proportional.
		if (from_file) {
			printf("Loaded font '%s'\n", path);
		}
		else {
			printf("Loaded embedded fallback font (file '%s' not found)\n", path);
		}

		// fetch the character dimensions from font sizes
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
		for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
			int32_t font_size = font_sizes[i] * this->scale;
			printf("Loading font size %i... ", font_size);
			fflush(stdout);
			FT_Set_Pixel_Sizes(face, 0, font_size);
			ui_font_st *font = &fonts[i];

			font->char_height = face->size->metrics.height / 64 / this->scale;

			build_font_atlas(font, face);

			printf("done\n");
			fflush(stdout);
		}
		FT_Done_Face(face);
	}
}

	// free the previously loaded fonts
static void load_fonts(void) {
	free_fonts();
	// install the fonts
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		printf("Could not init Freetype\n");
	}
	else {
		// Proportional UI font: use the font file if present (per-project
		// override), otherwise fall back to the font compiled into the binary.
		load_font_face(ft, "fonts/" DEFAULT_FONT, ui_fonts,
				embedded_font_ttf, embedded_font_ttf_len);
		// Monospace font for the log and device terminal views. Falls back to the
		// embedded monospace font if its file is missing, so the log/terminal stay
		// fixed-pitch regardless of the working directory (e.g. when run from an
		// installed location where no "fonts/" dir sits next to the binary).
		load_font_face(ft, "fonts/" MONO_FONT, ui_mono_fonts,
				embedded_mono_font_ttf, embedded_mono_font_ttf_len);
		FT_Done_FreeType(ft);
	}
}

static void free_fonts(void) {
	ui_font_st *arrays[2] = { ui_fonts, ui_mono_fonts };
	for (uint32_t a = 0; a < 2; a++) {
		for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
			if (arrays[a][i].char_height != 0) {
				glDeleteTextures(1, &arrays[a][i].atlas_tex);
				arrays[a][i].atlas_tex = 0;
			}
			arrays[a][i].char_height = 0;
		}
	}
}


// GLSL shader sources embedded into the binary, so the host UI works from any
// working directory. Previously these were fopen'd from a relative "shaders/"
// directory, so the UI only rendered correctly when run from the binary's own
// directory (otherwise the shaders failed to load and text/bitmaps disappeared).
// A matching file under shaders/ still takes precedence, allowing per-project
// overrides during development.
static const char *const text_shader_vs_src =
		"attribute vec4 coord;\n"
		"varying vec2 texpos;\n"
		"\n"
		"void main(void) {\n"
		"  gl_Position = vec4(coord.xy, 0, 1);\n"
		"  texpos = coord.zw;\n"
		"}\n";
static const char *const text_shader_fs_src =
		"varying vec2 texpos;\n"
		"uniform sampler2D tex;\n"
		"uniform vec4 color;\n"
		"\n"
		"void main(void) {\n"
		"  gl_FragColor = vec4(1, 1, 1, texture2D(tex, texpos).a) * color;\n"
		"}\n";
static const char *const bitmap_shader_vs_src =
		"#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec3 aColor;\n"
		"layout (location = 2) in vec2 aTexCoord;\n"
		"\n"
		"out vec3 ourColor;\n"
		"out vec2 TexCoord;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    gl_Position = vec4(aPos, 1.0);\n"
		"    ourColor = aColor;\n"
		"    TexCoord = aTexCoord;\n"
		"}\n";
static const char *const bitmap_shader_fs_src =
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"\n"
		"in vec3 ourColor;\n"
		"in vec2 TexCoord;\n"
		"\n"
		"uniform sampler2D ourTexture;\n"
		"uniform vec4 blendColor;\n"
		"\n"
		"void main()\n"
		"{\n"
		"    FragColor = texture(ourTexture, TexCoord) * blendColor;\n"
		"}\n";


// Compile a shader from GLSL source code. Returns the shader id, or 0 on error.
static GLuint compile_shader(GLenum shader_type, const char *src) {
	GLuint shader = glCreateShader(shader_type);
	if (shader) {
		glShaderSource(shader, 1, (const GLchar* const*) &src, NULL);
		glCompileShader(shader);

		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (!compiled) {
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen) {
				char* buf = (char*) malloc(infoLen);
				glGetShaderInfoLog(shader, infoLen, NULL, buf);
				printf("Shader compile error: %s\n", buf);
				free(buf);
			}
			glDeleteShader(shader);
			shader = 0;
		}
	}
	return shader;
}


// Load a shader: use the file at *path* if it exists (per-project override),
// otherwise fall back to the *embedded* source compiled into the binary.
static GLuint load_shader(GLenum shader_type, const char *path, const char *embedded) {
	char *file_src = NULL;
	FILE *f = fopen(path, "r");
	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		int32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		file_src = malloc(size + 1);
		memset(file_src, 0, size + 1);
		if (fread(file_src, 1, size, f)) {};
		fclose(f);
		printf("Loaded shader '%s' from file\n", path);
	}
	GLuint shader = compile_shader(shader_type, file_src ? file_src : embedded);
	free(file_src);
	return shader;
}


static unsigned int load_shader_program(const char *vertex_path, const char *vertex_src,
		const char *fragment_path, const char *fragment_src) {
	unsigned int ret = 0;
	GLuint vertex = load_shader(GL_VERTEX_SHADER, vertex_path, vertex_src);
	GLuint fragment = load_shader(GL_FRAGMENT_SHADER, fragment_path, fragment_src);
	if (vertex != 0 && fragment != 0) {
		ret = glCreateProgram();
		glAttachShader(ret, vertex);
		glAttachShader(ret, fragment);
		glLinkProgram(ret);
		GLint link_ok = GL_FALSE;
		glGetProgramiv(ret, GL_LINK_STATUS, &link_ok);
		if (!link_ok) {
			printf("ERROR linking shader program\n");
			glDeleteProgram(ret);
		}
	}
	else {
		printf("ERROR loading vertex & fragment shaders\n");
	}
	return ret;
}

static GLint get_uniform(GLuint shader_program, const char *name) {
	GLint uniform = glGetUniformLocation(shader_program, name);
	if (uniform == -1) {
		printf("ERROR: Could not bind uniform %s\n", name);
	}
	return uniform;
}


static GLint get_attrib(GLuint shader_program, const char *name) {
	GLint attribute = glGetAttribLocation(shader_program, name);
	if (attribute == -1) {
		printf("ERROR: Could not bind attribute %s\n", name);
	}
	return attribute;
}


bool uv_ui_init(void) {
	bool ret = true;
	this->refresh = false;
	this->resized = false;
	this->pressed = false;
	this->x = 0;
	this->y = 0;
	this->scalex = 1.0;
	this->scaley = 1.0;
	this->scale = 1.0;
	this->xoffset = 0;
	this->yoffset = 0;
	this->width = CONFIG_FT81X_HSIZE;
	this->height = CONFIG_FT81X_VSIZE;
	this->text_shader_program = 0;
	this->text_vbo = 0;

	// initialize the font sizes
	for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
		ui_fonts[i].char_height = font_sizes[i];
		ui_mono_fonts[i].char_height = font_sizes[i];
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

		printf("Creating GLFW window, version %s\n", glfwGetVersionString());
		// 4x multisampling for anti-aliasing
		glfwWindowHint(GLFW_SAMPLES, 8);
		/* Create a windowed mode window and its OpenGL context */
		this->window = glfwCreateWindow(CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE,
				CONFIG_UI_NAME, NULL, NULL);
		if (!this->window) {
			printf("GLFW terminated\n");
			ret = false;
		}
		else {
			// Make the window's context current
			glfwMakeContextCurrent(this->window);
			// add key listener
			glfwSetKeyCallback(this->window, &key_callback);
			glfwSetCharCallback(this->window, &char_callback);
			// add cursor position listener
			glfwSetCursorPosCallback(this->window, &cursor_position_callback);
			// add mouse button listener
			glfwSetMouseButtonCallback(this->window, &mouse_button_callback);
			glfwSetScrollCallback(this->window, &scroll_callback);
			glfwSetWindowSizeCallback(this->window, &resize_callback);

			// initialize GLEW
			if (glewInit() == GLEW_OK) {
				printf("OPENGL version %s\n", glGetString(GL_VERSION));

				//This sets up the viewport so that the coordinates (0, 0)
				// are at the top left of the window
				glViewport(0, 0, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE, 0, -10, 10);

				//Back to the modelview so we can draw stuff
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				// enable multisampling for anti-aliasing
				glEnable(GL_MULTISAMPLE);

				// enable stencil testing to masking
				glEnable(GL_STENCIL_TEST);

				// enable blend for text rendering
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				load_fonts();

				this->text_shader_program = load_shader_program(
						"shaders/text_shader.vs", text_shader_vs_src,
						"shaders/text_shader.ts", text_shader_fs_src);
				// Create the vertex buffer object
				glGenBuffers(1, &this->text_vbo);

				this->bitmap_shader_program = load_shader_program(
						"shaders/bitmap_shader.vs", bitmap_shader_vs_src,
						"shaders/bitmap_shader.ts", bitmap_shader_fs_src);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
			else {
				printf("GLEW init failed\n");
				ret = false;
			}
		}

		if (!ret) {
			glfwTerminate();
		}
	}

	return ret;
}



void uv_ui_destroy(void) {
	free_fonts();

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
