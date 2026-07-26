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
#include "uv_utilities.h"

#include <string.h>

/// @file: Tests for uv_utilities: the delay helper, the ring buffer and vector
/// containers, and the integer maths helpers.
///
/// These are the most widely reused pieces of uv_hal - the maths helpers in
/// particular are called from the graph, mapping and sensor scaling paths on
/// every device - so they are the highest value thing in the HAL to pin down.


/* ---------------------------------------------------------------------------
 * uv_delay
 * ------------------------------------------------------------------------ */

TEST(delay, does_not_end_before_the_time_has_passed) {
	uv_delay_st delay;
	uv_delay_init(&delay, 100);

	for (uint32_t i = 0; i < 5; i++) {
		TEST_ASSERT_FALSE(uv_delay(&delay, 20));
		TEST_ASSERT_FALSE(uv_delay_has_ended(&delay));
	}
}


TEST(delay, ends_exactly_once) {
	uv_delay_st delay;
	uv_delay_init(&delay, 100);
	uint32_t triggers = 0;

	/* the caller typically runs `if (uv_delay(&d, step_ms)) { ...do once... }`,
	 * so returning true more than once would run the action repeatedly */
	for (uint32_t i = 0; i < 50; i++) {
		if (uv_delay(&delay, 20)) {
			triggers++;
		}
	}
	TEST_ASSERT_EQ(triggers, 1);
	TEST_ASSERT_TRUE(uv_delay_has_ended(&delay));
}


TEST(delay, zero_length_delay_ends_on_the_first_step) {
	uv_delay_st delay;
	uv_delay_init(&delay, 0);

	TEST_ASSERT_TRUE(uv_delay(&delay, 20));
	TEST_ASSERT_TRUE(uv_delay_has_ended(&delay));
	TEST_ASSERT_FALSE(uv_delay(&delay, 20));
}


TEST(delay, end_stops_the_delay_from_triggering) {
	uv_delay_st delay;
	uv_delay_init(&delay, 100);

	uv_delay_end(&delay);
	TEST_ASSERT_TRUE(uv_delay_has_ended(&delay));

	for (uint32_t i = 0; i < 50; i++) {
		TEST_ASSERT_FALSE(uv_delay(&delay, 20));
	}
}


TEST(delay, trigger_ends_the_delay_immediately) {
	uv_delay_st delay;
	/* uv_delay_init takes the length as a uint16_t, so 60 s is the practical
	 * maximum a single delay can express */
	uv_delay_init(&delay, 60000);

	uv_delay_trigger(&delay);
	TEST_ASSERT_TRUE(uv_delay_triggered(&delay));
	TEST_ASSERT_TRUE(uv_delay(&delay, 20));
	TEST_ASSERT_TRUE(uv_delay_has_ended(&delay));
}


TEST(delay, a_null_pointer_is_tolerated) {
	/* uv_delay is called from step functions that may hold an optional delay */
	TEST_ASSERT_TRUE(uv_delay(NULL, 20));
}


TEST(delay, get_time_returns_the_remaining_time) {
	uv_delay_st delay;
	uv_delay_init(&delay, 100);

	TEST_ASSERT_EQ(uv_delay_get_time(&delay), 100);
	(void) uv_delay(&delay, 20);
	TEST_ASSERT_EQ(uv_delay_get_time(&delay), 80);
}


/* ---------------------------------------------------------------------------
 * uv_ring_buffer
 * ------------------------------------------------------------------------ */

TEST(ring_buffer, starts_empty) {
	int32_t storage[4] = { 0 };
	uv_ring_buffer_st rb;
	uv_ring_buffer_init(&rb, storage, 4, sizeof(int32_t));

	TEST_ASSERT_TRUE(uv_ring_buffer_empty(&rb));
	TEST_ASSERT_FALSE(uv_ring_buffer_is_full(&rb));
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_count(&rb), 0);
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_max_count(&rb), 4);
}


