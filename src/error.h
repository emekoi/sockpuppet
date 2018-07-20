#pragma once

#include "util.h"

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
  ERROR_IO_FAILED              = 522  /* General error. */
} ErrorIO;

ErrorIO	error_get_io_from_system(int32_t err_code);
ErrorIO	error_get_last_io();
const char *error_get_message();
int32_t error_get_code();
int32_t error_get_native_code();
void error_set_error(int32_t code, int32_t native_code, const char *message);
void error_set_code(int32_t code);
void error_set_native_code(int32_t native_code);
void error_set_message(const char *message);
void error_clear();
int32_t error_get_last_system();
int32_t error_get_last_net();
void error_set_last_system(int32_t code);
void error_set_last_net(int32_t code);
