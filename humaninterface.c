#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* For AF_INET sockets */
#define DEFAULT_PROTOCOL 0


int main (void) {

	sleep(1);
	int clientFd, serverLen, result;
	unsigned short int port = 1025;
	struct sockaddr_in serverINETAddress;
	struct sockaddr* serverSockAddrPtr;
	serverSockAddrPtr=(struct sockaddr*)
	&serverINETAddress;
	serverLen = sizeof (serverINETAddress);
	clientFd=socket(AF_INET,SOCK_STREAM,DEFAULT_PROTOCOL);
	serverINETAddress.sin_family = AF_INET;
	serverINETAddress.sin_port = htons (port);
	serverINETAddress.sin_addr.s_addr = htonl
	(INADDR_LOOPBACK);
	do {
/* Loop until a connection is made with the server */
		result=connect(clientFd, serverSockAddrPtr,serverLen);

		if (result == -1) {
			printf("connessione fallita");
			sleep (1);/* Wait, try again */
		}
	} while (result == -1);
	printf("Connected successesfully to Central Ecu\n");
	printf("Write anything: \n");

	char str[100];
	int pid = fork();
	printf("pid %d", pid);
	
	if( pid != 0){
		do {
			scanf("%s", &str);
			write(clientFd, str, strlen(str)+1);
		}while( strcmp(str,"$") != 0 );

		close (clientFd); /* Close the socket */
		exit (/* EXIT_SUCCESS */ 0); /* Done */
	}
	else {
		execlp("/usr/bin/xterm","xterm", "-e","tail -f  ./centralecu.log",0);
	} 
}