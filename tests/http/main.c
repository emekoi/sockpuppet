#include <stdio.h>
#include <stdlib.h>
#include "socket.h"
#include "error.h"
#include "util.h"

#define PORT 8888
#define BUF_SIZE 512
const char *RESPONSE = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 12\r\n\r\nhttp example\r\n";

int32_t main() {
#if defined(_WINDOWS)
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);
#endif

	if (!socket_init_once()) {
		ALERT_ERROR("failed to initialize");
	}

	atexit(socket_close_once);

	ssize_t RESPONSE_LEN = strlen(RESPONSE);

	Socket *server = socket_new(SOCKET_FAMILY_INET, SOCKET_TYPE_STREAM, SOCKET_PROTOCOL_TCP);
	if (!server) {
		ALERT_ERROR(error_get_message());
		return 0;
	}

	SocketAddress *address = socket_address_new_any(SOCKET_FAMILY_INET, PORT);
	if (!address) {
		ALERT_ERROR(error_get_message());
		return 0;
	}

	if (!socket_bind(server, address, true)) {
		ALERT_ERROR(error_get_message());
		return 0;
	}

	socket_address_free(address);

	if (!socket_listen(server)) {
		ALERT_ERROR(error_get_message());	
		return 0;
	}

	printf("serving http on port %u\n\n", PORT);
	while (true) {
		Socket *client = socket_accept(server);

		if (!client) {
			ALERT_ERROR(error_get_message());	
		}

		ssize_t recieved = 0;
		char buf[BUF_SIZE] = {0};
		socket_set_blocking(client, false);
		// socket_set_keepalive(client, true);

		SocketAddress *info = socket_get_remote_address(client);
		printf("new connection from ('%s', '%d')\n", socket_address_get_address(info), socket_address_get_port(info));
		socket_address_free(info);

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

		if (socket_send(client, RESPONSE, RESPONSE_LEN) != RESPONSE_LEN)  {
			ALERT_ERROR(error_get_message());
		}
		socket_free(client);
	}
}