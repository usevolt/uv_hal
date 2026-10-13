/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2026 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2026 usevolt
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


/*
 * The uv_ui backend of a web browser build (Emscripten): draws on an HTML
 * canvas through its 2D context.
 *
 * The page provides the canvas, as the element with the id "uvui", and the two
 * fonts the desktop backend loads from fonts/, as the CSS font families
 * "UvSans" and "UvMono". It should not start the program before those fonts
 * have loaded: the font metrics are read once, in uv_ui_init().
 *
 * The UI is laid out on a fixed CONFIG_FT81X_HSIZE x CONFIG_FT81X_VSIZE
 * display, the same as on the desktop. It is scaled to fit the canvas, which
 * follows the size of the page, with the aspect ratio kept and the rest of the
 * canvas filled with the display colour. The backing store is in physical
 * pixels, so text and shapes are as sharp as the screen allows.
 *
 * Drawing goes straight to the visible canvas: what is on it when the program
 * hands the thread back to the browser is what the page shows. So a frame is
 * left in place by uv_ui_dlswap_impl(), and the next one starts - clearing the
 * canvas - with its first drawing call. A frame is drawn without handing the
 * thread back in between, so a half drawn one is never seen.
 */


#include "uv_ui_common.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#include "ui/uv_uifont.h"
#include "uv_ui.h"
#include "lodepng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if CONFIG_UI && CONFIG_UI_CANVAS

#include <emscripten.h>


ui_font_st ui_fonts[UI_MAX_FONT_COUNT];
ui_font_st ui_mono_fonts[UI_MAX_FONT_COUNT];
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


/// @brief: A decoded bitmap. The pixels live in the page, as an offscreen
/// canvas; this keeps the name it is found by and the page's id for it.
typedef struct uimedia_ll_st {
	char filename[128];
	int32_t js_id;
	uint32_t width;
	uint32_t height;
	struct uimedia_ll_st *next_ptr;
} uimedia_ll_st;


typedef struct {
	bool inited;
	// the last frame is done: the next drawing call starts a new one
	bool frame_done;
	uint8_t brightness;
	uimedia_ll_st *uimediall;
	char clipboard[1024];
} ui_st;

static ui_st _ui = {
		.inited = false,
		.brightness = 100,
		.uimediall = NULL
};

#ifdef this
#undef this
#endif
#define this (&_ui)


// ---- the page side ----------------------------------------------------------
//
// Everything that touches the canvas or the page's events. The state is kept in
// Module.uvui; the event handlers only record what happened, and the program
// collects it when uv_ui asks for it.

