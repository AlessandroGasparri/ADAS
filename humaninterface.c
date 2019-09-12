#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <netinet/in.h> /* For AF_INET sockets */
#define DEFAULT_PROTOCOL 0
#define HUMAN_INTERFACE_PORT 1025
#define MAXLINE 1024


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

int main (void) {

	int hum_int_sockFd;
	struct sockaddr_in cen_ecu_addr;
	pid_t hum_int_pid;
	char buffer[MAXLINE];
	char* hello = "hello";
	pid_t output_pid;

	hum_int_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&cen_ecu_addr, 0 , sizeof(cen_ecu_addr));
	cen_ecu_addr.sin_family = AF_INET;
    cen_ecu_addr.sin_port = htons (1026);
    cen_ecu_addr.sin_addr.s_addr= INADDR_ANY;


	char str[100];
	printf("*****************************************************\n");
	printf("*****************HUMAN INTERFACE INPUT***************\n");
	printf("*****************************************************\n\n\n");	
	output_pid = fork();
	if( output_pid != 0){
		do {
			printf("Inserisci un comando: ");
			scanf("%s", str);
			sendto(hum_int_sockFd, (const char *) str, strlen(str) + 1, MSG_CONFIRM,
    		(const struct sockaddr *) &cen_ecu_addr, sizeof(cen_ecu_addr));
		}while( strcmp(str,"FINE") != 0 );

		kill(output_pid, SIGKILL);
		close (hum_int_sockFd); // Close the socket 
		exit ( 0); // Done 
	}
	else {
		execl("/usr/bin/xterm","xterm", "-e","tail -f  ./ECU.log",(char *)0);
	} 

    

    close(hum_int_sockFd);
    return 0;

}