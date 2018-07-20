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


#include <stdlib.h>
#include <string.h>
#include "socketaddress.h"

#ifndef _WINDOWS
  #include <arpa/inet.h>
  #include <netdb.h>
#endif

/* According to Open Group specifications */
#ifndef INET_ADDRSTRLEN
  #ifdef _WINDOWS
    /* On Windows it includes port number  */
    #define INET_ADDRSTRLEN 22
  #else
    #define INET_ADDRSTRLEN 16
  #endif
#endif

#ifdef AF_INET6
  #ifndef INET6_ADDRSTRLEN
    #ifdef _WINDOWS
      /* On Windows it includes port number */
      #define INET6_ADDRSTRLEN 65
    #else
      #define INET6_ADDRSTRLEN 46
    #endif
  #endif
#endif

#if !defined(VMS) || !defined(__VMS)
  #if defined(__x86_64__)
    #define addrinfo __addrinfo64
  #endif
#endif

#if defined(__BEOS__) || defined(__OS2__)
  #ifdef AF_INET6
    #undef AF_INET6
  #endif
#endif

struct SocketAddress {
  SocketFamily family;
  union {
    struct in_addr sin_addr;
#ifdef AF_INET6
    struct in6_addr sin6_addr;
#endif
  } addr;
  uint16_t port;
  uint32_t flowinfo;
  uint32_t scope_id;
};

SocketAddress *socket_address_new_from_native (const void *native, size_t len) {
  SocketAddress *ret;
  uint16_t family;

  if (UNLIKELY(native == NULL || len == 0)) {
    return NULL;
  }

  if (UNLIKELY((ret = calloc(sizeof(SocketAddress), 1)) == NULL)) {
    return NULL;
  }

  family = ((struct sockaddr *) native)->sa_family;

  if (family == AF_INET) {
    if (len < sizeof(struct sockaddr_in)) {
      ALERT_WARNING("SocketAddress::socket_address_new_from_native: invalid IPv4 native size");
      free(ret);
      return NULL;
    }

    memcpy(&ret->addr.sin_addr, &((struct sockaddr_in *) native)->sin_addr, sizeof(struct in_addr));
    ret->family = SOCKET_FAMILY_INET;
    ret->port = ntohs(((struct sockaddr_in *) native)->sin_port);
    return ret;
  }
#ifdef AF_INET6
  else if (family == AF_INET6) {
    if (len < sizeof(struct sockaddr_in6)) {
      ALERT_WARNING("SocketAddress::socket_address_new_from_native: invalid IPv6 native size");
      free(ret);
      return NULL;
    }

    memcpy(&ret->addr.sin6_addr,
      &((struct sockaddr_in6 *) native)->sin6_addr,
      sizeof(struct in6_addr));

    ret->family = SOCKET_FAMILY_INET6;
    ret->port = ntohs(((struct sockaddr_in *) native)->sin_port);
    ret->flowinfo = ((struct sockaddr_in6 *) native)->sin6_flowinfo;
    ret->scope_id = ((struct sockaddr_in6 *) native)->sin6_scope_id;
    return ret;
  }
#endif
  else {
    free(ret);
    return NULL;
  }
}

SocketAddress *socket_address_new(const char  *address, uint16_t port) {
  SocketAddress *ret;
#if defined(_WINDOWS)
  struct addrinfo hints;
  struct addrinfo *res;
#endif

#ifdef _WINDOWS
  struct sockaddr_storage  sa;
  struct sockaddr_in *sin = (struct sockaddr_in *) &sa;
	#ifdef AF_INET6
  	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) &sa;
	#endif /* AF_INET6 */
  int32_t len;
#endif /* _WINDOWS */

  if (UNLIKELY(address == NULL)) {
    return NULL;
  }

#if (defined(_WINDOWS) || defined(AF_INET6))
  if (strchr(address, ':') != NULL) {
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
#if !defined(__USLC__) || !defined(__UNIXWARE__)
    hints.ai_flags = AI_NUMERICHOST;
#endif

    if (UNLIKELY(getaddrinfo(address, NULL, &hints, &res) != 0)) {
      return NULL;
    }

    if (LIKELY(res->ai_family == AF_INET6 &&
            res->ai_addrlen == sizeof(struct sockaddr_in6))) {
      ((struct sockaddr_in6 *) res->ai_addr)->sin6_port = htons(port);
      ret = socket_address_new_from_native(res->ai_addr, res->ai_addrlen);
    } else
      ret = NULL;

    freeaddrinfo(res);

    return ret;
  }
