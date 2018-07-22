#include <stdio.h>
#include <stdlib.h>
#include "socket.h"
#include "error.h"
#include "util.h"

#define PORT 13
#define BUF_SIZE 512

/*
GET /api/users?page=2 HTTP/1.1
Host: https://reqres.in



*/

int32_t main(void) {
/* disables output buffering on mingw */
#if defined(_WINDOWS)
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
#endif

	if (!socket_init_once()) {
		ALERT_ERROR("failed to initialize");
	}

	atexit(socket_close_once);

	Socket *client = socket_new(SOCKET_FAMILY_INET6, SOCKET_TYPE_STREAM, SOCKET_PROTOCOL_TCP);
	if (!client) {
		ALERT_ERROR(error_get_message());
		return 0;
	}

	// SocketAddress *address = socket_address_new("https://www.reqres.in", PORT);
	SocketAddress *address = socket_address_new("time-nw.nist.gov", PORT);

	if (!address) {
		ALERT_ERROR("%d\n", error_get_last_net());
		return 0;
	}

	if (!socket_connect(client, address)) {
		ALERT_ERROR(error_get_message());	
		return 0;
	}

	socket_address_free(address);

	ssize_t recieved = 0;
	char buf[BUF_SIZE] = {0};
	socket_set_blocking(client, false);

	do {
		recieved = socket_receive(client, buf, BUF_SIZE);
		int32_t err = error_get_code();
		switch (err) {
			case ERROR_IO_WOULD_BLOCK:
				continue;
			default:
				ALERT_ERROR(error_code_to_string(err));
		}
	} while (recieved > 0);

	socket_free(client);
}