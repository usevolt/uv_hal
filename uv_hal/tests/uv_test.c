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

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/// @brief: The maximum number of test cases the runner can hold. Bumping this is
/// free, it only grows a static table in the test binary.
#define UV_TEST_MAX_COUNT		512
#define UV_TEST_MSG_LEN			512

/// @brief: Shown in the runner banner. This framework is shared - the uvcan
/// submodule builds its own suite against it - so each suite's makefile names
/// itself here.
#ifndef UV_TEST_SUITE_NAME
#define UV_TEST_SUITE_NAME		"uv_hal"
#endif


typedef struct {
	const char *suite;
	const char *name;
	uv_test_fn_t fn;
	bool xfail;
} uv_test_case_st;


static uv_test_case_st tests[UV_TEST_MAX_COUNT];
static unsigned int test_count = 0;

/// @brief: Jump target used to abort a test case on the first failed assertion
static jmp_buf fail_jmp;
static char fail_msg[UV_TEST_MSG_LEN];

static const char *col_red = "";
static const char *col_green = "";
static const char *col_yellow = "";
static const char *col_dim = "";
static const char *col_off = "";



void uv_test_register(const char *suite, const char *name,
		uv_test_fn_t fn, bool xfail) {
	if (test_count >= UV_TEST_MAX_COUNT) {
		fprintf(stderr,
				"uv_test: test table full (%u), raise UV_TEST_MAX_COUNT\n",
				(unsigned int) UV_TEST_MAX_COUNT);
		exit(2);
	}
	else {
		tests[test_count].suite = suite;
		tests[test_count].name = name;
		tests[test_count].fn = fn;
		tests[test_count].xfail = xfail;
		test_count++;
	}
}



void uv_test_fail(const char *file, int line, const char *fmt, ...) {
	va_list args;
	int len = snprintf(fail_msg, sizeof(fail_msg), "%s:%i: ", file, line);

	if ((len < 0) || ((unsigned int) len >= sizeof(fail_msg))) {
		len = 0;
	}
	va_start(args, fmt);
	vsnprintf(fail_msg + len, sizeof(fail_msg) - (unsigned int) len, fmt, args);
	va_end(args);

	longjmp(fail_jmp, 1);
}



/// @brief: Returns true if *name* contains *pattern*, or if no pattern was given
static bool matches(const char *pattern, const char *name) {
	return (pattern == NULL) || (strstr(name, pattern) != NULL);
}



static void enable_colors(void) {
	if (isatty(STDOUT_FILENO)) {
		col_red = "\033[31m";
		col_green = "\033[32m";
		col_yellow = "\033[33m";
		col_dim = "\033[2m";
		col_off = "\033[0m";
	}
}



int main(int argc, char **argv) {
	const char *pattern = (argc > 1) ? argv[1] : NULL;
	unsigned int passed = 0;
	unsigned int failed = 0;
	unsigned int xfailed = 0;
	unsigned int xpassed = 0;
	unsigned int skipped = 0;
	int ret;

	enable_colors();

	printf("%s unit tests: %u registered%s%s%s\n\n",
			UV_TEST_SUITE_NAME, test_count,
			(pattern != NULL) ? ", filter \"" : "",
			(pattern != NULL) ? pattern : "",
			(pattern != NULL) ? "\"" : "");

	for (unsigned int i = 0; i < test_count; i++) {
		char full_name[256];
		bool test_failed;

		snprintf(full_name, sizeof(full_name), "%s.%s",
				tests[i].suite, tests[i].name);

		if (!matches(pattern, full_name)) {
			skipped++;
		}
		else {
			fail_msg[0] = '\0';

			if (setjmp(fail_jmp) == 0) {
				tests[i].fn();
				test_failed = false;
			}
			else {
				test_failed = true;
			}

			if (!test_failed && !tests[i].xfail) {
				printf("%s[  ok  ]%s %s\n", col_green, col_off, full_name);
				passed++;
			}
			else if (test_failed && tests[i].xfail) {
				/* known defect, still present: informational only */
				printf("%s[xfail ]%s %s\n%s         %s%s\n",
						col_yellow, col_off, full_name,
						col_dim, fail_msg, col_off);
				xfailed++;
			}
			else if (!test_failed && tests[i].xfail) {
				/* the documented defect is gone: the test must be promoted to
				 * a plain TEST() so that it guards the fix from now on */
				printf("%s[xpass ]%s %s\n"
						"         known defect appears to be FIXED - "
						"change TEST_XFAIL to TEST\n",
						col_red, col_off, full_name);
				xpassed++;
			}
			else {
				printf("%s[ FAIL ]%s %s\n         %s\n",
						col_red, col_off, full_name, fail_msg);
				failed++;
			}
		}
	}

	printf("\n%u passed, %u failed, %u xfail, %u xpass",
			passed, failed, xfailed, xpassed);
	if (skipped != 0) {
		printf(", %u filtered out", skipped);
	}
	printf("\n");

	if ((failed == 0) && (xpassed == 0)) {
		printf("%sALL TESTS PASSED%s\n", col_green, col_off);
		ret = 0;
	}
	else {
		printf("%sTESTS FAILED%s\n", col_red, col_off);
		ret = 1;
	}

	return ret;
}