EM_JS(void, js_init, (int w, int h, int bg), {
	const canvas = document.getElementById('uvui');
	const ctx = canvas.getContext('2d');
	const ui = {
		canvas: canvas,
		ctx: ctx,
		w: w,
		h: h,
		bg: bg,
		scale: 1,
		ox: 0,
		oy: 0,
		dpr: 1,
		pressed: false,
		x: 0,
		y: 0,
		// whether uv_ui has seen the press, and how many more polls a
		// cancelled one is reported dragged away for (see js_get_touch)
		seen: false,
		cancel: 0,
		// the text fields of the frame on screen and of the one being drawn,
		// as [x, y, w, h] in UI pixels (see uv_ui_web_text_field())
		fields: [],
		fieldsNext: [],
		kbd: null,
		scroll: 0,
		keys: [],
		refresh: true,
		clipboard: "",
		images: [],
		tinted: new Map(),
		fonts: ['UvSans', 'UvMono']
	};
	Module.uvui = ui;

	function layout() {
		ui.dpr = window.devicePixelRatio || 1;
		const cw = canvas.clientWidth;
		const ch = canvas.clientHeight;
		canvas.width = Math.max(1, Math.round(cw * ui.dpr));
		canvas.height = Math.max(1, Math.round(ch * ui.dpr));
		ui.scale = Math.min(cw / ui.w, ch / ui.h);
		ui.ox = (cw - ui.w * ui.scale) / 2;
		ui.oy = (ch - ui.h * ui.scale) / 2;
		ui.refresh = true;
	}
	ui.layout = layout;
	layout();
	new ResizeObserver(layout).observe(canvas);
	window.addEventListener('resize', layout);

	function pos(e) {
		const r = canvas.getBoundingClientRect();
		ui.x = Math.round((e.clientX - r.left - ui.ox) / ui.scale);
		ui.y = Math.round((e.clientY - r.top - ui.oy) / ui.scale);
	}
	// A press which turns out to be the start of a pinch. uv_ui clicks where
	// the press started, whatever happens after, so simply letting go would
	// click whatever the first finger landed on. Instead the press is dragged
	// half the screen over for a couple of polls first, which uv_ui takes as a
	// drag that ended: no click.
	function cancel() {
		if (ui.pressed && ui.seen) {
			ui.cancel = 2;
		}
		else {
			ui.pressed = false;
		}
	}

	// A phone shows its keyboard only for a real text input which has the
	// focus, and only when it is given the focus during a touch. This one sits
	// over whichever text field was touched, out of sight, and what is typed
	// into it is passed on as key presses. Its value always starts with
	// KBD_SENT, so that a backspace has something to delete even when nothing
	// has been typed.
	const KBD_SENT = ' ';
	const kbd = document.createElement('input');
	kbd.type = 'text';
	kbd.autocomplete = 'off';
	kbd.setAttribute('autocorrect', 'off');
	kbd.setAttribute('autocapitalize', 'off');
	kbd.spellcheck = false;
	kbd.enterKeyHint = 'send';
	// 16px or more, or iOS zooms the page in on focus
	kbd.style.cssText = 'position:fixed;opacity:0;pointer-events:none;' +
		'border:0;padding:0;font-size:16px;caret-color:transparent;' +
		'left:0;top:0;width:1px;height:1px;';
	kbd.value = KBD_SENT;
	document.body.appendChild(kbd);
	ui.kbd = kbd;
	let kbdPrev = KBD_SENT;
	function kbdReset() {
		kbd.value = KBD_SENT;
		kbdPrev = KBD_SENT;
	}
	// What was typed is worked out from how the value changed, rather than
	// from key events: the keyboards of phones compose words and rewrite them,
	// and send no usable key codes while at it.
	kbd.addEventListener('input', () => {
		const v = kbd.value;
		let p = 0;
		while ((p < v.length) && (p < kbdPrev.length) && (v[p] === kbdPrev[p])) {
			p++;
		}
		for (let i = p; i < kbdPrev.length; i++) {
			ui.keys.push(8);
		}
		for (const ch of v.slice(p)) {
			const c = ch.charCodeAt(0);
			if (ch === '\n') {
				ui.keys.push(10);
			}
			else if ((c >= 0x20) && (c < 0x7F)) {
				ui.keys.push(c);
			}
		}
		if (!v.startsWith(KBD_SENT)) {
			kbdReset();
		}
		else {
			kbdPrev = v;
		}
	});
	kbd.addEventListener('keydown', (e) => {
		let c = -1;
		if (e.key === 'Enter') {
			c = 10;
			kbdReset();
		}
		else if ((e.key in special) && (e.key !== 'Backspace') && (e.key !== 'Delete')) {
			c = special[e.key];
		}
		if (c >= 0) {
			ui.keys.push(c);
			e.preventDefault();
		}
	});
	// the text field at UI pixel (x, y), or null
	function fieldAt(x, y) {
		return ui.fields.find((f) => (x >= f[0]) && (x < f[0] + f[2]) &&
				(y >= f[1]) && (y < f[1] + f[3])) || null;
	}
	function kbdShow(f) {
		const r = canvas.getBoundingClientRect();
		kbd.style.left = (r.left + ui.ox + f[0] * ui.scale) + 'px';
		kbd.style.top = (r.top + ui.oy + f[1] * ui.scale) + 'px';
		kbd.style.width = Math.max(1, f[2] * ui.scale) + 'px';
		kbd.style.height = Math.max(1, f[3] * ui.scale) + 'px';
		if (document.activeElement !== kbd) {
			kbdReset();
			kbd.focus();
		}
	}
	ui.kbdHide = () => {
		if (document.activeElement === kbd) {
			kbd.blur();
		}
	};

	canvas.addEventListener('pointerdown', (e) => {
		if (ui.pressed) {
			// a second finger: a pinch is starting, if the browser has not
			// already said so
			cancel();
			return;
		}
		pos(e);
		if (e.pointerType === 'mouse') {
			canvas.focus();
		}
		else {
			// a touch on a text field brings up the keyboard, and one
			// anywhere else puts it away
			const f = fieldAt(ui.x, ui.y);
			if (f !== null) {
				kbdShow(f);
			}
			else {
				ui.kbdHide();
			}
		}
		canvas.setPointerCapture(e.pointerId);
		ui.pressed = true;
		ui.seen = false;
		ui.cancel = 0;
		e.preventDefault();
	});
	canvas.addEventListener('pointermove', (e) => {
		if (ui.cancel === 0) {
			pos(e);
		}
	});
	canvas.addEventListener('pointerup', (e) => {
		if (ui.cancel === 0) {
			pos(e);
			ui.pressed = false;
		}
	});
	// the browser took the touch over, for a pinch
	canvas.addEventListener('pointercancel', cancel);
	canvas.addEventListener('wheel', (e) => {
		// one notch per event, positive away from the user like GLFW's
		ui.scroll += (e.deltaY < 0) ? 1 : ((e.deltaY > 0) ? -1 : 0);
		e.preventDefault();
	}, { passive: false });
	canvas.addEventListener('contextmenu', (e) => e.preventDefault());

	// The key codes are the ones the desktop backend sends (uv_ui_common.h)
	const special = {
		Backspace: 8, Enter: 10, Escape: 27, Delete: 127, Tab: 9,
		ArrowLeft: 0x11, ArrowRight: 0x12, Home: 0x01, End: 0x05
	};
	canvas.addEventListener('keydown', (e) => {
		let c = -1;
		if (e.key === 'Tab' && e.shiftKey) {
			c = 0x0f;
		}
		else if (e.key in special) {
			c = special[e.key];
		}
		else if ((e.ctrlKey || e.metaKey) && !e.altKey) {
			// ctrl+V arrives as a paste event, which carries the text; the
			// other shortcuts are the browser's
			return;
		}
		else if (e.key.length === 1 && e.key.charCodeAt(0) < 0x80) {
			c = e.key.charCodeAt(0);
		}
		if (c >= 0) {
			ui.keys.push(c);
			e.preventDefault();
		}
	});
	canvas.addEventListener('paste', (e) => {
		ui.clipboard = (e.clipboardData || window.clipboardData).getData('text') || "";
		ui.keys.push(0x16);
		e.preventDefault();
	});
	// the keyboard goes where the canvas is
	canvas.tabIndex = 0;
	canvas.focus();
});

