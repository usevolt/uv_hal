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


#include "ui/uv_uifileedit.h"
#include <string.h>
#include <uv_rtos.h>

#if CONFIG_UI

#if CONFIG_TARGET_WIN
#include <windows.h>
#include <commdlg.h>
#elif CONFIG_TARGET_LINUX
#include <stdio.h>
#include <stdlib.h>
#endif


static uv_uiobject_ret_e step(void *me, uint16_t step_ms);
static void touch(void *me, uv_touch_st *touch);


#define TITLE_OFFSET		4
/// @brief: Horizontal margin kept between the field edges and the path text.
#define FILEEDIT_HPADDING	6

#define this ((uv_uifileedit_st *) me)


/* ----------------------------------------------------------------------------
 * Native file-chooser backend.
 *
 * filedialog_open() pops up the operating system's native open-file dialog and,
 * on success, copies the chosen path (UTF-8, null-terminated) into *out*.
 * Returns true if the user picked a file, false if the dialog was cancelled or
 * no chooser is available. The call blocks (the dialog is modal) until the user
 * dismisses it.
 * ------------------------------------------------------------------------- */

#if CONFIG_TARGET_WIN

// Builds a GetOpenFileNameW double-null-terminated filter list ("desc\0pat\0
// desc\0pat\0\0") from the widget's filters. The Win32 pattern separator is ';'
// while the widget stores space-separated globs, so spaces are translated.
static void build_win_filter(const uv_uifileedit_filter_st *filters,
		uint8_t count, wchar_t *dst, size_t dst_len) {
	if ((filters == NULL) || (count == 0)) {
		// default: a single "All Files" entry
		static const wchar_t def[] = L"All Files\0*.*\0";
		size_t n = sizeof(def) / sizeof(def[0]);	// includes both NULs
		if (n > dst_len - 1) {
			n = dst_len - 1;
		}
		memcpy(dst, def, n * sizeof(wchar_t));
		dst[n] = L'\0';	// final list terminator
		return;
	}

	size_t di = 0;
	for (uint8_t i = 0; (i < count) && (di + 2 < dst_len); i++) {
		// description
		int w = MultiByteToWideChar(CP_UTF8, 0,
				(filters[i].name != NULL) ? filters[i].name : "files", -1,
				dst + di, (int) (dst_len - di));
		di += (w > 0) ? (size_t) w : 0;	// w counts the written NUL
		// patterns, with ' ' translated to the Win32 ';' separator
		char pat[256];
		const char *src = (filters[i].patterns != NULL) ? filters[i].patterns : "*.*";
		size_t pi = 0;
		for (; (src[pi] != '\0') && (pi < sizeof(pat) - 1); pi++) {
			pat[pi] = (src[pi] == ' ') ? ';' : src[pi];
		}
		pat[pi] = '\0';
		w = MultiByteToWideChar(CP_UTF8, 0, pat, -1, dst + di, (int) (dst_len - di));
		di += (w > 0) ? (size_t) w : 0;
	}
	dst[di] = L'\0';	// final list terminator
}

static bool filedialog_open(const char *title,
		const uv_uifileedit_filter_st *filters, uint8_t filter_count,
		char *out, uint16_t out_len) {
	if ((out == NULL) || (out_len == 0)) {
		return false;
	}
	wchar_t file[1024] = L"";
	wchar_t wtitle[256];
	wchar_t wfilter[512];
	OPENFILENAMEW ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = file;
	ofn.nMaxFile = sizeof(file) / sizeof(file[0]);
	build_win_filter(filters, filter_count, wfilter, sizeof(wfilter) / sizeof(wfilter[0]));
	ofn.lpstrFilter = wfilter;
	ofn.nFilterIndex = 1;
	// OFN_NOCHANGEDIR keeps the process' working directory untouched, otherwise
	// the dialog would silently chdir() the whole application.
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	if (title != NULL) {
		MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle,
				sizeof(wtitle) / sizeof(wtitle[0]));
		ofn.lpstrTitle = wtitle;
	}

	bool ret = false;
	if (GetOpenFileNameW(&ofn)) {
		WideCharToMultiByte(CP_UTF8, 0, file, -1, out, out_len, NULL, NULL);
		out[out_len - 1] = '\0';
		ret = true;
	}
	return ret;
}

