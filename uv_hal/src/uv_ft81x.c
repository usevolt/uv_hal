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



#include "uv_ft81x.h"
#include "uv_gpio.h"
#include "uv_rtos.h"
#include "uv_w25q128.h"
#include "ui/uv_uifont.h"
#include "uv_terminal.h"
#include <string.h>


#if CONFIG_FT81X



#define READ8_LEN						5
#define READ16_LEN						6
#define READ32_LEN						8
#define WRITE8_LEN						4
#define WRITE16_LEN						5
#define WRITE32_LEN						7
#define DRAW_LINE_BUF_LEN				6
#define FONT_METRICS_BASE_ADDR			0x201EE0
#define FONT_METRICS_FONT_LEN			148
#define FONT_METRICS_CHAR_WIDTH_OFFSET	0
#define FONT_METRICS_FONT_HEIGHT_OFFSET	140


/// @brief: Prefixes are the first 2 bytes sent to the device
typedef enum {
	FT81X_PREFIX_READ = 0x00,
	FT81X_PREFIX_WRITE = 0x80,
	FT81X_PREFIX_CMD = 0x40
} ft81x_rwcmd_prefix_e;


typedef enum {
	HOSTCMD_ACTIVE 				= 0x0,
	HOSTCMD_STANDBY 			= 0x41,
	HOSTCMD_SLEEP 				= 0x42,
	HOSTCMD_PWRDOWN 			= 0x43,
	HOSTCMD_PD_ROMS 			= 0x49,
	HOSTCMD_CLKEXT 				= 0x44,
	HOSTCMD_CLKINT 				= 0x48,
	HOSTCMD_CLKSEL 				= 0x61,
	HOSTCMD_RST_PULSE 			= 0x68,
	HOSTCMD_PINDRIVE 			= 0x70,
	HOSTCMD_PIN_PD_STATE 		= 0x71
} ft81x_hostcmds_e;


typedef enum {
	MEMMAP_RAM_G_BEGIN 			= 0x000000,
	MEMMAP_RAM_G_END 			= 0x0FFFFF,
	MEMMAP_ROM_FONT_BEGIN 		= 0x1E0000,
	MEMMAP_ROM_FONT_END 		= 0x2FFFFB,
	MEMMAP_ROM_FONT_ADDR_BEGIN 	= 0x2FFFFC,
	MEMMAP_ROM_FONT_ADDR_END 	= 0x2FFFFF,
	MEMMAP_RAM_DL_BEGIN 		= 0x300000,
	MEMMAP_RAM_DL_END 			= 0x301FFF,
	MEMMAP_RAM_REG_BEGIN 		= 0x302000,
	MEMMAP_RAM_REG_END 			= 0x302FFF,
	MEMMAP_RAM_CMD_BEGIN 		= 0x308000,
	MEMMAP_RAM_CMD_END 			= 0x309000
} ft81x_memmap_e;

#define RAMDL_SIZE		FT81X_RAMDL_SIZE
#define RAMCMD_SIZE		0x1000

typedef enum {
	REG_ID 						= 0x302000,
	REG_FRAMES 					= 0x302004,
	REG_CLOCK 					= 0x302008,
	REG_FREQUENCY 				= 0x30200C,
	REG_RENDERMODE 				= 0x302010,
	REG_SNAPY 					= 0x302014,
	REG_SNAPSHOT 				= 0x302018,
	REG_SNAPFORMAT 				= 0x30201C,
	REG_CPURESET 				= 0x302020,
	REG_TAP_CRC 				= 0x302024,
	REG_TAP_MASK 				= 0x302028,
	REG_HCYCLE 					= 0x30202C,
	REG_HOFFSET 				= 0x302030,
	REG_HSIZE 					= 0x302034,
	REG_HSYNC0 					= 0x302038,
	REG_HSYNC1 					= 0x30203C,
	REG_VCYCLE 					= 0x302040,
	REG_VOFFSET 				= 0x302044,
	REG_VSIZE 					= 0x302048,
	REG_VSYNC0 					= 0x30204C,
	REG_VSYNC1 					= 0x302050,
	REG_DLSWAP 					= 0x302054,
	REG_ROTATE 					= 0x302058,
	REG_OUTBITS 				= 0x30205C,
	REG_DITHER 					= 0x302060,
	REG_SWIZZLE 				= 0x302064,
	REG_CSPREAD 				= 0x302068,
	REG_PCLK_POL 				= 0x30206C,
	REG_PCLK 					= 0x302070,
	REG_TAG_X 					= 0x302074,
	REG_TAG_Y 					= 0x302078,
	REG_TAG 					= 0x30207C,
	REG_VOL_PB 					= 0x302080,
	REG_VOL_SOUND 				= 0x302084,
	REG_SOUND 					= 0x302088,
	REG_PLAY 					= 0x30208C,
	REG_GPIO_DIR 				= 0x302090,
	REG_GPIO 					= 0x302094,
	REG_GPIOX_DIR 				= 0x302098,
	REG_GPIOX 					= 0x30209C,
	REG_INT_FLAGS 				= 0x3020A8,
	REG_INT_EN 					= 0x3020AC,
	REG_INT_MASK 				= 0x3020B0,
	REG_PLAYBACK_START 			= 0x3020B4,
	REG_PLAYBACK_LENGTH 		= 0x3020B8,
	REG_PLAYBACK_READPTR 		= 0x3020BC,
	REG_PLAYBACK_FREQ 			= 0x3020C0,
	REG_PLAYBACK_FORMAT 		= 0x3020C4,
	REG_PLAYBACK_LOOP 			= 0x3020C8,
	REG_PLAYBACK_PLAY 			= 0x3020CC,
	REG_PWM_HZ 					= 0x3020D0,
	REG_PWM_DUTY 				= 0x3020D4,
	REG_MACRO_0 				= 0x3020D8,
	REG_MACRO_1 				= 0x3020DC,
	REG_CMD_READ 				= 0x3020F8,
	REG_CMD_WRITE 				= 0x3020FC,
	REG_CMD_DL 					= 0x302100,
	REG_TOUCH_MODE 				= 0x302104,
	REG_TOUCH_ADC_MODE 			= 0x302108,
	REG_TOUCH_CHARGE 			= 0x30210C,
	REG_TOUCH_SETTLE 			= 0x302110,
	REG_TOUCH_OVERSAMPLE 		= 0x302114,
	REG_TOUCH_RZTHRES 			= 0x302118,
	REG_TOUCH_RAW_XY 			= 0x30211C,
	REG_TOUCH_RZ 				= 0x302120,
	REG_TOUCH_SCREEN_XY 		= 0x302124,
	REG_TOUCH_TAG_ZY 			= 0x302128,
	REG_TOUCH_TAG 				= 0x30212C,
	REG_TOUCH_TAG1_XY 			= 0x302130,
	REG_TOUCH_TAG1 				= 0x302134,
	REG_TOUCH_TAG2_XY 			= 0x302138,
	REG_TOUCH_TAG2 				= 0x30213C,
	REG_TOUCH_TAG3_XY 			= 0x302140,
	REG_TOUCH_TAG3 				= 0x302144,
	REG_TOUCH_TAG4_XY 			= 0x302148,
	REG_TOUCH_TAG4 				= 0x30214C,
	REG_TOUCH_TRANSFORM_A 		= 0x302150,
	REG_TOUCH_TRANSFORM_B 		= 0x302154,
	REG_TOUCH_TRANSFORM_C 		= 0x302158,
	REG_TOUCH_TRANSFORM_D 		= 0x30215C,
	REG_TOUCH_TRANSFORM_E 		= 0x302160,
	REG_TOUCH_TRANSFORM_F 		= 0x302164,
	REG_TOUCH_CONFIG 			= 0x302168,
	REG_CTOUCH_TOUCH4_X 		= 0x30216C,
	REG_BIST_EN 				= 0x302174,
	REG_TRIM 					= 0x302180,
	REG_ANA_COMP 				= 0x302184,
	REG_SPI_WIDTH 				= 0x302188,
	REG_TOUCH_DIRECT_XY 		= 0x30218C,
	REG_TOUCH_DIRECT_Z1Z2 		= 0x302190,
	REG_DATESTAMP 				= 0x302564,
	REG_CMDB_SPACE 				= 0x302574,
	REG_CMDB_WRITE 				= 0x302578,
	REG_MEDIAFIFO_READ			= 0x309014,
	REG_MEDIAFIFO_WRITE			= 0x309018
} ft81x_reg_e;


