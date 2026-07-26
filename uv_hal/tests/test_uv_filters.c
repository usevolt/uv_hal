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
#include "uv_filters.h"

/// @file: Tests for uv_filters: the moving average, EWMA and hysteresis filters.
///
/// These three filters sit between practically every analogue input and the
/// control logic on every Usevolt device, so a regression here is silent and
/// wide reaching: a sensor reading that settles to the wrong value or an
/// on/off threshold that chatters looks like a hardware fault in the field.


/* ---------------------------------------------------------------------------
 * uv_moving_aver
 *
 * Note that despite the name this is not a true windowed moving average: it
 * keeps a single running sum and bleeds off one averaged element per step
 * instead of the actual oldest sample. That makes it behave as a leaky
 * integrator. The tests below cover the properties that the callers rely on
 * (steady state accuracy and monotone convergence) rather than the exact
 * intermediate values, which are an artefact of that implementation.
 * ------------------------------------------------------------------------ */

TEST(moving_aver, constant_input_gives_constant_output) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 4);

	for (uint32_t i = 0; i < 50; i++) {
		TEST_ASSERT_EQ(uv_moving_aver_step(&avr, 100), 100);
	}
}


TEST(moving_aver, first_sample_passes_through) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 8);

	/* with an empty filter there is nothing to average against, so the very
	 * first sample must come straight out */
	TEST_ASSERT_EQ(uv_moving_aver_step(&avr, 1234), 1234);
}


TEST(moving_aver, converges_to_a_step_change) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 10);

	for (uint32_t i = 0; i < 20; i++) {
		(void) uv_moving_aver_step(&avr, 0);
	}
	TEST_ASSERT_EQ(uv_moving_aver_get_val(&avr), 0);

	/* step the input up and let the filter settle */
	int32_t out = 0;
	for (uint32_t i = 0; i < 500; i++) {
		out = uv_moving_aver_step(&avr, 1000);
	}
	TEST_ASSERT_NEAR(out, 1000, 1);
}


TEST(moving_aver, output_never_overshoots_the_input_range) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 6);

	/* alternating extremes must stay bounded by those extremes */
	for (uint32_t i = 0; i < 200; i++) {
		int32_t out = uv_moving_aver_step(&avr, ((i % 2) == 0) ? -500 : 500);
		TEST_ASSERT_RANGE(out, -500, 500);
	}
}


TEST(moving_aver, converges_monotonically_when_rising) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 10);
	int32_t prev = uv_moving_aver_step(&avr, 0);

	for (uint32_t i = 0; i < 300; i++) {
		int32_t out = uv_moving_aver_step(&avr, 1000);
		TEST_ASSERT_TRUE(out >= prev);
		prev = out;
	}
	TEST_ASSERT_NEAR(prev, 1000, 1);
}


TEST(moving_aver, is_full_reports_when_enough_samples_were_fed) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 5);

	TEST_ASSERT_FALSE(uv_moving_aver_is_full(&avr));
	for (uint32_t i = 0; i < 4; i++) {
		(void) uv_moving_aver_step(&avr, 10);
		TEST_ASSERT_FALSE(uv_moving_aver_is_full(&avr));
	}
	(void) uv_moving_aver_step(&avr, 10);
	TEST_ASSERT_TRUE(uv_moving_aver_is_full(&avr));

	/* and it must stay full from then on, never wrapping back around */
	for (uint32_t i = 0; i < 20; i++) {
		(void) uv_moving_aver_step(&avr, 10);
		TEST_ASSERT_TRUE(uv_moving_aver_is_full(&avr));
	}
}


TEST(moving_aver, reset_clears_the_accumulated_value) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 4);

	for (uint32_t i = 0; i < 20; i++) {
		(void) uv_moving_aver_step(&avr, 800);
	}
	TEST_ASSERT_NE(uv_moving_aver_get_val(&avr), 0);

	uv_moving_aver_reset(&avr);
	TEST_ASSERT_EQ(uv_moving_aver_get_val(&avr), 0);
	TEST_ASSERT_FALSE(uv_moving_aver_is_full(&avr));
	/* after a reset the next sample passes through again */
	TEST_ASSERT_EQ(uv_moving_aver_step(&avr, 55), 55);
}


