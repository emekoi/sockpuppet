/*
 * The MIT License
 *
 * Copyright (C) 2010-2017 Alexander Saprykin <saprykin.spb@gmail.com>
 * Some workarounds have been used from Glib (comments are kept)
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

#include "socket.h"
#ifdef P_OS_SCO
#  include "ptimeprofiler.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifndef _WINDOWS
  #include <fcntl.h>
  #include <errno.h>
  #include <unistd.h>
  #include <signal.h>
  #if !defined(VMS) || !defined(__VMS)
    #include <stropts.h>
  #endif
#endif

#ifndef _WINDOWS
  #if defined(__BEOS__) || defined(__APPLE__) || defined(Macintosh) ||\
      defined(macintosh) || defined(__OS2__)  || defined(AMIGA) || defined(__amigaos__)
    #define SOCKET_USE_SELECT
    #include <sys/select.h>
    #include <sys/time.h>
  #else
    #define SOCKET_USE_POLL
    #include <sys/poll.h>
  #endif
#endif

/* On old Solaris systems SOMAXCONN is set to 5 */
#define SOCKET_DEFAULT_BACKLOG  5

struct Socket {
  SocketFamily family;
  SocketProtocol protocol;
  SocketType type;
  int32_t fd;
  int32_t listen_backlog;
  int32_t timeout;
  uint32_t blocking  : 1;
  uint32_t keepalive : 1;
  uint32_t closed    : 1;
  uint32_t connected : 1;
  uint32_t listening : 1;
#ifdef _WINDOWS
  WSAEVENT events;
#endif
#ifdef _SCO_DS
  PTimeProfiler *timer;
#endif
};

#ifndef SHUT_RD
	#define SHUT_RD 0
#endif

#ifndef SHUT_WR
	#define SHUT_WR 1
#endif

#ifndef SHUT_RDWR
	#define SHUT_RDWR 2
#endif

#ifdef MSG_NOSIGNAL
	#define SOCKET_DEFAULT_SEND_FLAGS MSG_NOSIGNAL
#else
	#define SOCKET_DEFAULT_SEND_FLAGS 0
#endif

static bool _XX__socket_set_fd_blocking(int32_t fd, bool blocking);
static bool _XX__socket_check(const Socket *socket);
static bool _XX__socket_set_details_from_fd(Socket *socket);

static bool _XX__socket_set_fd_blocking(int32_t fd, bool blocking) {
#ifndef _WINDOWS
  int32_t arg;
#else
  uint32_t arg;
#endif

#ifndef _WINDOWS
	#if !defined(VMS) || !defined(__VMS)
  arg = !blocking;
	#ifdef __x86_64__
		#pragma __pointer_size 32
	#endif
  /* Explicit (void *) cast is necessary */
  if (UNLIKELY(ioctl(fd, FIONBIO, (void *) &arg) < 0)) {
	#ifdef __x86_64__
		#pragma __pointer_size 64
	#endif
	#else
  if (UNLIKELY((arg = fcntl (fd, F_GETFL, NULL)) < 0)) {
    ALERT_WARNING("Socket::_XX__socket_set_fd_blocking: fcntl() failed");
    arg = 0;
  }

  arg = (!blocking) ? (arg | O_NONBLOCK) : (arg & ~O_NONBLOCK);

  if (UNLIKELY(fcntl(fd, F_SETFL, arg) < 0)) {
	#endif
#else
  arg = !blocking;

  if (UNLIKELY(ioctlsocket(fd, FIONBIO, &arg) == SOCKET_ERROR)) {
#endif
    error_set_error(
    	(int32_t)error_get_io_from_system(error_get_last_net()),
      (int32_t)error_get_last_net(),
      "Failed to set socket blocking flags"
     );
    return false;
  }

  return true;
}

static bool _XX__socket_check(const Socket *socket) {
  if (UNLIKELY(socket->closed))  {
    error_set_error((int32_t)ERROR_IO_NOT_AVAILABLE, 0, "Socket is already closed");
    return false;
  }

  return true;
}

static bool _XX__socket_set_details_from_fd(Socket *socket) {
#ifdef SO_DOMAIN
  SocketFamily family;
#endif
  struct sockaddr_storage  address;
  int32_t fd, value;
  socklen_t addrlen, optlen;
#ifdef _WINDOWS
  /* See comment below */
  BOOL bool_val = false;
#else
  int32_t bool_val;
#endif

  fd = socket->fd;
  optlen = sizeof(value);

  if (UNLIKELY(getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *) &value, &optlen) != 0)) {
    error_set_error(
			(int32_t)error_get_io_from_system(error_get_last_net()),
			(int32_t)error_get_last_net(),
			"Failed to call getsockopt() to get socket info for fd"
		);
    return false;
  }

  if (UNLIKELY(optlen != sizeof(value))) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Failed to get socket info for fd, bad option length");
    return false;
  }

  switch (value) {
	  case SOCK_STREAM:
	    socket->type = SOCKET_TYPE_STREAM;
	    break;

	  case SOCK_DGRAM:
	    socket->type = SOCKET_TYPE_DATAGRAM;
	    break;

#ifdef SOCK_SEQPACKET
	  case SOCK_SEQPACKET:
	    socket->type = SOCKET_TYPE_SEQPACKET;
	    break;
#endif

	  default:
	    socket->type = SOCKET_TYPE_UNKNOWN;
	    break;
  }

  addrlen = sizeof(address);

  if (UNLIKELY(getsockname(fd, (struct sockaddr *) &address, &addrlen) != 0)) {
    error_set_error(
			(int32_t)error_get_io_from_system(error_get_last_net()),
			(int32_t)error_get_last_net(),
			"Failed to call getsockname() to get socket address info"
		);
    return false;
  }

