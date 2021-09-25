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
#include "uv_ui.h"
#include "uv_can.h"
#include "lodepng.h"
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#if CONFIG_UI && CONFIG_UI_OPENGL


#if !defined(CONFIG_UI_NAME)
#warning "CONFIG_UI_NAME not defined. The name of the window defaults to 'Window'"
#define CONFIG_UI_NAME		"Window"
#endif

#define DEFAULT_FONT	"LiberationSans-Regular.ttf"


static void key_callback(GLFWwindow* window,
		int key, int scancode, int action, int mods);
static void cursor_position_callback(GLFWwindow* window,
		double xpos, double ypos);
static void mouse_button_callback(GLFWwindow* window,
		int button, int action, int mods);
static void resize_callback(GLFWwindow* window,
		int width, int height);
static void load_fonts(void);
static void free_fonts(void);
static GLint get_uniform(GLuint shader_program, const char *name);
static GLint get_attrib(GLuint program, const char *name);

ui_font_st ui_fonts[UI_MAX_FONT_COUNT] = {};
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
uimedia_ll_st *uimedia_ll_st_create(char *filename, uint8_t* data,
		uint32_t width, uint32_t height, uint32_t data_len);

/// @brief: Finds the already existing uimedia_ll struct and returns it.
/// Returns NULL if not found.
uimedia_ll_st *uimedia_ll_st_find(char *filename);

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



