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


void main(){
    FILE *fptr;
    fptr = fopen("/dev/random", "r");
    
    for(int j = 0; j< 10; j++){
        char randomBytes[4];
        fread(randomBytes, 1, 4, fptr);
        for(int i = 0; i < 4; ++i){
            printf("sring: %s\n", randomBytes);
            //printf("Integer: %d\n", randomBytes[i]);
        }
    }
    fclose(fptr);
    exit(0);
}