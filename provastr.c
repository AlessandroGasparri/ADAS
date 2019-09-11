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
    fptr = fopen("/dev/urandom", "r");
    
    char randomBytes[4];
    fread(randomBytes, 1, 4, fptr);
    
    fclose(fptr);
    exit(0);
}