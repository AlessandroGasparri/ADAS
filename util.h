/*util.h*/
#include <netinet/in.h> /* For AFINET sockets */

extern int readLine(int, char *); 

extern void logOutput(char [], char [], int);

extern char* substring(char *, int);

extern void sendToCenEcu(int, char []);

extern int searchForBytes(char *, int , char *, int);

extern void createSocket(int* , struct sockaddr* , int* , int );
    

extern void createUDPSocket(int*, int);
    