EM_JS(int, js_font_height, (int mono, int px), {
	const ctx = Module.uvui.ctx;
	ctx.font = px + 'px ' + Module.uvui.fonts[mono];
	const m = ctx.measureText('Hgjy');
	let h = m.fontBoundingBoxAscent + m.fontBoundingBoxDescent;
	if (!(h > 0)) {
		h = px * 1.15;
	}
	return Math.round(h);
});

EM_JS(bool, js_take_refresh, (void), {
	const r = Module.uvui.refresh;
	Module.uvui.refresh = false;
	return r;
});

EM_JS(int, js_get_touch, (void), {
	const ui = Module.uvui;
	let x = ui.x;
	if (ui.cancel > 0) {
		// a cancelled press, dragged half the screen over (see cancel() in
		// js_init)
		x += (x < ui.w / 2) ? (ui.w >> 1) : -(ui.w >> 1);
		ui.cancel--;
		if (ui.cancel === 0) {
			ui.pressed = false;
		}
		return (1 << 31) | ((x & 0x7FFF) << 15) | (ui.y & 0x7FFF);
	}
	ui.seen = ui.pressed;
	// x and y in 15 bits each, pressed in bit 31
	return ((ui.pressed ? 1 : 0) << 31) | ((x & 0x7FFF) << 15) | (ui.y & 0x7FFF);
});

EM_JS(int, js_take_scroll, (void), {
	const s = Module.uvui.scroll;
	Module.uvui.scroll = 0;
	return s;
});

EM_JS(int, js_peek_key, (void), {
	return (Module.uvui.keys.length > 0) ? Module.uvui.keys[0] : 0;
});

