/*
 * MIT License
 *
 * Copyright (C) 2018 emekoi
 * Copyright (C) 2010-2016 Alexander Saprykin <saprykin.spb@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#pragma once

#include <stdio.h>
#include <stdint.h>

#define ALERT_WARNING(msg, ...) do { \
	printf("** Warning [line %d] **\n", __LINE__); \
	printf(msg, ##__VA_ARGS__); \
	printf("\n\n"); \
} while(0)

#define ALERT_ERROR(msg, ...) do { \
	printf("** Error [line %d] **\n", __LINE__); \
	printf(msg, ##__VA_ARGS__); \
	printf("\n\n"); \
} while(0)

#define ALERT_DEBUG(msg, ...) do { \
	printf("** Debug [line %d] **\n", __LINE__); \
	printf(msg, ##__VA_ARGS__); \
	printf("\n\n"); \
} while(0)

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif

#define UNUSED(x) ((void) (x))

#if (defined(__GNUC__) && (__GNUC__ > 2 && __GNUC_MINOR__ > 0)) || \
    (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 800) || \
    (defined(__xlc__) || defined(__xlC__) && __xlC__ >= 0x0900) || \
    __has_builtin(__builtin_expect)
	#define LIKELY(x) __builtin_expect(!!(x), 1)
	#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
	#define LIKELY(x) (x)
	#define UNLIKELY(x) (x)
#endif

#if !defined(__x86_64__)
  #if defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) || \
  defined(__sparc64__) || \
	defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || \
	defined(__64BIT__) || defined(_LP64) || defined(__LP64__)
		#define __x86_64__
	#endif
#endif

#if defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) || \
    defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
  #define _WINDOWS
#endif

int32_t sys_close(int32_t pid);