#ifdef SO_DOMAIN
  if (!(addrlen > 0)) {
    optlen = sizeof(family);

    if (UNLIKELY(getsockopt(socket->fd,
              SOL_SOCKET,
              SO_DOMAIN,
              (void *) &family,
              &optlen) != 0)) {
      error_set_error(
        (int32_t)error_get_io_from_system(error_get_last_net()),
        (int32_t)error_get_last_net(),
        "Failed to call getsockopt() to get socket SO_DOMAIN option"
      );
      return false;
    }
  }
#endif

  switch (address.ss_family) {
	  case SOCKET_FAMILY_INET:
	    socket->family = SOCKET_FAMILY_INET;
	    break;
#ifdef AF_INET6
	  case SOCKET_FAMILY_INET6:
	    socket->family = SOCKET_FAMILY_INET6;
	    break;
#endif
	  default:
	    socket->family = SOCKET_FAMILY_UNKNOWN;
	    break;
  }

#ifdef AF_INET6
  if (socket->family == SOCKET_FAMILY_INET6 || socket->family == SOCKET_FAMILY_INET) {
#else
  if (socket->family == SOCKET_FAMILY_INET) {
#endif
    switch (socket->type) {
	    case SOCKET_TYPE_STREAM:
	      socket->protocol = SOCKET_PROTOCOL_TCP;
	      break;
	    case SOCKET_TYPE_DATAGRAM:
	      socket->protocol = SOCKET_PROTOCOL_UDP;
	      break;
	    case SOCKET_TYPE_SEQPACKET:
	      socket->protocol = SOCKET_PROTOCOL_SCTP;
	      break;
	    case SOCKET_TYPE_UNKNOWN:
	      break;
    }
  }

  if (LIKELY(socket->family != SOCKET_FAMILY_UNKNOWN)) {
    addrlen = sizeof(address);

    if (getpeername(fd, (struct sockaddr *) &address, &addrlen) >= 0) {
      socket->connected = true;
    }
  }

  optlen = sizeof(bool_val);

  if (getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *) &bool_val, &optlen) == 0)  {
#ifndef _WINDOWS
    /* Experimentation indicates that the SO_KEEPALIVE value is
     * actually a char on Windows, even if documentation claims it
     * to be a BOOL which is a typedef for int. */
    if (optlen != sizeof(bool_val))
      ALERT_WARNING("Socket::_XX__socket_set_details_from_fd: getsockopt() with SO_KEEPALIVE failed");
#endif
    socket->keepalive = !!bool_val;
  }  else
    /* Can't read, maybe not supported, assume false */
    socket->keepalive = false;

  return true;
}

bool socket_init_once(void) {
#ifdef _WINDOWS
  WORD ver_req;
  WSADATA wsa_data;

  ver_req = MAKEWORD(2, 2);

  if (UNLIKELY(WSAStartup(ver_req, &wsa_data) != 0)) {
    return false;
  }

  if (UNLIKELY(LOBYTE(wsa_data.wVersion) != 2 || HIBYTE(wsa_data.wVersion) != 2)) {
    WSACleanup();
    return false;
  }
#else
	#ifdef SIGPIPE
  signal(SIGPIPE, SIG_IGN);
	#endif
#endif
  return true;
}

void socket_close_once(void) {
#ifdef _WINDOWS
  WSACleanup();
#endif
}