EM_JS(int, js_pop_key, (void), {
	return (Module.uvui.keys.length > 0) ? Module.uvui.keys.shift() : 0;
});

EM_JS(void, js_get_clipboard, (char *dest, int len), {
	stringToUTF8(Module.uvui.clipboard, dest, len);
});

EM_JS(void, js_begin_frame, (void), {
	const ui = Module.uvui;
	const ctx = ui.ctx;
	ctx.restore();
	ctx.setTransform(1, 0, 0, 1, 0, 0);
	ctx.fillStyle = ui.bg;
	ctx.fillRect(0, 0, ui.canvas.width, ui.canvas.height);
	ctx.setTransform(ui.scale * ui.dpr, 0, 0, ui.scale * ui.dpr,
			ui.ox * ui.dpr, ui.oy * ui.dpr);
	ctx.save();
	ui.fieldsNext = [];
});

EM_JS(void, js_end_frame, (void), {
	const ui = Module.uvui;
	ui.fields = ui.fieldsNext;
	if (ui.fields.length === 0) {
		// the field typed into is gone, and the keyboard with it
		ui.kbdHide();
	}
});

EM_JS(void, js_text_field, (int x, int y, int w, int h), {
	Module.uvui.fieldsNext.push([x, y, w, h]);
});

EM_JS(void, js_set_bg, (int r, int g, int b), {
	Module.uvui.bg = 'rgb(' + r + ',' + g + ',' + b + ')';
});

EM_JS(void, js_clear, (int r, int g, int b, double a), {
	const ui = Module.uvui;
	const ctx = ui.ctx;
	// a clear covers the whole display, whatever the mask is
	ctx.restore();
	ctx.save();
	ctx.fillStyle = 'rgba(' + r + ',' + g + ',' + b + ',' + a + ')';
	ctx.fillRect(0, 0, ui.w, ui.h);
});

EM_JS(void, js_mask, (int x, int y, int w, int h), {
	const ctx = Module.uvui.ctx;
	ctx.restore();
	ctx.save();
	ctx.beginPath();
	ctx.rect(x, y, w, h);
	ctx.clip();
});

EM_JS(void, js_rrect, (int x, int y, int w, int h, int r,
		int cr, int cg, int cb, double ca), {
	const ctx = Module.uvui.ctx;
	ctx.fillStyle = 'rgba(' + cr + ',' + cg + ',' + cb + ',' + ca + ')';
	if (r <= 0) {
		ctx.fillRect(x, y, w, h);
	}
	else {
		ctx.beginPath();
		ctx.roundRect(x, y, w, h, Math.min(r, w / 2, h / 2));
		ctx.fill();
	}
});

EM_JS(void, js_point, (int x, int y, double d, int cr, int cg, int cb, double ca), {
	const ctx = Module.uvui.ctx;
	ctx.fillStyle = 'rgba(' + cr + ',' + cg + ',' + cb + ',' + ca + ')';
	ctx.beginPath();
	ctx.arc(x, y, d / 2, 0, 2 * Math.PI);
	ctx.fill();
});

EM_JS(void, js_line, (int x1, int y1, int x2, int y2, int w,
		int cr, int cg, int cb, double ca), {
	const ctx = Module.uvui.ctx;
	ctx.strokeStyle = 'rgba(' + cr + ',' + cg + ',' + cb + ',' + ca + ')';
	ctx.lineWidth = Math.max(1, w);
	ctx.lineCap = 'butt';
	ctx.beginPath();
	// on the pixel centres, like the GL lines, so that a 1 px line is sharp
	const off = (Math.max(1, w) % 2 === 1) ? 0.5 : 0;
	ctx.moveTo(x1 + off, y1 + off);
	ctx.lineTo(x2 + off, y2 + off);
	ctx.stroke();
});

