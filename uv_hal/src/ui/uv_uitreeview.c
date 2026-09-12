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


#include "ui/uv_uitreeview.h"


#if CONFIG_UI

#define this ((uv_uitreeobject_st*) me)

#define XOFFSET	20
// How far a tree object's content is indented from the object's own left edge.
// The header draws the '+' / '-' marker at XOFFSET and the name one marker's
// width to the right of it; the content starts where the name does, so that the
// rows below a header read as belonging to it rather than as another level of
// the tree.
#define INDENT(font)	(XOFFSET * 2 + uv_ui_get_font_height(font))
// Gap between the guide line's horizontal tick and the row it points at.
#define GUIDE_GAP	4
// The white uv_uitabwindow draws its header line in. The guides are structural
// lines of the same kind, and in the window background's own shades they were
// too dim to do the job of showing which rows hang off which header.
#define GUIDE_C		C(0xFFFFFFFF)


static int16_t uitreeview_recalc_height(void *me);
static void uv_uitreeobject_draw(void *me, const uv_bounding_box_st *pbb);
static void uitreeobject_draw_guides(void *me);
static void touch(void *me, uv_touch_st *touch);



void uv_uitreeobject_init(void *me, uv_uiobject_st **object_array,
		const char *name, void (*show_callb)(uv_uitreeobject_st *me_ptr), const uv_uistyle_st* style) {
	uv_uiwindow_init(this, object_array, style);
	this->text_c = style->text_color;
	this->font = style->font;
	// the content sits below the header row and indented under the header's
	// name, i.e. in a coordinate space of its own that starts at the top left
	// corner of what this object owns
	uv_uiwindow_set_content_bb_default_pos(this,
			INDENT(this->font), CONFIG_UI_TREEVIEW_ITEM_HEIGHT);
	this->name = name;
	this->show_callb = show_callb;
	this->open = false;
	this->one_active = true;
	((uv_uiobject_st*) this)->step_callb = &uv_uiwindow_step;
	uv_uiobject_set_draw_callb(this, &uv_uitreeobject_draw);
	uv_uiobject_set_touch_callb(this, &touch);
	uv_uiwindow_set_transparent(this, true);
}




static void touch(void *me, uv_touch_st *touch) {
	if ((touch->action == TOUCH_CLICKED) &&
			(touch->y >= 0) &&
			(touch->y < CONFIG_UI_TREEVIEW_ITEM_HEIGHT)) {
		// the header row opens and closes this object
		if (((uv_uiobject_st*) this)->parent != NULL) {
			if (this->open) {
				uv_uitreeview_close(((uv_uiobject_st*) this)->parent, this);
			}
			else {
				uv_uitreeview_open(((uv_uiobject_st*) this)->parent, this);
			}
		}
		touch->action = TOUCH_NONE;
	}
	else if (this->open) {
		// Everything else belongs to the content. This touch callback replaced
		// uv_uiwindow's own, which is what passes a touch on to the children,
		// so it has to be called here: without it nothing inside an open tree
		// object ever saw a touch and a button in there could not be clicked.
		//
		// Only while open: a closed object shows nothing, and its children are
		// laid out under the rows of the objects below it.
		_uv_uiwindow_touch(this, touch);
	}
	else {
	}
}



void uv_uitreeobject_clear(void *me) {
	uv_uiwindow_clear(me);
	// uv_uiwindow_clear() ends by installing the plain window draw function,
	// which would cost this object its +/- marker, its name and its separator
	// line - and, since the children keep being drawn, leave the rows of an
	// open object floating with no header above them. Clearing the contents
	// does not stop it being a treeobject, so put the draw callback back.
	uv_uiobject_set_draw_callb(this, &uv_uitreeobject_draw);
}