Socket *socket_new_from_fd(int32_t fd) {
  Socket *ret;
#if !defined(_WINDOWS) && defined(SO_NOSIGPIPE)
  int32_t flags;
#endif

  if (UNLIKELY(fd < 0)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Unable to create socket from bad fd");
    return NULL;
  }

  if (UNLIKELY((ret = calloc(sizeof(Socket), 1)) == NULL)) {
    error_set_error((int32_t)ERROR_IO_NO_RESOURCES, 0, "Failed to allocate memory for socket");
    return NULL;
  }

  ret->fd = fd;

  if (UNLIKELY(_XX__socket_set_details_from_fd(ret, error) == false)) {
    free(ret);
    return NULL;
  }

  if (UNLIKELY(_XX__socket_set_fd_blocking(ret->fd, false, error) == false)) {
    free(ret);
    return NULL;
  }

#if !defined(_WINDOWS) && defined(SO_NOSIGPIPE)
  flags = 1;

  if (setsockopt(ret->fd, SOL_SOCKET, SO_NOSIGPIPE, &flags, sizeof(flags)) < 0) {
    ALERT_WARNING("Socket::socket_new_from_fd: setsockopt() with SO_NOSIGPIPE failed");
  }
#endif

  socket_set_listen_backlog(ret, SOCKET_DEFAULT_BACKLOG);

  ret->timeout = 0;
  ret->blocking = true;

#ifdef P_OS_SCO
  if (UNLIKELY((ret->timer = time_profiler_new()) == NULL)) {
    error_set_error( (int32_t)ERROR_IO_NO_RESOURCES, 0, "Failed to allocate memory for internal timer");
    free(ret);
    return NULL;
  }
#endif

#ifdef _WINDOWS
  if (UNLIKELY((ret->events = WSACreateEvent()) == WSA_INVALID_EVENT)) {
    error_set_error(error,
      (int32_t)ERROR_IO_FAILED,
      (int32_t)error_get_last_net(),
      "Failed to call WSACreateEvent() on socket"
     );
    free(ret);
    return NULL;
  }
#endif

  return ret;
}

Socket *socket_new(SocketFamily family, SocketType type, SocketProtocol protocol) {
  Socket  *ret;
  int32_t native_type, fd;
#ifndef _WINDOWS
  int32_t flags;
#endif

  if (UNLIKELY(family == SOCKET_FAMILY_UNKNOWN ||
      type == SOCKET_TYPE_UNKNOWN ||
      protocol == SOCKET_PROTOCOL_UNKNOWN)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input socket family, type or protocol");
    return NULL;
  }

  switch (type) {
	  case SOCKET_TYPE_STREAM:
	    native_type = SOCK_STREAM;
	    break;

	  case SOCKET_TYPE_DATAGRAM:
	    native_type = SOCK_DGRAM;
	    break;

#ifdef SOCK_SEQPACKET
	  case SOCKET_TYPE_SEQPACKET:
	    native_type = SOCK_SEQPACKET;
	    break;
#endif

	  default:
	    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Unable to create socket with unknown family");
	    return NULL;
  }

  if (UNLIKELY((ret = calloc(sizeof(Socket), 1)) == NULL)) {
    error_set_error((int32_t)ERROR_IO_NO_RESOURCES, 0, "Failed to allocate memory for socket");
    return NULL;
  }

#ifdef P_OS_SCO
  if (UNLIKELY((ret->timer = time_profiler_new()) == NULL)) {
    error_set_error(error, (int32_t)ERROR_IO_NO_RESOURCES, 0, "Failed to allocate memory for internal timer");
    free (ret);
    return NULL;
  }
#endif

#ifdef SOCK_CLOEXEC
  native_type |= SOCK_CLOEXEC;
#endif
  if (UNLIKELY((fd = (int32_t)socket(family, native_type, protocol)) < 0)) {
    error_set_error(error,
      (int32_t)error_get_io_from_system(error_get_last_net()),
      (int32_t)error_get_last_net(),
      "Failed to call socket() to create socket"
    );
#ifdef P_OS_SCO
    time_profiler_free(ret->timer);
#endif
    free(ret);
    return NULL;
  }

#ifndef _WINDOWS
  flags = fcntl(fd, F_GETFD, 0);

  if (LIKELY(flags != -1 && (flags & FD_CLOEXEC) == 0)) {
    flags |= FD_CLOEXEC;

    if (UNLIKELY(fcntl (fd, F_SETFD, flags) < 0)) {
      ALERT_WARNING("Socket::socket_new: fcntl() with FD_CLOEXEC failed");
    }
  }
#endif

  ret->fd = fd;

#ifdef _WINDOWS
  ret->events = WSA_INVALID_EVENT;
#endif

  if (UNLIKELY(_XX__socket_set_fd_blocking(ret->fd, false, error) == false)) {
    socket_free (ret);
    return NULL;
  }

#if !defined(_WINDOWS) && defined(SO_NOSIGPIPE)
  flags = 1;

  if (setsockopt (ret->fd, SOL_SOCKET, SO_NOSIGPIPE, &flags, sizeof(flags)) < 0) {
    ALERT_WARNING("Socket::socket_new: setsockopt() with SO_NOSIGPIPE failed");
  }
#endif

  ret->timeout  = 0;
  ret->blocking = true;
  ret->family   = family;
  ret->protocol = protocol;
  ret->type     = type;

  socket_set_listen_backlog (ret, SOCKET_DEFAULT_BACKLOG);

#ifdef _WINDOWS
  if (UNLIKELY((ret->events = WSACreateEvent()) == WSA_INVALID_EVENT)) {
    error_set_error(error,
      (int32_t)ERROR_IO_FAILED,
      (int32_t)error_get_last_net(),
      "Failed to call WSACreateEvent() on socket"
    );
    socket_free (ret);
    return NULL;
  }
#endif

  return ret;
}