uimedia_ll_st *uimedia_ll_st_create(char *filename, uint8_t* data,
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

uimedia_ll_st *uimedia_ll_st_find(char *filename) {
	uimedia_ll_st *ret = this->uimediall;

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
	// load the list of CAN devices
#if CONFIG_CAN
	uint32_t index = 0;
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
	uimedia_ll_st *media = uimedia_ll_st_find(bitmap->filename);

	if (media == NULL) {
		printf("ERROR: load bitmap with uv_ui_media_loadbitmapexmem before drawing it.\n");
	}
	else {
		glUseProgram(this->bitmap_shader_program);
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




void uv_ui_draw_point(int16_t x, int16_t y, color_t col, uint16_t diameter) {

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






void uv_ui_draw_rrect(const int16_t x, const int16_t y,
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



void uv_ui_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
	color_st c = uv_uic(color);
	glColor4ub(c.r, c.g, c.b, c.a);
	glLineWidth(width);
	glBegin(GL_LINE);
	glVertex2i(start_x, start_y);
	glVertex2i(end_x, end_y);
	glEnd();
}



void uv_ui_draw_linestrip(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
	if (point_count) {
		color_st c = uv_uic(color);
		glColor4ub(c.r, c.g, c.b, c.a);
		glLineWidth(line_width);

		if (type ==  UI_STRIP_TYPE_ABOVE) {
			glBegin(GL_POLYGON);
			glVertex2i(points[0].x, 0);
		}
		else if (type == UI_STRIP_TYPE_BELOW) {
			glBegin(GL_POLYGON);
			glVertex2i(points[0].x, CONFIG_FT81X_VSIZE);
		}
		else if (type == UI_STRIP_TYPE_LEFT) {
			glBegin(GL_POLYGON);
			glVertex2i(0, points[0].y);
		}
		else if (type == UI_STRIP_TYPE_RIGHT) {
			glBegin(GL_POLYGON);
			glVertex2i(CONFIG_FT81X_HSIZE, points[0].y);
		}
		else {
			glBegin(GL_LINE_LOOP);
			glVertex2i(points[0].x, points[0].y);
		}

		for (uint16_t i = 0; i < point_count; i++) {
			glVertex2i(points[i].x, points[i].y);
		}

		if (type ==  UI_STRIP_TYPE_ABOVE) {
			glVertex2i(points[point_count - 1].x, 0);
		}
		else if (type == UI_STRIP_TYPE_BELOW) {
			glVertex2i(points[point_count - 1].x, CONFIG_FT81X_VSIZE);
		}
		else if (type == UI_STRIP_TYPE_LEFT) {
			glVertex2i(0, points[point_count - 1].y);
		}
		else if (type == UI_STRIP_TYPE_RIGHT) {
			glVertex2i(CONFIG_FT81X_HSIZE, points[point_count - 1].y);
		}
		else {

		}

		glEnd();
	}
}



void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix) {

}



static void render_line(char *str, ui_font_st *font,
		int16_t x, int16_t y, color_t color) {
	// coordinates are in application space, and since uv_ui uses fixed window size,
	// we resize these accordingly
	x *= this->scalex;
	y *= this->scaley;

	// activate corresponding render state
	glUseProgram(this->text_shader_program);

	color_st col = uv_uic(color);
	GLfloat c[4] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f };

	GLint uniform_color;
	GLint uniform_tex;
	GLint attribute_coord;
	uniform_tex = get_uniform(this->text_shader_program, "tex");
	uniform_color = get_uniform(this->text_shader_program, "color");
	attribute_coord = get_attrib(this->text_shader_program, "coord");
	if (uniform_tex == -1 ||
			uniform_color == -1 ||
			attribute_coord == -1) {
		printf("ERROR loading OPENGL text shader program\n");
	}

	glUniform4fv(uniform_color, 1, c);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_tex, 0);

	/* We require 1 byte alignment when uploading texture data */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	/* Set up the VBO for our vertex data */
	glEnableVertexAttribArray(attribute_coord);
	glBindBuffer(GL_ARRAY_BUFFER, this->text_vbo);
	glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

	/* Loop through all characters */
	for (char *p = str; *p; p++) {
		glBindTexture(GL_TEXTURE_2D, font->ft_char[(unsigned char) *p].TextureID);

		float sx = 2.0f / this->width;
		float sy = 2.0f / this->height;

		/* Calculate the vertex and texture coordinates */
		float x2 = (x + font->ft_char[(unsigned char) *p].bearing_x) * sx - 1.0f;
		float y2 = (y - font->ft_char[(unsigned char) *p].bearing_y) * sy - 1.0f +
				(font->ft_char['H'].size_y * sy);
		float w = (font->ft_char[(unsigned char) *p].size_x) * sx;
		float h = (font->ft_char[(unsigned char) *p].size_y) * sy;

		struct point {
			GLfloat x;
			GLfloat y;
			GLfloat s;
			GLfloat t;
		} box[4] = {
				{x2, -y2, 0, 0},
				{x2 + w, -y2, 1, 0},
				{x2, -y2 - h, 0, 1},
				{x2 + w, -y2 - h, 1, 1},
		};


		/* Draw the character on the screen */
		glBufferData(GL_ARRAY_BUFFER, sizeof box, box, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		/* Advance the cursor to the start of the next character */
		x += (font->ft_char[(unsigned char) *p].advance / 64);
	}

	glDisableVertexAttribArray(attribute_coord);

	glUseProgram(0);
}

void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {

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
	if (align & VALIGN_CENTER) {
		// reduce the y by the number of line counts
		y -= (line_count * font->char_height / 2) / this->scaley;
	}

	for (uint32_t i = 0; i < strlen(str) + 1; i++) {
		if (s[i] == '\r' || s[i] == '\n' || s[i] == '\0') {
			s[i] = '\0';
			int16_t lx = x, ly = y;
			int32_t str_width = uv_ui_get_string_width(last_s, font);

			// draw one line of text at the time
			if (align & HALIGN_CENTER) {
				lx -= str_width / 2;
			}
			else if (align & HALIGN_RIGHT) {
				lx -= str_width;
			}
			else {

			}
			if (strlen(last_s)) {
				render_line(last_s, font, lx, ly, color);
				y += font->char_height;
			}
			last_s = &s[i + 1];
		}
	}
	free(s);
}



void uv_ui_set_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
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
	for (int i = 0; i < strlen(str); i++) {
		if (str[i] == '\n' ||
				str[i] == '\r') {
			if (line_width > ret) {
				line_width = 0;
			}
		}
		else {
			line_width += font->ft_char[(unsigned char) str[i]].advance / 64;
			if (line_width > ret) {
				ret = line_width;
			}
		}
	}

	return (ret / this->scalex);
}


uint32_t uv_uimedia_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename) {
	uimedia_ll_st *media = uimedia_ll_st_find(filename);
	if (media == NULL) {
		// load the pixel data
		unsigned error;
		unsigned char* image = NULL;
		unsigned width, height;
		printf("Loading bitmap '%s'\n", filename);
		error = lodepng_decode32_file(&image, &width, &height, filename);
		if (error) {
			printf("Loading bitmap '%s' failed\n", filename);
		}
		media = uimedia_ll_st_create(filename, image,
				width, height, width * height * 4);
	}
	bitmap->filename = media->filename;
	bitmap->width = media->width;
	bitmap->height = media->height;
	bitmap->type = UV_UIMEDIA_IMAGE;
	bitmap->size = media->data_len;

	return media->data_len;
}


void uv_ui_touchscreen_set_transform_matrix(ui_transfmat_st *transform_matrix) {

}



void uv_ui_dlswap(void) {
	if (this->window) {
		if (!glfwWindowShouldClose(this->window)) {

			// Swap front and back buffers
			glfwSwapBuffers(this->window);

			glClearStencil(1);
		    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		    this->refresh = false;

		    if (this->resized) {
		    	load_fonts();
		    	this->refresh = true;
		    	this->resized = false;
		    }
		}
		else {
			printf("Terminating GLFW\n");
			glfwTerminate();
			this->window = NULL;
			uv_ui_destroy();
			exit(0);
		}
	}
}