#endif

  if (UNLIKELY((ret = calloc(sizeof(SocketAddress), 1)) == NULL)) {
    ALERT_ERROR("SocketAddress::socket_address_new: failed to allocate memory");
    return NULL;
  }

  ret->port = port;

#ifdef _WINDOWS
  memset(&sa, 0, sizeof(sa));
  len = sizeof(sa);
  sin->sin_family = AF_INET;

  if (WSAStringToAddressA((LPSTR) address, AF_INET, NULL, (LPSOCKADDR) &sa, &len) == 0) {
    memcpy(&ret->addr.sin_addr, &sin->sin_addr, sizeof(struct in_addr));
    ret->family = SOCKET_FAMILY_INET;
    return ret;
  }
	#ifdef AF_INET6
  else {
    sin6->sin6_family = AF_INET6;

    if (WSAStringToAddressA((LPSTR) address, AF_INET6, NULL, (LPSOCKADDR) &sa, &len) == 0) {
      memcpy(&ret->addr.sin6_addr, &sin6->sin6_addr, sizeof(struct in6_addr));
      ret->family = SOCKET_FAMILY_INET6;
      return ret;
    }
  }
	#endif /* AF_INET6 */
#else /* _WINDOWS */
  if (inet_pton(AF_INET, address, &ret->addr.sin_addr) > 0) {
    ret->family = SOCKET_FAMILY_INET;
    return ret;
  }
	#ifdef AF_INET6
  else if (inet_pton(AF_INET6, address, &ret->addr.sin6_addr) > 0) {
    ret->family = SOCKET_FAMILY_INET6;
    return ret;
  }
	#endif /* AF_INET6 */
#endif /* _WINDOWS */

  free(ret);
  return NULL;
}

SocketAddress *socket_address_new_any(SocketFamily  family, uint16_t port) {
  SocketAddress *ret;
  uint8_t any_addr[] = {0, 0, 0, 0};
#ifdef AF_INET6
  struct in6_addr any6_addr = IN6ADDR_ANY_INIT;
#endif

  if (UNLIKELY((ret = calloc(sizeof(SocketAddress), 1)) == NULL)) {
    ALERT_ERROR("SocketAddress::socket_address_new_any: failed to allocate memory");
    return NULL;
  }

  if (family == SOCKET_FAMILY_INET) {
    memcpy(&ret->addr.sin_addr, any_addr, sizeof(any_addr));
  }
#ifdef AF_INET6
  else if (family == SOCKET_FAMILY_INET6) {
    memcpy(&ret->addr.sin6_addr, &any6_addr.s6_addr, sizeof(any6_addr.s6_addr));
  }
#endif
  else {
    free(ret);
    return NULL;
  }

  ret->family = family;
  ret->port = port;

  return ret;
}

SocketAddress *socket_address_new_loopback(SocketFamily family, uint16_t port) {
  SocketAddress *ret;
  uint8_t loop_addr[] = {127, 0, 0, 0};
#ifdef AF_INET6
  struct in6_addr loop6_addr = IN6ADDR_LOOPBACK_INIT;
#endif

  if (UNLIKELY((ret = calloc(sizeof(SocketAddress), 1)) == NULL)) {
    ALERT_ERROR("SocketAddress::socket_address_new_loopback: failed to allocate memory");
    return NULL;
  }

  if (family == SOCKET_FAMILY_INET) {
    memcpy(&ret->addr.sin_addr, loop_addr, sizeof(loop_addr));
  }
#ifdef AF_INET6
  else if (family == SOCKET_FAMILY_INET6) {
    memcpy(&ret->addr.sin6_addr, &loop6_addr.s6_addr, sizeof(loop6_addr.s6_addr));
  }
#endif
  else {
    free(ret);
    return NULL;
  }

  ret->family = family;
  ret->port = port;

  return ret;
}

