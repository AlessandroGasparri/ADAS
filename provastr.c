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

#include <stdarg.h>

void killProc(int count, ...){
	va_list args;
	va_start(args, count);
	for(int i = 0; i< count; i++){
		int temp = va_arg(args, int);
		printf("%d", temp);
	}
	va_end(args);
}

int main() {
    killProc(4,1,2,3,4);
}