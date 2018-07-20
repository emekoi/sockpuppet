/*
 * MIT License
 *
 * Copyright (C) 2018 emekoi
 * Copyright (C) 2010-2017 Alexander Saprykin <saprykin.spb@gmail.com>
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

#ifndef _WINDOWS
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#else
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
  #ifndef _WIN32_WINNT
	  #define _WIN32_WINNT 0x501
	#endif
  #ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
#endif

#include <stdbool.h>
#include <stdint.h>

/* Socket address family. */
typedef enum {
  SOCKET_FAMILY_UNKNOWN = 0,       /* Unknown family. */
  SOCKET_FAMILY_INET    = AF_INET, /* IPv4 family. */
#ifdef AF_INET6
  SOCKET_FAMILY_INET6   = AF_INET6 /* IPv6 family. */
#else
  SOCKET_FAMILY_INET6   = -1       /* No IPv6 family. */
#endif
} SocketFamily;

/* Socket address opaque structure. */
typedef struct SocketAddress SocketAddress;

SocketAddress *socket_address_new_from_native(const void *native, size_t len);
SocketAddress *socket_address_new(const char *address, uint16_t port);
SocketAddress *socket_address_new_any(SocketFamily family, uint16_t port);
SocketAddress *socket_address_new_loopback(SocketFamily family, uint16_t port);
bool socket_address_to_native(const SocketAddress *addr, void *dest, size_t destlen);
size_t socket_address_get_native_size(const SocketAddress  *addr);
SocketFamily socket_address_get_family(const SocketAddress  *addr);
char *socket_address_get_address(const SocketAddress *addr);
uint16_t socket_address_get_port(const SocketAddress *addr);
uint32_t socket_address_get_flow_info(const SocketAddress *addr);
uint32_t socket_address_get_scope_id(const SocketAddress *addr);
void socket_address_set_flow_info(SocketAddress *addr, uint32_t flowinfo);
void socket_address_set_scope_id(SocketAddress *addr, uint32_t scope_id);
bool socket_address_is_flow_info_supported();
bool socket_address_is_scope_id_supported();
bool socket_address_is_ipv6_supported();
bool socket_address_is_any(const SocketAddress *addr);
bool socket_address_is_loopback(const SocketAddress *addr);
void socket_address_free(SocketAddress *addr);