// DISPLAY LIST COMMANDS
#define ALPHA_FUNC_NEVER	0
#define ALPHA_FUNC_LESS		1
#define ALPHA_FUNC_LEQUAL	2
#define ALPHA_FUNC_GREATER	3
#define ALPHA_FUNC_GEQUAL	4
#define ALPHA_FUNC_EQUAL	5
#define ALPHA_FUNC_NOTEQUAL	6
#define ALPHA_FUNC_ALWAYS	7

#define BEGIN_BITMAPS		1
#define BEGIN_POINTS		2
#define BEGIN_LINES			3
#define BEGIN_LINE_STRIP	4
#define BEGIN_EDGE_STRIP_R	5
#define BEGIN_EDGE_STRIP_L	6
#define BEGIN_EDGE_STRIP_A	7
#define BEGIN_EDGE_STRIP_B	8
#define BEGIN_RECTS			9

#define BITMAP_LAYOUT_ARGB1555			0
#define BITMAP_LAYOUT_L1				1
#define BITMAP_LAYOUT_L4				2
#define BITMAP_LAYOUT_L8				3
#define BITMAP_LAYOUT_RGB332			4
#define BITMAP_LAYOUT_ARGB2				5
#define BITMAP_LAYOUT_ARGB4				6
#define BITMAP_LAYOUT_RGB565			7
#define BITMAP_LAYOUT_TEXT8X8			9
#define BITMAP_LAYOUT_TEXTVGA			10
#define BITMAP_LAYOUT_BARGRAPH			11
#define BITMAP_LAYOUT_PALETTED565		14
#define BITMAP_LAYOUT_PALETTED4444		15
#define BITMAP_LAYOUT_PALETTED8			16
#define BITMAP_LAYOUT_L2				17

#define BITMAP_SIZE_FILTER_NEAREST		0
#define BITMAP_SIZE_FILTER_BILINEAR		1
#define BITMAP_SIZE_WRAP_REPEAT			1
#define BITMAP_SIZE_WRAP_BORDER			0

#define BLEND_FUNC_ZERO					0
#define BLEND_FUNC_ONE					1
#define BLEND_FUNC_SRC_ALPHA			2
#define BLEND_FUNC_DST_ALPHA			3
#define BLEND_FUNC_ONE_MINUS_SRC_ALPHA	4
#define BLEND_FUNC_ONE_MINUS_DEST_ALPHA	5

#define MACRO_0							0
#define MACRO_1							1

#define STENCIL_FUNC_NEVER				0
#define STENCIL_FUNC_LESS				1
#define STENCIL_FUNC_LEQUAL				2
#define STENCIL_FUNC_GREATER			3
#define STENCIL_FUNC_GEQUAL				4
#define STENCIL_FUNC_EQUAL				5
#define STENCIL_FUNC_NOTEQUAL			6
#define STENCIL_FUNC_ALWAYS				7

#define STENCIL_OP_ZERO					0
#define STENCIL_OP_KEEP					1
#define STENCIL_OP_REPLACE				2
#define STENCIL_OP_INCR					3
#define STENCIL_OP_DECR					4
#define STENCIL_OP_INVERT				5

#define OPT_RGB565						0
#define OPT_MONO						1
#define OPT_NODL						2
#define OPT_MEDIAFIFO					16

#define ALPHA_FUNC(func,ref) 								((0x9 << 24) | ((func) << 8) | (ref))
#define BEGIN(prim)											((0x1F << 24) | (prim))
#define BITMAP_HANDLE(handle) 								((0x5 << 24) | (handle))
#define BITMAP_LAYOUT_H(linestride, height) 				((0x28 << 24) | ((linestride) << 2) | (height))
#define BITMAP_LAYOUT(format, linestride, height)			((0x7 << 24) | ((format) << 19) | ((linestride & 1023) << 9) | (height & 511))
#define BITMAP_SIZE(filter, wrapx, wrapy, width, height)	((0x8 << 24) | ((filter) << 20) | ((wrapx) << 19) | ((wrapy) << 18) | ((width & 511) << 9) | (height & 511))
#define BITMAP_SIZE_H(width, height)						((0x29 << 24) | ((width) << 2) | (height))
#define BITMAP_SOURCE(addr)									((0x1 << 24) | (addr))
#define BITMAP_TRANSFORM_A(a)								((0x15 << 24) | (a))
#define BITMAP_TRANSFORM_B(b)								((0x16 << 24) | (b))
#define BITMAP_TRANSFORM_C(c)								((0x17 << 24) | (c))
#define BITMAP_TRANSFORM_D(d)								((0x18 << 24) | (d))
#define BITMAP_TRANSFORM_E(e)								((0x19 << 24) | (e))
#define BITMAP_TRANSFORM_F(f)								((0x1A << 24) | (f))
#define BLEND_FUNC(src, dst)								((0xB << 24) | ((src) << 3) | (dst))
#define CALL(dest)											((0x1D << 24) | (dest))
#define CELL(cell)											((0x6 << 24) | (cell))
#define CLEAR(color, stencil, tag)							((0x26 << 24) | ((color) << 2) | ((stencil) << 1) | (tag))
#define CLEAR_COLOR_A(alpha)								((0xF << 24) | (alpha))
#define CLEAR_COLOR_RGB(r, g, b)							((0x2 << 24) | ((r) << 16) | ((g) << 8) | (b))
#define CLEAR_STENCIL(s)									((0x11 << 24) | (s))
#define CLEAR_TAG(t)										((0x12 << 24) | (t))
#define COLOR_A(alpha)										((0x10 << 24) | (alpha))
#define COLOR_MASK(r, g, b, a)								((0x20 << 24) | ((r) << 3) | ((g) << 2) | ((b) << 1) | (a))
#define COLOR_RGB(r, g, b)									((0x4 << 24) | ((r) << 16) | ((g) << 8) | (b))
#define DISPLAY()											(0)
#define END()												(0x21 << 24)
#define JUMP(dest)											((0x1E << 24) | (dest))
#define LINE_WIDTH(width)									((0xE << 24) | (width))
#define MACRO(m)											((0x25 << 24) | (m))
#define NOP()												(0x2D << 24)
#define PALETTE_SOURCE(addr)								((0x2A << 24) | (addr))
#define POINT_SIZE(size)									((0xD << 24) | (size))
#define RESTORE_CONTENT()									(0x23 << 24)
#define RETURN()											(0x24 << 24)
#define SAVE_CONTENT()										(0x22 << 24)
#define SCISSOR_SIZE(width, height)							((0x1C << 24) | ((width) << 12) | (height))
#define SCISSORS_XY(x, y)									((0x1B << 24) | ((x) << 11) | (y))
#define STENCIL_FUNC(func, ref, mask)						((0xA << 24) | ((func) << 16) | ((ref) << 8) | (mask))
#define STENIL_MASK(mask)									((0x13 << 24) | (mask))
#define STENCIL_OP(sfail, spass)							((0xC << 24) | ((sfail) << 3) | (spass))
#define TAG(s)												((0x3 << 24) | (s))
#define TAG_MASK(mask)										((0x14 << 24) | (mask))
#define VERTEX2F(x, y)										((0x1 << 30) | ((x) << 15) | (y))
#define VERTEX2II(x, y, handle, cell)						((0x2 << 30) | ((x) << 21) | ((y) << 12) | ((handle) << 7) | (cell))
#define VERTEX_FORMAT(frac)									((0x27 << 24) | (frac))
#define VERTEX_TRANSLATE_X(x)								((0x2B << 24) | (x))
#define VERTEX_TRANSLATE_Y(y)								((0x2C << 24) | (y))


