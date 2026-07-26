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

#include "uv_test.h"
#include "uv_pid.h"

/// @file: Tests for the uv_pid closed loop controller.
///
/// The P, I and D factors are fixed point: the gains are scaled for a nominal
/// 20 ms step time and divided by 0x10000 (P and I) or 0x1000 (D). A unity P
/// gain at a 20 ms cycle is therefore 0x10000 and a unity D gain is 0x1000.
/// Those scalings are what the tests below pin down - if they ever shift, every
/// tuned closed loop on every device silently changes behaviour.


/// @brief: Unity gain constants at the nominal 20 ms step time
#define PID_UNITY_P		0x10000
#define PID_UNITY_D		0x1000
#define PID_STEP_MS		20


TEST(pid, zero_error_gives_zero_output) {
	uv_pid_st pid;
	uv_pid_init(&pid, PID_UNITY_P, PID_UNITY_P, PID_UNITY_D);
	uv_pid_set_target(&pid, 0);

	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);

	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);
}


TEST(pid, p_term_is_proportional_to_the_error) {
	uv_pid_st pid;
	uv_pid_init(&pid, PID_UNITY_P, 0, 0);
	uv_pid_set_target(&pid, 100);

	/* unity P gain at the nominal step time means output == error */
	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 100);

	uv_pid_step(&pid, PID_STEP_MS, 40);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 60);

	/* overshooting the target must reverse the output sign */
	uv_pid_step(&pid, PID_STEP_MS, 150);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), -50);
}


TEST(pid, p_term_scales_with_the_step_time) {
	uv_pid_st fast;
	uv_pid_st slow;
	uv_pid_init(&fast, PID_UNITY_P, 0, 0);
	uv_pid_init(&slow, PID_UNITY_P, 0, 0);
	uv_pid_set_target(&fast, 100);
	uv_pid_set_target(&slow, 100);

	/* the gains are normalised to a 20 ms cycle, so running the loop at 40 ms
	 * must halve the per-step P contribution to keep the overall response the
	 * same */
	uv_pid_step(&fast, PID_STEP_MS, 0);
	uv_pid_step(&slow, PID_STEP_MS * 2, 0);

	TEST_ASSERT_EQ(uv_pid_get_output(&fast), 100);
	TEST_ASSERT_EQ(uv_pid_get_output(&slow), 50);
}


TEST(pid, i_term_accumulates_the_error_over_time) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, PID_UNITY_P, 0);
	uv_pid_set_target(&pid, 10);

	/* with a constant error the integrator must ramp linearly */
	for (int32_t i = 1; i <= 10; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
		TEST_ASSERT_EQ(uv_pid_get_output(&pid), 10 * i);
	}
}


TEST(pid, i_term_unwinds_when_the_error_reverses) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, PID_UNITY_P, 0);
	uv_pid_set_target(&pid, 10);

	for (uint32_t i = 0; i < 10; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
	}
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 100);

	/* an equal and opposite error must wind the integrator back down */
	for (uint32_t i = 0; i < 10; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 20);
	}
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);
}


TEST(pid, i_term_is_clamped_to_max_sum) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, PID_UNITY_P, 0);
	uv_pid_set_target(&pid, 100);
	uv_pid_set_max_sum(&pid, 500);

	/* integrator windup is the classic way a closed loop slams an actuator to
	 * its end stop after a long period of unreachable target, so the clamp
	 * must hold no matter how long the error persists */
	for (uint32_t i = 0; i < 1000; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
		TEST_ASSERT_RANGE(uv_pid_get_output(&pid), 0, 500);
	}
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 500);
}


TEST(pid, i_term_is_clamped_to_min_sum) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, PID_UNITY_P, 0);
	uv_pid_set_target(&pid, -100);
	uv_pid_set_min_sum(&pid, -500);

	for (uint32_t i = 0; i < 1000; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
		TEST_ASSERT_RANGE(uv_pid_get_output(&pid), -500, 0);
	}
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), -500);
}


TEST(pid, i_term_defaults_to_int16_limits) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, PID_UNITY_P, 0);
	uv_pid_set_target(&pid, 1000);

	for (uint32_t i = 0; i < 1000; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
	}
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), INT16_MAX);
}


TEST(pid, d_term_reacts_to_error_change_only) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, 0, PID_UNITY_D);
	uv_pid_set_target(&pid, 100);

	/* first step: the error jumps from 0 to 100 */
	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 100);

	/* the error is now constant, so the derivative is zero */
	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);

	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);

	/* the error shrinking by 30 is a derivative of -30 */
	uv_pid_step(&pid, PID_STEP_MS, 30);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), -30);
}


