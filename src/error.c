/*
 * MIT License
 *
 * Copyright (C) 2018 emekoi
 * Copyright (C) 2016-2018 Alexander Saprykin <saprykin.spb@gmail.com>
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


#include <stdlib.h>
#include <string.h>
#include "error.h"

#ifndef _WINDOWS
  #if defined(__OS2__)
    #define INCL_DOSERRORS
    #include <os2.h>
    #include <sys/socket.h>
  #endif
  #include <errno.h>
#else
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
#endif

static struct {
  int32_t code;
  int32_t native_code;
  char *message;
} CURRENT_ERROR = {0};

ErrorIO error_get_io_from_system(int32_t err_code) {
  switch (err_code) {
    case 0:
      return ERROR_IO_NONE;
  #if defined(_WINDOWS)
    #ifdef WSAEADDRINUSE
    case WSAEADDRINUSE:
      return ERROR_IO_ADDRESS_IN_USE;
    #endif
    #ifdef WSAEWOULDBLOCK
    case WSAEWOULDBLOCK:
      return ERROR_IO_WOULD_BLOCK;
    #endif
    #ifdef WSAEACCES
    case WSAEACCES:
      return ERROR_IO_ACCESS_DENIED;
    #endif
    #ifdef WSA_INVALID_HANDLE
    case WSA_INVALID_HANDLE:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef WSA_INVALID_PARAMETER
    case WSA_INVALID_PARAMETER:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef WSAEBADF
    case WSAEBADF:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef WSAENOTSOCK
    case WSAENOTSOCK:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef WSAEINVAL
    case WSAEINVAL:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef WSAESOCKTNOSUPPORT
    case WSAESOCKTNOSUPPORT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
    #ifdef WSAEOPNOTSUPP
    case WSAEOPNOTSUPP:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
    #ifdef WSAEPFNOSUPPORT
    case WSAEPFNOSUPPORT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
    #ifdef WSAEAFNOSUPPORT
    case WSAEAFNOSUPPORT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
    #ifdef WSAEPROTONOSUPPORT
    case WSAEPROTONOSUPPORT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
    #ifdef WSAECANCELLED
    case WSAECANCELLED:
      return ERROR_IO_ABORTED;
    #endif
    #ifdef ERROR_ALREADY_EXISTS
    case ERROR_ALREADY_EXISTS:
      return ERROR_IO_EXISTS;
    #endif
    #ifdef ERROR_FILE_NOT_FOUND
    case ERROR_FILE_NOT_FOUND:
      return ERROR_IO_NOT_EXISTS;
    #endif
    #ifdef ERROR_NO_MORE_FILES
    case ERROR_NO_MORE_FILES:
      return ERROR_IO_NO_MORE;
    #endif
    #ifdef ERROR_ACCESS_DENIED
    case ERROR_ACCESS_DENIED:
      return ERROR_IO_ACCESS_DENIED;
    #endif
    #ifdef ERROR_OUTOFMEMORY
    case ERROR_OUTOFMEMORY:
      return ERROR_IO_NO_RESOURCES;
    #endif
    #ifdef ERROR_NOT_ENOUGH_MEMORY
    case ERROR_NOT_ENOUGH_MEMORY:
      return ERROR_IO_NO_RESOURCES;
    #endif
    #ifdef ERROR_INVALID_HANDLE
      #if !defined(WSA_INVALID_HANDLE) || (ERROR_INVALID_HANDLE != WSA_INVALID_HANDLE)
    case ERROR_INVALID_HANDLE:
      return ERROR_IO_INVALID_ARGUMENT;
      #endif
    #endif
    #ifdef ERROR_INVALID_PARAMETER
      #if !defined(WSA_INVALID_PARAMETER) || (ERROR_INVALID_PARAMETER != WSA_INVALID_PARAMETER)
    case ERROR_INVALID_PARAMETER:
      return ERROR_IO_INVALID_ARGUMENT;
      #endif
    #endif
    #ifdef ERROR_NOT_SUPPORTED
    case ERROR_NOT_SUPPORTED:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
  #elif defined(__OS2__)
    #ifdef ERROR_FILE_NOT_FOUND
    case ERROR_FILE_NOT_FOUND:
      return ERROR_IO_NOT_EXISTS;
    #endif
    #ifdef ERROR_PATH_NOT_FOUND
    case ERROR_PATH_NOT_FOUND:
      return ERROR_IO_NOT_EXISTS;
    #endif
    #ifdef ERROR_TOO_MANY_OPEN_FILES
    case ERROR_TOO_MANY_OPEN_FILES:
      return ERROR_IO_NO_RESOURCES;
    #endif
    #ifdef ERROR_NOT_ENOUGH_MEMORY
    case ERROR_NOT_ENOUGH_MEMORY:
      return ERROR_IO_NO_RESOURCES;
    #endif
    #ifdef ERROR_ACCESS_DENIED
    case ERROR_ACCESS_DENIED:
      return ERROR_IO_ACCESS_DENIED;
    #endif
    #ifdef ERROR_INVALID_HANDLE
    case ERROR_INVALID_HANDLE:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef ERROR_INVALID_PARAMETER
    case ERROR_INVALID_PARAMETER:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef ERROR_INVALID_ADDRESS
    case ERROR_INVALID_ADDRESS:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif
    #ifdef ERROR_NO_MORE_FILES
    case ERROR_NO_MORE_FILES:
      return ERROR_IO_NO_MORE;
    #endif
    #ifdef ERROR_NOT_SUPPORTED
    case ERROR_NOT_SUPPORTED:
      return ERROR_IO_NOT_SUPPORTED;
    #endif
    #ifdef ERROR_FILE_EXISTS
    case ERROR_FILE_EXISTS:
      return ERROR_IO_EXISTS;
    #endif
  #else /* !_WINDOWS && !__OS2__ */
    #ifdef EACCES
    case EACCES:
      return ERROR_IO_ACCESS_DENIED;
    #endif

    #ifdef EPERM
    case EPERM:
      return ERROR_IO_ACCESS_DENIED;
    #endif

    #ifdef ENOMEM
    case ENOMEM:
      return ERROR_IO_NO_RESOURCES;
    #endif

    #ifdef ENOSR
    case ENOSR:
      return ERROR_IO_NO_RESOURCES;
    #endif

    #ifdef ENOBUFS
    case ENOBUFS:
      return ERROR_IO_NO_RESOURCES;
    #endif

    #ifdef ENFILE
    case ENFILE:
      return ERROR_IO_NO_RESOURCES;
    #endif

    #ifdef ENOSPC
    case ENOSPC:
      return ERROR_IO_NO_RESOURCES;
    #endif

    #ifdef EMFILE
    case EMFILE:
      return ERROR_IO_NO_RESOURCES;
    #endif

    #ifdef EINVAL
    case EINVAL:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif

    #ifdef EBADF
    case EBADF:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif

    #ifdef ENOTSOCK
    case ENOTSOCK:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif

    #ifdef EFAULT
    case EFAULT:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif

    #ifdef EPROTOTYPE
    case EPROTOTYPE:
      return ERROR_IO_INVALID_ARGUMENT;
    #endif

    /* On Linux these errors can have same codes */
    #if defined(ENOTSUP) && (!defined(EOPNOTSUPP) || ENOTSUP != EOPNOTSUPP)
    case ENOTSUP:
        return ERROR_IO_NOT_SUPPORTED;
    #endif

    #ifdef ENOPROTOOPT
    case ENOPROTOOPT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif

    #ifdef EPROTONOSUPPORT
    case EPROTONOSUPPORT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif

    #ifdef EAFNOSUPPORT
    case EAFNOSUPPORT:
      return ERROR_IO_NOT_SUPPORTED;
    #endif

    #ifdef EOPNOTSUPP
    case EOPNOTSUPP:
      return ERROR_IO_NOT_SUPPORTED;
    #endif

    #ifdef EADDRNOTAVAIL
    case EADDRNOTAVAIL:
      return ERROR_IO_NOT_AVAILABLE;
    #endif

    #ifdef ENETUNREACH
    case ENETUNREACH:
      return ERROR_IO_NOT_AVAILABLE;
    #endif

    #ifdef ENETDOWN
    case ENETDOWN:
      return ERROR_IO_NOT_AVAILABLE;
    #endif

    #ifdef EHOSTDOWN
    case EHOSTDOWN:
      return ERROR_IO_NOT_AVAILABLE;
    #endif

    #ifdef ENONET
    case ENONET:
      return ERROR_IO_NOT_AVAILABLE;
    #endif

    #ifdef EHOSTUNREACH
    case EHOSTUNREACH:
      return ERROR_IO_NOT_AVAILABLE;
    #endif

    #ifdef EINPROGRESS
    case EINPROGRESS:
      return ERROR_IO_IN_PROGRESS;
    #endif

    #ifdef EALREADY
    case EALREADY:
      return ERROR_IO_IN_PROGRESS;
    #endif

    #ifdef EISCONN
    case EISCONN:
      return ERROR_IO_CONNECTED;
    #endif

    #ifdef ECONNREFUSED
    case ECONNREFUSED:
      return ERROR_IO_CONNECTION_REFUSED;
    #endif

    #ifdef ENOTCONN
    case ENOTCONN:
      return ERROR_IO_NOT_CONNECTED;
    #endif

    #ifdef ECONNABORTED
    case ECONNABORTED:
      return ERROR_IO_ABORTED;
    #endif

    #ifdef EADDRINUSE
    case EADDRINUSE:
      return ERROR_IO_ADDRESS_IN_USE;
    #endif

    #ifdef ETIMEDOUT
    case ETIMEDOUT:
      return ERROR_IO_TIMED_OUT;
    #endif

    #ifdef EDQUOT
    case EDQUOT:
      return ERROR_IO_QUOTA;
    #endif

    #ifdef EISDIR
    case EISDIR:
      return ERROR_IO_IS_DIRECTORY;
    #endif

    #ifdef ENOTDIR
    case ENOTDIR:
      return ERROR_IO_NOT_DIRECTORY;
    #endif

    #ifdef EEXIST
    case EEXIST:
      return ERROR_IO_EXISTS;
    #endif

    #ifdef ENOENT
    case ENOENT:
      return ERROR_IO_NOT_EXISTS;
    #endif

    #ifdef ENAMETOOLONG
    case ENAMETOOLONG:
      return ERROR_IO_NAMETOOLONG;
    #endif

    #ifdef ENOSYS
    case ENOSYS:
      return ERROR_IO_NOT_IMPLEMENTED;
    #endif

    /* Some magic to deal with EWOULDBLOCK and EAGAIN.
     * Apparently on HP-UX these are actually definedto different values,
     * but on Linux, for example, they are the same. */
    #if defined(EWOULDBLOCK) && defined(EAGAIN) && EWOULDBLOCK == EAGAIN
    /* We have both and they are the same: only emit one case. */
    case EAGAIN:
      return ERROR_IO_WOULD_BLOCK;
    #else
    /* Else: consider each of them separately. This handles both the
     * case of having only one and the case where they are different values. */
      #ifdef EAGAIN
    case EAGAIN:
      return ERROR_IO_WOULD_BLOCK;
      #endif

      #ifdef EWOULDBLOCK
    case EWOULDBLOCK:
      return ERROR_IO_WOULD_BLOCK;
      #endif
    #endif
  #endif /* !_WINDOWS */
    default:
      return ERROR_IO_FAILED;
  }
}

