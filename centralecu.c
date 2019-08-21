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
#define HUMAN_INTERFACE_PORT 1025
#define UDP_CENECU_PORT 1026
#define MAXLINE 1024

int readLine(int fd, char*str) {
    int n; 
    do {
        n = read (fd,str,1);
    } while ( n> 0 && *str++ != '\0');
    return (n>0);
}


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



int main(){

/*int cen_ecu_Fd, hum_int_len, hum_int_Fd, hum_int_status;
struct sockaddr* hum_int_SockAddrPtr;
pid_t hum_int_pid;
createSocket(&cen_ecu_Fd, hum_int_SockAddrPtr, &hum_int_len, HUMAN_INTERFACE_PORT); //Genereting socket for human interface
hum_int_pid = fork();
if(hum_int_pid == 0 )
{
    execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL); // execute human interface in a focked process on a new terminal
    exit(1);
}
else {
    printf("waiting for the human interface...\n");
    hum_int_Fd=accept(cen_ecu_Fd,hum_int_SockAddrPtr,&hum_int_len); //waiting the connection of the human interface generated
    printf("human interface connected\n %d", hum_int_Fd);
    char str[200];
    while(readLine(hum_int_Fd, str) && strcmp(str,"FINE") != 0){
        printf("%s\n", str);
    }
    waitpid(hum_int_pid, &hum_int_status, 0);

    printf("\nFINE %d", hum_int_status);
    close (hum_int_Fd); // Close the socket 
    exit (0); // Terminate 


}*/

int cen_ecu_sockFd, hum_int_len, hum_int_Fd;
struct sockaddr* clientAddr;
pid_t hum_int_pid;
char buffer[MAXLINE];

createUDPSocket(&cen_ecu_sockFd, UDP_CENECU_PORT); //Genereting socket for human interface
hum_int_pid = fork();
if(hum_int_pid == 0 )
{
    execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL); // execute human interface in a focked process on a new terminal
    exit(1);
}
else {
    
    do{
        int n, len;
        printf("Waiting for a message...\n");
        n = recvfrom(cen_ecu_sockFd,(char *)buffer, MAXLINE, MSG_WAITALL, clientAddr, &len);
        buffer[n] = '\0';
        printf("Client %s\n", buffer);
    }while(strcmp(buffer,"FINE") != 0);
}
return 1;
}