typedef enum {
	CMD_DLSTART = 		0xFFFFFF00,
	CMD_SWAP = 			0xFFFFFF01,
	CMD_COLDSTART = 	0xFFFFFF32,
	CMD_INTERRUPT = 	0xFFFFFF02,
	CMD_APPEND = 		0xFFFFFF1E,
	CMD_REGREAD = 		0xFFFFFF19,
	CMD_MEMWRITE = 		0xFFFFFF1A,
	CMD_INFLATE = 		0xFFFFFF22,
	CMD_LOADIMAGE = 	0xFFFFFF24,
	CMD_MEDIAFIFO = 	0xFFFFFF39,
	CMD_PLAYVIDEO = 	0xFFFFFF3A,
	CMD_VIDEOSTART = 	0xFFFFFF40,
	CMD_VIDEOFRAME = 	0xFFFFFF41,
	CMD_MEMCRC = 		0xFFFFFF18,
	CMD_MEMZERO = 		0xFFFFFF1C,
	CMD_MEMSET = 		0xFFFFFF1B,
	CMD_MEMCPY = 		0xFFFFFF1D,
	CMD_BUTTON = 		0xFFFFFF0D,
	CMD_CLOCK = 		0xFFFFFF14,
	CMD_FGCOLOR = 		0xFFFFFF0A,
	CMD_BGCOLOR = 		0xFFFFFF09,
	CMD_GRADCOLOR = 	0xFFFFFF34,
	CMD_GAUGE = 		0xFFFFFF13,
	CMD_GRADIENT = 		0xFFFFFF0B,
	CMD_KEYS = 			0xFFFFFF0E,
	CMD_PROGRESS = 		0xFFFFFF0F,
	CMD_SCROLLBAR = 	0xFFFFFF11,
	CMD_SLIDER = 		0xFFFFFF10,
	CMD_DIAL = 			0xFFFFFF2D,
	CMD_TOGGLE = 		0xFFFFFF12,
	CMD_TEXT = 			0xFFFFFF0C,
	CMD_SETBASE = 		0xFFFFFF38,
	CMD_NUMBER = 		0xFFFFFF2E,
	CMD_LOADIDENTITY = 	0xFFFFFF26,
	CMD_SETMATRIX = 	0xFFFFFF2A,
	CMD_GETMATRIX = 	0xFFFFFF33,
	CMD_GETPTR = 		0xFFFFFF23,
	CMD_GETPROPS = 		0xFFFFFF25,
	CMD_SCALE = 		0xFFFFFF28,
	CMD_ROTATE = 		0xFFFFFF29,
	CMD_TRANSLATE = 	0xFFFFFF27,
	CMD_CALIBRATE = 	0xFFFFFF15,
	CMD_SETROTATE = 	0xFFFFFF36,
	CMD_SPINNER = 		0xFFFFFF16,
	CMD_SCREENSAVER = 	0xFFFFFF2F,
	CMD_SKETCH = 		0xFFFFFF30,
	CMD_STOP = 			0xFFFFFF17,
	CMD_SETFONT = 		0xFFFFFF2B,
	CMD_SETFONT2 = 		0xFFFFFF3B,
	CMD_SETSCRATCH = 	0xFFFFFF3C,
	CMD_ROMFONT = 		0xFFFFFF3F,
	CMD_TRACK = 		0xFFFFFF2C,
	CMD_SNAPSHOT = 		0xFFFFFF1F,
	CMD_SNAPSHOT2 = 	0xFFFFFF37,
	CMD_SETBITMAP = 	0xFFFFFF43,
	CMD_LOGO = 			0xFFFFFF31
} ft81x_cmds_e;


#define _DEBUG		0

#if defined(DEBUG)
#undef DEBUG
#endif
#if _DEBUG
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(f_, ...) ;
#endif



static inline void writedl(uint32_t data);
static uint8_t read8(const ft81x_reg_e address);
static uint16_t read16(const ft81x_reg_e address);
static uint32_t read32(const ft81x_reg_e address);
static void writestr(const ft81x_reg_e address, const char *src, char *dest, const uint16_t len);
static void write8(const ft81x_reg_e address, uint8_t value);
static void write16(const ft81x_reg_e address, uint16_t value);
static void write32(const ft81x_reg_e address, uint32_t value);
static void writehostcmd(const ft81x_hostcmds_e hostcmd, uint8_t parameter);

static void init_values(void);
static void set_color(color_t c);
static void set_begin(uint8_t begin_type);
static void set_point_size(uint16_t diameter);
static void set_line_diameter(const uint16_t diameter);
static void set_font(const uint8_t font);
static void set_cell(const uint8_t cell);
static bool cmd_wait(void);
static void cmd_romfont(uint8_t bitmap_handle, uint8_t font_number);
static void draw_line(char *str, ft81x_font_st *font,
		int16_t x, int16_t y, ft81x_align_e align, color_t color, uint16_t len);
bool visible(const int16_t x, const int16_t y, const int16_t w, const int16_t height);



/// @brief: Strucutre for assigning coordinates to VERTEX2F macro.
/// Transforms signed coordinates to 15-bit space. Use **ux** and **uy**
/// unsigned formats for bitwise operations.
typedef struct {
	union {
		signed int sx	: 15;
		unsigned int ux	: 15;
	};
	int _reserved 	: 1;
	union {
		signed int sy	: 15;
		unsigned int uy	: 15;
	};
	int _reserved2	: 1;
} vertex2f_st;

/// @brief: Main FT81X data structure which contains the current values for
/// colors, etc.
typedef struct {
	uint32_t dl_index;
	uint32_t dl_index_max;
	color_st color;
	color_st clear_color;
	ft81x_color_modes_e color_mode;
	int8_t grayscale_luminosity;
	bool color_init;
	bool clear_color_init;
	uint8_t backlight;
	uint8_t begin_type;
	uint16_t point_diameter;
	uint16_t line_width;
	uint8_t font;
	uint8_t cell;
	// tells the next address for the co-processor ring buffer
	uint32_t cmdwriteaddr;
	struct {
		int16_t x;
		int16_t y;
		int16_t width;
		int16_t height;
	} mask;

} uv_ft81x_st;

ft81x_font_st ft81x_fonts[FT81X_MAX_FONT_COUNT];

uv_ft81x_st ft81x;
#define this (&ft81x)

/// @brief: Defines the bitmap handle start for graphics
#define GRAPHIC_HANDLE			0
/// @brief: Defines the number of handles used for graphics
#define GRAPHIC_HANDLE_COUNT	FONT_HANDLE
/// @brief: defines the bitmap handle for the first font. All handles bigger than this
/// are reserved for fonts
#define FONT_HANDLE				(30 - FT81X_MAX_FONT_COUNT)
#define FONT_HANDLE_DISABLED	UIBITMAP_DISABLED
/// @brief: Scratch handle for ft81x pre-processor
#define SCRATCH_HANDLE			31