// *pts* is x, y pairs of int16_t. *edge* is how the shape closes: -1 not at all
// (a line strip), 0 a plain polygon, 1..4 onto the top, bottom, left or right
// edge of the display (the FT81X edge strips).
EM_JS(void, js_path, (const int16_t *pts, int count, int edge, int w,
		int cr, int cg, int cb, double ca), {
	const ui = Module.uvui;
	const ctx = ui.ctx;
	const p = HEAP16.subarray(pts >> 1, (pts >> 1) + count * 2);
	const style = 'rgba(' + cr + ',' + cg + ',' + cb + ',' + ca + ')';
	ctx.beginPath();
	if (edge <= 0) {
		ctx.moveTo(p[0], p[1]);
		for (let i = 1; i < count; i++) {
			ctx.lineTo(p[i * 2], p[i * 2 + 1]);
		}
	}
	else {
		const proj = (i) => {
			const x = p[i * 2];
			const y = p[i * 2 + 1];
			return (edge === 1) ? [x, 0] : (edge === 2) ? [x, ui.h] :
					(edge === 3) ? [0, y] : [ui.w, y];
		};
		const s = proj(0);
		ctx.moveTo(s[0], s[1]);
		for (let i = 0; i < count; i++) {
			ctx.lineTo(p[i * 2], p[i * 2 + 1]);
		}
		const e = proj(count - 1);
		ctx.lineTo(e[0], e[1]);
	}
	if (edge < 0) {
		ctx.strokeStyle = style;
		ctx.lineWidth = Math.max(1, w);
		ctx.lineJoin = 'round';
		ctx.lineCap = 'round';
		ctx.stroke();
	}
	else {
		ctx.closePath();
		ctx.fillStyle = style;
		ctx.fill();
	}
});

EM_JS(void, js_text, (const char *str, int mono, int px, int x, int y, int align,
		int line_h, int cr, int cg, int cb, double ca), {
	const ctx = Module.uvui.ctx;
	ctx.font = px + 'px ' + Module.uvui.fonts[mono];
	ctx.fillStyle = 'rgba(' + cr + ',' + cg + ',' + cb + ',' + ca + ')';
	ctx.textBaseline = 'alphabetic';
	ctx.textAlign = (align === 1) ? 'center' : ((align === 2) ? 'right' : 'left');
	// The desktop backend puts the top of a capital letter at y, so the baseline
	// is a cap height below it.
	const cap = ctx.measureText('H').actualBoundingBoxAscent;
	ctx.fillText(UTF8ToString(str), x, y + cap);
});

EM_JS(int, js_text_width, (const char *str, int mono, int px), {
	const ctx = Module.uvui.ctx;
	ctx.font = px + 'px ' + Module.uvui.fonts[mono];
	return Math.round(ctx.measureText(UTF8ToString(str)).width);
});

// Makes an offscreen canvas of RGBA pixels and returns its id.
EM_JS(int, js_image_new, (const uint8_t *rgba, int w, int h), {
	const ui = Module.uvui;
	const c = document.createElement('canvas');
	c.width = w;
	c.height = h;
	const img = new ImageData(new Uint8ClampedArray(HEAPU8.subarray(rgba, rgba + w * h * 4)), w, h);
	c.getContext('2d').putImageData(img, 0, 0);
	ui.images.push(c);
	return ui.images.length - 1;
});

EM_JS(void, js_image_draw, (int id, int x, int y, int w, int h,
		int cr, int cg, int cb, double ca), {
	const ui = Module.uvui;
	let src = ui.images[id];
	if (src) {
		if (cr !== 255 || cg !== 255 || cb !== 255) {
			// multiplied by the blend colour, like the FT81X and the GL
			// backend; made once per image and colour
			const key = id + ':' + cr + ',' + cg + ',' + cb;
			let t = ui.tinted.get(key);
			if (!t) {
				t = document.createElement('canvas');
				t.width = src.width;
				t.height = src.height;
				const tc = t.getContext('2d');
				tc.drawImage(src, 0, 0);
				tc.globalCompositeOperation = 'multiply';
				tc.fillStyle = 'rgb(' + cr + ',' + cg + ',' + cb + ')';
				tc.fillRect(0, 0, t.width, t.height);
				tc.globalCompositeOperation = 'destination-in';
				tc.drawImage(src, 0, 0);
				ui.tinted.set(key, t);
			}
			src = t;
		}
		const ctx = ui.ctx;
		ctx.globalAlpha = ca;
		ctx.drawImage(src, x, y, w, h);
		ctx.globalAlpha = 1;
	}
});



// ---- uv_ui interface ---------------------------------------------------------


