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

#include "util.h"
#include <stdint.h>

typedef enum {
  ERROR_IO_NONE                = 500,  /* No error. */
  ERROR_IO_NO_RESOURCES        = 501,  /* Operating system hasn't enough resources. */
  ERROR_IO_NOT_AVAILABLE       = 502,  /* Resource isn't available. */
  ERROR_IO_ACCESS_DENIED       = 503,  /* Access denied. */
  ERROR_IO_CONNECTED           = 504,  /* Already connected. */
  ERROR_IO_IN_PROGRESS         = 505,  /* Operation in progress. */
  ERROR_IO_ABORTED             = 506,  /* Operation aborted. */
  ERROR_IO_INVALID_ARGUMENT    = 507,  /* Invalid argument specified. */
  ERROR_IO_NOT_SUPPORTED       = 508,  /* Operation not supported. */
  ERROR_IO_TIMED_OUT           = 509,  /* Operation timed out. */
  ERROR_IO_WOULD_BLOCK         = 510,  /* Operation cannot be completed immediatly. */
  ERROR_IO_ADDRESS_IN_USE      = 511,  /* Address is already under usage. */
  ERROR_IO_CONNECTION_REFUSED  = 512,  /* Connection refused. */
  ERROR_IO_NOT_CONNECTED       = 513,  /* Connection required first. */
  ERROR_IO_QUOTA               = 514,  /* User quota exceeded. */
  ERROR_IO_IS_DIRECTORY        = 515,  /* Trying to open directory for writing. */
  ERROR_IO_NOT_DIRECTORY       = 516,  /* Component of the path prefix is not a directory.  */
  ERROR_IO_NAMETOOLONG         = 517,  /* Specified name is too long. */
  ERROR_IO_EXISTS              = 518,  /* Specified entry already exists. */
  ERROR_IO_NOT_EXISTS          = 519,  /* Specified entry doesn't exist. */
  ERROR_IO_NO_MORE             = 520,  /* No more data left. */
  ERROR_IO_NOT_IMPLEMENTED     = 521,  /* Operation is not implemented. */
  ERROR_IO_FAILED              = 522   /* General error. */
} ErrorIO;

ErrorIO	error_get_io_from_system(int32_t err_code);
ErrorIO	error_get_last_io(void);
const char *error_code_to_string(ErrorIO error);
const char *error_get_message(void);
int32_t error_get_code(void);
int32_t error_get_native_code(void);
void error_set_error(int32_t code, int32_t native_code, const char *message);
void error_set_code(int32_t code);
void error_set_native_code(int32_t native_code);
void error_set_message(const char *message);
void error_clear(void);
int32_t error_get_last_system(void);
int32_t error_get_last_net(void);
void error_set_last_system(int32_t code);
void error_set_last_net(int32_t code);