TEST(ring_buffer, pops_in_fifo_order) {
	int32_t storage[4] = { 0 };
	uv_ring_buffer_st rb;
	int32_t out;
	uv_ring_buffer_init(&rb, storage, 4, sizeof(int32_t));

	for (int32_t i = 1; i <= 4; i++) {
		TEST_ASSERT_EQ(uv_ring_buffer_push(&rb, &i), ERR_NONE);
	}
	TEST_ASSERT_TRUE(uv_ring_buffer_is_full(&rb));

	for (int32_t i = 1; i <= 4; i++) {
		TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &out), ERR_NONE);
		TEST_ASSERT_EQ(out, i);
	}
	TEST_ASSERT_TRUE(uv_ring_buffer_empty(&rb));
}


TEST(ring_buffer, push_reports_overflow_when_full) {
	int32_t storage[2] = { 0 };
	uv_ring_buffer_st rb;
	int32_t val = 7;
	uv_ring_buffer_init(&rb, storage, 2, sizeof(int32_t));

	TEST_ASSERT_EQ(uv_ring_buffer_push(&rb, &val), ERR_NONE);
	TEST_ASSERT_EQ(uv_ring_buffer_push(&rb, &val), ERR_NONE);
	TEST_ASSERT_EQ(uv_ring_buffer_push(&rb, &val), ERR_BUFFER_OVERFLOW);
	/* the rejected push must not have corrupted the count */
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_count(&rb), 2);
}


TEST(ring_buffer, pop_reports_empty) {
	int32_t storage[2] = { 0 };
	uv_ring_buffer_st rb;
	int32_t out;
	uv_ring_buffer_init(&rb, storage, 2, sizeof(int32_t));

	TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &out), ERR_BUFFER_EMPTY);
	TEST_ASSERT_EQ(uv_ring_buffer_peek(&rb, &out), ERR_BUFFER_EMPTY);
}


TEST(ring_buffer, peek_does_not_consume) {
	int32_t storage[4] = { 0 };
	uv_ring_buffer_st rb;
	int32_t val = 42;
	int32_t out = 0;
	uv_ring_buffer_init(&rb, storage, 4, sizeof(int32_t));

	uv_ring_buffer_push(&rb, &val);
	TEST_ASSERT_EQ(uv_ring_buffer_peek(&rb, &out), ERR_NONE);
	TEST_ASSERT_EQ(out, 42);
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_count(&rb), 1);

	out = 0;
	TEST_ASSERT_EQ(uv_ring_buffer_peek(&rb, &out), ERR_NONE);
	TEST_ASSERT_EQ(out, 42);
}


TEST(ring_buffer, wraps_around_the_end_of_the_storage) {
	int32_t storage[3] = { 0 };
	uv_ring_buffer_st rb;
	int32_t out;
	uv_ring_buffer_init(&rb, storage, 3, sizeof(int32_t));

	/* push and pop enough times that head and tail both wrap several times */
	for (int32_t i = 0; i < 100; i++) {
		int32_t in = i;
		TEST_ASSERT_EQ(uv_ring_buffer_push(&rb, &in), ERR_NONE);
		TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &out), ERR_NONE);
		TEST_ASSERT_EQ(out, i);
	}
	TEST_ASSERT_TRUE(uv_ring_buffer_empty(&rb));
}


TEST(ring_buffer, push_force_drops_the_oldest_element) {
	int32_t storage[3] = { 0 };
	uv_ring_buffer_st rb;
	int32_t out;
	uv_ring_buffer_init(&rb, storage, 3, sizeof(int32_t));

	for (int32_t i = 1; i <= 5; i++) {
		uv_ring_buffer_push_force(&rb, &i);
	}
	/* only the three newest survive, still in oldest-first order */
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_count(&rb), 3);
	for (int32_t i = 3; i <= 5; i++) {
		TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &out), ERR_NONE);
		TEST_ASSERT_EQ(out, i);
	}
}