TEST(moving_aver, handles_negative_values) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 4);

	for (uint32_t i = 0; i < 50; i++) {
		TEST_ASSERT_EQ(uv_moving_aver_step(&avr, -100), -100);
	}
}


TEST(moving_aver, set_count_rejects_zero) {
	uv_moving_aver_st avr;
	uv_moving_aver_init(&avr, 4);

	/* a count of zero would make the filter meaningless, so the setter
	 * substitutes 1 */
	uv_moving_aver_set_count(&avr, 0);
	for (uint32_t i = 0; i < 10; i++) {
		(void) uv_moving_aver_step(&avr, 100);
	}
	TEST_ASSERT_NEAR(uv_moving_aver_get_val(&avr), 100, 1);
}


/* uv_moving_aver_init() used to assign the count straight into the struct while
 * uv_moving_aver_set_count() substituted 1 for a count of 0, so the two entry
 * points disagreed. init() now routes through the setter. */
TEST(moving_aver, init_rejects_zero_count_like_set_count_does) {
	uv_moving_aver_st a;
	uv_moving_aver_st b;

	uv_moving_aver_init(&a, 0);

	uv_moving_aver_init(&b, 4);
	uv_moving_aver_set_count(&b, 0);

	TEST_ASSERT_EQ(a.count, b.count);
}


/* ---------------------------------------------------------------------------
 * uv_ewma
 * ------------------------------------------------------------------------ */

TEST(ewma, zero_tau_passes_the_input_straight_through) {
	uv_ewma_st ewma;
	uv_ewma_init(&ewma, 0, 0);

	TEST_ASSERT_EQ(uv_ewma_step(&ewma, 500, 20), 500);
	TEST_ASSERT_EQ(uv_ewma_step(&ewma, -500, 20), -500);
	TEST_ASSERT_EQ(uv_ewma_step(&ewma, 0, 20), 0);
}


TEST(ewma, init_value_is_the_starting_output) {
	uv_ewma_st ewma;
	uv_ewma_init(&ewma, 1000, 750);

	TEST_ASSERT_EQ(uv_ewma_get_val(&ewma), 750);
}


TEST(ewma, converges_exactly_to_a_constant_input) {
	uv_ewma_st ewma;
	uv_ewma_init(&ewma, 1000, 0);

	for (uint32_t i = 0; i < 1000; i++) {
		(void) uv_ewma_step(&ewma, 1234, 20);
	}
	/* An exponential filter implemented in integer maths can stall short of
	 * the target once the per-step delta rounds down to zero. uv_ewma_step
	 * guards against that by snapping to the input when the state stops
	 * changing, so the filter must land exactly on the target rather than
	 * leaving a permanent offset. */
	TEST_ASSERT_EQ(uv_ewma_get_val(&ewma), 1234);
}


TEST(ewma, converges_exactly_to_a_small_constant_input) {
	uv_ewma_st ewma;
	uint32_t steps = 0;
	uv_ewma_init(&ewma, 10000, 0);

	/* A long time constant combined with a tiny target is the case most likely
	 * to stall in fixed point: the per-step delta rounds away long before the
	 * output reaches the target. Rather than guessing a step count, run until
	 * the filter settles and assert that it does settle - the loop bound only
	 * exists so that a regression fails instead of hanging. */
	while ((uv_ewma_get_val(&ewma) != 3) && (steps < 5000000)) {
		(void) uv_ewma_step(&ewma, 3, 1);
		steps++;
	}
	TEST_ASSERT_EQ(uv_ewma_get_val(&ewma), 3);
}


TEST(ewma, larger_tau_filters_more_heavily) {
	uv_ewma_st fast;
	uv_ewma_st slow;
	uv_ewma_init(&fast, 200, 0);
	uv_ewma_init(&slow, 5000, 0);

	int32_t fast_out = 0;
	int32_t slow_out = 0;
	for (uint32_t i = 0; i < 5; i++) {
		fast_out = uv_ewma_step(&fast, 1000, 20);
		slow_out = uv_ewma_step(&slow, 1000, 20);
	}
	TEST_ASSERT_TRUE(fast_out > slow_out);
}