// Draws the lines which tie an open object's rows to the header they belong to:
// one vertical line running down from the header's marker, with a short
// horizontal tick reaching from it out to each row. The indent alone leaves the
// rows looking like rows of the tree itself - with more than one object open
// there is nothing but the amount of white space to say where one object's
// contents end and the next object begins.
//
// The ticks point at the leftmost column of children only. A row is often
// several objects wide (a label and a button, say), and a tick for every one of
// them would draw ticks straight across the row.
static void uitreeobject_draw_guides(void *me) {
	uv_uiwindow_st *win = (uv_uiwindow_st*) this;
	int16_t minx = INT16_MAX;
	for (uint16_t i = 0; i < win->objects_count; i++) {
		if (win->objects[i]->visible &&
				(uv_uibb(win->objects[i])->x < minx)) {
			minx = uv_uibb(win->objects[i])->x;
		}
		else {
		}
	}
	// down the middle of the marker the header drew at XOFFSET, starting at the
	// marker's lower edge - starting at the header row's bottom instead leaves
	// half a row of empty space between the marker and the line hanging from it,
	// which reads as two separate things
	int16_t linex = uv_ui_get_xglobal(this) + XOFFSET +
			uv_ui_get_string_width("-", this->font) / 2;
	int16_t topy = uv_ui_get_yglobal(this) + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2 +
			uv_ui_get_font_height(this->font) / 2;
	int16_t lasty = topy;
	for (uint16_t i = 0; i < win->objects_count; i++) {
		uv_uiobject_st *obj = win->objects[i];
		if (obj->visible &&
				(uv_uibb(obj)->x == minx)) {
			// a nested object is met at its own header row, not at the middle
			// of everything it has open below it
			int16_t h = (obj->vrtl_draw == &uv_uitreeobject_draw) ?
					CONFIG_UI_TREEVIEW_ITEM_HEIGHT : uv_uibb(obj)->height;
			int16_t ticky = uv_ui_get_yglobal(obj) + h / 2;
			int16_t tickx = uv_ui_get_xglobal(obj) - GUIDE_GAP;
			if (tickx > linex) {
				uv_ui_draw_line(linex, ticky, tickx, ticky, 1, GUIDE_C);
			}
			else {
			}
			if (ticky > lasty) {
				lasty = ticky;
			}
			else {
			}
		}
		else {
		}
	}
	if (lasty > topy) {
		uv_ui_draw_line(linex, topy, linex, lasty, 1, GUIDE_C);
	}
	else {
	}
}


static void uv_uitreeobject_draw(void *me, const uv_bounding_box_st *pbb) {

	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	color_t line_c = uv_uic_brighten(((uv_uiwindow_st*) this)->bg_c, 30);
	// '+' means "opens", '-' means "closes": the marker shows what a click
	// would do, so a closed object carries the '+'.
	if (!this->open) {
		uv_ui_draw_string("+", this->font,
				x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2, ALIGN_CENTER_LEFT,
				this->text_c);
	}
	else {
		// super draw function
		uv_uiwindow_draw(this, pbb);

		uv_ui_draw_string("-", this->font,
				x + XOFFSET, y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
				ALIGN_CENTER_LEFT, this->text_c);
	}

	uv_ui_draw_string((char*) this->name, this->font,
			x + XOFFSET * 2 +
			uv_ui_get_font_height(this->font),
			y + CONFIG_UI_TREEVIEW_ITEM_HEIGHT / 2,
			ALIGN_CENTER_LEFT, this->text_c);

	if (this->open) {
		y += uv_uibb(this)->height;
	}
	else {
		y += CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	}
	uv_ui_draw_line(x, y - 1, x + w, y - 1, 1, line_c);

	// Only while open. Drawing them unconditionally meant closing an object
	// hid nothing: its rows stayed on screen, without the window background
	// that the open branch above paints, so closing merely made the contents
	// look dimmed instead of collapsing them.
	if (this->open) {
		// before the children: they are drawn to the right of the guides, but a
		// child painting its own background would cover a tick reaching up to it
		uitreeobject_draw_guides(this);

		_uv_uiwindow_draw_children(this, pbb);

		// scroll bars on top of the children
		uv_uiwindow_draw_scrollbars(this, pbb);
	}
}