TEST(ring_buffer, clear_empties_the_buffer) {
	int32_t storage[4] = { 0 };
	uv_ring_buffer_st rb;
	int32_t val = 1;
	uv_ring_buffer_init(&rb, storage, 4, sizeof(int32_t));

	uv_ring_buffer_push(&rb, &val);
	uv_ring_buffer_push(&rb, &val);
	uv_ring_buffer_clear(&rb);

	TEST_ASSERT_TRUE(uv_ring_buffer_empty(&rb));
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_max_count(&rb), 4);

	/* and the buffer must still be usable afterwards */
	val = 9;
	TEST_ASSERT_EQ(uv_ring_buffer_push(&rb, &val), ERR_NONE);
	val = 0;
	TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &val), ERR_NONE);
	TEST_ASSERT_EQ(val, 9);
}


TEST(ring_buffer, pop_front_reports_empty) {
	int32_t storage[2] = { 0 };
	uv_ring_buffer_st rb;
	int32_t out;
	uv_ring_buffer_init(&rb, storage, 2, sizeof(int32_t));

	TEST_ASSERT_EQ(uv_ring_buffer_pop_front(&rb, &out), ERR_BUFFER_EMPTY);
}


/* uv_ring_buffer_pop_front() is the counterpart of uv_ring_buffer_pop() (alias
 * uv_ring_buffer_pop_back()): pop takes the oldest element from the tail, so
 * pop_front takes the *newest* element from the head.
 *
 * It used to copy out the element the head pointed at - the next free slot,
 * never written - and then advance the head forwards, the same direction a push
 * moves it, returning uninitialised data and leaving head/tail/element_count
 * inconsistent. uvcan/hhead_dia.c relies on this call for the "reverse feeding"
 * case of the diameter-vs-length measurement buffer, so that corrupted the
 * recorded diameter profile the bucking decisions are made from. */
TEST(ring_buffer, pop_front_removes_the_newest_element) {
	int32_t storage[4] = { 0 };
	uv_ring_buffer_st rb;
	int32_t out = 0;
	uv_ring_buffer_init(&rb, storage, 4, sizeof(int32_t));

	for (int32_t i = 1; i <= 3; i++) {
		uv_ring_buffer_push(&rb, &i);
	}

	TEST_ASSERT_EQ(uv_ring_buffer_pop_front(&rb, &out), ERR_NONE);
	TEST_ASSERT_EQ(out, 3);
	TEST_ASSERT_EQ(uv_ring_buffer_get_element_count(&rb), 2);

	/* the remaining elements must still come out oldest first */
	TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &out), ERR_NONE);
	TEST_ASSERT_EQ(out, 1);
	TEST_ASSERT_EQ(uv_ring_buffer_pop(&rb, &out), ERR_NONE);
	TEST_ASSERT_EQ(out, 2);
	TEST_ASSERT_TRUE(uv_ring_buffer_empty(&rb));
}


/* ---------------------------------------------------------------------------
 * uv_vector
 * ------------------------------------------------------------------------ */

TEST(vector, starts_empty) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	TEST_ASSERT_EQ(uv_vector_size(&vec), 0);
	TEST_ASSERT_EQ(uv_vector_max_size(&vec), 4);
}


TEST(vector, push_back_appends_in_order) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	for (int32_t i = 1; i <= 4; i++) {
		TEST_ASSERT_EQ(uv_vector_push_back(&vec, &i), ERR_NONE);
		TEST_ASSERT_EQ(uv_vector_size(&vec), i);
	}
	for (int32_t i = 0; i < 4; i++) {
		TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, i), i + 1);
	}
}


TEST(vector, push_back_reports_overflow) {
	int32_t storage[2] = { 0 };
	uv_vector_st vec;
	int32_t val = 5;
	uv_vector_init(&vec, storage, 2, sizeof(int32_t));

	TEST_ASSERT_EQ(uv_vector_push_back(&vec, &val), ERR_NONE);
	TEST_ASSERT_EQ(uv_vector_push_back(&vec, &val), ERR_NONE);
	TEST_ASSERT_EQ(uv_vector_push_back(&vec, &val), ERR_BUFFER_OVERFLOW);
	TEST_ASSERT_EQ(uv_vector_size(&vec), 2);
}


