/*
 * uv_uiobject.c
 *
 *  Created on: Dec 23, 2016
 *      Author: usevolt
 */


#include "ui/uv_uiobject.h"


#if CONFIG_LCD

#define this ((uv_uiobject_st*) me)

/// @brief: Initializes the bounding box
void uv_bounding_box_init(uv_bounding_box_st *bb,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	bb->x = x;
	bb->y = y;
	bb->width = width;
	bb->height = height;
}



void uv_uiobject_init(void *me) {
	uv_bounding_box_init(&this->bb, 0, 0, 0, 0);
	this->parent = NULL;
	this->step_callb = NULL;
	this->visible = true;
	this->refresh = true;
	this->enabled = true;
}


#endif
