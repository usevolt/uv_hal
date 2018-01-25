/*
 * uv_uitransition.c
 *
 *  Created on: Aug 1, 2017
 *      Author: usevolt
 */


#include <uv_hal_config.h>
#include "ui/uv_uitransition.h"


#if CONFIG_UI

void uv_uicolortransition_calc(void *me);
void uv_uiscalartransition_calc(void *me);



#define this	((uv_uitransition_st *)me)

void uv_uitransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, void (*calc_callb)(void *me)) {
	this->easing = easing;
	this->duration_ms = duration_ms;
	this->calc_callb = calc_callb;
	this->current_time_ms = 0;
	this->speed_ppt = 1000;
	this->parallel = NULL;
	this->series = NULL;
	this->state = UITRANSITION_INIT;
}


void uv_uitransition_step(void *me, void *parent, uint16_t step_ms) {
	if (this->state == UITRANSITION_PLAY) {
		// call virtual function to calculate changes in the variables
		this->calc_callb(this);
		// update parent object
		uv_ui_refresh(parent);
		this->current_time_ms += step_ms * this->speed_ppt / 1000;
		if (this->current_time_ms >= this->duration_ms) {
			this->current_time_ms = this->duration_ms;
			this->state = UITRANSITION_FINISH;
			if (this->series) {
				uv_uitransition_play(this->series);
			}
		}
	}
	else if (this->state == UITRANSITION_REVERSEPLAY) {
		// update parent object
		uv_ui_refresh(parent);
		this->current_time_ms -= step_ms * this->speed_ppt / 1000;
		if (this->current_time_ms < 0) {
			this->current_time_ms = 0;
			this->state = UITRANSITION_INIT;

			if (this->series) {
				uv_uitransition_reverseplay(this->series);
			}
		}
		// call virtual function to calculate changes in the variables
		this->calc_callb(this);
	}
	else if (this->state == UITRANSITION_FINISH) {
		// calculate the last value change
		if (this->current_time_ms == this->duration_ms) {
			this->calc_callb(this);
			uv_ui_refresh(parent);
			this->current_time_ms += step_ms * this->speed_ppt / 1000;
		}
		// only call next transition if this one is finished
		// to prevent infinite step-function recursion
		if (this->series) {
			uv_uitransition_step(this->series, parent, step_ms);
		}
	}
	// always call parallel transition
	if (this->parallel) {
		uv_uitransition_step(this->parallel, parent, step_ms);
	}
}




/// @brief: Starts the uitransition
void uv_uitransition_play(void *me) {
	if (this->state == UITRANSITION_FINISH) {
		this->current_time_ms = 0;
	}
	if (this->current_time_ms < this->duration_ms) {
		this->state = UITRANSITION_PLAY;
	}
	else {
		this->state = UITRANSITION_FINISH;
	}
	if (this->parallel) {
		uv_uitransition_play(this->parallel);
	}
}

void uv_uitransition_reverseplay(void *me) {
	if (this->state == UITRANSITION_INIT) {
		this->current_time_ms = this->duration_ms;
	}
	if (this->current_time_ms > 0) {
		this->state = UITRANSITION_REVERSEPLAY;
	}
	else {
		this->state = UITRANSITION_INIT;
	}
	if (this->parallel) {
		uv_uitransition_reverseplay(this->parallel);
	}
}




/// @brief: Stops the uitransition
void uv_uitransition_pause(void *me) {
	this->state = UITRANSITION_PAUSE;
	if (this->parallel) {
		uv_uitransition_pause(this->parallel);
	}
}




#undef this
#define this	((uv_uiscalartransition_st *) me)


void uv_uiscalartransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, int16_t start_val, int16_t end_val, int16_t *val_ptr) {
	uv_uitransition_init(this, easing, duration_ms,
			&uv_uiscalartransition_calc);
	this->start_val = start_val;
	this->end_val = end_val;
	this->cur_val = val_ptr;
}


void uv_uiscalartransition_calc(void *me) {
	if (((uv_uitransition_st*) this)->easing == UITRANSITION_EASING_LINEAR) {
		int16_t rel = uv_reli(((uv_uitransition_st*) this)->current_time_ms,
				0, ((uv_uitransition_st*) this)->duration_ms);
		*this->cur_val = uv_lerpi(rel, this->start_val, this->end_val);
	}
	else if (((uv_uitransition_st*) this)->easing == UITRANSITION_EASING_IN_QUAD) {
		int16_t rel = uv_reli(((uv_uitransition_st*) this)->current_time_ms,
				0, ((uv_uitransition_st*) this)->duration_ms);
		*this->cur_val = uv_lerpi(rel * rel / 1000, this->start_val, this->end_val);
	}
	else if (((uv_uitransition_st*) this)->easing == UITRANSITION_EASING_OUT_QUAD) {
		int16_t rel = uv_reli(((uv_uitransition_st*) this)->current_time_ms,
				0, ((uv_uitransition_st*) this)->duration_ms);
		*this->cur_val = uv_lerpi(- rel * (rel - 2000) / 1000, this->start_val, this->end_val);
	}
	else if (((uv_uitransition_st*) this)->easing == UITRANSITION_EASING_INOUT_QUAD) {
		int16_t rel = uv_reli(((uv_uitransition_st*) this)->current_time_ms,
				0, ((uv_uitransition_st*) this)->duration_ms / 2);
		if (rel < 1000) {
			*this->cur_val = uv_lerpi(rel * rel / 1000, this->start_val,
					this->end_val + (this->start_val - this->end_val) / 2);
		}
		else {
			rel -= 1000;
			*this->cur_val = uv_lerpi(- (rel * (rel - 2000) / 1000 - 1010), this->start_val,
					(this->end_val + (this->start_val - this->end_val) / 2));
		}
	}
	else {

	}
}




#undef this
#define this	((uv_uicolortransition_st *) me)

void uv_uicolortransition_init(void *me, uv_uitransition_easing_e easing,
		uint16_t duration_ms, color_t start_c, color_t end_c, color_t *c_ptr) {
	uv_uitransition_init(this, easing, duration_ms,
			&uv_uicolortransition_calc);
	this->start_c = start_c;
	this->end_c = end_c;
	this->cur_c = c_ptr;
	*this->cur_c = start_c;
}


void uv_uicolortransition_calc(void *me) {

}


#undef this


#endif