TEST(vector, pop_back_returns_the_last_element) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	int32_t out;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	for (int32_t i = 1; i <= 3; i++) {
		uv_vector_push_back(&vec, &i);
	}
	for (int32_t i = 3; i >= 1; i--) {
		TEST_ASSERT_EQ(uv_vector_pop_back(&vec, &out), ERR_NONE);
		TEST_ASSERT_EQ(out, i);
	}
	TEST_ASSERT_EQ(uv_vector_size(&vec), 0);
	TEST_ASSERT_EQ(uv_vector_pop_back(&vec, &out), ERR_BUFFER_EMPTY);
}


TEST(vector, insert_places_the_element_at_the_index) {
	int32_t storage[5] = { 0 };
	uv_vector_st vec;
	int32_t val;
	uv_vector_init(&vec, storage, 5, sizeof(int32_t));

	for (int32_t i = 1; i <= 3; i++) {
		uv_vector_push_back(&vec, &i);
	}

	val = 99;
	TEST_ASSERT_EQ(uv_vector_insert(&vec, 1, &val), ERR_NONE);

	TEST_ASSERT_EQ(uv_vector_size(&vec), 4);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 0), 1);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 1), 99);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 2), 2);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 3), 3);
}


TEST(vector, insert_past_the_end_appends) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	int32_t val = 1;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	uv_vector_push_back(&vec, &val);
	val = 77;
	TEST_ASSERT_EQ(uv_vector_insert(&vec, 100, &val), ERR_NONE);

	TEST_ASSERT_EQ(uv_vector_size(&vec), 2);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 1), 77);
}


TEST(vector, remove_deletes_a_range) {
	int32_t storage[6] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 6, sizeof(int32_t));

	for (int32_t i = 1; i <= 5; i++) {
		uv_vector_push_back(&vec, &i);
	}

	TEST_ASSERT_EQ(uv_vector_remove(&vec, 1, 2), ERR_NONE);

	TEST_ASSERT_EQ(uv_vector_size(&vec), 3);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 0), 1);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 1), 4);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 2), 5);
}


TEST(vector, remove_rejects_an_out_of_range_request) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	for (int32_t i = 1; i <= 2; i++) {
		uv_vector_push_back(&vec, &i);
	}
	TEST_ASSERT_EQ(uv_vector_remove(&vec, 1, 5), ERR_INDEX_OVERFLOW);
	TEST_ASSERT_EQ(uv_vector_size(&vec), 2);
}


TEST(vector, clear_empties_the_vector) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	int32_t val = 1;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	uv_vector_push_back(&vec, &val);
	uv_vector_push_back(&vec, &val);
	uv_vector_clear(&vec);

	TEST_ASSERT_EQ(uv_vector_size(&vec), 0);
}


static int compare_int32(void *element1, void *element2) {
	int32_t a = *(int32_t*) element1;
	int32_t b = *(int32_t*) element2;
	int ret;

	if (a > b) {
		ret = 1;
	}
	else if (a < b) {
		ret = -1;
	}
	else {
		ret = 0;
	}
	return ret;
}


TEST(vector, binary_search_finds_every_element) {
	int32_t storage[8] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 8, sizeof(int32_t));

	for (int32_t i = 0; i < 8; i++) {
		int32_t val = i * 10;
		uv_vector_push_back(&vec, &val);
	}

	for (int32_t i = 0; i < 8; i++) {
		int32_t match = i * 10;
		void *found = uv_vector_binary_search(&vec, &match, &compare_int32);
		TEST_ASSERT_NOT_NULL(found);
		TEST_ASSERT_EQ(*(int32_t*) found, i * 10);
	}
}


TEST(vector, binary_search_returns_null_when_not_found) {
	int32_t storage[8] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 8, sizeof(int32_t));

	for (int32_t i = 0; i < 8; i++) {
		int32_t val = i * 10;
		uv_vector_push_back(&vec, &val);
	}

	int32_t match = 35;
	TEST_ASSERT_NULL(uv_vector_binary_search(&vec, &match, &compare_int32));
	match = -1;
	TEST_ASSERT_NULL(uv_vector_binary_search(&vec, &match, &compare_int32));
	match = 1000;
	TEST_ASSERT_NULL(uv_vector_binary_search(&vec, &match, &compare_int32));
}


