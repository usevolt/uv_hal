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

#ifndef UV_HAL_TESTS_UV_TEST_H_
#define UV_HAL_TESTS_UV_TEST_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/// @file: A minimal host-side unit test framework for uv_hal.
///
/// This framework is deliberately tiny and dependency free: it is compiled with
/// the host gcc, it has no external library requirements, and the whole runner
/// is a single binary. Test cases register themselves automatically, so adding a
/// test never requires touching any other file.
///
/// Note that this header is *test* code and is never compiled into firmware, so
/// unlike the rest of uv_hal it is free to use host-only facilities such as
/// constructor attributes, longjmp and variadic macros.
///
/// @example:
///		TEST(filters, hysteresis_triggers_above_trigger_value) {
///			uv_hysteresis_st h;
///			uv_hysteresis_init(&h, 100, 20, false);
///			TEST_ASSERT_FALSE(uv_hysteresis_step(&h, 50));
///			TEST_ASSERT_TRUE(uv_hysteresis_step(&h, 150));
///			TEST_ASSERT_EQ(uv_hysteresis_get_output(&h), 1);
///		}


typedef void (*uv_test_fn_t)(void);


/// @brief: Registers a single test case. Called automatically by the TEST macros,
/// there should be no reason to call this directly.
///
/// @param suite: The name of the group this test belongs to
/// @param name: The name of the test case, unique inside the suite
/// @param fn: The test body
/// @param xfail: If true, the test is expected to fail. See TEST_XFAIL.
void uv_test_register(const char *suite, const char *name,
		uv_test_fn_t fn, bool xfail);


/// @brief: Reports a failed assertion and aborts the running test case.
/// Called by the TEST_ASSERT_* macros.
void uv_test_fail(const char *file, int line, const char *fmt, ...)
		__attribute__((format(printf, 3, 4))) __attribute__((noreturn));


/// @brief: Defines a test case and registers it to the runner.
///
/// @param suite_: Group name, used for filtering on the command line
/// @param name_: Test case name
#define TEST(suite_, name_) \
	static void uv_test_body_##suite_##_##name_(void); \
	static void __attribute__((constructor)) \
			uv_test_reg_##suite_##_##name_(void) { \
		uv_test_register(#suite_, #name_, \
				&uv_test_body_##suite_##_##name_, false); \
	} \
	static void uv_test_body_##suite_##_##name_(void)


/// @brief: Defines a test case that is *expected to fail*, and registers it.
///
/// Use this to pin down behaviour that is believed to be a defect: the test
/// asserts what the code *should* do, and the runner reports it as XFAIL instead
/// of failing the build. If the underlying defect is ever fixed the runner
/// reports XPASS and fails, which forces the test to be promoted to a plain
/// TEST. This way a known bug is documented in executable form without either
/// freezing the buggy behaviour into an assertion or breaking CI.
///
/// Always reference the reason in a comment above the test.
#define TEST_XFAIL(suite_, name_) \
	static void uv_test_body_##suite_##_##name_(void); \
	static void __attribute__((constructor)) \
			uv_test_reg_##suite_##_##name_(void) { \
		uv_test_register(#suite_, #name_, \
				&uv_test_body_##suite_##_##name_, true); \
	} \
	static void uv_test_body_##suite_##_##name_(void)


/// @brief: Unconditionally fails the running test with a printf-style message
#define TEST_FAIL(...) \
	uv_test_fail(__FILE__, __LINE__, __VA_ARGS__)


