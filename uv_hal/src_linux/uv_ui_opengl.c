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
	// pointer to the next uimedia_ll_st
	void *next_ptr;
	void *surface;
} uimedia_ll_st;


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
	unsigned int text_vao;
	unsigned int text_vbo;

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
}




void uv_ui_draw_point(int16_t x, int16_t y, color_t col, uint16_t diameter) {

	color_st c = uv_uic(col);
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_TRIANGLE_FAN);

    double r = diameter / 2;
    glVertex2i(x, y); // Center
	int resolution = MAX(this->height / 50 * diameter / 100, 4);
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




void uv_ui_draw_string(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
	color_st c = uv_uic(color);

	// activate corresponding render state
	glUseProgram(this->text_shader_program);

	glUniform3f(glGetUniformLocation(this->text_shader_program, "textColor"),
			c.r / 255.0f, c.g / 255.0f, c.b / 255.0f);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(this->text_vao);

	// iterate through all characters
	printf("Drawing text '%s'\n", str);
	for (uint32_t i = 0; i < strlen(str); i++) {
		unsigned char c = str[i];


		float xpos = x + font->ft_char[c].bearing_x * this->scale;
		float ypos = y - (font->ft_char[c].size_y - font->ft_char[c].bearing_y) * this->scale;

		float w = font->ft_char[c].size_x * this->scale;
		float h = font->ft_char[c].size_y * this->scale;
		// update VBO for each character
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, font->ft_char[c].TextureID);
		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, this->text_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		// bitshift by 6 to get value in pixels (2^6 = 64)
		x += ((float) font->ft_char[c].advance / 64) * this->scale;
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
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

;



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



static void load_fonts(void) {
	// free the previously loaded fonts
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
						glGenTextures(1, &texture);
						glBindTexture(GL_TEXTURE_2D, texture);
						glTexImage2D(
							GL_TEXTURE_2D,
							0,
							GL_RED,
							face->glyph->bitmap.width,
							face->glyph->bitmap.rows,
							0,
							GL_RED,
							GL_UNSIGNED_BYTE,
							face->glyph->bitmap.buffer
						);
						// set texture options
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	this->text_vao = 0;
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

				//This sets up the viewport so that the coordinates (0, 0) are at the top left of the window
				glViewport(0, 0, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();
				glOrtho(0, CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE, 0, -10, 10);

				//Back to the modelview so we can draw stuff
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();

				// enable multisampling for anti-aliasing
				glEnable(GL_MULTISAMPLE);

				// enable blend for text rendering
//				glEnable(GL_BLEND);
//				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				//Clear the screen and depth buffer
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				load_fonts();

				GLuint vertex_shader, gradient_shader;
				vertex_shader = load_shader(GL_VERTEX_SHADER, "shaders/text_shader.vs");
				gradient_shader = load_shader(GL_FRAGMENT_SHADER, "shaders/text_shader.ts");
				if (vertex_shader != 0 && gradient_shader != 0) {
					this->text_shader_program = glCreateProgram();
					glAttachShader(this->text_shader_program, vertex_shader);
					glAttachShader(this->text_shader_program, gradient_shader);
					glLinkProgram(this->text_shader_program);
					glDeleteShader(vertex_shader);
					glDeleteShader(gradient_shader);


					glGenVertexArrays(1, &this->text_vao);
					glGenBuffers(1, &this->text_vbo);
					glBindVertexArray(this->text_vao);
					glBindBuffer(GL_ARRAY_BUFFER, this->text_vbo);
					glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
					glEnableVertexAttribArray(0);
					glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindVertexArray(0);
				}
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
		if (this->text_shader_program) {
			glDeleteProgram(this->text_shader_program);
			glDeleteBuffers(1, &this->text_vbo);
			glDeleteVertexArrays(1, &this->text_vao);
		}
	}
}



#endif
