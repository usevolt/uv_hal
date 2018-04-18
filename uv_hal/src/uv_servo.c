/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/



#include "uv_servo.h"

#if CONFIG_SERVO





void uv_servo_set(uv_pwm_channel_t chn, uint32_t pos_mdeg) {
	if (pos_mdeg > SERVO_MAX_MANGLE) {
		pos_mdeg = SERVO_MAX_MANGLE;
	}
	int32_t t = uv_reli(pos_mdeg, SERVO_MIN_MANGLE, SERVO_MAX_MANGLE);
	uv_pwm_set(chn, uv_lerpi(t, SERVO_POS_0_DC, SERVO_POS_180_DC));
}





#endif