/// @brief: Starts a new frame if the last one is done. Called by every
/// drawing function before it draws.
static void frame_begin(void) {
	if (this->frame_done) {
		this->frame_done = false;
		js_begin_frame();
	}
}


/// @brief: true when *font* is one of the monospace fonts
static int font_is_mono(const ui_font_st *font) {
	return ((font >= &ui_mono_fonts[0]) &&
			(font < &ui_mono_fonts[UI_MAX_FONT_COUNT])) ? 1 : 0;
}


/// @brief: The size *font* is drawn with, in pixels
static int font_px(const ui_font_st *font) {
	int ret = font_sizes[1];
	const ui_font_st *base = font_is_mono(font) ? ui_mono_fonts : ui_fonts;
	ptrdiff_t i = font - base;
	if ((i >= 0) && (i < UI_MAX_FONT_COUNT)) {
		ret = font_sizes[i];
	}
	return ret;
}


bool uv_ui_init(void) {
	if (!this->inited) {
		color_st bg = uv_uic(CONFIG_UI_STYLE_DISPLAY_C_1);
		js_init(CONFIG_FT81X_HSIZE, CONFIG_FT81X_VSIZE, 0);
		js_set_bg(bg.r, bg.g, bg.b);
		for (uint32_t i = 0; i < UI_MAX_FONT_COUNT; i++) {
			ui_fonts[i].char_height = (uint16_t) js_font_height(0, font_sizes[i]);
			ui_mono_fonts[i].char_height = (uint16_t) js_font_height(1, font_sizes[i]);
		}
		js_begin_frame();
		this->inited = true;
	}
	return false;
}


void uv_ui_destroy(void) {
	// the canvas belongs to the page, and so do the images
}


void uv_ui_confwindow_exec(const uv_uistyle_st *style) {
	// there is no CAN interface in a browser to configure
	(void) style;
}


bool uv_ui_get_refresh_request(void) {
	return js_take_refresh();
}


void uv_ui_set_backlight(uint8_t percent) {
	this->brightness = percent;
}


uint8_t uv_ui_get_backlight(void) {
	return this->brightness;
}


bool uv_ui_get_touch_impl(int16_t *x, int16_t *y) {
	uint32_t t = (uint32_t) js_get_touch();
	*x = (int16_t) ((t >> 15) & 0x7FFFU);
	*y = (int16_t) (t & 0x7FFFU);
	return ((t & 0x80000000U) != 0U);
}


int16_t uv_ui_get_scroll(void) {
	return (int16_t) js_take_scroll();
}


const char *uv_ui_get_clipboard(void) {
	js_get_clipboard(this->clipboard, sizeof(this->clipboard));
	return this->clipboard;
}


char uv_ui_peek_key_press(void) {
	return (char) js_peek_key();
}


char uv_ui_get_key_press(void) {
	return (char) js_pop_key();
}


void uv_ui_clear_impl(color_t col) {
	frame_begin();
	color_st c = uv_uic(col);
	js_clear(c.r, c.g, c.b, (double) c.a / 255.0);
}


void uv_ui_draw_point_impl(int16_t x, int16_t y, color_t col, uint16_t diameter) {
	frame_begin();
	color_st c = uv_uic(col);
	js_point(x, y, (double) diameter, c.r, c.g, c.b, (double) c.a / 255.0);
}


void uv_ui_draw_rrect_impl(const int16_t x, const int16_t y,
		const uint16_t w, const uint16_t h,
		const uint16_t radius, const color_t col) {
	frame_begin();
	color_st c = uv_uic(col);
	js_rrect(x, y, w, h, radius, c.r, c.g, c.b, (double) c.a / 255.0);
}


void uv_ui_draw_line_impl(const int16_t start_x, const int16_t start_y,
		const int16_t end_x, const int16_t end_y,
		const uint16_t width, const color_t color) {
	frame_begin();
	color_st c = uv_uic(color);
	js_line(start_x, start_y, end_x, end_y, width, c.r, c.g, c.b,
			(double) c.a / 255.0);
}