bool socket_address_to_native(const SocketAddress *addr, void * dest, size_t destlen) {
  struct sockaddr_in *sin;
#ifdef AF_INET6
  struct sockaddr_in6 *sin6;
#endif

  if (UNLIKELY(addr == NULL || dest == NULL || destlen == 0)) {
    return false;
  }

  sin = (struct sockaddr_in *) dest;
#ifdef AF_INET6
  sin6 = (struct sockaddr_in6 *) dest;
#endif

  if (addr->family == SOCKET_FAMILY_INET) {
    if (UNLIKELY(destlen < sizeof(struct sockaddr_in))) {
      ALERT_WARNING("SocketAddress::socket_address_to_native: invalid buffer size for IPv4");
      return false;
    }

    memcpy(&sin->sin_addr, &addr->addr.sin_addr, sizeof(struct in_addr));
    sin->sin_family = AF_INET;
    sin->sin_port = htons(addr->port);
    memset(sin->sin_zero, 0, sizeof(sin->sin_zero));
    return true;
  }
#ifdef AF_INET6
  else if (addr->family == SOCKET_FAMILY_INET6) {
    if (UNLIKELY(destlen < sizeof(struct sockaddr_in6))) {
      ALERT_ERROR("SocketAddress::socket_address_to_native: invalid buffer size for IPv6");
      return false;
    }

    memcpy(&sin6->sin6_addr, &addr->addr.sin6_addr, sizeof(struct in6_addr));
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons(addr->port);
    sin6->sin6_flowinfo = addr->flowinfo;
    sin6->sin6_scope_id = addr->scope_id;
    return true;
  }
#endif
  else {
    ALERT_WARNING("SocketAddress::socket_address_to_native: unsupported socket address");
    return false;
  }
}

size_t socket_address_get_native_size(const SocketAddress *addr) {
  if (UNLIKELY(addr == NULL)) {
    return 0;
  }

  if (addr->family == SOCKET_FAMILY_INET) {
    return sizeof(struct sockaddr_in);
  }
#ifdef AF_INET6
  else if (addr->family == SOCKET_FAMILY_INET6) {
    return sizeof(struct sockaddr_in6);
  }
#endif
  else {
    ALERT_WARNING("SocketAddress::socket_address_get_native_size: unsupported socket family");
    return 0;
  }
}

SocketFamily socket_address_get_family(const SocketAddress *addr) {
  if (UNLIKELY(addr == NULL)) {
    return SOCKET_FAMILY_UNKNOWN;
  }

  return addr->family;
}

char *socket_address_get_address(const SocketAddress *addr) {
#ifdef AF_INET6
  char buffer[INET6_ADDRSTRLEN];
#else
  char buffer[INET_ADDRSTRLEN];
#endif

#ifdef _WINDOWS
  DWORD buflen = sizeof(buffer);
  DWORD addrlen;
  struct sockaddr_storage sa;
  struct sockaddr_in *sin;
#  ifdef AF_INET6
  struct sockaddr_in6 *sin6;
#  endif /* AF_INET6 */
#endif /* _WINDOWS */

  if (UNLIKELY(addr == NULL || addr->family == SOCKET_FAMILY_UNKNOWN)) {
    return NULL;
  }
#ifdef _WINDOWS
  sin = (struct sockaddr_in *) &sa;
#  ifdef AF_INET6
  sin6 = (struct sockaddr_in6 *) &sa;
#  endif /* AF_INET6 */
#endif /* _WINDOWS */

#ifdef _WINDOWS
  memset(&sa, 0, sizeof(sa));
#endif

#ifdef _WINDOWS
  sa.ss_family = addr->family;

  if (addr->family == SOCKET_FAMILY_INET) {
    addrlen = sizeof(struct sockaddr_in);
    memcpy(&sin->sin_addr, &addr->addr.sin_addr, sizeof(struct in_addr));
  }
#  ifdef AF_INET6
  else {
    addrlen = sizeof(struct sockaddr_in6);
    memcpy(&sin6->sin6_addr, &addr->addr.sin6_addr, sizeof(struct in6_addr));
  }
#  endif /* AF_INET6 */

  if (UNLIKELY(WSAAddressToStringA ((LPSOCKADDR) &sa,
               addrlen,
               NULL,
               (LPSTR) buffer,
               &buflen) != 0)) {
    return NULL;
  }
#else /* !_WINDOWS */
  if (addr->family == SOCKET_FAMILY_INET) {
    inet_ntop(AF_INET, &addr->addr.sin_addr, buffer, sizeof(buffer));
  }
#  ifdef AF_INET6
  else {
    inet_ntop(AF_INET6, &addr->addr.sin6_addr, buffer, sizeof(buffer));
  }
#  endif /* AF_INET6 */
#endif /* _WINDOWS */

  return strdup(buffer);
}