TEST(vector, binary_search_on_an_empty_vector_returns_null) {
	int32_t storage[4] = { 0 };
	uv_vector_st vec;
	int32_t match = 1;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	TEST_ASSERT_NULL(uv_vector_binary_search(&vec, &match, &compare_int32));
}


/* uv_vector_push_front() used to decrement this->len instead of incrementing
 * it, so pushing an element made the vector report one element *fewer* - and on
 * an empty vector the unsigned length wrapped to 65535, after which every
 * subsequent write landed far outside the caller's buffer. It also moved only a
 * single element out of the way rather than the whole existing contents. */
TEST(vector, push_front_prepends_and_grows_the_vector) {
	int32_t storage[8] = { 0 };
	uv_vector_st vec;
	int32_t val;
	uv_vector_init(&vec, storage, 8, sizeof(int32_t));

	/* start non-empty so that the buggy len-- cannot wrap and run wild */
	for (int32_t i = 1; i <= 3; i++) {
		uv_vector_push_back(&vec, &i);
	}

	val = 99;
	TEST_ASSERT_EQ(uv_vector_push_front(&vec, &val), ERR_NONE);

	TEST_ASSERT_EQ(uv_vector_size(&vec), 4);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 0), 99);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 1), 1);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 2), 2);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 3), 3);
}


/* uv_vector_pop_front() used to memmove() only a single element_size worth of
 * data instead of the (len - 1) elements that follow the one being removed, so
 * everything from index 1 onwards kept its old position while the length shrank
 * and the vector silently lost and duplicated elements. */
TEST(vector, pop_front_shifts_the_remaining_elements_down) {
	int32_t storage[8] = { 0 };
	uv_vector_st vec;
	int32_t out = 0;
	uv_vector_init(&vec, storage, 8, sizeof(int32_t));

	for (int32_t i = 1; i <= 4; i++) {
		uv_vector_push_back(&vec, &i);
	}

	TEST_ASSERT_EQ(uv_vector_pop_front(&vec, &out), ERR_NONE);
	TEST_ASSERT_EQ(out, 1);
	TEST_ASSERT_EQ(uv_vector_size(&vec), 3);

	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 0), 2);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 1), 3);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 2), 4);
}


/* uv_vector_remove() used to compute the number of trailing elements to move as
 * (len - index - (count - 1)), one more than the (len - index - count) that
 * actually follow the removed range, so it always read one element past the end
 * of the live data - and past the end of the caller's buffer entirely when the
 * vector was full. The visible contents came out correct either way, which is
 * exactly why this needed a test to catch: it was a silent out of bounds read. */
TEST(vector, remove_does_not_read_past_the_end_of_the_buffer) {
	/* storage is deliberately one element larger than the vector so that the
	 * over-read lands on a canary instead of unrelated memory */
	int32_t storage[5] = { 0 };
	uv_vector_st vec;
	uv_vector_init(&vec, storage, 4, sizeof(int32_t));

	for (int32_t i = 1; i <= 4; i++) {
		uv_vector_push_back(&vec, &i);
	}
	storage[4] = 0x5A5A5A5A;

	TEST_ASSERT_EQ(uv_vector_remove(&vec, 0, 1), ERR_NONE);

	TEST_ASSERT_EQ(uv_vector_size(&vec), 3);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 0), 2);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 1), 3);
	TEST_ASSERT_EQ(*(int32_t*) uv_vector_at(&vec, 2), 4);

	/* the canary sits one element past the vector's own buffer, so a correct
	 * implementation can never have copied it in */
	TEST_ASSERT_NE(storage[3], 0x5A5A5A5A);
}


/* ---------------------------------------------------------------------------
 * integer maths helpers
 * ------------------------------------------------------------------------ */

TEST(math, lerpi_interpolates_between_the_endpoints) {
	TEST_ASSERT_EQ(uv_lerpi(0, 10, 20), 10);
	TEST_ASSERT_EQ(uv_lerpi(1000, 10, 20), 20);
	TEST_ASSERT_EQ(uv_lerpi(500, 10, 20), 15);
	TEST_ASSERT_EQ(uv_lerpi(250, 0, 1000), 250);
	TEST_ASSERT_EQ(uv_lerpi(500, -100, 100), 0);
}