void uv_ui_draw_linestrip_impl(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const uint16_t line_width, const color_t color,
		const uv_ui_strip_type_e type) {
	frame_begin();
	if (point_count > 0U) {
		color_st c = uv_uic(color);
		int16_t *pts = malloc(sizeof(int16_t) * 2U * point_count);
		if (pts != NULL) {
			for (uint16_t i = 0; i < point_count; i++) {
				pts[i * 2] = points[i].x;
				pts[i * 2 + 1] = points[i].y;
			}
			int edge;
			if (type == UI_STRIP_TYPE_ABOVE) {
				edge = 1;
			}
			else if (type == UI_STRIP_TYPE_BELOW) {
				edge = 2;
			}
			else if (type == UI_STRIP_TYPE_LEFT) {
				edge = 3;
			}
			else if (type == UI_STRIP_TYPE_RIGHT) {
				edge = 4;
			}
			else {
				edge = -1;
			}
			js_path(pts, point_count, edge, line_width, c.r, c.g, c.b,
					(double) c.a / 255.0);
			free(pts);
		}
	}
}


void uv_ui_draw_polygon_impl(const uv_ui_linestrip_point_st *points,
		const uint16_t point_count, const color_t color) {
	frame_begin();
	if (point_count >= 3U) {
		color_st c = uv_uic(color);
		int16_t *pts = malloc(sizeof(int16_t) * 2U * point_count);
		if (pts != NULL) {
			for (uint16_t i = 0; i < point_count; i++) {
				pts[i * 2] = points[i].x;
				pts[i * 2 + 1] = points[i].y;
			}
			js_path(pts, point_count, 0, 1, c.r, c.g, c.b, (double) c.a / 255.0);
			free(pts);
		}
	}
}


void uv_ui_draw_string_impl(char *str, ui_font_st *font,
		int16_t x, int16_t y, ui_align_e align, color_t color) {
	frame_begin();
	if ((str != NULL) && (font != NULL) && (str[0] != '\0')) {
		color_st c = uv_uic(color);
		int mono = font_is_mono(font);
		int px = font_px(font);
		int halign = (align & HALIGN_CENTER) ? 1 : ((align & HALIGN_RIGHT) ? 2 : 0);
		size_t len = strlen(str);
		char *s = malloc(len + 1U);
		if (s != NULL) {
			memcpy(s, str, len + 1U);
			uint16_t line_count = (uint16_t) uv_str_get_line_count(s);
			if (align & VALIGN_CENTER) {
				y -= (int16_t) (line_count * font->char_height / 2);
			}
			// one line at a time; like the desktop backend, an empty line does
			// not advance the next one
			char *line = s;
			for (size_t i = 0; i <= len; i++) {
				if ((s[i] == '\r') || (s[i] == '\n') || (s[i] == '\0')) {
					s[i] = '\0';
					if (line[0] != '\0') {
						js_text(line, mono, px, x, y, halign, font->char_height,
								c.r, c.g, c.b, (double) c.a / 255.0);
						y += (int16_t) font->char_height;
					}
					line = &s[i + 1U];
				}
			}
			free(s);
		}
	}
}


int16_t uv_ui_get_string_width(char *str, ui_font_st *font) {
	int16_t ret = 0;
	if ((str != NULL) && (font != NULL)) {
		int mono = font_is_mono(font);
		int px = font_px(font);
		size_t len = strlen(str);
		char *s = malloc(len + 1U);
		if (s != NULL) {
			memcpy(s, str, len + 1U);
			char *line = s;
			for (size_t i = 0; i <= len; i++) {
				if ((s[i] == '\r') || (s[i] == '\n') || (s[i] == '\0')) {
					s[i] = '\0';
					int w = js_text_width(line, mono, px);
					if (w > ret) {
						ret = (int16_t) w;
					}
					line = &s[i + 1U];
				}
			}
			free(s);
		}
	}
	return ret;
}


void uv_ui_set_mask_impl(int16_t x, int16_t y, int16_t width, int16_t height) {
	frame_begin();
	js_mask(x, y, width, height);
}


void uv_ui_force_mask(int16_t x, int16_t y, int16_t width, int16_t height) {
	uv_ui_set_mask_impl(x, y, width, height);
}


void uv_ui_touchscreen_calibrate(ui_transfmat_st *transform_matrix) {
	(void) transform_matrix;
}


void uv_ui_touchscreen_set_transform_matrix(ui_transfmat_st *transform_matrix) {
	(void) transform_matrix;
}