static void init_values(void) {
	// initialize all values which should be initializes when swapping the dl buffer
	this->begin_type = 0xFF;
	this->point_diameter = 0xFFFF;
	this->line_width = 0xFFFF;
	this->font = 0xFF;
	this->cell = 0xFF;
	this->color_init = false;
	this->clear_color_init = false;
}



bool uv_ft81x_init(void) {
	bool ret = false;

	this->dl_index = 0;
	this->dl_index_max = 0;
	this->backlight = 50;
	this->mask.x = 0;
	this->mask.y = 0;
	this->mask.width = LCD_W_PX;
	this->mask.height = LCD_H_PX;
	this->grayscale_luminosity = 0;
	this->color_mode = COLOR_MODE_RGB;
	init_values();

	// toggle PD pin to reset the FT81X
	UV_GPIO_INIT_OUTPUT(CONFIG_FT81X_PD_IO, true);
	uv_rtos_task_delay(300);
	UV_GPIO_SET(CONFIG_FT81X_PD_IO, false);
	uv_rtos_task_delay(100);
	UV_GPIO_SET(CONFIG_FT81X_PD_IO, true);
	uv_rtos_task_delay(100);

	// FT81X is now in sleep state. Set it to active state
	writehostcmd(HOSTCMD_ACTIVE, 0);
	uv_rtos_task_delay(100);

	// read ID, should be 0x7C
	uint8_t id = read8(REG_ID);
	if (id == 0x7C) {
		DEBUG("FT81X id: 0x%x\n", id);

		// read co-processor command fifo buffer address.
		// Address is 0 only if power-on-reset happened,
		// otherwise it can be anything.
		this->cmdwriteaddr = read16(REG_CMD_WRITE);

		// configure display registers
		write16(REG_HCYCLE, CONFIG_FT81X_HCYCLE);
		write16(REG_HOFFSET, CONFIG_FT81X_HOFFSET);
		write16(REG_HSYNC0, CONFIG_FT81X_HSYNC0);
		write16(REG_HSYNC1, CONFIG_FT81X_HSYNC1);
		write16(REG_VCYCLE, CONFIG_FT81X_VCYCLE);
		write16(REG_VSYNC0, CONFIG_FT81X_VSYNC0);
		write16(REG_VSYNC1, CONFIG_FT81X_VSYNC1);
		write16(REG_SWIZZLE, 0);
		write16(REG_PCLK_POL, CONFIG_FT81X_PCLK_POL);
		write16(REG_CSPREAD, CONFIG_FT81X_CSPREAD);
		write16(REG_HSIZE, CONFIG_FT81X_HSIZE);
		write16(REG_VSIZE, CONFIG_FT81X_VSIZE);

		uv_ft81x_clear(C(0xFFFFFFFF));
		uv_ft81x_dlswap();

		uint8_t pclk = (60000000 / CONFIG_FT81X_PCLK_HZ) % 0x7F;
		write8(REG_PCLK, pclk);

		// set backlight PWM to half brightness
		write16(REG_PWM_HZ, CONFIG_FT81X_BACKLIGHT_PWM_FREQ_HZ);
		uv_ft81x_set_backlight(this->backlight);

		// enable DISPLAY pin
		write8(REG_GPIO_DIR, 0x80  | read8(REG_GPIO_DIR));
		write8(REG_GPIO, 0x80 | read8(REG_GPIO));

		uv_ft81x_dlswap();



		// download font height data for normal fonts
		// Note: This modifies the bitmap handles to contain handles only to anti-aliased
		// fonts. This was made when implementing the bitmap drawing, but
		// was later noticed to be useless.
		for (int32_t i = 0; i < FT81X_MAX_FONT_COUNT; i++) {
			// we support only anti-aliased fonts starting from font index 26
			ft81x_fonts[i].index = 26 + i;
			ft81x_fonts[i].char_height = read32(FONT_METRICS_BASE_ADDR +
					(10 + i) * FONT_METRICS_FONT_LEN +
					FONT_METRICS_FONT_HEIGHT_OFFSET);
			ft81x_fonts[i].handle = FONT_HANDLE + i;
			cmd_romfont(ft81x_fonts[i].handle, ft81x_fonts[i].index);
			DEBUG("Font %u height: %u\n", i, ft81x_fonts[i].char_height);
		}

		// lastly wait until the screen is not pressed.
		// If the screen is pressed more than 1 second, start screen
		// calibration
		uv_delay_st d;
		uv_delay_init(&d, 100);
		while (uv_ft81x_get_touch(NULL, NULL)) {
			uv_delay(&d, 20);
			uv_rtos_task_delay(20);
		}
		if (uv_delay_has_ended(&d)) {
			ret = true;
		}
	}
	else {
		printf("Couldn't read FT81X device ID.\n");
	}
	return ret;
}





static uint8_t read8(const ft81x_reg_e address) {
	uint16_t wb[READ8_LEN] = {};
	uint16_t rb[READ8_LEN] = {};
	wb[0] = FT81X_PREFIX_READ | ((address >> 16) & 0x3F);
	wb[1] = (address >> 8) & 0xFF;
	wb[2] = (address) & 0xFF;

	uv_spi_readwrite_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL,
				wb, rb, 8, READ8_LEN);
	return (uint8_t) rb[4];
}



static uint16_t read16(const ft81x_reg_e address) {
	uint16_t wb[READ16_LEN] = {};
	uint16_t rb[READ16_LEN] = {};
	wb[0] = FT81X_PREFIX_READ | ((address >> 16) & 0x3F);
	wb[1] = (address >> 8) & 0xFF;
	wb[2] = address & 0xFF;

	uv_spi_readwrite_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL, wb, rb, 8, READ16_LEN);

	uint16_t ret = rb[4] + (rb[5] << 8);
	return ret;
}



static uint32_t read32(const ft81x_reg_e address) {
	uint16_t wb[READ32_LEN] = {};
	uint16_t rb[READ32_LEN] = {};
	wb[0] = FT81X_PREFIX_READ | ((address >> 16) & 0x3F);
	wb[1] = (address >> 8) & 0xFF;
	wb[2] = (address) & 0xFF;

	uv_spi_readwrite_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL, wb, rb, 8, READ32_LEN);

	uint32_t ret = ((uint32_t) rb[7] << 24) + ((uint32_t) rb[6] << 16) + ((uint32_t) rb[5] << 8) + rb[4];
	return ret;
}


#define ADDR_OFFSET		3
#define WRITESTR_BUFFER_LEN	64
static void writestr(const ft81x_reg_e address,
		const char *src, char *dest, const uint16_t len) {
	uint32_t addr = (unsigned int) address;
	uint32_t written = 0;
	// wrap the writing to multiple blocks, to save RAM memory from stack
	while (written < len) {
		// exception: If co-processor buffer is written, check that we dont overflow it
		if (addr >= MEMMAP_RAM_CMD_END) {
			addr -= MEMMAP_RAM_CMD_END - MEMMAP_RAM_CMD_BEGIN;
		}
		uint32_t l = uv_mini(WRITESTR_BUFFER_LEN, len);
		uint16_t wb[l + ADDR_OFFSET];
		wb[0] = FT81X_PREFIX_WRITE | ((addr >> 16) & 0x3F);
		wb[1] = (addr >> 8) & 0xFF;
		wb[2] = (addr) & 0xFF;
		for (uint16_t i = 0; i < l; i++) {
			wb[i + ADDR_OFFSET] = src[written + i];
		}
		if (dest == NULL) {
			uv_spi_write_sync(CONFIG_FT81X_SPI_CHANNEL,
					CONFIG_FT81X_SSEL, wb, 8, l + ADDR_OFFSET);
		}
		else {
			uv_spi_readwrite_sync(CONFIG_FT81X_SPI_CHANNEL,
					CONFIG_FT81X_SSEL, wb, wb, 8, l + ADDR_OFFSET);
			for (uint16_t i = 0; i < l; i++) {
				dest[written + i] = wb[i + ADDR_OFFSET];
			}
		}

		addr += l;
		written += l;
	}

}