int32_t
socket_get_fd (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return -1;

  return socket->fd;
}

SocketFamily
socket_get_family (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return SOCKET_FAMILY_UNKNOWN;

  return socket->family;
}

SocketType
socket_get_type (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return SOCKET_TYPE_UNKNOWN;

  return socket->type;
}

SocketProtocol
socket_get_protocol (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return SOCKET_PROTOCOL_UNKNOWN;

  return socket->protocol;
}

bool
socket_get_keepalive (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return false;

  return socket->keepalive;
}

bool
socket_get_blocking (Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return false;

  return socket->blocking;
}

int
socket_get_listen_backlog (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return -1;

  return socket->listen_backlog;
}

int32_t
socket_get_timeout (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return -1;

  return socket->timeout;
}

SocketAddress *
socket_get_local_address (const Socket  *socket,
          Error    **error)
{
  struct sockaddr_storage  buffer;
  socklen_t    len;
  SocketAddress    *ret;

  if (UNLIKELY(socket == NULL)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input argument");
    return NULL;
  }

  len = sizeof(buffer);

  if (UNLIKELY(getsockname (socket->fd, (struct sockaddr *) &buffer, &len) < 0)) {
    error_set_error(error,
			(int32_t)error_get_io_from_system(error_get_last_net()),
			(int32_t)error_get_last_net(),
			"Failed to call getsockname() to get local socket address"
		);
    return NULL;
  }

  ret = socket_address_new_from_native(&buffer, (size_t)len);

  if (UNLIKELY(ret == NULL))
    error_set_error((int32_t)ERROR_IO_FAILED, 0, "Failed to create socket address from native structure");

  return ret;
}

SocketAddress *socket_get_remote_address(const Socket *socket) {
  struct sockaddr_storage  buffer;
  socklen_t len;
  SocketAddress *ret;

  if (UNLIKELY(socket == NULL)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input argument");
    return NULL;
  }

  len = sizeof(buffer);

  if (UNLIKELY(getpeername(socket->fd, (struct sockaddr *) &buffer, &len) < 0)) {
    error_set_error(error,
      (int32_t)error_get_io_from_system(error_get_last_net()),
      (int32_t)error_get_last_net(),
      "Failed to call getpeername() to get remote socket address"
    );
    return NULL;
  }

#ifdef P_OS_SYLLABLE
  /* Syllable has a bug with a wrong byte order for a TCP port,
   * as it only supports IPv4 we can easily fix it here. */
  ((struct sockaddr_in *) &buffer)->sin_port = htons(((struct sockaddr_in *) &buffer)->sin_port);
#endif

  ret = socket_address_new_from_native (&buffer, (size_t) len);

  if (UNLIKELY(ret == NULL))
    error_set_error((int32_t)ERROR_IO_FAILED, 0, "Failed to create socket address from native structure");

  return ret;
}

bool
socket_is_connected (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return false;

  return socket->connected;
}

bool
socket_is_closed (const Socket *socket)
{
  if (UNLIKELY(socket == NULL))
    return true;

  return socket->closed;
}

bool
socket_check_connect_result (Socket  *socket,
             Error  **error)
{
  socklen_t  optlen;
  int32_t    val;

  if (UNLIKELY(socket == NULL)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input argument");
    return false;
  }

  optlen = sizeof(val);

  if (UNLIKELY(getsockopt(socket->fd, SOL_SOCKET, SO_ERROR, (void *) &val, &optlen) < 0)) {
    error_set_error(
      (int32_t)error_get_io_from_system(error_get_last_net()),
      (int32_t)error_get_last_net(),
      "Failed to call getsockopt() to get connection status"
    );
    return false;
  }

  if (UNLIKELY(val != 0))
    error_set_error((int32_t)error_get_io_from_system(val),
             val,
             "Error in socket layer");

  socket->connected = (val == 0);

  return (val == 0);
}

void
socket_set_keepalive (Socket    *socket,
      bool  keepalive)
{
#ifdef _WINDOWS
  char value;
#else
  int32_t value;
#endif

  if (UNLIKELY(socket == NULL))
    return;

  if (socket->keepalive == (uint32_t)!!keepalive)
    return;

#ifdef _WINDOWS
  value = !! (char)keepalive;
#else
  value = !! (int32_t)keepalive;
#endif
  if (setsockopt (socket->fd, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value)) < 0) {
    ALERT_WARNING("Socket::socket_set_keepalive: setsockopt() with SO_KEEPALIVE failed");
    return;
  }

  socket->keepalive = !!(int32_t)keepalive;
}

