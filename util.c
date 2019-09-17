#include "util.h"
#include "constants.h"

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

int readLine(int fd, char*str) {
    int n; 
    do {
        n = read(fd,str,1);
    } while ( n> 0 && *str++ != '\0');
    return (n>0);
}

void logOutput(char path[100], char str[MAXLINE], int binary){
    FILE *fptr;
    if(!binary)
        fptr = fopen(path, "a");
    else{
        fptr = fopen(path, "wb");
    }
    if(fptr == NULL){
        printf("File not found\n");
    }
    else{
        if(binary){
            fwrite(str, 1, strlen(str), fptr);
        }
        else{
            fputs(str, fptr);
            fputs("\n", fptr);
        }
        fclose(fptr);
    }
}

char* substring(char *src, int from){

    int len = 0, i = 0;
    while(*(src + from + len) != '\0'){
        len++;
    }
    len++;

    
    char *dest = (char*) malloc(sizeof(char) * (len + 1));
    
    for (i = from; i< (from + len); i++ ){
        *dest = *(src + i);
        dest ++;
    }
    dest -= len;
    return dest;
}



int searchForBytes(char *toSearchIn, int lenght, char *values, int nValues){
    //0x 17 2A , ii) 0xD693, iiI) 0xBDD8, iv) 0xFAEE, v) 0x4300
    for(int i = 0; i < lenght-1; i++){
        for(int j = 0; j < nValues-1; j+=2){
            if(toSearchIn[i] == values[j] && toSearchIn[i+1] == values[j+1]){
                return 1;
            }
        }
    }
    return 0;
}


//Socket and communication functions
void createSocket(int* serverFd, struct sockaddr* clientSockAddrPtr, int* clientLen, int port){
    int serverLen;
    struct sockaddr_in serverINETAddress;/*Server address*/
    struct sockaddr_in clientINETAddress;/*Client address*/
    struct sockaddr* serverSockAddrPtr;/*Ptr to server*/
    /* Ignore death-of-child signals to prevent zombies */
    signal (SIGCHLD, SIG_IGN);
    serverSockAddrPtr=(struct sockaddr*) &serverINETAddress;
    serverLen = sizeof(serverINETAddress);
    clientSockAddrPtr=(struct sockaddr*) &clientINETAddress;
    *clientLen=sizeof (clientINETAddress);/*Create a INET socket, bidirectional, default protocol */
    *serverFd=socket (AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    serverINETAddress.sin_family = AF_INET;
    serverINETAddress.sin_port = htons (port);
    serverINETAddress.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    bind(*serverFd, serverSockAddrPtr, serverLen);
    listen (*serverFd, 2);
}

void createUDPSocket(int* serverFd, int port){
    int serverLen;
    struct sockaddr_in serverINETAddress;/*Server address*/
   // struct sockaddr_in clientINETAddress;/*Client address*/
    struct sockaddr* serverSockAddrPtr;/*Ptr to server*/
    /* Ignore death-of-child signals to prevent zombies */
    signal (SIGCHLD, SIG_IGN);
    *serverFd=socket (AF_INET, SOCK_DGRAM, DEFAULT_PROTOCOL);

    memset(&serverINETAddress, 0 , sizeof(serverINETAddress));
    serverSockAddrPtr=(struct sockaddr*) &serverINETAddress;
    serverLen = sizeof(serverINETAddress);
    //clientSockAddrPtr=(struct sockaddr*) &clientINETAddress;
   // *clientLen=sizeof (clientINETAddress);/*Create a INET socket, bidirectional, default protocol */
    
    serverINETAddress.sin_family = AF_INET;
    serverINETAddress.sin_port = htons (port);
    serverINETAddress.sin_addr.s_addr= INADDR_ANY;
    bind(*serverFd, serverSockAddrPtr, serverLen);
}