ErrorIO error_get_last_io(void) {
  return error_get_io_from_system(error_get_last_system());
}

const char *error_code_to_string(ErrorIO error) {
  switch (error) {
    case ERROR_IO_NONE: return "No error.";
    case ERROR_IO_NO_RESOURCES: return "Operating system hasn't enough resources.";
    case ERROR_IO_NOT_AVAILABLE: return "Resource isn't available.";
    case ERROR_IO_ACCESS_DENIED: return "Access denied.";
    case ERROR_IO_CONNECTED: return "Already connected.";
    case ERROR_IO_IN_PROGRESS: return "Operation in progress.";
    case ERROR_IO_ABORTED: return "Operation aborted.";
    case ERROR_IO_INVALID_ARGUMENT: return "Invalid argument specified.";
    case ERROR_IO_NOT_SUPPORTED: return "Operation not supported.";
    case ERROR_IO_TIMED_OUT: return "Operation timed out.";
    case ERROR_IO_WOULD_BLOCK: return "Operation cannot be completed immediatly.";
    case ERROR_IO_ADDRESS_IN_USE: return "Address is already under usage.";
    case ERROR_IO_CONNECTION_REFUSED: return "Connection refused.";
    case ERROR_IO_NOT_CONNECTED: return "Connection required first.";
    case ERROR_IO_QUOTA: return "User quota exceeded.";
    case ERROR_IO_IS_DIRECTORY: return "Trying to open directory for writing.";
    case ERROR_IO_NOT_DIRECTORY: return "Component of the path prefix is not a directory. ";
    case ERROR_IO_NAMETOOLONG: return "Specified name is too long.";
    case ERROR_IO_EXISTS: return "Specified entry already exists.";
    case ERROR_IO_NOT_EXISTS: return "Specified entry doesn't exist.";
    case ERROR_IO_NO_MORE: return "No more data left.";
    case ERROR_IO_NOT_IMPLEMENTED: return "Operation is not implemented.";
    case ERROR_IO_FAILED: return "General error.";
  }
  return "?";
}

