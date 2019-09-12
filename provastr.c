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


void runForwardFacingRadar(){
    int fd;
    fd = open("/dev/random", O_RDONLY);
    size_t randomDataLen = 0;
    while (1){
        char randomBytes[24];
        randomBytes[0] = 'A';
        ssize_t result = read(fd, randomBytes+1, 23);
        printf("SIZE %d\n", sizeof randomBytes);
        //if (result < 24){
            //Ho letto meno di 24 byte in teoria
            printf("%ld\n", result);
            for(int i = 0; i < result; i++){
                printf("%d ",randomBytes[i]);
            }
            printf("\n");
        //}
        sleep(2);
    }
    close(fd);
}

int main() {
    runForwardFacingRadar();
}