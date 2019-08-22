#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* For AF_INET sockets */
#define DEFAULT_PROTOCOL 0
#define THROTTLE_CONTROL_PORT 1025
#define MAXLINE 1024




int main (void) {

	sleep(1); //delay to wait central ecu to open the socket
	int thr_con_Fd, cen_ecu_Len, result;
	struct sockaddr_in cen_ecu_INETAddress;
	struct sockaddr*  cen_ecu_SockAddrPtr;
	cen_ecu_SockAddrPtr=(struct sockaddr*)
	&cen_ecu_INETAddress;
	cen_ecu_Len = sizeof (cen_ecu_INETAddress);
	thr_con_Fd=socket(AF_INET,SOCK_STREAM,DEFAULT_PROTOCOL);
	cen_ecu_INETAddress.sin_family = AF_INET;
	cen_ecu_INETAddress.sin_port = htons (THROTTLE_CONTROL_PORT);
	cen_ecu_INETAddress.sin_addr.s_addr = htonl
	(INADDR_LOOPBACK);

	do {
	//Loop until a connection is made with the server 
		result=connect(thr_con_Fd,  cen_ecu_SockAddrPtr,cen_ecu_Len);

		if (result == -1) {
			printf("connessione fallita");
			sleep (1);// Wait, try again 
		}
	} while (result == -1);
	printf("TC Connected successesfully to Central Ecu\n");
	printf("TC Waiting for commands\n");

}