#elif CONFIG_TARGET_LINUX

// Shell single-quote *src* into *dst* so it can be embedded safely in a
// /bin/sh command line regardless of its contents.
static void shell_quote(const char *src, char *dst, size_t dst_len) {
	size_t di = 0;
	if (dst_len == 0) {
		return;
	}
	if (di < dst_len - 1) {
		dst[di++] = '\'';
	}
	for (size_t si = 0; (src != NULL) && (src[si] != '\0'); si++) {
		if (src[si] == '\'') {
			// a literal ' is escaped as the sequence '\''
			const char *esc = "'\\''";
			for (uint8_t k = 0; (esc[k] != '\0') && (di < dst_len - 1); k++) {
				dst[di++] = esc[k];
			}
		}
		else {
			if (di < dst_len - 1) {
				dst[di++] = src[si];
			}
		}
	}
	if (di < dst_len - 1) {
		dst[di++] = '\'';
	}
	dst[di] = '\0';
}

// Native chooser frontend families. zenity, qarma and matedialog share the
// zenity command-line syntax; kdialog has its own.
typedef enum {
	BACKEND_ZENITY,
	BACKEND_KDIALOG,
} backend_type_e;

// Appends this widget's file-type filters to *cmd* in the syntax expected by
// *type*. *cmd*/*cmdlen* are the command buffer and its capacity, *n* the
// current write offset (advanced by this call).
static void append_linux_filters(char *cmd, size_t cmdlen, int *n,
		backend_type_e type, const uv_uifileedit_filter_st *filters, uint8_t count) {
	if ((filters == NULL) || (count == 0)) {
		return;
	}
	if (type == BACKEND_ZENITY) {
		// one repeatable --file-filter='Name | *.a *.b' per entry
		for (uint8_t i = 0; i < count; i++) {
			char spec[320];
			snprintf(spec, sizeof(spec), "%s | %s",
					(filters[i].name != NULL) ? filters[i].name : "files",
					(filters[i].patterns != NULL) ? filters[i].patterns : "*");
			char qspec[384];
			shell_quote(spec, qspec, sizeof(qspec));
			*n += snprintf(cmd + *n, (*n < (int) cmdlen) ? cmdlen - *n : 0,
					" --file-filter=%s", qspec);
		}
	}
	else {
		// kdialog takes a single trailing 'pat pat|Name\npat|Name' argument
		char spec[512];
		size_t si = 0;
		for (uint8_t i = 0; i < count; i++) {
			si += snprintf(spec + si, (si < sizeof(spec)) ? sizeof(spec) - si : 0,
					"%s%s|%s", (i == 0) ? "" : "\n",
					(filters[i].patterns != NULL) ? filters[i].patterns : "*",
					(filters[i].name != NULL) ? filters[i].name : "files");
		}
		char qspec[600];
		shell_quote(spec, qspec, sizeof(qspec));
		*n += snprintf(cmd + *n, (*n < (int) cmdlen) ? cmdlen - *n : 0, " %s", qspec);
	}
}