void uv_uitreeobject_set_font(void *me, uv_font_st *font) {
	if (font != NULL) {
		this->font = font;
		// the indent is a marker's width, so it follows the font
		uv_uiwindow_set_content_bb_default_pos(this,
				INDENT(this->font), CONFIG_UI_TREEVIEW_ITEM_HEIGHT);
		uv_ui_refresh(this);
	}
	else {
	}
}


uv_bounding_box_st uv_uitreeobject_get_content_bb(void *me) {
	uv_bounding_box_st bb = uv_uiwindow_get_contentbb(this);
	bb.height -= CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
	// the content starts INDENT pixels in from this object's left edge, so that
	// much less of the width is left for it
	bb.width -= ((uv_uiwindow_st*) this)->content_bb_xdef;
	return bb;
}


#undef this
#define this ((uv_uitreeview_st*) me)


/// @brief: True when *obj* is a node of the tree, as opposed to a plain widget
/// or the tree view at its root.
///
/// The draw callback is what tells them apart: uv_uitreeobject_init() installs
/// it and nothing else does, which is also why uv_uitreeobject_clear() puts it
/// back. A node and the tree view both extend uv_uiwindow_st but carry
/// different fields after it, so reading one as if it were the other is exactly
/// what this exists to prevent -- a nested node's container is another node,
/// and before this the open/close path read its name pointer as the tree view's
/// one_active flag.
static bool is_treeobject(const void *obj) {
	return ((obj != NULL) &&
			(((const uv_uiobject_st*) obj)->vrtl_draw == &uv_uitreeobject_draw));
}


/// @brief: Whether *container* keeps one of its child nodes open at a time. A
/// node holds a branch of the same tree, so it behaves the way the root does.
static bool container_one_active(const void *container) {
	return is_treeobject(container) ?
			((const uv_uitreeobject_st*) container)->one_active :
			((const uv_uitreeview_st*) container)->one_active;
}


/// @brief: The width a container's child nodes are given: its content area,
/// which for a node is already short of the indent its content starts at.
static int16_t container_content_width(void *container) {
	return is_treeobject(container) ?
			uv_uitreeobject_get_content_bb(container).width :
			uv_uiwindow_get_contentbb(container).width;
}


/// @brief: The tree view the whole thing hangs off, found by walking up for as
/// long as the container is still a node.
///
/// Opening a node changes the height of every node above it and of the view
/// itself, so the layout is always redone from the root rather than from the
/// container the node happens to sit in.
static void *tree_root(void *container) {
	void *ret = container;
	while (is_treeobject(ret) &&
			(((uv_uiobject_st*) ret)->parent != NULL)) {
		ret = ((uv_uiobject_st*) ret)->parent;
	}
	return ret;
}


/// @brief: Lays out *container*'s child nodes and returns what its content
/// comes to in pixels.
///
/// The nodes are stacked in the order they were added, below whatever plain
/// widgets the container also holds: a directory's own rows come first and the
/// subdirectories follow them, so the rows keep the positions the application
/// gave them however the subtrees below them are opened and closed.
///
/// Recursive, because an open node is as tall as its own content and that is in
/// turn whatever its children come to. Which is what lets a node several levels
/// down push everything below it -- all the way up to the root -- into place.
static int16_t uitreeview_recalc_height(void *me) {
	uv_uiwindow_st *win = (uv_uiwindow_st*) me;
	int16_t h = 0;
	for (uint16_t i = 0; i < win->objects_count; i++) {
		uv_uiobject_st *obj = win->objects[i];
		if (obj->visible && !is_treeobject(obj)) {
			int16_t bottom = uv_uibb(obj)->y + uv_uibb(obj)->height;
			if (bottom > h) {
				h = bottom;
			}
			else {
			}
		}
		else {
		}
	}
	for (uint16_t i = 0; i < win->objects_count; i++) {
		uv_uiobject_st *obj = win->objects[i];
		if (obj->visible && is_treeobject(obj)) {
			uv_uitreeobject_st *node = (uv_uitreeobject_st*) obj;
			uv_uibb(node)->y = h;
			// re-measured rather than kept from when the node was added: the
			// content narrows by the scroll bar's width the moment the tree grows
			// taller than its window, and rows added on either side of that point
			// would otherwise end at different places
			uv_uibb(node)->width = container_content_width(me);
			int16_t node_h = CONFIG_UI_TREEVIEW_ITEM_HEIGHT;
			if (node->open) {
				int16_t content = uitreeview_recalc_height(node);
				// the height declared when the node was added is a floor, for
				// one that builds its rows only when it is opened
				if (node->content_h > content) {
					content = node->content_h;
				}
				else {
				}
				node_h += content;
			}
			else {
				// Closed: the header row alone. Without this a closed node
				// keeps the height of its open state, so it still covers the
				// rows beneath it and its own hidden children stay in the way
				// of touches meant for them.
			}
			uv_uibb(node)->height = node_h;
			h += node_h;
		}
		else {
		}
	}
	return h;
}


