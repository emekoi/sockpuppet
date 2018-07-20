#include <stdio.h>
#include "socket.h"

int main() {
	#ifdef _WIN32
		printf("%s\n", "sss");
	#endif
	return 0;
}