static inline void writedl(uint32_t data) {
	DEBUG("index: %u, writedl: 0x%x 0x%x\n", (unsigned int) this->dl_index,
			(unsigned int) data >> 24, (unsigned int) data & ~(0xFFFF << 24));
	write32(MEMMAP_RAM_DL_BEGIN + this->dl_index, data);
	this->dl_index += 4;
}


static void write8(const ft81x_reg_e address, uint8_t value) {
	uint16_t wb[WRITE8_LEN] = {};
	wb[0] = FT81X_PREFIX_WRITE | ((address >> 16) & 0x3F);
	wb[1] = (address >> 8) & 0xFF;
	wb[2] = (address) & 0xFF;
	wb[3] = value;

	uv_spi_write_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL, wb, 8, WRITE8_LEN);
}



static void write16(const ft81x_reg_e address, uint16_t value) {
	uint16_t wb[WRITE16_LEN] = {};
	wb[0] = FT81X_PREFIX_WRITE | ((address >> 16) & 0x3F);
	wb[1] = (address >> 8) & 0xFF;
	wb[2] = (address) & 0xFF;
	wb[3] = value & 0xFF;
	wb[4] = (value >> 8) & 0xFF;

	uv_spi_write_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL, wb, 8, WRITE16_LEN);
}



static void write32(const ft81x_reg_e address, uint32_t value) {
	uint16_t wb[WRITE32_LEN] = {};
	wb[0] = FT81X_PREFIX_WRITE | ((address >> 16) & 0x3F);
	wb[1] = (address >> 8) & 0xFF;
	wb[2] = (address) & 0xFF;
	wb[3] = value & 0xFF;
	wb[4] = (value >> 8) & 0xFF;
	wb[5] = (value >> 16) & 0xFF;
	wb[6] = (value >> 24) & 0xFF;

	uv_spi_write_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL, wb, 8, WRITE32_LEN);
}



static void writehostcmd(const ft81x_hostcmds_e hostcmd, uint8_t parameter) {
	uint16_t wb[3] = {};
	wb[0] = hostcmd;
	wb[1] = parameter;
	wb[2] = 0;
	uv_spi_write_sync(CONFIG_FT81X_SPI_CHANNEL, CONFIG_FT81X_SSEL, wb, 8, 3);
}



static void set_color(color_t c) {
	if (this->color_mode == COLOR_MODE_GRAYSCALE) {
		c = uv_uic_grayscale(c);
	}
	if ((!this->color_init) ||
			(((color_st*) &c)->a != this->color.a)) {
		DEBUG("set alpha\n");
		writedl(COLOR_A(((color_st*) &c)->a));
		this->color.a = ((color_st*) &c)->a;
	}
	if ((!this->color_init) ||
			(*((color_t*) &this->color) != c)) {
		DEBUG("set color\n");
		writedl(COLOR_RGB(((color_st*) &c)->r, ((color_st*) &c)->g, ((color_st*) &c)->b));
		*((color_t*) &this->color) = c;
		this->color_init = true;
	}
}



static void set_begin(uint8_t begin_type) {
	// set the begin type to whatever is active right now
	if ((begin_type <= BEGIN_RECTS) && (begin_type != this->begin_type)) {
		DEBUG("set begin\n");
		writedl(BEGIN(begin_type));
		this->begin_type = begin_type;
	}
}



static void set_point_size(uint16_t diameter) {
	// maximum point size is 1023
	if (diameter > ((0x1FFF * 2) / 16)) {
		diameter = 0x1FFF * 2 / 16;
	}
	if (this->point_diameter != diameter) {
		DEBUG("set point diameter\n");
		writedl(POINT_SIZE(diameter * 16 / 2));
		this->point_diameter = diameter;
	}
}


static void set_line_diameter(const uint16_t diameter) {
	if (this->line_width != diameter) {
		DEBUG("set line diameter\n");
		if (diameter == 0) {
			writedl(LINE_WIDTH(16));
		}
		else {
			writedl(LINE_WIDTH(diameter * 16));
		}
		this->line_width = diameter;
	}
}

static void set_font(const uint8_t font) {
	if (this->font != font) {
		DEBUG("Setting fobitmap handle (font)\n");
		writedl(BITMAP_HANDLE(font));
	}
}


static void set_cell(const uint8_t cell) {
	if (this->cell != cell) {
		DEBUG("Setting cell\n");
		writedl(CELL(cell));
	}
}

/// @brief: Waits until co-processor has processed all transactions
static bool cmd_wait(void) {
	bool ret = true;
	// co-processor might modify the begin type, thus set it to undefined
	this->begin_type = 0xFF;
	uint16_t cmdread = read16(REG_CMD_READ);
	uint16_t cmdwrite = read16(REG_CMD_WRITE);

	while (cmdread != cmdwrite) {
		if (cmdread == 0xFFF) {
			// fault. Recover from it and return error code
			// printf("co-processor fault!!\n");
			write8(REG_CPURESET, 1);
			write16(REG_CMD_READ, this->cmdwriteaddr);
			write16(REG_CMD_WRITE, this->cmdwriteaddr);
			write8(REG_CPURESET, 0);
			ret = false;
			break;
		}
		cmdread = read16(REG_CMD_READ);
		cmdwrite = read16(REG_CMD_WRITE);
		uv_rtos_task_yield();
	}
	return ret;
}

static void cmd_romfont(uint8_t bitmap_handle, uint8_t font_number) {
	// set the RAMDL offset where co-processor writes the DL entries
	write16(REG_CMD_DL, this->dl_index);

	DEBUG("Loading ROM font %u to bitmap handle %u\n",
			font_number, bitmap_handle);
	// create rom font structure, based on communication manual data
	uint32_t cmd[3];
	cmd[0] = CMD_ROMFONT;
	cmd[1] = bitmap_handle;
	cmd[2] = font_number;
	writestr(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr,
			(const char*) cmd, NULL, sizeof(cmd));

	// increase cmdwriteaddr by the length of this command
	this->cmdwriteaddr = (this->cmdwriteaddr + sizeof(cmd)) % RAMCMD_SIZE;

	write16(REG_CMD_WRITE, this->cmdwriteaddr);

	// last thing is to wait for the co-processor to finish
	// and update current dl_index
	cmd_wait();
	this->dl_index = read16(REG_CMD_DL);
}


static void draw_line(char *str, ft81x_font_st *font,
		int16_t x, int16_t y, ft81x_align_e align, color_t color, uint16_t len) {
	set_color(color);

	// set the RAMDL offset where co-processor writes the DL entries
	write16(REG_CMD_DL, this->dl_index);

	// write header information as a bulk transfer
	// FT81X automatically wraps continuous writes to RAM_CMD ring buffer space
	uint16_t buf[DRAW_LINE_BUF_LEN];
	*((uint32_t*) &buf[0]) = CMD_TEXT;
	buf[2] = x;
	buf[3] = y;
	buf[4] = font->handle;
	buf[5] = align;
	writestr(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, (const char*) buf, NULL, DRAW_LINE_BUF_LEN * 2);
	this->cmdwriteaddr = (this->cmdwriteaddr + DRAW_LINE_BUF_LEN * 2) % RAMCMD_SIZE;

	DEBUG("Writing '%s'\n", str);
	// write the whole string and termination character
	writestr(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, str, NULL, len);
	this->cmdwriteaddr = (this->cmdwriteaddr + len) % RAMCMD_SIZE;

	// write null termination marks until cmdwriteaddr is in world boundary
	uint8_t nul = 4 - (this->cmdwriteaddr % 4);
	write32(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, 0);
	this->cmdwriteaddr = (this->cmdwriteaddr + nul) % RAMCMD_SIZE;

	write16(REG_CMD_WRITE, this->cmdwriteaddr);

	// last thing is to wait for the co-processor to finish
	// and update current dl_index
	cmd_wait();
	this->dl_index = read16(REG_CMD_DL);

}


