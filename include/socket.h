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
#include <stdbool.h>
#include "socketaddress.h"

/* Socket protocols specified by the IANA.  */
typedef enum {
  SOCKET_PROTOCOL_UNKNOWN  = -1, /* Unknown protocol. */
  SOCKET_PROTOCOL_DEFAULT  = 0,  /* Default protocol. */
  SOCKET_PROTOCOL_TCP      = 6,  /* TCP protocol. */
  SOCKET_PROTOCOL_UDP      = 17, /* UDP protocol. */
  SOCKET_PROTOCOL_SCTP     = 132 /* SCTP protocol. */
} SocketProtocol;

/* Socket types. */
typedef enum {
  SOCKET_TYPE_UNKNOWN   = 0,  /* Unknown type. */
  SOCKET_TYPE_STREAM    = 1,  /* Connection oritented, reliable, stream of bytes (i.e. TCP). */
  SOCKET_TYPE_DATAGRAM  = 2,  /* Connection-less, unreliable, datagram passing (i.e. UDP). */
  SOCKET_TYPE_SEQPACKET = 3   /* Connection-less, reliable, datagram passing (i.e. SCTP). */
} SocketType;

/* Socket direction for data operations. */
typedef enum {
  SOCKET_DIRECTION_SND = 0, /* Send direction. */
  SOCKET_DIRECTION_RCV = 1  /* Receive direction. */
} SocketDirection;

/* Socket IO waiting (polling) conditions. */
typedef enum {
  SOCKET_IO_CONDITION_POLLIN  = 1, /* Ready to read. */
  SOCKET_IO_CONDITION_POLLOUT = 2  /* Ready to write. */
} SocketIOCondition;

/* Socket opaque structure. */
typedef struct Socket Socket;

bool socket_init_once(void);
void socket_close_once(void);

Socket *socket_new_from_fd(int32_t fd);
Socket *socket_new(SocketFamily family, SocketType type, SocketProtocol protocol);
int32_t socket_get_fd(const Socket *socket);
SocketFamily socket_get_family(const Socket *socket);
SocketType socket_get_type(const Socket *socket);
SocketProtocol socket_get_protocol(const Socket *socket);
bool socket_get_keepalive(const Socket *socket);
bool socket_get_blocking(Socket *socket);

// bool socket_get_delay(Socket *socket);

int32_t socket_get_listen_backlog(const Socket *socket);
int32_t socket_get_timeout(const Socket *socket);
SocketAddress *socket_get_local_address(const Socket *socket);
SocketAddress *socket_get_remote_address(const Socket *socket);
bool socket_is_connected(const Socket *socket);
bool socket_is_closed(const Socket *socket);
bool socket_check_connect_result(Socket *socket);
void socket_set_keepalive (Socket *socket, bool keepalive);
void socket_set_blocking(Socket *socket, bool blocking);

// void socket_set_delay(Socket *socket, bool delay);

void socket_set_listen_backlog(Socket *socket, int32_t backlog);
void socket_set_timeout(Socket *socket, int32_t timeout);
bool socket_bind(const Socket *socket, SocketAddress *address, bool allow_reuse);
bool socket_connect(Socket *socket, SocketAddress *address);
bool socket_listen(Socket *socket);
Socket *socket_accept(const Socket *socket);
ssize_t socket_receive(const Socket *socket, char *buffer, size_t buflen);
ssize_t socket_receive_from(const Socket *socket, SocketAddress **address, char *buffer, size_t buflen);
ssize_t socket_send(const Socket *socket, const char *buffer, size_t buflen);
ssize_t socket_send_to(const Socket *socket, SocketAddress *address, const char *buffer, size_t buflen);
bool socket_close(Socket *socket);
bool socket_shutdown(Socket *socket, bool shutdown_read, bool shutdown_write);
void socket_free(Socket *socket);
bool socket_set_buffer_size(const Socket *socket, SocketDirection dir, size_t size);
bool socket_io_condition_wait(const Socket *socket, SocketIOCondition condition);