const char *error_get_message(void) {
  return CURRENT_ERROR.message;
}

int32_t error_get_code(void) {
  return CURRENT_ERROR.code;
}

int32_t error_get_native_code(void) {
  return CURRENT_ERROR.native_code;
}

void error_set_error(int32_t code, int32_t native_code, const char *message) {
  if (CURRENT_ERROR.message != NULL) {
    free(CURRENT_ERROR.message);
  }

  CURRENT_ERROR.code = code;
  CURRENT_ERROR.native_code = native_code;
  CURRENT_ERROR.message = strdup(message);
}

void error_set_code(int32_t code) {
  CURRENT_ERROR.code = code;
}

void error_set_native_code(int32_t native_code) {
	CURRENT_ERROR.native_code = native_code;
}

void error_set_message(const char *message) {
  if (CURRENT_ERROR.message != NULL) {
    free (CURRENT_ERROR.message);
  }

  CURRENT_ERROR.message = strdup(message);
}

void error_clear(void) {
  if (CURRENT_ERROR.message != NULL) {
    free(CURRENT_ERROR.message);
  }

  CURRENT_ERROR.message = NULL;
  CURRENT_ERROR.code = 0;
  CURRENT_ERROR.native_code = 0;
}

int32_t error_get_last_system(void) {
#ifdef _WINDOWS
  return (int32_t)GetLastError();
#else
  #if !defined(VMS) || !defined(__VMS)
  int32_t error_code = errno;

  if (error_code == EVMSERR)
    return vaxc$errno;
  else
    return error_code;
  #else
  return errno;
  #endif
#endif
}

int32_t error_get_last_net(void) {
#if defined(_WINDOWS)
  return WSAGetLastError();
#elif defined(__OS2__)
  return sock_errno();
#else
  return error_get_last_system();
#endif
}

void error_set_last_system(int32_t code) {
#ifdef _WINDOWS
  SetLastError(code);
#else
  errno = code;
#endif
}

void error_set_last_net(int32_t code) {
#if defined(_WINDOWS)
  WSASetLastError(code);
#elif defined(__OS2__)
  UNUSED(code);
#else
  errno = code;
#endif
}