TEST(math, lerpi_handles_a_descending_range) {
	TEST_ASSERT_EQ(uv_lerpi(0, 100, 0), 100);
	TEST_ASSERT_EQ(uv_lerpi(1000, 100, 0), 0);
	TEST_ASSERT_EQ(uv_lerpi(500, 100, 0), 50);
}


TEST(math, reli_returns_parts_per_thousand) {
	TEST_ASSERT_EQ(uv_reli(0, 0, 100), 0);
	TEST_ASSERT_EQ(uv_reli(50, 0, 100), 500);
	TEST_ASSERT_EQ(uv_reli(100, 0, 100), 1000);
	TEST_ASSERT_EQ(uv_reli(-50, -100, 100), 250);
}


TEST(math, reli_guards_against_a_zero_width_range) {
	/* the graph module feeds user configured points straight into uv_reli, so
	 * two points sharing an x value must not divide by zero */
	TEST_ASSERT_EQ(uv_reli(5, 5, 5), 0);
	TEST_ASSERT_EQ(uv_reli(0, 5, 5), 0);
}


TEST(math, reli_is_the_inverse_of_lerpi) {
	for (int32_t v = 0; v <= 1000; v += 25) {
		TEST_ASSERT_EQ(uv_lerpi(uv_reli(v, 0, 1000), 0, 1000), v);
	}
}


TEST(math, reli_extrapolates_outside_the_range) {
	/* callers such as the graph module clamp the result themselves, so this
	 * documents that uv_reli itself does not */
	TEST_ASSERT_EQ(uv_reli(150, 0, 100), 1500);
	TEST_ASSERT_EQ(uv_reli(-50, 0, 100), -500);
}


TEST(math, maxi_and_mini) {
	TEST_ASSERT_EQ(uv_maxi(1, 2), 2);
	TEST_ASSERT_EQ(uv_maxi(2, 1), 2);
	TEST_ASSERT_EQ(uv_maxi(-1, -2), -1);
	TEST_ASSERT_EQ(uv_maxi(5, 5), 5);
	TEST_ASSERT_EQ(uv_mini(1, 2), 1);
	TEST_ASSERT_EQ(uv_mini(2, 1), 1);
	TEST_ASSERT_EQ(uv_mini(-1, -2), -2);
	TEST_ASSERT_EQ(uv_mini(5, 5), 5);
}


TEST(math, ctz_counts_trailing_zeroes) {
	TEST_ASSERT_EQ(uv_ctz(1), 0);
	TEST_ASSERT_EQ(uv_ctz(2), 1);
	TEST_ASSERT_EQ(uv_ctz(3), 0);
	TEST_ASSERT_EQ(uv_ctz(8), 3);
	TEST_ASSERT_EQ(uv_ctz(12), 2);
	TEST_ASSERT_EQ(uv_ctz(0x80000000u), 31);
	/* no bits set at all */
	TEST_ASSERT_EQ(uv_ctz(0), 32);
}


TEST(math, ctz_matches_every_single_bit_position) {
	/* the mapping module uses uv_ctz to turn an input/output bit flag back into
	 * an index, so every bit position has to be exact */
	for (uint32_t i = 0; i < 32; i++) {
		TEST_ASSERT_EQ(uv_ctz((uint32_t) 1 << i), i);
	}
}


TEST(math, countofbit_counts_set_and_cleared_bits) {
	TEST_ASSERT_EQ(uv_countofbit(0, 1), 0);
	TEST_ASSERT_EQ(uv_countofbit(0, 0), 32);
	TEST_ASSERT_EQ(uv_countofbit(0xFFFFFFFFu, 1), 32);
	TEST_ASSERT_EQ(uv_countofbit(0xFFFFFFFFu, 0), 0);
	TEST_ASSERT_EQ(uv_countofbit(0xF, 1), 4);
	TEST_ASSERT_EQ(uv_countofbit(0xF, 0), 28);
	/* any non-zero value is treated as "set" */
	TEST_ASSERT_EQ(uv_countofbit(0xF, 7), 4);
}