static bool filedialog_open(const char *title,
		const uv_uifileedit_filter_st *filters, uint8_t filter_count,
		char *out, uint16_t out_len) {
	if ((out == NULL) || (out_len == 0)) {
		return false;
	}
	char qtitle[256];
	shell_quote((title != NULL) ? title : "Select file", qtitle, sizeof(qtitle));

	// Native chooser frontends in preference order. Each shells out the same
	// way tinyfiledialogs does.
	static const struct {
		const char *bin;
		backend_type_e type;
	} backends[] = {
		{ "zenity",     BACKEND_ZENITY },
		{ "qarma",      BACKEND_ZENITY },
		{ "matedialog", BACKEND_ZENITY },
		{ "kdialog",    BACKEND_KDIALOG },
	};

	bool ret = false;
	bool found = false;
	for (uint16_t i = 0; i < sizeof(backends) / sizeof(backends[0]); i++) {
		char checkcmd[64];
		snprintf(checkcmd, sizeof(checkcmd), "command -v %s >/dev/null 2>&1",
				backends[i].bin);
		if (system(checkcmd) != 0) {
			// this frontend is not installed, try the next one
			continue;
		}
		found = true;

		char cmd[1024];
		int n = 0;
		if (backends[i].type == BACKEND_ZENITY) {
			n += snprintf(cmd + n, sizeof(cmd) - n,
					"%s --file-selection --title=%s", backends[i].bin, qtitle);
		}
		else {
			n += snprintf(cmd + n, sizeof(cmd) - n,
					"%s --title %s --getopenfilename .", backends[i].bin, qtitle);
		}
		append_linux_filters(cmd, sizeof(cmd), &n, backends[i].type,
				filters, filter_count);
		snprintf(cmd + n, (n < (int) sizeof(cmd)) ? sizeof(cmd) - n : 0,
				" 2>/dev/null");

		FILE *p = popen(cmd, "r");
		if (p == NULL) {
			continue;
		}
		char line[1024];
		line[0] = '\0';
		char *r = fgets(line, sizeof(line), p);
		pclose(p);
		if ((r != NULL) && (line[0] != '\0') && (line[0] != '\n')) {
			// strip the trailing newline the dialog prints
			size_t l = strlen(line);
			while ((l > 0) && ((line[l - 1] == '\n') || (line[l - 1] == '\r'))) {
				line[--l] = '\0';
			}
			if (l > 0) {
				strncpy(out, line, out_len - 1);
				out[out_len - 1] = '\0';
				ret = true;
			}
		}
		else {
			// dialog was shown but the user cancelled (no output)
		}
		// a chooser was found and displayed; do not fall through to another one
		break;
	}
	if (!found) {
		printf("uv_uifileedit: no native file chooser found. "
				"Install one of: zenity, qarma, matedialog, kdialog "
				"(e.g. 'sudo apt-get install zenity').\n");
	}
	return ret;
}

#else

// MCU targets have no host file system; the field is inert.
static bool filedialog_open(const char *title,
		const uv_uifileedit_filter_st *filters, uint8_t filter_count,
		char *out, uint16_t out_len) {
	(void) title;
	(void) filters;
	(void) filter_count;
	(void) out;
	(void) out_len;
	return false;
}

#endif


/* ------------------------------------------------------------------------- */


// Builds the string actually drawn on the field into *dst*. If the path fits
// the field width it is copied verbatim; otherwise it is shortened to a
// leading "..." followed by the tail of the path that fits.
static void fileedit_display_text(void *me, char *dst, uint16_t dst_len) {
	uv_font_st *font = ((uv_uilabel_st*) this)->font;
	const char *src = this->buffer;

	if (dst_len == 0) {
		return;
	}
	if (src[0] == '\0') {
		dst[0] = '\0';
		return;
	}

	int16_t avail = (int16_t) uv_uibb(this)->width - FILEEDIT_HPADDING * 2;
	if ((avail <= 0) ||
			(uv_ui_get_string_width((char*) src, font) <= avail)) {
		strncpy(dst, src, dst_len - 1);
		dst[dst_len - 1] = '\0';
		return;
	}

	// path is too wide: prepend "..." and keep the longest fitting tail
	const char *ellipsis = "...";
	int16_t ell_w = uv_ui_get_string_width((char*) ellipsis, font);
	uint16_t len = strlen(src);
	uint16_t start = len;	// worst case: only the ellipsis fits
	for (uint16_t s = 1; s <= len; s++) {
		if (ell_w + uv_ui_get_string_width((char*) (src + s), font) <= avail) {
			start = s;
			break;
		}
	}
	snprintf(dst, dst_len, "%s%s", ellipsis, src + start);
}


