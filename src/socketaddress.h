/*
 * The MIT License
 *
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

/*
 * @file socketaddress.h
 * @brief Socket address wrapper
 * @author Alexander Saprykin
 *
 * A socket address is usually represented by a network address (IPv4 or IPv6)
 * and a port number (though some other naming schemes and parameters are
 * possible).
 *
 * Socket address parameters are stored inside a special system (native)
 * structure in the binary (raw) form. The native structure varies with an
 * operating system and a network protocol. #SocketAddress acts like a thin
 * wrapper around that native address structure and unifies manipulation of
 * socket address data.
 *
 * #SocketAddress supports IPv4 and IPv6 addresses which consist of an IP
 * address and a port number. IPv6 support is system dependent and not provided
 * for all the platforms. Sometimes you may also need to enable IPv6 support in
 * the system to make it working.
 *
 * Convenient methods to create special addresses are provided: for the loopback
 * interface use p_socket_address_new_loopback(), for the any-address interface
 * use p_socket_address_new_any().
 *
 * If you want to get the underlying native address structure for further usage
 * in system calls use p_socket_address_to_native(), and
 * p_socket_address_new_from_native() for a vice versa conversion.
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
  #include <windows.h>
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

/*
 * @brief Creates new #SocketAddress from the native socket address raw data.
 * @param native Pointer to the native socket address raw data.
 * @param len Raw data length, in bytes.
 * @return Pointer to #SocketAddress in case of success, NULL otherwise.
 * @since 0.0.1
 */
SocketAddress *socket_address_new_from_native(const void *native, size_t len);

/*
 * @brief Creates new #SocketAddress.
 * @param address String representation of an address (i.e. "172.146.45.5").
 * @param port Port number.
 * @return Pointer to #SocketAddress in case of success, NULL otherwise.
 * @since 0.0.1
 * @note It tries to automatically detect a socket family.
 *
 * If the @a address is an IPv6 address, it can also contain a scope index
 * separated from the address by the '%' literal). Most target platforms should
 * correctly parse such an address though some old operating systems may fail in
 * case of lack of the getaddrinfo() call.
 */
SocketAddress *socket_address_new(const char *address, uint16_t port);

/*
 * @brief Creates new #SocketAddress for the any-address representation.
 * @param family Socket family.
 * @param port Port number.
 * @return Pointer to #SocketAddress in case of success, NULL otherwise.
 * @since 0.0.1
 * @note This call creates a network address for the set of all possible
 * addresses, so you can't use it for receiving or sending data on a particular
 * network address. If you need to bind a socket to the specific address
 * (i.e. 127.0.0.1) use p_socket_address_new() instead.
 */
SocketAddress *socket_address_new_any(SocketFamily family, uint16_t port);

/*
 * @brief Creates new #SocketAddress for the loopback interface.
 * @param family Socket family.
 * @param port Port number.
 * @return Pointer to #SocketAddress in case of success, NULL otherwise.
 * @since 0.0.1
 * @note This call creates a network address for the entire loopback network
 * interface, so you can't use it for receiving or sending data on a particular
 * network address. If you need to bind a socket to the specific address
 * (i.e. 127.0.0.1) use p_socket_address_new() instead.
 */
SocketAddress *socket_address_new_loopback(SocketFamily family, uint16_t port);

/*
 * @brief Converts #SocketAddress to the native socket address raw data.
 * @param addr #SocketAddress to convert.
 * @param[out] dest Output buffer for raw data.
 * @param destlen Length in bytes of the @a dest buffer.
 * @return TRUE in case of success, FALSE otherwise.
 * @since 0.0.1
 */
bool socket_address_to_native(const SocketAddress *addr, void *dest, size_t destlen);

/*
 * @brief Gets the size of the native socket address raw data, in bytes.
 * @param addr #SocketAddress to get the size of native address raw data for.
 * @return Size of the native socket address raw data in case of success, 0
 * otherwise.
 * @since 0.0.1
 */
size_t socket_address_get_native_size(const SocketAddress  *addr);

/*
 * @brief Gets a family of a socket address.
 * @param addr #SocketAddress to get the family for.
 * @return #SocketFamily of the socket address.
 * @since 0.0.1
 */