bool visible(const int16_t x, const int16_t y,
		const int16_t width, const int16_t height) {
	bool ret = true;
	if (((x + width) < this->mask.x) ||
			(x > (this->mask.x + this->mask.width)) ||
			((y + height) < this->mask.y) ||
			(y > (this->mask.y + this->mask.height))) {
		return false;
	}

	return ret;
}




/* PUBLIC FUNCTIONS */



void uv_ft81x_dlswap(void) {
	writedl(DISPLAY());
	write8(REG_DLSWAP, 0x2);
	DEBUG("ramdl index: 0x%x\n", (unsigned int) this->dl_index);
	if (this->dl_index_max < this->dl_index) {
		this->dl_index_max = this->dl_index;
	}
	this->dl_index = 0;
	init_values();
	DEBUG("\n\ndlswap\n\n");
	uint8_t dl;
	while (true) {
		// wait until the dlswap register reads as 0.
		// It is safe to write to RAMDL after it.
		dl = read8(REG_DLSWAP);
		if (dl == 0) {
			break;
		}
		uv_rtos_task_yield();
	}
	// set the vertex format to pixel precision
	writedl(VERTEX_FORMAT(0));
}



void uv_ft81x_set_backlight(uint8_t percent) {
	if (percent > 100) {
		percent = 100;
	}
	else if (percent < 4) {
		percent = 4;
	}
	write8(REG_PWM_DUTY,
#if CONFIG_FT81X_BACKLIGHT_INVERT
			128 - (uint32_t) percent * 128 / 100);
#else
			(uint32_t) (percent) * 128 / 100);
#endif
	this->backlight = percent;
}



uint8_t uv_ft81x_get_backlight(void) {
	return this->backlight;
}



void uv_ft81x_clear(color_t c) {
	if ((!this->clear_color_init) ||
			(((color_st*) &c)->a != this->clear_color.a)) {
		DEBUG("set clear alpha\n");
		writedl(CLEAR_COLOR_A(((color_st*) &c)->a));
		this->clear_color.a = ((color_st*) &c)->a;
	}
	if ((!this->clear_color_init) ||
			(*((color_t*) &this->clear_color) != c)) {
		DEBUG("set clear color\n");
		writedl(CLEAR_COLOR_RGB(((color_st*) &c)->r, ((color_st*) &c)->g, ((color_st*) &c)->b));
		*((color_t*) &this->clear_color) = c;
		this->clear_color_init = true;
	}
	DEBUG("clear\n");
	writedl(CLEAR(1, 1, 1));
}



uint32_t uv_ft81x_get_ramdl_usage(void) {
	return this->dl_index_max;
}




#define LOAD_BUFFER_LEN		60
uint32_t uv_ft81x_loadbitmapexmem(uv_uimedia_st *bitmap,
		uint32_t dest_addr, uv_w25q128_st *exmem, char *filename) {
	uint8_t cmd_header_len_bytes = 12;
	uint32_t buffer[LOAD_BUFFER_LEN / 4 + cmd_header_len_bytes / 4];
	uint32_t offset = 0;
	uv_fd_st fd;
	uint32_t size = 0;

	bool found = uv_exmem_find(exmem, filename, &fd);

	bitmap->addr = dest_addr;
	bitmap->type = UV_UIMEDIA_IMAGE;
	bitmap->filename = filename;

	// specify the bitmap format
	if (strstr(bitmap->filename, ".jp" ) != NULL ||
			strstr(bitmap->filename, ".JP") != NULL) {
		// color jpgs are in RGB565
		bitmap->format = BITMAP_FORMAT_RGB565;
	}
	else {
		// PNG image, check the color type from png format
		// This hard-coded thingie is PNG format dependent
		uv_exmem_read_fd(exmem, &fd, (void*) buffer, 26, 0);
		uint8_t bitdepth = ((uint8_t*) buffer)[24];
		uint8_t ctype = ((uint8_t*) buffer)[25];

		if (bitdepth != 8) {
			uv_terminal_enable();
			printf("Error parsing PNG bitmap %s: Bit depth has to be 8, but was %u\n",
					bitmap->filename, bitdepth);
		}
		if (ctype & 0x1) {
			// paletted image
			bitmap->format = BITMAP_FORMAT_PALETTED4444;
		}
		else if (ctype > 4) {
			// true color with alpha
			bitmap->format = BITMAP_FORMAT_ARGB4;
		}
		else {
			// true color without alpha
			bitmap->format = BITMAP_FORMAT_RGB565;
		}
	}


	// CONFIG_FT81X_MEDIA_MAXSIZE specifies the maximum media size available
	if (found && fd.file_size <= CONFIG_FT81X_MEDIA_MAXSIZE) {
		// set the RAMDL offset where co-processor writes the DL entries
		write16(REG_CMD_DL, this->dl_index);

		// setup a mediafifo
		buffer[0] = CMD_MEDIAFIFO;
		buffer[1] = FT81X_MEDIAFIFO_ADDR;
		buffer[2] = CONFIG_FT81X_MEDIA_MAXSIZE;
		writestr(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, (const char*) buffer, NULL, 12);
		this->cmdwriteaddr = (this->cmdwriteaddr + 12) % RAMCMD_SIZE;
		// start the co-processor
		write16(REG_CMD_WRITE, this->cmdwriteaddr);
		// wait for the co-processor to finish
		cmd_wait();
		write32(REG_MEDIAFIFO_WRITE, 0);
		write32(REG_MEDIAFIFO_READ, 0);

		// download the bitmap to mediafifo
		size = uv_exmem_read_fd(exmem, &fd,
				(void*) buffer, LOAD_BUFFER_LEN, offset);

		do {
			writestr(FT81X_MEDIAFIFO_ADDR + offset, (const char *) buffer, NULL, size);
			offset += size;
			size = uv_exmem_read_fd(exmem, &fd,
					buffer, LOAD_BUFFER_LEN, offset);
		}
		while (size != 0);

		// last, align memory to 4 bytes border
		if (offset % 4) {
			uint8_t stuff = 4 - (offset % 4);
			memset(buffer, 0, stuff);
			writestr(FT81X_MEDIAFIFO_ADDR + offset,
					(const char *) buffer, NULL, stuff);
			offset += stuff;
		}
		// update mediafifo write register
		write32(REG_MEDIAFIFO_WRITE, offset);

		// create bitmap structure, based on communication manual data
		buffer[0] = CMD_LOADIMAGE;
		buffer[1] = bitmap->addr;
		buffer[2] = OPT_NODL | OPT_MEDIAFIFO;
		uint32_t len = cmd_header_len_bytes;
		writestr(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, (const char*) buffer, NULL, len);
		this->cmdwriteaddr = (this->cmdwriteaddr + len) % RAMCMD_SIZE;

		// start the co-processor
		write16(REG_CMD_WRITE, this->cmdwriteaddr);
		// wait for the co-processor to finish
		// and update current dl_index
		bool nofaults = cmd_wait();

		// wait until mediafifo is empty
		while (true) {
			uint16_t read = read16(REG_MEDIAFIFO_READ);
			uint16_t write = read16(REG_MEDIAFIFO_WRITE);
			nofaults = cmd_wait();
			if (read == write || !nofaults) {
				break;
			}
			uv_rtos_task_yield();
		}

		if (nofaults) {

			// lastly check the size of the image and return that
			cmd_header_len_bytes = 16;
			memset((void*) buffer, 0, cmd_header_len_bytes);
			buffer[0] = CMD_GETPROPS;
			writestr(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, (const char*) buffer,
					NULL, cmd_header_len_bytes);
			uint32_t x = this->cmdwriteaddr;
			// increase cmdwriteaddr by the length of this command
			this->cmdwriteaddr = (this->cmdwriteaddr + cmd_header_len_bytes) % RAMCMD_SIZE;

			// execute the command
			write16(REG_CMD_WRITE, this->cmdwriteaddr);
			// wait for the co-processor to finish
			cmd_wait();
			// now read the result from the previous cmdwriteaddr
			uint32_t ptr = read32(MEMMAP_RAM_CMD_BEGIN + ((x + 4) % RAMCMD_SIZE));
			bitmap->size = ptr - bitmap->addr;
			// size should be 4-byte aligned
			if (bitmap->size % 4) {
				bitmap->size += 4 - (bitmap->size % 4);
			}
			bitmap->width = read32(MEMMAP_RAM_CMD_BEGIN + ((x + 8) % RAMCMD_SIZE));
			bitmap->height = read32(MEMMAP_RAM_CMD_BEGIN + ((x + 12) % RAMCMD_SIZE));
			// for paletted images calculate the palette size
			if (bitmap->format >= BITMAP_FORMAT_PALETTED565) {
				bitmap->palette_size = (ptr - bitmap->addr) - (bitmap->width * bitmap->height);
			}
			else {
				bitmap->palette_size = 0;
			}

			// if any dimensions are null, something has happened and image loading failed
			if (!bitmap->width || !bitmap->height) {
				size = 0;
			}
			else {
				size = bitmap->size;
			}
		}
		else {
			size = 0;
		}
	}
	else {
		size = 0;
	}

	if (size == 0) {
		// error occurred, specify the settings for bitmap
		// data which wont cause anything unexpected
		bitmap->width = 40;
		bitmap->height = 40;
		bitmap->size = bitmap->width * bitmap->height * 2;
		if (bitmap->size % 4) {
			bitmap->size += 4 - (bitmap->size % 4);
		}
		bitmap->format = BITMAP_FORMAT_RGB565;
		bitmap->palette_size = 0;
	}

	return size;
}

