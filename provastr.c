#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> /* For AFINET sockets */
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include "constants.h"

#include <unistd.h>



int main() {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 900000000;
	int res;

	do {
		res = nanosleep(&ts, &ts);
	} while(res && errno == EINTR);
	printf("ciao 2\n");
}