TEST(math, isqrt_of_perfect_squares_is_exact) {
	TEST_ASSERT_EQ(uv_isqrt(0), 0);
	TEST_ASSERT_EQ(uv_isqrt(1), 1);
	TEST_ASSERT_EQ(uv_isqrt(4), 2);
	TEST_ASSERT_EQ(uv_isqrt(100), 10);
	TEST_ASSERT_EQ(uv_isqrt(10000), 100);
	TEST_ASSERT_EQ(uv_isqrt(1000000), 1000);
}


TEST(math, isqrt_rounds_to_the_nearest_integer) {
	TEST_ASSERT_EQ(uv_isqrt(2), 1);
	TEST_ASSERT_EQ(uv_isqrt(3), 2);
	TEST_ASSERT_EQ(uv_isqrt(6), 2);
	TEST_ASSERT_EQ(uv_isqrt(7), 3);
	TEST_ASSERT_EQ(uv_isqrt(15), 4);
}


TEST(math, isqrt_handles_large_values) {
	TEST_ASSERT_EQ(uv_isqrt((uint64_t) 1 << 40), (uint64_t) 1 << 20);
	TEST_ASSERT_EQ(uv_isqrt((uint64_t) 4000000000u * 4000000000u),
			(uint64_t) 4000000000u);
}


TEST(math, isqrt_never_strays_more_than_one_from_the_true_root) {
	for (uint64_t n = 0; n < 5000; n++) {
		uint64_t r = uv_isqrt(n);
		/* r must be the nearest integer root, so r*r brackets n closely */
		TEST_ASSERT_TRUE((r * r <= n + r) && (n <= (r + 1) * (r + 1)));
	}
}


TEST(math, isdigit_accepts_only_decimal_digits) {
	for (char c = '0'; c <= '9'; c++) {
		TEST_ASSERT_TRUE(uv_isdigit(c));
	}
	TEST_ASSERT_FALSE(uv_isdigit('a'));
	TEST_ASSERT_FALSE(uv_isdigit('Z'));
	TEST_ASSERT_FALSE(uv_isdigit('/'));
	TEST_ASSERT_FALSE(uv_isdigit(':'));
	TEST_ASSERT_FALSE(uv_isdigit(' '));
	TEST_ASSERT_FALSE(uv_isdigit('\0'));
}


TEST(math, ntouint_decodes_big_endian_wire_data) {
	uint8_t raw16[2] = { 0x12, 0x34 };
	uint8_t raw32[4] = { 0x12, 0x34, 0x56, 0x78 };
	uint8_t raw64[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
	uint16_t n16;
	uint32_t n32;
	uint64_t n64;

	memcpy(&n16, raw16, sizeof(n16));
	memcpy(&n32, raw32, sizeof(n32));
	memcpy(&n64, raw64, sizeof(n64));

	TEST_ASSERT_EQ(ntouint16(n16), 0x1234);
	TEST_ASSERT_EQ(ntouint32(n32), 0x12345678);
	TEST_ASSERT_EQ((int64_t) ntouint64(n64), (int64_t) 0x0123456789ABCDEFll);
}


TEST(math, limits_macro_clamps_both_ends) {
	int32_t val;

	val = 150;
	LIMITS(val, 0, 100);
	TEST_ASSERT_EQ(val, 100);

	val = -50;
	LIMITS(val, 0, 100);
	TEST_ASSERT_EQ(val, 0);

	val = 50;
	LIMITS(val, 0, 100);
	TEST_ASSERT_EQ(val, 50);
}


TEST(math, limit_min_and_max_macros) {
	int32_t val;

	val = 5;
	LIMIT_MIN(val, 10);
	TEST_ASSERT_EQ(val, 10);

	val = 20;
	LIMIT_MIN(val, 10);
	TEST_ASSERT_EQ(val, 20);

	val = 20;
	LIMIT_MAX(val, 10);
	TEST_ASSERT_EQ(val, 10);

	val = 5;
	LIMIT_MAX(val, 10);
	TEST_ASSERT_EQ(val, 5);
}