void uv_ft81x_draw_bitmap_ext(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {

	if (visible(x, y, bitmap->width, bitmap->height)) {
		// set the blend color
		set_color(c);

		// ARGB4 and RGB565 images take 2 bytes per pixel, paletted images take 1 byte per pixel
		uint32_t linestride = (bitmap->format >= BITMAP_FORMAT_PALETTED565) ?
				bitmap->width : bitmap->width * 2;
		uint32_t source = bitmap->addr;
		if (bitmap->format == BITMAP_FORMAT_PALETTED4444 ||
				bitmap->format == BITMAP_FORMAT_PALETTED565 ||
				bitmap->format == BITMAP_FORMAT_PALETTED8) {
			writedl(PALETTE_SOURCE(bitmap->addr));
			source = bitmap->addr + bitmap->palette_size;
		}
		writedl(BITMAP_SOURCE(source));
		writedl(BITMAP_LAYOUT(bitmap->format, linestride, bitmap->height));
		if ((linestride >= 1024) ||
				(bitmap->height >= 512)) {
			writedl(BITMAP_LAYOUT_H(linestride / 1024, bitmap->height / 512));
		}
		writedl(BITMAP_SIZE(BITMAP_SIZE_FILTER_NEAREST, wrap,
				wrap, w, h));
		if ((w >= 512) || (h >= 512)) {
			writedl(BITMAP_SIZE_H(w / 512, h / 512));
		}
		writedl(BEGIN(BEGIN_BITMAPS));
		vertex2f_st v;
		v.sx = x;
		v.sy = y;
		writedl(VERTEX2F(v.ux, v.uy));
	}
}




void uv_ft81x_draw_point(int16_t x, int16_t y, color_t color, uint16_t diameter) {
	if (visible(x - diameter / 2, y - diameter / 2, diameter, diameter)) {
		set_color(color);
		set_begin(BEGIN_POINTS);
		set_point_size(diameter);
		DEBUG("drawing point\n");
		volatile vertex2f_st v;
		v.sx = x;
		v.sy = y;
		writedl(VERTEX2F(v.ux, v.uy));
	}
}


void uv_ft81x_draw_shadowpoint(int16_t x, int16_t y,
		color_t color, color_t highlight_c, color_t shadow_c, uint16_t diameter) {
	uv_ft81x_draw_point(x - 2, y - 2, shadow_c, diameter);
	uv_ft81x_draw_point(x + 2, y + 2, highlight_c, diameter);
	uv_ft81x_draw_point(x, y, color, diameter);
}


void uv_ft81x_draw_rrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color) {
	if (visible(x, y, width, height)) {
		set_color(color);
		set_begin(BEGIN_RECTS);
		set_line_diameter(radius);
		DEBUG("Drawing rectangle\n");
		volatile vertex2f_st v;
		v.sx = x + radius;
		v.sy = y + radius;
		writedl(VERTEX2F(v.ux, v.uy));
		v.sx = x + width - radius;
		v.sy = y + height - radius;
		if (v.sx <= x + radius) {
			v.sx = x + radius;
		}
		if (v.sy <= y + radius) {
			v.sy = y + radius;
		}
		writedl(VERTEX2F(v.ux, v.uy));
	}
}

void uv_ft81x_draw_shadowrrect(const int16_t x, const int16_t y,
		const uint16_t width, const uint16_t height,
		const uint16_t radius, const color_t color,
		const color_t highlight_c, const color_t shadow_c) {
	uv_ft81x_draw_rrect(x, y, width - 4, height - 4, radius, shadow_c);
	uv_ft81x_draw_rrect(x + 4, y + 4, width - 4, height - 4, radius, highlight_c);
	uv_ft81x_draw_rrect(x + 2, y + 2, width - 4, height - 4, radius, color);
}


void uv_ft81x_draw_line(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
	if (visible(start_x, start_y, end_x, end_y)) {
		set_color(color);
		set_begin(BEGIN_LINES);
		set_line_diameter(width);
		DEBUG("Drawing line\n");
		vertex2f_st v;
		v.sx = start_x;
		v.sy = start_y;
		writedl(VERTEX2F(v.ux, v.uy));
		v.sx = end_x;
		v.sy = end_y;
		writedl(VERTEX2F(v.ux, v.uy));
	}
}




void uv_ft81x_touchscreen_calibrate(ft81x_transfmat_st *transform_matrix) {
	DEBUG("Starting the screen calibration\n");

	uv_ft81x_dlswap();
	uv_ft81x_set_mask(0, 0, LCD_W_PX, LCD_H_PX);
	uv_ft81x_clear(C(0xFF002040));
	uv_ft81x_draw_string("Calibrate the touchscreen\nby touching the flashing points", &font28,
			LCD_W(0.5f), LCD_H(0.1f), FT81X_ALIGN_CENTER_TOP, C(0xFFFFFFFF));
	write16(REG_CMD_DL, this->dl_index);

	while (true) {
		// wait until the screen is not pressed
		while (uv_ft81x_get_touch(NULL, NULL));

		write32(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr, CMD_CALIBRATE);
		this->cmdwriteaddr += 8;
		if (this->cmdwriteaddr >= 4096) {
			this->cmdwriteaddr %= 4096;
		}
		write16(REG_CMD_WRITE, this->cmdwriteaddr);

		uint16_t cmdread = read16(REG_CMD_READ);
		while (cmdread != this->cmdwriteaddr) {
			uv_rtos_task_delay(1);
			cmdread = read16(REG_CMD_READ);
		}
		uint32_t result = read32(MEMMAP_RAM_CMD_BEGIN + this->cmdwriteaddr - 4);
		if (result != 0) {
			break;
		}
		else {
			DEBUG("Calibration failed. Retrying...\n");
		}
	}
	this->dl_index = read16(REG_CMD_DL);
	DEBUG("screen calibration done\n");
	uv_ft81x_dlswap();
	uv_ft81x_clear(CONFIG_FT81X_SCREEN_COLOR);
	uv_ft81x_dlswap();

	if (transform_matrix) {
		transform_matrix->mat[0] = read32(REG_TOUCH_TRANSFORM_A);
		transform_matrix->mat[1] = read32(REG_TOUCH_TRANSFORM_B);
		transform_matrix->mat[2] = read32(REG_TOUCH_TRANSFORM_C);
		transform_matrix->mat[3] = read32(REG_TOUCH_TRANSFORM_D);
		transform_matrix->mat[4] = read32(REG_TOUCH_TRANSFORM_E);
		transform_matrix->mat[5] = read32(REG_TOUCH_TRANSFORM_F);
	}
}