uint16_t socket_address_get_port(const SocketAddress *addr) {
  if (UNLIKELY(addr == NULL)) {
    return 0;
  }

  return addr->port;
}

uint32_t socket_address_get_flow_info(const SocketAddress *addr) {
  if (UNLIKELY(addr == NULL)) {
    return 0;
  }

#if !defined(AF_INET6) || !defined(PLIBSYS_SOCKADDR_IN6_HAS_FLOWINFO)
  return 0;
#else
  if (UNLIKELY(addr->family != SOCKET_FAMILY_INET6)) {
    return 0;
  }

  return addr->flowinfo;
#endif
}

uint32_t socket_address_get_scope_id(const SocketAddress *addr) {
  if (UNLIKELY(addr == NULL)) {
    return 0;
  }

#if !defined(AF_INET6) || !defined(PLIBSYS_SOCKADDR_IN6_HAS_SCOPEID)
  return 0;
#else
  if (UNLIKELY(addr->family != SOCKET_FAMILY_INET6)) {
    return 0;
  }

  return addr->scope_id;
#endif
}

void socket_address_set_flow_info(SocketAddress *addr, uint32_t flowinfo) {
  if (UNLIKELY(addr == NULL)) {
    return;
  }

#if !defined(AF_INET6) || !defined(PLIBSYS_SOCKADDR_IN6_HAS_FLOWINFO)
  UNUSED(flowinfo);
  return;
#else
  if (UNLIKELY(addr->family != SOCKET_FAMILY_INET6)) {
    return;
  }

  addr->flowinfo = flowinfo;
#endif
}

void socket_address_set_scope_id(SocketAddress *addr, uint32_t scope_id) {
  if (UNLIKELY(addr == NULL)) {
    return;
  }

#if !defined(AF_INET6) || !defined(PLIBSYS_SOCKADDR_IN6_HAS_SCOPEID)
  UNUSED(scope_id);
  return;
#else
  if (UNLIKELY(addr->family != SOCKET_FAMILY_INET6)) {
    return;
  }

  addr->scope_id = scope_id;
#endif
}

bool socket_address_is_flow_info_supported() {
#ifdef AF_INET6
  return true;
#else
  return false;
#endif
}

bool socket_address_is_scope_id_supported() {
#ifdef AF_INET6
  return true;
#else
  return false;
#endif
}

bool socket_address_is_ipv6_supported() {
#ifdef AF_INET6
  return true;
#else
  return false;
#endif
}

bool socket_address_is_any(const SocketAddress *addr) {
  uint32_t addr4;

  if (UNLIKELY(addr == NULL || addr->family == SOCKET_FAMILY_UNKNOWN)) {
    return false;
  }

  if (addr->family == SOCKET_FAMILY_INET) {
    addr4 = ntohl(* ((uint32_t *) &addr->addr.sin_addr));

    return (addr4 == INADDR_ANY);
  }
#ifdef AF_INET6
  else {
    return IN6_IS_ADDR_UNSPECIFIED (&addr->addr.sin6_addr);
  }
#else
  else {
    return false;
  }
#endif
}

bool socket_address_is_loopback (const SocketAddress *addr) {
  uint32_t addr4;

  if (UNLIKELY(addr == NULL || addr->family == SOCKET_FAMILY_UNKNOWN)) {
    return false;
  }

  if (addr->family == SOCKET_FAMILY_INET) {
    addr4 = ntohl(* ((uint32_t *) &addr->addr.sin_addr));

    /* 127.0.0.0/8 */
    return ((addr4 & 0xff000000) == 0x7f000000);
  }
#ifdef AF_INET6
  else {
    return IN6_IS_ADDR_LOOPBACK (&addr->addr.sin6_addr);
  }
#else
  else {
    return false;
  }
#endif
}

void socket_address_free (SocketAddress *addr) {
  if (UNLIKELY(addr == NULL)) {
    return;
  }

  free(addr);
}
