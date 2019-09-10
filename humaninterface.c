#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
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

/*	sleep(1); //delay to wait central ecu to open the socket
	int hum_int_Fd, cen_ecu_Len, result;
	struct sockaddr_in cen_ecu_INETAddress;
	struct sockaddr*  cen_ecu_SockAddrPtr;
	cen_ecu_SockAddrPtr=(struct sockaddr*)
	&cen_ecu_INETAddress;
	cen_ecu_Len = sizeof (cen_ecu_INETAddress);
	hum_int_Fd=socket(AF_INET,SOCK_STREAM,DEFAULT_PROTOCOL);
	cen_ecu_INETAddress.sin_family = AF_INET;
	cen_ecu_INETAddress.sin_port = htons (HUMAN_INTERFACE_PORT);
	cen_ecu_INETAddress.sin_addr.s_addr = htonl
	(INADDR_LOOPBACK);
	pid_t output_pid;
	do {
//Loop until a connection is made with the server 
		result=connect(hum_int_Fd,  cen_ecu_SockAddrPtr,cen_ecu_Len);

		if (result == -1) {
			printf("connessione fallita");
			sleep (1);// Wait, try again 
		}
	} while (result == -1);
	printf("Connected successesfully to Central Ecu\n");
	printf("Write anything: \n");

	char str[100];
	output_pid = fork();
	printf("output_pid %d", output_pid);
	
	if( output_pid != 0){
		do {
			scanf("%s", &str);
			write(hum_int_Fd, str, strlen(str)+1);
		}while( strcmp(str,"FINE") != 0 );

		printf('killing');
		sleep(3);
		close (hum_int_Fd); // Close the socket 
		exit ( 0); // Done 
	}
	else {
		execl("/usr/bin/xterm","xterm", "-e","tail -f  ./centralecu.log",0);
	} */

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
	output_pid = fork();
	printf("output_pid %d", output_pid);
	
	if( output_pid != 0){
		do {
			scanf("%s", &str);
			sendto(hum_int_sockFd, (const char *) str, strlen(str) + 1, MSG_CONFIRM,
    		(const struct sockaddr *) &cen_ecu_addr, sizeof(cen_ecu_addr));
		}while( strcmp(str,"FINE") != 0 );

		kill(output_pid, SIGKILL);
		close (hum_int_sockFd); // Close the socket 
		exit ( 0); // Done 
	}
	else {
		execl("/usr/bin/xterm","xterm", "-e","tail -f  ./centralecu.log",0);
	} 

    

    close(hum_int_sockFd);
    return 0;

}