void uv_ft81x_touchscreen_set_transform_matrix(ft81x_transfmat_st *transform_matrix) {
	write32(REG_TOUCH_TRANSFORM_A, transform_matrix->mat[0]);
	write32(REG_TOUCH_TRANSFORM_B, transform_matrix->mat[1]);
	write32(REG_TOUCH_TRANSFORM_C, transform_matrix->mat[2]);
	write32(REG_TOUCH_TRANSFORM_D, transform_matrix->mat[3]);
	write32(REG_TOUCH_TRANSFORM_E, transform_matrix->mat[4]);
	write32(REG_TOUCH_TRANSFORM_F, transform_matrix->mat[5]);
}



bool uv_ft81x_get_touch(int16_t *x, int16_t *y) {
	uint32_t t = read32(REG_TOUCH_SCREEN_XY);
	int16_t tx = t >> 16;
	int16_t ty = t & 0xFFFF;
	bool ret = (t == 0x80008000) ? false : true;
	if (ret) {
		if (x) {
			*x = tx;
		}
		if (y) {
			*y = ty;
		}
	}
	return ret;
}


void uv_ft81x_draw_char(const char c, const uint16_t font,
		int16_t x, int16_t y, color_t color) {
	set_color(color);
	set_begin(BEGIN_BITMAPS);
	if ((x > 0) && (y > 0)) {
		DEBUG("Drawing char\n");
		writedl(VERTEX2II(x, y, font, c));
	}
	else {
		set_font(font);
		set_cell(c);
		vertex2f_st v;
		v.sx = x;
		v.sy = y;
		DEBUG("Drawing char\n");
		writedl(VERTEX2F(v.ux, v.uy));
	}

}


int16_t uv_ft81x_get_string_height(char *str, ft81x_font_st *font) {
	int16_t ret = 0;
	uint16_t line_count = (*str == '\0') ? 0 : 1;
	while (*str != '\0') {
		if (*str++ == '\n') {
			line_count++;
		}
	}
	ret = uv_ft81x_get_font_height(font) * line_count;

	return ret;
}



int16_t uv_ft81x_get_string_width(char *str, ft81x_font_st *font) {
	int16_t ret = 0;
	int16_t line_len = 0;

	for (uint32_t i = 0; i < strlen(str); i++) {
		// clear the calculated width on every new line character
		if (str[i] == '\n') {
			line_len = 0;
		}
		else {
			// fetch the width of this character from FT81X
			int16_t w = read8(FONT_METRICS_BASE_ADDR +
					(font->index - 16) * FONT_METRICS_FONT_LEN +
					FONT_METRICS_CHAR_WIDTH_OFFSET + str[i]);
			line_len += w;
		}
		if (line_len > ret) {
			ret = line_len;
		}
	}
	return ret;
}



void uv_ft81x_draw_string(char *str, ft81x_font_st *font,
		int16_t x, int16_t y, ft81x_align_e align, color_t color) {
	char *str_ptr = str;
	int16_t len = 0;

	// find out the line count to adjust start y
	if ((align == FT81X_ALIGN_CENTER) ||
			(align == FT81X_ALIGN_LEFT_CENTER) ||
			(align == FT81X_ALIGN_RIGHT_CENTER)) {
		uint16_t line_count = 0;
		while (*str_ptr != '\0') {
			if (*str_ptr++ == '\n') {
				line_count++;
			}
		}
		y -= uv_ft81x_get_font_height(font) * line_count / 2;
		str_ptr = str;
	}

	while (true) {
		if (*str_ptr == '\0') {
			draw_line(str, font, x, y, align, color, len);
			break;
		}
		else if (*str_ptr == '\n') {
			draw_line(str, font, x, y, align, color, len);
			y += uv_ft81x_get_font_height(font);
			str = str_ptr + 1;
			len = -1;
		}
		else {

		}

		str_ptr++;
		len++;
	}

}





void uv_ft81x_set_mask(int16_t x, int16_t y, uint16_t width, uint16_t height) {
	if (x < 0) {
		width += x;
		x = 0;
	}
	else if (x > 2047) {
		x = 2047;
	}
	if (y < 0) {
		height += y;
		y = 0;
	}
	else if (y > 2047) {
		y = 2047;
	}
	if ((x != this->mask.x) || (y != this->mask.y)) {
		DEBUG("Setting scissor x: %u, y: %u\n", x, y);
		writedl(SCISSORS_XY(x , y));
		this->mask.x = x;
		this->mask.y = y;
	}
	if (width > 2048) {
		width = 2048;
	}
	if (height > 2048) {
		height = 2048;
	}
	if ((width != this->mask.width) || (height != this->mask.height)) {
		DEBUG("Setting schissor width: %u, height: %u\n", width, height);
		writedl(SCISSOR_SIZE(width, height));
		this->mask.width = width;
		this->mask.height = height;
	}
}



color_t uv_uic_brighten(color_t c, int8_t value) {
	color_t ret = (c & 0xFF000000);
	for (uint8_t i = 0; i < 3; i++) {
		int32_t col = (c >> (i * 8)) & 0xFF;
		col += value;
		if (col < 0) {
			col = 0;
		}
		else if (col > 0xFF) {
			col = 0xFF;
		}
		else {

		}
		ret += (col << (i * 8));
	}
	return ret;
}


color_t uv_uic_grayscale(color_t c) {
	color_st *cst = (void*) &c;
	uint32_t shade = ((uint32_t) cst->r + cst->g + cst->b) / 3;
	if (shade > UINT8_MAX) {
		shade = UINT8_MAX;
	}
	if (this->grayscale_luminosity) {
		int32_t reli = uv_reli(this->grayscale_luminosity, INT8_MIN + 1, INT8_MAX);
		LIMITS(reli, 0, 1000);
		shade = shade * reli / 500;
	}
	cst->r = shade;
	cst->g = shade;
	cst->b = shade;
	memcpy(&c, cst, sizeof(c));

	return c;
}


color_t uv_uic_lerpi(int32_t t, color_t ca, color_t cb) {
	uint32_t ret = 0;
	for (uint8_t i = 0; i < 4; i++) {
		int16_t val, start_val, end_val;
		start_val = (ca >> (i * 8)) & 0xFF;
		end_val = (cb >> (i * 8)) & 0xFF;

		val = uv_lerpi(t, start_val, end_val);
		ret += ((val & 0xFF) << (i * 8));
	}
	return (color_t) ret;
}



void uv_ft81x_set_color_mode(ft81x_color_modes_e value) {
	this->color_mode = value;
}


void uv_ft81x_set_grayscale_luminosity(int8_t value) {
	this->grayscale_luminosity = value;
}




#endif