void uv_uifileedit_init(void *me, char *buffer, uint16_t buf_len,
		const uv_uistyle_st *style) {
	uv_uilabel_init(this, style->font, ALIGN_CENTER, style->text_color, "");
	this->style = style;
	this->buffer = buffer;
	this->buf_len = buf_len;
	this->title = NULL;
	this->bg_color = style->bg_c;
	this->changed = false;
	this->filters = NULL;
	this->filter_count = 0;

	// ensure the buffer is null-terminated
	bool nullterm = false;
	for (uint16_t i = 0; i < buf_len; i++) {
		if (buffer[i] == '\0') {
			nullterm = true;
			break;
		}
	}
	if (!nullterm) {
		buffer[0] = '\0';
	}

	// keep the inherited uilabel's str pointer pointing at our buffer so
	// uilabel-aware code paths see the live text.
	uv_uilabel_set_text(this, this->buffer);

	uv_uiobject_set_step_callb(this, &step);
	uv_uiobject_set_draw_callb(this, &uv_uifileedit_draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


void uv_uifileedit_set_path(void *me, const char *path) {
	if ((this->buf_len == 0) || (path == NULL)) {
		return;
	}
	strncpy(this->buffer, path, this->buf_len - 1);
	this->buffer[this->buf_len - 1] = '\0';
	uv_ui_refresh(this);
}


void uv_uifileedit_draw(void *me, const uv_bounding_box_st *pbb) {
	uint16_t x = uv_ui_get_xglobal(this),
			y = uv_ui_get_yglobal(this);
	uv_font_st *font = ((uv_uilabel_st*) this)->font;
	color_t text_color = ((uv_uilabel_st*) this)->color;

	char disp[this->buf_len + 4];	// room for the leading "..."
	fileedit_display_text(this, disp, sizeof(disp));

	uint16_t text_height = (disp[0] == '\0') ?
			uv_ui_get_font_height(font) :
			uv_ui_get_string_height(disp, font);
	uint16_t height = text_height + TITLE_OFFSET * 2;
	int16_t title_height = 0;
	if (this->title != NULL) {
		title_height = uv_ui_get_string_height(this->title, font);
	}
	y += (uv_uibb(this)->h - (height + title_height + TITLE_OFFSET)) / 2;

	uv_ui_draw_shadowrrect(x, y, uv_uibb(this)->width, height, 0,
			this->bg_color, uv_uic_brighten(this->bg_color, -30),
			uv_uic_brighten(this->bg_color, 30));

	uv_ui_draw_string(disp, font,
			x + uv_uibb(this)->width / 2, y + height / 2 + CONFIG_UI_RADIUS,
			UI_ALIGN_CENTER, text_color);

	if (this->title) {
		uv_ui_draw_string(this->title, font,
				x + uv_uibb(this)->width / 2, y + height + TITLE_OFFSET,
				ALIGN_TOP_CENTER, text_color);
	}
}


static uv_uiobject_ret_e step(void *me, uint16_t step_ms) {
	(void) step_ms;
	// value_changed() is only true for the cycle following a user choice
	this->changed = false;
	return UIOBJECT_RETURN_ALIVE;
}


static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		touch->action = TOUCH_NONE;
		// snapshot to detect whether the user actually changed the path
		char snapshot[this->buf_len];
		memcpy(snapshot, this->buffer, this->buf_len);
		// The native chooser blocks (popen/fgets on Linux, GetOpenFileName on
		// Windows). When the UI runs under the FreeRTOS scheduler the periodic
		// tick signal (SIGALRM on the POSIX port) would interrupt that blocking
		// call (EINTR) and the chosen path would be lost. Disable preemption
		// around it, the same idiom used for protected blocking stdin reads.
#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
		portDISABLE_INTERRUPTS();
#endif
		bool opened = filedialog_open(this->title, this->filters,
				this->filter_count, this->buffer, this->buf_len);
#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
		portENABLE_INTERRUPTS();
#endif
		if (opened) {
			if (strncmp(snapshot, this->buffer, this->buf_len) != 0) {
				this->changed = true;
			}
		}
		uv_ui_refresh(this);
	}
}


#endif
