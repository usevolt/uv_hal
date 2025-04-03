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


#include "uv_wdt.h"


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "uv_rtos.h"

#if !defined(PRINT)
#define PRINT(...) printf(__VA_ARGS__)
#endif


#if CONFIG_WDT

struct {
	int32_t counter;
} _this;
#define this (&_this)


static void *wdt_task(void *ptr) {
	while (true) {
		usleep(1000000);

		this->counter--;
		if (this->counter < 0) {
			PRINT("Watchdog timer triggered\n");
			break;
		}
	}
	return NULL;
}

#endif

void _uv_wdt_init(void) {
#if CONFIG_WDT
	pthread_t thread;
	pthread_create(&thread, NULL, &wdt_task, NULL);
	printf("watchdog timer started\n");
#endif
}

void uv_wdt_update(void) {
#if CONFIG_WDT
	this->counter++;
#endif
}