TEST(ewma, output_stays_within_the_input_range) {
	uv_ewma_st ewma;
	uv_ewma_init(&ewma, 2000, 0);

	for (uint32_t i = 0; i < 500; i++) {
		int32_t out = uv_ewma_step(&ewma, ((i % 2) == 0) ? -1000 : 1000, 20);
		TEST_ASSERT_RANGE(out, -1000, 1000);
	}
}


TEST(ewma, reacts_faster_with_a_longer_step_time) {
	uv_ewma_st slow_cycle;
	uv_ewma_st fast_cycle;
	uv_ewma_init(&slow_cycle, 1000, 0);
	uv_ewma_init(&fast_cycle, 1000, 0);

	/* alpha is proportional to step_ms / tau, so one 100 ms step must move
	 * further than one 10 ms step */
	int32_t big_step = uv_ewma_step(&slow_cycle, 1000, 100);
	int32_t small_step = uv_ewma_step(&fast_cycle, 1000, 10);

	TEST_ASSERT_TRUE(big_step > small_step);
}


TEST(ewma, handles_negative_targets) {
	uv_ewma_st ewma;
	uv_ewma_init(&ewma, 500, 0);

	for (uint32_t i = 0; i < 500; i++) {
		(void) uv_ewma_step(&ewma, -800, 20);
	}
	TEST_ASSERT_EQ(uv_ewma_get_val(&ewma), -800);
}


TEST(ewma, set_tau_takes_effect) {
	uv_ewma_st ewma;
	uv_ewma_init(&ewma, 100000, 0);
	uv_ewma_set_tau(&ewma, 0);

	/* tau 0 means "no filtering", so the next step should pass through */
	TEST_ASSERT_EQ(uv_ewma_step(&ewma, 640, 20), 640);
}


/* ---------------------------------------------------------------------------
 * uv_hysteresis
 * ------------------------------------------------------------------------ */

TEST(hysteresis, starts_low_when_not_inverted) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, false);

	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);
}


TEST(hysteresis, triggers_above_the_trigger_value) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, false);

	/* below the trigger, nothing happens */
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 50));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);

	/* exactly at the trigger is not yet above it */
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 100));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);

	/* above the trigger the output goes high, and the step function reports
	 * the change for exactly one cycle */
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 101));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 101));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
}


TEST(hysteresis, releases_only_below_trigger_minus_hysteresis) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, false);

	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 150));

	/* inside the hysteresis band the output must hold */
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 99));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 80));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);

	/* one below the band it releases */
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 79));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);
}


TEST(hysteresis, inverted_starts_high) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, true);

	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
}


TEST(hysteresis, inverted_releases_above_trigger_plus_hysteresis) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, true);

	/* inside the band, hold */
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 110));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 120));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);

	/* above the band it drops */
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 121));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);

	/* and comes back below the trigger value */
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 100));
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 99));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
}


TEST(hysteresis, does_not_chatter_around_the_trigger_value) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, false);
	uint32_t changes = 0;

	/* a noisy signal sitting right on the trigger value must not toggle the
	 * output once the hysteresis band has been entered */
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 150));
	for (uint32_t i = 0; i < 100; i++) {
		if (uv_hysteresis_step(&hyst, ((i % 2) == 0) ? 95 : 105)) {
			changes++;
		}
	}
	TEST_ASSERT_EQ(changes, 0);
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
}


TEST(hysteresis, zero_hysteresis_toggles_at_the_trigger_value) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 0, 0, false);

	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 1));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, -1));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);
}


TEST(hysteresis, set_trigger_value_moves_the_threshold) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 10, false);

	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 50));
	uv_hysteresis_set_trigger_value(&hyst, 20);
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 50));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
}


TEST(hysteresis, works_with_negative_trigger_values) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, -100, 20, false);

	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, -200));
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, -99));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, -119));
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, -121));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 0);
}