static void key_callback(GLFWwindow* window,
		int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		char c = (char) key;
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



	// free the previously loaded fonts
static void load_fonts(void) {
	free_fonts();
	// install the font
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		printf("Could not init Freetype\n");
	}
	else {
		FT_Face face;
		if (FT_New_Face(ft, "fonts/LiberationSans-Regular.ttf", 0, &face)) {
			printf("Could not load font '%s'\n", DEFAULT_FONT);
		}
		else {
			printf("Loaded font '%s'\n", DEFAULT_FONT);

			// fetch the character dimensions from font sizes
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
			for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
				int32_t font_size = font_sizes[i] * this->scale;
				printf("Loading font size %i... ", font_size);
				fflush(stdout);
				FT_Set_Pixel_Sizes(face, 0, font_size);
				ui_font_st *font = &ui_fonts[i];

				font->char_height = font_size;

				for (unsigned char c = 0; c < 128; c++) {
					// load character glyph
					if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
						printf("Failed to load glyph '%c' from font '%s'\n", c, DEFAULT_FONT);
						fflush(stdout);
						memset(&font->ft_char[c], 0, sizeof(font->ft_char[c]));
					}
					else {
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
						/* Upload the "bitmap", which contains an 8-bit grayscale image,
						 * as an alpha texture */
						glTexImage2D(
								GL_TEXTURE_2D,
								0,
								GL_ALPHA,
								face->glyph->bitmap.width,
								face->glyph->bitmap.rows,
								0,
								GL_ALPHA,
								GL_UNSIGNED_BYTE,
								face->glyph->bitmap.buffer);
						// now store character for later use
						font->ft_char[c].TextureID = texture;
						font->ft_char[c].advance = face->glyph->advance.x;
						font->ft_char[c].bearing_x = face->glyph->bitmap_left;
						font->ft_char[c].bearing_y = face->glyph->bitmap_top;
						font->ft_char[c].size_x = face->glyph->bitmap.width;
						font->ft_char[c].size_y = face->glyph->bitmap.rows;
					}
				}
				printf("done\n");
				fflush(stdout);
			}
			FT_Done_Face(face);
		}
		FT_Done_FreeType(ft);
	}
}

static void free_fonts(void) {
	for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
		if (ui_fonts[i].char_height != 0) {
			for (unsigned char c = 0; c < 128; c++) {
				glDeleteTextures(1, &ui_fonts[i].ft_char[c].TextureID);
			}
		}
		ui_fonts[i].char_height = 0;
	}
}


static GLuint load_shader(GLenum shader_type, const char* source) {
	// Create the shader
	GLuint shader = 0;
	printf("Loading shader '%s'\n", source);
	FILE *f = fopen(source, "r");
	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		int32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char *s = malloc(size + 1);
		memset(s, 0, size + 1);
		if (fread(s, 1, size, f)) {};

		shader = glCreateShader(shader_type);
		if ( shader ) {
			// Pass the shader source code
			glShaderSource(shader, 1, (const GLchar* const*) &s, NULL);

			// Compile the shader source code
			glCompileShader(shader);

			// Check the status of compilation
			GLint compiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
			if (!compiled) {

				// Get the info log for compilation failure
				GLint infoLen = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
				if (infoLen) {
					char* buf = (char*) malloc(infoLen);
					glGetShaderInfoLog(shader, infoLen, NULL, buf);
					printf("%s: %s\n", source, buf);
					free(buf);

					// Delete the shader program
					glDeleteShader(shader);
					shader = 0;
				}
			}
		}
		free(s);
	}
	else {
		printf("Failed to open file '%s'\n", source);
	}
	return shader;
}


static unsigned int load_shader_program(const char *vertex_shader, const char *fragment_shader) {
	unsigned int ret = 0;
	GLuint vertex, fragment;
	vertex = load_shader(GL_VERTEX_SHADER, vertex_shader);
	fragment = load_shader(GL_FRAGMENT_SHADER, fragment_shader);
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
		printf("ERROR loading vertez & fragment shaders\n");
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
			// add cursor position listener
			glfwSetCursorPosCallback(this->window, &cursor_position_callback);
			// add mouse button listener
			glfwSetMouseButtonCallback(this->window, &mouse_button_callback);
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
						"shaders/text_shader.vs", "shaders/text_shader.ts");
				// Create the vertex buffer object
				glGenBuffers(1, &this->text_vbo);

				this->bitmap_shader_program = load_shader_program(
						"shaders/bitmap_shader.vs", "shaders/bitmap_shader.ts");

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