#define TEST_ASSERT_TRUE(cond_) \
	do { \
		if (!(cond_)) { \
			uv_test_fail(__FILE__, __LINE__, \
					"expected TRUE: %s", #cond_); \
		} \
	} while (0)


#define TEST_ASSERT_FALSE(cond_) \
	do { \
		if ((cond_)) { \
			uv_test_fail(__FILE__, __LINE__, \
					"expected FALSE: %s", #cond_); \
		} \
	} while (0)


/// @brief: Asserts that two integer expressions are equal
#define TEST_ASSERT_EQ(actual_, expected_) \
	do { \
		int64_t uv_test_a_ = (int64_t) (actual_); \
		int64_t uv_test_e_ = (int64_t) (expected_); \
		if (uv_test_a_ != uv_test_e_) { \
			uv_test_fail(__FILE__, __LINE__, \
					"%s: expected %lld, got %lld", \
					#actual_, (long long) uv_test_e_, \
					(long long) uv_test_a_); \
		} \
	} while (0)


/// @brief: Asserts that two integer expressions differ
#define TEST_ASSERT_NE(actual_, expected_) \
	do { \
		int64_t uv_test_a_ = (int64_t) (actual_); \
		int64_t uv_test_e_ = (int64_t) (expected_); \
		if (uv_test_a_ == uv_test_e_) { \
			uv_test_fail(__FILE__, __LINE__, \
					"%s: expected anything but %lld", \
					#actual_, (long long) uv_test_e_); \
		} \
	} while (0)


/// @brief: Asserts that an integer expression is within *tol_* of *expected_*.
/// Useful for filters and controllers where the exact fixed point rounding is
/// an implementation detail but the settling value is not.
#define TEST_ASSERT_NEAR(actual_, expected_, tol_) \
	do { \
		int64_t uv_test_a_ = (int64_t) (actual_); \
		int64_t uv_test_e_ = (int64_t) (expected_); \
		int64_t uv_test_t_ = (int64_t) (tol_); \
		int64_t uv_test_d_ = (uv_test_a_ > uv_test_e_) ? \
				(uv_test_a_ - uv_test_e_) : (uv_test_e_ - uv_test_a_); \
		if (uv_test_d_ > uv_test_t_) { \
			uv_test_fail(__FILE__, __LINE__, \
					"%s: expected %lld +-%lld, got %lld (off by %lld)", \
					#actual_, (long long) uv_test_e_, \
					(long long) uv_test_t_, (long long) uv_test_a_, \
					(long long) uv_test_d_); \
		} \
	} while (0)


/// @brief: Asserts that an integer expression lies inside an inclusive range
#define TEST_ASSERT_RANGE(actual_, min_, max_) \
	do { \
		int64_t uv_test_a_ = (int64_t) (actual_); \
		int64_t uv_test_min_ = (int64_t) (min_); \
		int64_t uv_test_max_ = (int64_t) (max_); \
		if ((uv_test_a_ < uv_test_min_) || (uv_test_a_ > uv_test_max_)) { \
			uv_test_fail(__FILE__, __LINE__, \
					"%s: expected %lld ... %lld, got %lld", \
					#actual_, (long long) uv_test_min_, \
					(long long) uv_test_max_, (long long) uv_test_a_); \
		} \
	} while (0)


#define TEST_ASSERT_STR_EQ(actual_, expected_) \
	do { \
		const char *uv_test_a_ = (actual_); \
		const char *uv_test_e_ = (expected_); \
		if ((uv_test_a_ == NULL) || (uv_test_e_ == NULL) || \
				(strcmp(uv_test_a_, uv_test_e_) != 0)) { \
			uv_test_fail(__FILE__, __LINE__, \
					"%s: expected \"%s\", got \"%s\"", \
					#actual_, \
					(uv_test_e_ != NULL) ? uv_test_e_ : "(null)", \
					(uv_test_a_ != NULL) ? uv_test_a_ : "(null)"); \
		} \
	} while (0)


#define TEST_ASSERT_NULL(ptr_) \
	do { \
		if ((ptr_) != NULL) { \
			uv_test_fail(__FILE__, __LINE__, \
					"expected NULL: %s", #ptr_); \
		} \
	} while (0)


#define TEST_ASSERT_NOT_NULL(ptr_) \
	do { \
		if ((ptr_) == NULL) { \
			uv_test_fail(__FILE__, __LINE__, \
					"expected non-NULL: %s", #ptr_); \
		} \
	} while (0)


#endif /* UV_HAL_TESTS_UV_TEST_H_ */
