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
#include "constants.h"

int tc_sock_pair[2]; //TCP socket descriptors to communicate with throttle
int bbw_sock_pair[2]; //TCP socked descriptors to communicate with break by wire

int readLine(int fd, char*str) {
    int n; 
    do {
        n = read(fd,str,1);
    } while ( n> 0 && *str++ != '\0');
    return (n>0);
}

void logOutput(char path[100], char str[MAXLINE]){
    FILE *fptr;

    fptr = fopen(path, "a");
    if(fptr == NULL){
        printf("File not found\n");
    }
    else{
        fputs(str, fptr);
        fputs("\n", fptr);
        fclose(fptr);
    }
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

void runThrottleControl(int fd, int* speed){
   // printf("Throttle running, speed: %d\n",*speed);
    char str[MAXLINE];
    while(readLine(fd, str)){
       // printf("Throttle: %s\n", str);
    }
    exit(0);
}

void sendToCenEcu(int fd, char str[MAXLINE]){
    struct sockaddr_in cen_ecu_addr;
    memset(&cen_ecu_addr, 0 , sizeof(cen_ecu_addr));
    cen_ecu_addr.sin_family = AF_INET;
    cen_ecu_addr.sin_port = htons (UDP_CENECU_PORT);
    cen_ecu_addr.sin_addr.s_addr= INADDR_ANY;

    sendto(fd, (const char *) str, strlen(str) + 1, MSG_CONFIRM,
        (const struct sockaddr *) &cen_ecu_addr, sizeof(cen_ecu_addr));
}

void runFrontWindshieldCamera(){

    printf("Front camera is running\n");
    int fc_sockFd;
    FILE *fptr;
    fptr = fopen("input/frontCamera.data", "r");
    int speedLimit;
    char line[MAXLINE];
    char output[MAXLINE];
    output[0] = FRONT_CAMERA_CODE;

    fc_sockFd = socket(AF_INET, SOCK_DGRAM, 0);

    while (fgets(line, sizeof(line), fptr)){
        //speedLimit = atoi(line);
        strcpy(output + 1, line);
        output[strlen(output)-1] = '\0';
        //printf("line: %s ,Lunga: %d", line, strlen(line));
        //printf("outp: %s ,Lunga: %d\n\n", output, strlen(output));

        sendToCenEcu(fc_sockFd, output);
        sleep(1);
    }
    fclose(fptr);
    close(fc_sockFd);
    exit(0);
}

void runBrakeByWire(int fd){
    char str[MAXLINE];
    while(1){
        printf("PIPPOOOO");
        int n = 0;
        n = read(fd, str, 1);
        if(n != 0){
            readLine(fd, str+1);
            printf("COMANDO: %s", str);
        }
        else{
            printf("NO ACTION");
        }
        sleep(1);
    }
    exit(0);
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

/*void handleSensorInput(char input[MAXLINE]){
    char code;
    code = input[0];
    char *command;
    command = substring(input, 1);
    switch(code){
        case FRONT_CAMERA_CODE:
            if(command[0] > '0' && command[0] < '9'){ //There is a number to process
                int newSpeed = atoi(command);
                if(newSpeed < speed){
                    write(bbw_sock_pair[0], command, strlen(command) + 1);
                }
            }else{ // We have a string to process
                printf("command %s", command);
            }
            break;
        default: printf("Unknown command.\n");
    }

    printf("command code: %c\n", code);

    printf("command %s\n", command);
}
*/



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

    int cen_ecu_sockUDPFd; //UDP socket descriptor Central ECU
    int hum_int_len; // NON USATO DA DEFINIRE
    int hum_int_Fd; //NON USATO DA DEFINIRE
    struct sockaddr* clientAddr;
    pid_t fwc_pid; //PID front wide camera process
    pid_t tc_pid; //PID throttle control process
    pid_t hum_int_pid; //PID for human interface
    pid_t bbw_pid; //PID for break by wire
    int speed = 200; //Car speed
    char buffer[MAXLINE]; //Buffer to store UDP messages
    int started = 0; //Boolean variable to tell if process has received INIZIO

    createUDPSocket(&cen_ecu_sockUDPFd, UDP_CENECU_PORT); //Generating socket for human interface
    socketpair(AF_UNIX, SOCK_STREAM, 0, tc_sock_pair); //Handshaking of TCP sockets now in tc_sock_pair i have 2 connected sockets
    socketpair(AF_UNIX, SOCK_STREAM, 0, bbw_sock_pair);
    
    printf("Processo padre %d\n", getpid());
    if((hum_int_pid = fork()) == 0){ //I'm the child humaninterface
        execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL); // execute human interface in a forked process on a new terminal
        exit(0);
    }
    else if((tc_pid = fork()) == 0){
        printf("  trottol da %d\n", getpid());
        close(tc_sock_pair[0]); //I'm the child and i close father socket
        runThrottleControl(tc_sock_pair[1], &speed); //funzione per controllare la velocita TODO
    }
    else if( (fwc_pid = fork()) == 0 ) {
        printf("  camera da %d\n", getpid());
        runFrontWindshieldCamera();
    }
    else if( (bbw_pid = fork()) == 0){
        printf("Break by wire %d\n", getpid());
        close(bbw_sock_pair[0]); 
        runBrakeByWire(bbw_sock_pair[1]);
    }
    else {//In this else i'm the father
        close(tc_sock_pair[1]); //I'm the father and i close child socket
        close(bbw_sock_pair[1]);
        do{
            int n, len;
            n = recvfrom(cen_ecu_sockUDPFd,(char *)buffer, MAXLINE, MSG_WAITALL, clientAddr, &len);
            printf("%s\n", buffer);
            int bufferlenght = strlen(buffer);
            printf("Lunghezza buffer: %d\n", bufferlenght);
            
            if(!started){
                if(strcmp(buffer,"INIZIO") == 0){
                    started = 1;
                }

            }
            else{
                if(strcmp(buffer,"FINE") != 0){
                    //handleSensorInput(buffer);
                    //write(tc_sock_pair[0], buffer, strlen(buffer)+1);
                    char code;
                    code = buffer[0];
                    char *command;
                    command = substring(buffer, 1);
                    switch(code){
                        case FRONT_CAMERA_CODE:
                            if(command[0] > '0' && command[0] < '9'){ //There is a number to process
                                int newSpeed = atoi(command);
                                if(newSpeed < speed){
                                    char stroutput[MAXLINE];
                                    char valueBuffer[5];
                                    sprintf(valueBuffer, "%d", speed);
                                    strcpy(stroutput, valueBuffer);
                                    strcat(stroutput, "-");
                                    strcat(stroutput, command);
                                    write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                }
                            }else{ // We have a string to process
                                //printf(" %s", command);
                            }
                            break;
                        default: printf("Unknown command. %s\n",command);
                    }

                    
                    logOutput("centralecu.log", buffer);
                }
                
            }
        }while(strcmp(buffer,"FINE") != 0);
    }
    close(cen_ecu_sockUDPFd);

    kill(fwc_pid, SIGKILL);
    kill(tc_pid, SIGKILL);
    kill(bbw_pid, SIGKILL);
    printf("FINE\n");
    exit(0);
    return 0;
}


