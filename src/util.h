#pragma once

#include <stdio.h>
#include <stdint.h>

#define ALERT_WARNING(msg) printf ("** Warning: %s **\n", msg)
#define ALERT_ERROR(msg) printf ("** Error: %s **\n", msg)
#define ALERT_DEBUG(msg) printf ("** Debug: %s **\n", msg)

#ifndef __has_builtin
#  define __has_builtin(x) 0
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

#include "error.h"