SocketFamily socket_address_get_family(const SocketAddress  *addr);

/*
 * @brief Gets a socket address in a string representation, i.e. "172.146.45.5".
 * @param addr #SocketAddress to get address string for.
 * @return Pointer to the string representation of the socket address in case of
 * success, NULL otherwise. The caller takes ownership of the returned pointer.
 * @since 0.0.1
 */
char *socket_address_get_address(const SocketAddress *addr);

/*
 * @brief Gets a port number of a socket address.
 * @param addr #SocketAddress to get the port number for.
 * @return Port number in case of success, 0 otherwise.
 * @since 0.0.1
 */
uint16_t socket_address_get_port(const SocketAddress *addr);

/*
 * @brief Gets IPv6 traffic class and flow information.
 * @param addr #SocketAddress to get flow information for.
 * @return IPv6 traffic class and flow information.
 * @since 0.0.1
 * @note This call is valid only for an IPv6 address, otherwise 0 is returned.
 * @note Some operating systems may not support this property.
 * @sa p_socket_address_is_flow_info_supported()
 */
uint32_t socket_address_get_flow_info(const SocketAddress *addr);

/*
 * @brief Gets an IPv6 set of interfaces for a scope.
 * @param addr #SocketAddress to get the set of interfaces for.
 * @return Index that identifies the set of interfaces for a scope.
 * @since 0.0.1
 * @note This call is valid only for an IPv6 address, otherwise 0 is returned.
 * @note Some operating systems may not support this property.
 * @sa p_socket_address_is_scope_id_supported()
 */
uint32_t socket_address_get_scope_id(const SocketAddress *addr);

/*
 * @brief Sets IPv6 traffic class and flow information.
 * @param addr #SocketAddress to set flow information for.
 * @param flowinfo Flow information to set.
 * @since 0.0.1
 * @note This call is valid only for an IPv6 address.
 * @note Some operating systems may not support this property.
 * @sa p_socket_address_is_flow_info_supported()
 */
void socket_address_set_flow_info(SocketAddress *addr, uint32_t flowinfo);

/*
 * @brief Sets an IPv6 set of interfaces for a scope.
 * @param addr #SocketAddress to set the set of interfaces for.
 * @param scope_id Index that identifies the set of interfaces for a scope.
 * @since 0.0.1
 * @note This call is valid only for an IPv6 address.
 * @note Some operating systems may not support this property.
 * @sa p_socket_address_is_scope_id_supported()
 */
void socket_address_set_scope_id(SocketAddress *addr, uint32_t scope_id);

/*
 * @brief Checks whether flow information is supported in IPv6.
 * @return TRUE in case of success, FALSE otherwise.
 * @since 0.0.1
 */
bool socket_address_is_flow_info_supported();

/*
 * @brief Checks whether a set of interfaces for a scope is supported in IPv6.
 * @return TRUE in case of success, FALSE otherwise.
 * @since 0.0.1
 */
bool socket_address_is_scope_id_supported();

/*
 * @brief Checks whether IPv6 protocol is supported.
 * @return TRUE in case of success, FALSE otherwise.
 * @since 0.0.3
 */
bool socket_address_is_ipv6_supported();

/*
 * @brief Checks whether a given socket address is an any-address
 * representation. Such an address is a 0.0.0.0.
 * @param addr #SocketAddress to check.
 * @return TRUE if the @a addr is the any-address representation, FALSE
 * otherwise.
 * @since 0.0.1
 * @sa p_socket_address_new_any()
 */
bool socket_address_is_any(const SocketAddress *addr);

/*
 * @brief Checks whether a given socket address is for the loopback interface.
 * Such an address is a 127.x.x.x.
 * @param addr #SocketAddress to check.
 * @return TRUE if the @a addr is for the loopback interface, FALSE otherwise.
 * @since 0.0.1
 * @sa p_socket_address_new_loopback()
 */
bool socket_address_is_loopback(const SocketAddress *addr);

/*
 * @brief Frees a socket address structure and its resources.
 * @param addr #SocketAddress to free.
 * @since 0.0.1
 */
void socket_address_free(SocketAddress *addr);