/// @brief: Redoes the layout of the whole tree *container* belongs to and sizes
/// the root's content box to match, so the scroll bar follows what is open.
static void uitreeview_relayout(void *container) {
	void *root = tree_root(container);
	int16_t h = uitreeview_recalc_height(root);
	uv_uiwindow_set_contentbb(root, uv_uibb(root)->width, h);
	uv_ui_refresh(root);
}


void uv_uitreeview_init(void *me,
		uv_uitreeobject_st ** const object_array, const uv_uistyle_st * style) {
	uv_uiwindow_init(this, (uv_uiobject_st ** const) object_array, style);
	this->one_active = true;
}


void uv_uitreeview_set_oneactive(void *me, bool value) {
	if (is_treeobject(me)) {
		((uv_uitreeobject_st*) me)->one_active = value;
	}
	else {
		((uv_uitreeview_st*) me)->one_active = value;
	}
	uv_ui_refresh(me);
}


void uv_uitreeview_open(void *me, uv_uitreeobject_st *obj) {
	uv_uiwindow_st *win = (uv_uiwindow_st*) me;
	if (container_one_active(me)) {
		for (uint16_t i = 0; i < win->objects_count; i++) {
			// the nodes only: a container may hold plain widgets as well, and
			// they have no open state to close
			if (is_treeobject(win->objects[i]) &&
					(win->objects[i] != (uv_uiobject_st*) obj) &&
					((uv_uitreeobject_st*) win->objects[i])->open) {
				uv_uitreeview_close(me, (uv_uitreeobject_st*) win->objects[i]);
			}
			else {
			}
		}
	}
	else {
	}
	obj->open = true;
	// before the layout, not after it: an object that builds its rows when it
	// is opened has to have them before there is anything to measure
	if (obj->show_callb) {
		obj->show_callb(obj);
	}
	else {
	}
	uitreeview_relayout(me);

	// Scroll the tree so the node that was just opened is at the top of the
	// view. Accumulated up to the root, because the node may sit several levels
	// down and it is the root that scrolls; each level adds its own position
	// and the offset its content starts at.
	int16_t y = uv_uibb(obj)->y;
	void *c = me;
	while (is_treeobject(c)) {
		y += ((uv_uiwindow_st*) c)->content_bb.y + uv_uibb(c)->y;
		c = ((uv_uiobject_st*) c)->parent;
	}
	uv_uiwindow_content_move_to(c, 0, y);
}


void uv_uitreeview_close(void *me, uv_uitreeobject_st *obj) {
	obj->open = false;
	uitreeview_relayout(me);
}


void uv_uitreeview_add(void *me, uv_uitreeobject_st * const object,
		const int16_t content_height, const bool active) {
	// the position is settled by the layout below; the height likewise, and
	// the one passed here is what the object is worth until it holds anything
	uv_uiwindow_addxy((uv_uiwindow_st*) me, object, 0, 0,
			container_content_width(me),
			CONFIG_UI_TREEVIEW_ITEM_HEIGHT + content_height);
	object->content_h = content_height;
	if (active) {
		uv_uitreeview_open(me, object);
	}
	else {
		uv_uitreeview_close(me, object);
	}
}



#endif


