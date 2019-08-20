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
#define DEFAULT_PROTOCOL 0

int readLine(int fd, char*str) {
    int n; 
    do {
        n = read (fd,str,1);
    } while ( n> 0 && *str++ != '\0');
    return (n>0);
}



int main(){

int serverFd, clientFd, serverLen, clientLen;
unsigned short int port = 1025;
struct sockaddr_in serverINETAddress;/*Server address*/
struct sockaddr_in clientINETAddress;/*Client address*/
struct sockaddr* serverSockAddrPtr;/*Ptr to server*/
struct sockaddr* clientSockAddrPtr; /*Ptr to client
/* Ignore death-of-child signals to prevent zombies */
signal (SIGCHLD, SIG_IGN);
serverSockAddrPtr=(struct sockaddr*) &serverINETAddress;
serverLen = sizeof(serverINETAddress);
clientSockAddrPtr=(struct sockaddr*) &clientINETAddress;
clientLen=sizeof (clientINETAddress);/*Create a INET socket, bidirectional, default protocol */
serverFd=socket (AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
serverINETAddress.sin_family = AF_INET;
serverINETAddress.sin_port = htons (port);
serverINETAddress.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
bind(serverFd, serverSockAddrPtr, serverLen);
listen (serverFd, 2);
pid_t pid; 
int i = 0;
int status;

    pid = fork();

    if(pid == 0 )
    {
        printf("sono il client");
        execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL);
        exit(1);
    }
    else {
        printf("sono il server");
        printf("waiting for the client\n");
        clientFd=accept(serverFd,clientSockAddrPtr,&clientLen);

        printf("connected\n %d", clientFd);
            char str[200];
            while(readLine(clientFd, str) && strcmp(str,"$") != 0){
                printf("%s\n", str);
            }
            printf("\n fine zebra");
            close (clientFd); /* Close the socket */
            exit (/* EXIT_SUCCESS */ 0); /* Terminate */

    }

   




return 1;
}