TEST(pid, terms_sum_together) {
	uv_pid_st p_only;
	uv_pid_st i_only;
	uv_pid_st d_only;
	uv_pid_st all;

	uv_pid_init(&p_only, PID_UNITY_P, 0, 0);
	uv_pid_init(&i_only, 0, PID_UNITY_P, 0);
	uv_pid_init(&d_only, 0, 0, PID_UNITY_D);
	uv_pid_init(&all, PID_UNITY_P, PID_UNITY_P, PID_UNITY_D);

	uv_pid_set_target(&p_only, 100);
	uv_pid_set_target(&i_only, 100);
	uv_pid_set_target(&d_only, 100);
	uv_pid_set_target(&all, 100);

	for (uint32_t i = 0; i < 5; i++) {
		uv_pid_step(&p_only, PID_STEP_MS, 10);
		uv_pid_step(&i_only, PID_STEP_MS, 10);
		uv_pid_step(&d_only, PID_STEP_MS, 10);
		uv_pid_step(&all, PID_STEP_MS, 10);
	}

	TEST_ASSERT_EQ(uv_pid_get_output(&all),
			uv_pid_get_output(&p_only) + uv_pid_get_output(&i_only) +
			uv_pid_get_output(&d_only));
}


TEST(pid, zero_gains_give_zero_output) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, 0, 0);
	uv_pid_set_target(&pid, 1000);

	for (uint32_t i = 0; i < 20; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
		TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);
	}
}


TEST(pid, reset_clears_the_integrator) {
	uv_pid_st pid;
	uv_pid_init(&pid, 0, PID_UNITY_P, 0);
	uv_pid_set_target(&pid, 10);

	for (uint32_t i = 0; i < 10; i++) {
		uv_pid_step(&pid, PID_STEP_MS, 0);
	}
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 100);

	uv_pid_reset(&pid);

	/* the accumulated history is gone, so the next step starts from scratch */
	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 10);
}


TEST(pid, getters_return_the_configured_gains) {
	uv_pid_st pid;
	uv_pid_init(&pid, 1, 2, 3);

	TEST_ASSERT_EQ(uv_pid_get_p(&pid), 1);
	TEST_ASSERT_EQ(uv_pid_get_i(&pid), 2);
	TEST_ASSERT_EQ(uv_pid_get_d(&pid), 3);

	uv_pid_set_p(&pid, 10);
	uv_pid_set_i(&pid, 20);
	uv_pid_set_d(&pid, 30);

	TEST_ASSERT_EQ(uv_pid_get_p(&pid), 10);
	TEST_ASSERT_EQ(uv_pid_get_i(&pid), 20);
	TEST_ASSERT_EQ(uv_pid_get_d(&pid), 30);
}


TEST(pid, enable_after_disable_resumes_control) {
	uv_pid_st pid;
	uv_pid_init(&pid, PID_UNITY_P, 0, 0);
	uv_pid_set_target(&pid, 0);

	uv_pid_disable(&pid);
	uv_pid_step(&pid, PID_STEP_MS, 0);

	uv_pid_enable(&pid);
	uv_pid_set_target(&pid, 100);
	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 100);
}


/* uv_pid_disable() requests a shutdown by moving to PID_STATE_OFF_REQ. Once the
 * output has decayed inside PID_OFF_REQ_TOLERANCE, uv_pid_step() moves to
 * PID_STATE_OFF and takes the `on == false` branch, which calls uv_pid_reset().
 *
 * uv_pid_reset() used to assign `state = PID_STATE_ON`, so the controller
 * re-enabled itself on the very step that was supposed to shut it down. It now
 * only clears the accumulated history, so the header's documented behaviour -
 * "When disabling, PID drives itself to zero, then remains there" - holds. */
TEST(pid, stays_disabled_once_it_has_reached_zero) {
	uv_pid_st pid;
	uv_pid_init(&pid, PID_UNITY_P, 0, 0);
	uv_pid_set_target(&pid, 0);

	/* settle at zero, then request a shutdown */
	uv_pid_step(&pid, PID_STEP_MS, 0);
	uv_pid_disable(&pid);

	/* this step observes |output| < PID_OFF_REQ_TOLERANCE and switches off */
	uv_pid_step(&pid, PID_STEP_MS, 0);

	/* a disabled controller must not act on an error appearing at its input */
	uv_pid_step(&pid, PID_STEP_MS, 500);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);
}


/* Same root cause as the test above: clearing the accumulated history and
 * changing the enable state are now separate concerns, so resetting a
 * deliberately disabled controller no longer silently re-enables it. */
TEST(pid, reset_does_not_re_enable_a_disabled_controller) {
	uv_pid_st pid;
	uv_pid_init(&pid, PID_UNITY_P, 0, 0);
	uv_pid_set_target(&pid, 0);
	uv_pid_disable(&pid);

	uv_pid_reset(&pid);

	uv_pid_set_target(&pid, 100);
	uv_pid_step(&pid, PID_STEP_MS, 0);
	TEST_ASSERT_EQ(uv_pid_get_output(&pid), 0);
}