void socket_set_blocking(Socket *socket, bool blocking) {
  if (UNLIKELY(socket == NULL)) {
    return;
  }

  socket->blocking = !!blocking;
}

void socket_set_listen_backlog(Socket *socket, int32_t backlog) {
  if (UNLIKELY(socket == NULL || socket->listening)) {
    return;
  }

  socket->listen_backlog = backlog;
}

void socket_set_timeout(Socket *socket, int32_t timeout) {
  if (UNLIKELY(socket == NULL)) {
    return;
  }

  if (timeout < 0) {
    timeout = 0;
  }

  socket->timeout = timeout;
}

bool socket_bind(const Socket *socket, SocketAddress  *address, bool allow_reuse) {
  struct sockaddr_storage addr;

#ifdef SO_REUSEPORT
  bool reuse_port;
#endif

#ifdef _WINDOWS
  char value;
#else
  int32_t value;
#endif

  if (UNLIKELY(socket == NULL || address == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  /* Windows allows to reuse the same address even for an active TCP
   * connection, that's why on Windows we should use SO_REUSEADDR only
   * for UDP sockets, UNIX doesn't have such behavior
   *
   * Ignore errors here, the only likely error is "not supported", and
   * this is a "best effort" thing mainly */

#ifdef _WINDOWS
  value = !!(char)(allow_reuse && (socket->type == SOCKET_TYPE_DATAGRAM));
#else
  value = !!(int32_t)allow_reuse;
#endif

  if (setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0)
    ALERT_WARNING("Socket::socket_bind: setsockopt() with SO_REUSEADDR failed");

#ifdef SO_REUSEPORT
  reuse_port = allow_reuse && (socket->type == SOCKET_TYPE_DATAGRAM);

#ifdef _WINDOWS
  value = !!(char)reuse_port;
#else
  value = !!(int32_t)reuse_port;
#endif

  if (setsockopt(socket->fd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) < 0)
    ALERT_WARNING("Socket::socket_bind: setsockopt() with SO_REUSEPORT failed");
#endif

  if (UNLIKELY(socket_address_to_native(address, &addr, sizeof(addr)) == false)) {
    error_set_error(error,
             (int32_t)ERROR_IO_FAILED,
             0,
             "Failed to convert socket address to native structure");
    return false;
  }

  if (UNLIKELY(bind(socket->fd,
            (struct sockaddr *)&addr,
            (socklen_t)socket_address_get_native_size(address)) < 0)) {
    error_set_error(error,
             (int32_t)error_get_io_from_system(error_get_last_net()),
             (int32_t)error_get_last_net(),
             "Failed to call bind() on socket");
    return false;
  }

  return true;
}

bool socket_connect(Socket *socket, SocketAddress *address) {
  struct sockaddr_storage buffer;
  int32_t err_code;
  int32_t conn_result;
  ErrorIO sock_err;

  if (UNLIKELY(socket == NULL || address == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  if (UNLIKELY(socket_address_to_native(address, &buffer, sizeof(buffer)) == false)) {
    error_set_error(error,
             (int32_t)ERROR_IO_FAILED,
             0,
             "Failed to convert socket address to native structure");
    return false;
  }

#if !defined(_WINDOWS) && defined(EINTR)
  for (;;) {
    conn_result = connect(socket->fd, (struct sockaddr *)&buffer,
               (socklen_t)socket_address_get_native_size(address));

    if (LIKELY(conn_result == 0)) {
      break;
    }

    err_code = error_get_last_net();

    if (err_code == EINTR){
      continue;
    } else {
      break;
    }
  }
#else
  conn_result = connect(socket->fd, (struct sockaddr *)&buffer,
             (int32_t)socket_address_get_native_size(address));

  if (conn_result != 0) {
    err_code = error_get_last_net();
  }
#endif

  if (conn_result == 0) {
    socket->connected = true;
    return true;
  }

  sock_err = error_get_io_from_system(err_code);

  if (LIKELY(sock_err == ERROR_IO_WOULD_BLOCK || sock_err == ERROR_IO_IN_PROGRESS)) {
    if (socket->blocking) {
      if (socket_io_condition_wait(socket,
              SOCKET_IO_CONDITION_POLLOUT,
              error) == true &&
          socket_check_connect_result(socket, error) == true) {
        socket->connected = true;
        return true;
      }
    } else {
      error_set_error(error,
               (int32_t)sock_err,
               err_code,
               "Couldn't block non-blocking socket");
    }
  } else {
    error_set_error(error,
             (int32_t)sock_err,
             err_code,
             "Failed to call connect() on socket");
  }

  return false;
}

bool socket_listen(Socket *socket) {
  if (UNLIKELY(socket == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  if (UNLIKELY(listen(socket->fd, socket->listen_backlog) < 0)) {
    error_set_error(error,
             (int32_t)error_get_io_from_system(error_get_last_net()),
             (int32_t)error_get_last_net(),
             "Failed to call listen() on socket");
    return false;
  }

  socket->listening = true;
  return true;
}

Socket *socket_accept(const Socket  *socket) {
  Socket *ret;
  ErrorIO sock_err;
  int32_t res;
  int32_t err_code;
#ifndef _WINDOWS
  int32_t flags;
#endif

  if (UNLIKELY(socket == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return NULL;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return NULL;
  }

  for (;;) {
    if (socket->blocking &&
        socket_io_condition_wait(socket,
            SOCKET_IO_CONDITION_POLLIN,
            error) == false) {
      return NULL;
	  }

    if ((res = (int32_t)accept(socket->fd, NULL, 0)) < 0) {
      err_code = error_get_last_net();
#if !defined(_WINDOWS) && defined(EINTR)
      if (error_get_last_net() == EINTR) {
        continue;
      }
#endif
      sock_err = error_get_io_from_system(err_code);

      if (socket->blocking && sock_err == ERROR_IO_WOULD_BLOCK) {
        continue;
      }

      error_set_error(error,
               (int32_t)sock_err,
               err_code,
               "Failed to call accept() on socket");

      return NULL;
    }

    break;
  }

#ifdef _WINDOWS
  /* The socket inherits the accepting sockets event mask and even object,
   * we need to remove that */
  WSAEventSelect(res, NULL, 0);
#else
  flags = fcntl(res, F_GETFD, 0);

  if (LIKELY(flags != -1 && (flags & FD_CLOEXEC) == 0)) {
    flags |= FD_CLOEXEC;

    if (UNLIKELY(fcntl(res, F_SETFD, flags) < 0))
      ALERT_WARNING("Socket::socket_accept: fcntl() with FD_CLOEXEC failed");
  }
#endif

  if (UNLIKELY((ret = socket_new_from_fd(res, error)) == NULL)) {
    if (UNLIKELY(sys_close(res) != 0)) {
      ALERT_WARNING("Socket::socket_accept: sys_close() failed");
    }
  } else {
    ret->protocol = socket->protocol;
  }

  return ret;
}

ssize_t socket_receive(const Socket *socket, char *buffer, size_t buflen) {
  ErrorIO sock_err;
  ssize_t ret;
  int32_t err_code;

  if (UNLIKELY(socket == NULL || buffer == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return -1;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return -1;
  }

  for (;;) {
    if (socket->blocking &&
        socket_io_condition_wait(socket,
            SOCKET_IO_CONDITION_POLLIN,
            error) == false) {
      return -1;
	  }

    if ((ret = recv(socket->fd, buffer, (socklen_t) buflen, 0)) < 0) {
      err_code = error_get_last_net();

#if !defined(_WINDOWS) && defined(EINTR)
      if (err_code == EINTR) {
        continue;
      }
#endif
      sock_err = error_get_io_from_system(err_code);

      if (socket->blocking && sock_err == ERROR_IO_WOULD_BLOCK)
        continue;

      error_set_error(error,
               (int32_t)sock_err,
               err_code,
               "Failed to call recv() on socket");

      return -1;
    }

    break;
  }

  return ret;
}

ssize_t socket_receive_from(const Socket *socket, SocketAddress  **address, char *buffer, size_t buflen) {
  ErrorIO sock_err;
  struct sockaddr_storage sa;
  socklen_t optlen;
  ssize_t ret;
  int32_t err_code;

  if (UNLIKELY(socket == NULL || buffer == NULL || buflen == 0)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return -1;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return -1;
  }

  optlen = sizeof(sa);

  for (;;) {
    if (socket->blocking &&
        socket_io_condition_wait(socket,
            SOCKET_IO_CONDITION_POLLIN,
            error) == false) {
      return -1;
	  }

    if ((ret = recvfrom(socket->fd,
             buffer,
             (socklen_t)buflen,
             0,
             (struct sockaddr *)&sa,
             &optlen)) < 0) {
      err_code = error_get_last_net();

#if !defined(_WINDOWS) && defined(EINTR)
      if (err_code == EINTR) {
        continue;
      }
#endif
      sock_err = error_get_io_from_system(err_code);

      if (socket->blocking && sock_err == ERROR_IO_WOULD_BLOCK) {
        continue;
      }

      error_set_error(error,
               (int32_t)sock_err,
               err_code,
               "Failed to call recvfrom() on socket");

      return -1;
    }

    break;
  }

  if (address != NULL) {
    *address = socket_address_new_from_native(&sa, optlen);
  }

  return ret;
}

ssize_t socket_send(const Socket *socket, const char *buffer, size_t buflen, Error  **error) {
  ErrorIO sock_err;
  ssize_t ret;
  int32_t err_code;

  if (UNLIKELY(socket == NULL || buffer == NULL || buflen == 0)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return -1;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return -1;
  }

  for (;;) {
    if (socket->blocking &&
        socket_io_condition_wait(socket,
            SOCKET_IO_CONDITION_POLLOUT,
            error) == false) {
      return -1;
	  }

    if ((ret = send (socket->fd,
         buffer,
         (socklen_t) buflen,
         SOCKET_DEFAULT_SEND_FLAGS)) < 0) {
      err_code = error_get_last_net();

#if !defined(_WINDOWS) && defined(EINTR)
      if (err_code == EINTR) {
        continue;
      }
#endif
      sock_err = error_get_io_from_system(err_code);

      if (socket->blocking && sock_err == ERROR_IO_WOULD_BLOCK) {
        continue;
      }

      error_set_error(error,
               (int32_t)sock_err,
               err_code,
               "Failed to call send() on socket");

      return -1;
    }

    break;
  }

  return ret;
}

ssize_t socket_send_to(const Socket *socket, SocketAddress *address, const char *buffer, size_t buflen) {
  ErrorIO sock_err;
  struct sockaddr_storage sa;
  socklen_t optlen;
  ssize_t ret;
  int32_t err_code;

  if (!socket || !address || !buffer) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return -1;
  }

  if (!_XX__socket_check(socket, error)) {
    return -1;
  }

  if (!socket_address_to_native(address, &sa, sizeof(sa))) {
    error_set_error(error,
             (int32_t)ERROR_IO_FAILED,
             0,
             "Failed to convert socket address to native structure");
    return -1;
  }

  optlen = (socklen_t)socket_address_get_native_size(address);

  for (;;) {
    if (socket->blocking &&
        socket_io_condition_wait(socket, SOCKET_IO_CONDITION_POLLOUT, error) == false) {
      return -1;
	  }

    if ((ret = sendto (socket->fd,
           buffer,
           (socklen_t) buflen,
           0,
           (struct sockaddr *) &sa,
           optlen)) < 0) {
      err_code = error_get_last_net();

#if !defined(_WINDOWS) && defined(EINTR)
      if (err_code == EINTR) {
        continue;
      }
#endif
      sock_err = error_get_io_from_system(err_code);

      if (socket->blocking && sock_err == ERROR_IO_WOULD_BLOCK) {
        continue;
      }

      error_set_error(error,
               (int32_t)sock_err,
               err_code,
               "Failed to call sendto() on socket");

      return -1;
    }

    break;
  }

  return ret;
}

bool socket_close(Socket *socket) {
  int32_t err_code;

  if (UNLIKELY(socket == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return false;
  }

  if (socket->closed) {
    return true;
  }

  if (LIKELY(sys_close(socket->fd) == 0)) {
    socket->connected = false;
    socket->closed    = true;
    socket->listening = false;
    socket->fd        = -1;

    return true;
  } else {
    err_code = error_get_last_net();

    error_set_error(error,
             (int32_t)error_get_io_from_system(err_code),
             err_code,
             "Failed to close socket");

    return false;
  }
}

bool socket_shutdown(Socket *socket, bool shutdown_read, bool shutdown_write) {
  int32_t how;

  if (UNLIKELY(socket == NULL)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  if (UNLIKELY(shutdown_read == false && shutdown_write == false)) {
    return true;
  }

#ifndef _WINDOWS
  if (shutdown_read == true && shutdown_write == true) {
    how = SHUT_RDWR;
  } else if (shutdown_read == true) {
    how = SHUT_RD;
  } else {
    how = SHUT_WR;
  }
#else
  if (shutdown_read == true && shutdown_write == true) {
    how = SD_BOTH;
  } else if (shutdown_read == true) {
    how = SD_RECEIVE;
  } else {
    how = SD_SEND;
  }
#endif

  if (UNLIKELY(shutdown (socket->fd, how) != 0)) {
    error_set_error(error,
             (int32_t)error_get_io_from_system(error_get_last_net()),
             (int32_t)error_get_last_net(),
             "Failed to call shutdown() on socket");
    return false;
  }

  if (shutdown_read == true && shutdown_write == true) {
    socket->connected = false;
  }

  return true;
}

void socket_free(Socket *socket) {
  if (UNLIKELY(socket == NULL)) {
    return;
  }

#ifdef _WINDOWS
  if (LIKELY(socket->events != WSA_INVALID_EVENT)) {
    WSACloseEvent (socket->events);
  }
#endif

  socket_close(socket, NULL);

#ifdef P_OS_SCO
  if (LIKELY(socket->timer != NULL)) {
    time_profiler_free (socket->timer);
  }
#endif

  free (socket);
}

bool socket_set_buffer_size(const Socket *socket, SocketDirection dir, size_t size) {
  int32_t optname;
  int32_t optval;

  if (UNLIKELY(socket == NULL)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  optname = (dir == SOCKET_DIRECTION_RCV) ? SO_RCVBUF : SO_SNDBUF;
  optval = (int32_t)size;

  if (UNLIKELY(setsockopt(socket->fd,
            SOL_SOCKET,
            optname,
            (const void *) &optval,
            sizeof(optval)) != 0)) {
    error_set_error(error,
             (int32_t)error_get_io_from_system(error_get_last_net()),
             (int32_t)error_get_last_net(),
             "Failed to call setsockopt() on socket to set buffer size");
    return false;
  }

  return true;
}

bool socket_io_condition_wait(const Socket *socket, SocketIOCondition condition) {
#if defined(_WINDOWS)
  int32_t network_events;
  int32_t evret;
  int32_t timeout;

  if (UNLIKELY(socket == NULL)) {
    error_set_error((int32_t)ERROR_IO_INVALID_ARGUMENT, 0, "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  timeout = socket->timeout > 0 ? socket->timeout : WSA_INFINITE;

  if (condition == SOCKET_IO_CONDITION_POLLIN) {
    network_events = FD_READ | FD_ACCEPT;
  } else {
    network_events = FD_WRITE | FD_CONNECT;
  }

  WSAResetEvent(socket->events);
  WSAEventSelect(socket->fd, socket->events, network_events);

  evret = WSAWaitForMultipleEvents(1, (const HANDLE *) &socket->events, true, timeout, false);

  if (evret == WSA_WAIT_EVENT_0) {
    return true;
  } else if (evret == WSA_WAIT_TIMEOUT) {
    error_set_error(error,
             (int32_t)ERROR_IO_TIMED_OUT,
             (int32_t)error_get_last_net(),
             "Timed out while waiting socket condition");
    return false;
  } else {
    error_set_error(error,
             (int32_t)error_get_io_from_system(error_get_last_net()),
             (int32_t)error_get_last_net(),
             "Failed to call WSAWaitForMultipleEvents() on socket");
    return false;
  }
#elif defined(SOCKET_USE_POLL)
  struct pollfd pfd;
  int32_t evret;
  int32_t timeout;

  if (UNLIKELY(socket == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  timeout = socket->timeout > 0 ? socket->timeout : -1;

  pfd.fd = socket->fd;
  pfd.revents = 0;

  if (condition == SOCKET_IO_CONDITION_POLLIN) {
    pfd.events = POLLIN;
  } else {
    pfd.events = POLLOUT;
  }

#ifdef P_OS_SCO
  time_profiler_reset(socket->timer);
#endif

  while (true) {
    evret = poll(&pfd, 1, timeout);

#ifdef EINTR
    if (evret == -1 && error_get_last_net() == EINTR) {
	#ifdef P_OS_SCO
      if (timeout < 0 ||
          (time_profiler_elapsed_usecs(socket->timer) / 1000) < (uint32_t64)timeout)
        continue;
      else
        evret = 0;
	#else
      continue;
	#endif
    }
#endif

    if (evret == 1) {
      return true;
    } else if (evret == 0) {
      error_set_error(error,
               (int32_t)ERROR_IO_TIMED_OUT,
               (int32_t)error_get_last_net(),
               "Timed out while waiting socket condition");
      return false;
    } else {
      error_set_error(error,
               (int32_t)error_get_io_from_system(error_get_last_net()),
               (int32_t)error_get_last_net(),
               "Failed to call poll() on socket");
      return false;
    }
  }
#else
  fd_set fds;
  struct timeval tv;
  struct timeval *ptv;
  int32_t evret;

  if (UNLIKELY(socket == NULL)) {
    error_set_error(error,
             (int32_t)ERROR_IO_INVALID_ARGUMENT,
             0,
             "Invalid input argument");
    return false;
  }

  if (UNLIKELY(_XX__socket_check(socket, error) == false)) {
    return false;
  }

  if (socket->timeout > 0) {
    ptv = &tv;
  } else {
    ptv = NULL;
  }

  while (true) {
    FD_ZERO(&fds);
    FD_SET(socket->fd, &fds);

    if (socket->timeout > 0) {
      tv.tv_sec  = socket->timeout / 1000;
      tv.tv_usec = (socket->timeout % 1000) * 1000;
    }

    if (condition == SOCKET_IO_CONDITION_POLLIN) {
      evret = select (socket->fd + 1, &fds, NULL, NULL, ptv);
    } else {
      evret = select (socket->fd + 1, NULL, &fds, NULL, ptv);
    }

#ifdef EINTR
    if (evret == -1 && error_get_last_net() == EINTR) {
      continue;
    }
#endif

    if (evret == 1) {
      return true;
    } else if (evret == 0) {
      error_set_error(error,
               (int32_t)ERROR_IO_TIMED_OUT,
               (int32_t)error_get_last_net(),
               "Timed out while waiting socket condition");
      return false;
    } else {
      error_set_error(error,
               (int32_t)error_get_io_from_system(error_get_last_net()),
               (int32_t)error_get_last_net(),
               "Failed to call select() on socket");
      return false;
    }
  }
#endif
}