void uv_ui_dlswap_impl(void) {
	// The browser presents the canvas by itself, as it is when the thread is
	// handed back. The frame is left there until the next one starts.
	if (!this->frame_done) {
		js_end_frame();
	}
	this->frame_done = true;
}


void uv_ui_web_text_field(int16_t x, int16_t y, int16_t width, int16_t height) {
	js_text_field(x, y, width, height);
}


static uimedia_ll_st *media_find(const char *name) {
	uimedia_ll_st *m = this->uimediall;
	while ((m != NULL) && (strcmp(m->filename, name) != 0)) {
		m = m->next_ptr;
	}
	return m;
}


static uimedia_ll_st *media_add(const char *name, uint8_t *rgba,
		uint32_t w, uint32_t h) {
	uimedia_ll_st *m = malloc(sizeof(uimedia_ll_st));
	if (m != NULL) {
		strncpy(m->filename, name, sizeof(m->filename) - 1U);
		m->filename[sizeof(m->filename) - 1U] = '\0';
		m->width = w;
		m->height = h;
		m->js_id = js_image_new(rgba, (int) w, (int) h);
		m->next_ptr = this->uimediall;
		this->uimediall = m;
	}
	return m;
}


static uint32_t media_to_bitmap(uv_uimedia_st *bitmap, uimedia_ll_st *m) {
	uint32_t ret = 0;
	if (m != NULL) {
		bitmap->filename = m->filename;
		bitmap->width = (uint16_t) m->width;
		bitmap->height = (uint16_t) m->height;
		bitmap->size = m->width * m->height * 4U;
		ret = bitmap->size;
	}
	return ret;
}


uint32_t uv_uimedia_newbitmapexmem(uv_uimedia_st *bitmap,
		uv_w25q128_st *exmem, const char *filename) {
	(void) exmem;
	memset(bitmap, 0, sizeof(*bitmap));
	bitmap->filename = "";
	bitmap->visible = true;
	bitmap->type = UV_UIMEDIA_IMAGE;

	uimedia_ll_st *m = NULL;
	if ((filename != NULL) && (filename[0] != '\0')) {
		m = media_find(filename);
		if (m == NULL) {
			unsigned char *image = NULL;
			unsigned w, h;
			if (lodepng_decode32_file(&image, &w, &h, filename) == 0) {
				m = media_add(filename, image, w, h);
			}
			else {
				printf("Loading bitmap '%s' failed\n", filename);
			}
			free(image);
		}
	}
	return media_to_bitmap(bitmap, m);
}


uint32_t uv_uimedia_newbitmapexmem_mem(uv_uimedia_st *bitmap,
		const char *name, const uint8_t *data, uint32_t datalen) {
	memset(bitmap, 0, sizeof(*bitmap));
	bitmap->filename = "";
	bitmap->visible = true;
	bitmap->type = UV_UIMEDIA_IMAGE;

	uimedia_ll_st *m = NULL;
	if ((name != NULL) && (name[0] != '\0')) {
		m = media_find(name);
		if ((m == NULL) && (data != NULL) && (datalen != 0U)) {
			unsigned char *image = NULL;
			unsigned w, h;
			if (lodepng_decode32(&image, &w, &h, data, datalen) == 0) {
				m = media_add(name, image, w, h);
			}
			else {
				printf("Loading embedded bitmap '%s' failed\n", name);
			}
			free(image);
		}
	}
	return media_to_bitmap(bitmap, m);
}


void uv_uimedia_free(uv_uimedia_st *bitmap) {
	memset(bitmap, 0, sizeof(*bitmap));
}


void uv_ui_draw_bitmap_ext_impl(uv_uimedia_st *bitmap, int16_t x, int16_t y,
		int16_t w, int16_t h, uint32_t wrap, color_t c) {
	frame_begin();
	(void) wrap;
	if (bitmap->visible && (bitmap->filename != NULL)) {
		uimedia_ll_st *m = media_find(bitmap->filename);
		if (m != NULL) {
			color_st blend = uv_uic(c);
			js_image_draw(m->js_id, x, y, w, h, blend.r, blend.g, blend.b,
					(double) blend.a / 255.0);
		}
	}
}

#endif
