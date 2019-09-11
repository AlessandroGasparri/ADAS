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

//Generic functions that can be in a util file
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

void sendToCenEcu(int fd, char str[MAXLINE]){
    struct sockaddr_in cen_ecu_addr;
    memset(&cen_ecu_addr, 0 , sizeof(cen_ecu_addr));
    cen_ecu_addr.sin_family = AF_INET;
    cen_ecu_addr.sin_port = htons (UDP_CENECU_PORT);
    cen_ecu_addr.sin_addr.s_addr= INADDR_ANY;

    sendto(fd, (const char *) str, strlen(str) + 1, MSG_CONFIRM,
        (const struct sockaddr *) &cen_ecu_addr, sizeof(cen_ecu_addr));
}


//Sensors functions
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

//Actuators functionz
void runBrakeByWire(int fd){
    char str[MAXLINE];
    char ack[2];
    int newSpeed;
    int bbw_sockFd; 
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    bbw_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    while(1){
        
            printf("faccio qualcosa e sono bbw");
        int n = 0;
        n = read(fd, str, 1);
        if(n > 0){
            readLine(fd, str+1);
            char *ptr = substring(str, 5); //strlen("FRENO ") = 5
            newSpeed = atoi(ptr);
            if(newSpeed >= 0){
                while(newSpeed > 0){
                    n = read(fd, str, 1);
                    if(n > 0){ // PERICOLO 
                        ack[0] = HALT_CODE;
                        ack[1] = '\0';
                        sendToCenEcu(bbw_sockFd, ack);
                        logOutput("brake.log", "ARRESTO AUTO");
                        break;
                    }
                    newSpeed -= 5;
                    ack[0] = BRAKE_BY_WIRE_CODE;
                    ack[1] = '\0';
                    sendToCenEcu(bbw_sockFd, ack);
                    logOutput("brake.log", "DECREMENTO 5");
                    sleep(1);
                }
            }
            else{ //PERICOLO
                ack[0] = HALT_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack);
                logOutput("brake.log", "ARRESTO AUTO");
            }
        }
        else{
            logOutput("brake.log", "NO ACTION");
        }
        sleep(1);
    }
    close(bbw_sockFd);//TODO qui non ci arriva mai credo
    exit(0);
}

void runThrottleControl(int fd){
    char str[MAXLINE];
    char ack[2];
    ack[0] = THROTTLE_CONTROL_CODE;
    ack[1] = '\0';
    int newSpeed;
    int tc_sockFd;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    tc_sockFd = socket(AF_INET, SOCK_DGRAM, 0);

    while(1){
        int n = 0;
        n = read(fd, str, 1);
        if(n > 0){
            readLine(fd, str+1);
            //splits into two string by delimiter "-" and puts the current speed integer value in speeds[0] the one to reach in speeds[1]
            char *ptr = substring(str, 10); //strlen("INCREMENTO ") = 10
            newSpeed = atoi(ptr);
            while(newSpeed > 0){
                newSpeed -= 5;
                sendToCenEcu(tc_sockFd, ack);
                logOutput("throttle.log", "AUMENTO 5");        
                sleep(1);
            }
        }
        else{
            logOutput("throttle.log", "NO ACTION");        
        }
        sleep(1);
    }
    close(tc_sockFd);
    exit(0);
}

void runSteerByWire(int fd){
    char str[10];
    char ack[2];
    ack[0] = STEER_BY_WIRE_CODE;
    ack[1] = '\0';
    int newSpeed;
    int sbw_sockFd;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);//non bloccante
    sbw_sockFd = socket(AF_INET, SOCK_DGRAM, 0);

    while(1){
        int n = 0;
        n = read(fd, str, 1);
        if(n > 0){
            readLine(fd, str+1);
            char toWrite[40];
            strcpy(toWrite, "STO GIRANDO A ");
            strcat(toWrite, str);
            int i;
            for(i = 0; i < 4; i++){
                logOutput("steer.log", toWrite);
                printf("%s\n", toWrite);
                sleep(1);
            }
            sendToCenEcu(sbw_sockFd, ack);
        }
        else{
            logOutput("steer.log", "NO ACTION");        
        }
        sleep(1);
    }
    close(sbw_sockFd);
    exit(0);
}

void runParkAssist(int fd){
    char str[12];
    int pa_sockFd;
    int i = 0;
    int n = 0;
    pa_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    FILE *fptr;
    fptr = fopen("/dev/urandom", "r");
    readLine(fd, str); //Legge la prima volta bloccante poi deve leggere non bloccante per una richiesta facoltativa quella di ricominciare il parcheggio
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);//non bloccante
    while(i < 30){ //Do it for 30 seconds
        n = read(fd, str, 1);
        printf("Sto parcheggiando\n");
        if(n == 0){
            char randomBytes[4];
            fread(randomBytes, 1, 4, fptr); //Read 4 times 1 byte of data and puts in a byte array
            char toSend[6];
            toSend[0] = PARK_ASSIST_CODE; //Formatting toSend to conform to standard of a ECU message
            for(int j = 1; j < 5; j++)
                toSend[j] = randomBytes[j];
            toSend[5]= '\0';
            sendToCenEcu(pa_sockFd, toSend);
        }
        else{
            i = 0; //Restart parking procedure
        }
        sleep(1);
    }
    close(pa_sockFd);
    exit(0);
}

int main(){
    int cen_ecu_sockUDPFd; //UDP socket descriptor Central ECU
    int hum_int_len; // NON USATO DA DEFINIRE
    int hum_int_Fd; //NON USATO DA DEFINIRE
    struct sockaddr* clientAddr;
    int tc_sock_pair[2]; //TCP socket descriptors to communicate with throttle
    int bbw_sock_pair[2]; //TCP socked descriptors to communicate with break by wire
    int sbw_sock_pair[2]; //TCP socked descriptors to communicate with steer by wire
    int pa_sock_pair[2]; //TCP socked descriptors to communicate with park asssit
    pid_t fwc_pid; //PID front wide camera process
    pid_t tc_pid; //PID throttle control process
    pid_t hum_int_pid; //PID for human interface
    pid_t bbw_pid; //PID for break by wire
    pid_t sbw_pid; //PID for steer by wire
    pid_t pa_pid; //PID for park assist
    int speed = 20; //Car speed
    int updatingSpeed = 0; // semaphore to prevent cen ecu to send data to break by wire
    int steering = 0;
    int newSpeed = speed;
    char buffer[MAXLINE]; //Buffer to store UDP messages
    int started = 1; //Boolean variable to tell if process has received INIZIO
    int danger = 0;
    int parking = 0;
    int slowingToPark = 0;
    int countParking = 0;

    createUDPSocket(&cen_ecu_sockUDPFd, UDP_CENECU_PORT); //Generating socket for human interface
    socketpair(AF_UNIX, SOCK_STREAM, 0, tc_sock_pair); //Handshaking of TCP sockets now in tc_sock_pair i have 2 connected sockets
    socketpair(AF_UNIX, SOCK_STREAM, 0, bbw_sock_pair);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sbw_sock_pair);

    printf("Processo padre %d\n", getpid());
    if((hum_int_pid = fork()) == 0){ //I'm the child humaninterface
        execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL); // execute human interface in a forked process on a new terminal
        exit(0);
    }
    else if((tc_pid = fork()) == 0){
        printf("  trottol da %d\n", getpid());
        close(tc_sock_pair[0]); //I'm the child and i close father socket
        runThrottleControl(tc_sock_pair[1]); //funzione per controllare la velocita TODO
    }
    else if((fwc_pid = fork()) == 0 ) {
        printf("  camera da %d\n", getpid());
        runFrontWindshieldCamera();
    }
    else if((bbw_pid = fork()) == 0){
        printf("Break by wire %d\n", getpid());
        close(bbw_sock_pair[0]); 
        runBrakeByWire(bbw_sock_pair[1]);
    }
    else if((sbw_pid = fork()) == 0){
        printf("Steer by wire %d\n", getpid());
        close(sbw_sock_pair[0]);
        runSteerByWire(sbw_sock_pair[1]);
    }
    else {//In this else i'm the father
        close(tc_sock_pair[1]); //I'm the father and i close child socket
        close(bbw_sock_pair[1]);
        do{
            int n, len;
            n = recvfrom(cen_ecu_sockUDPFd,(char *)buffer, MAXLINE, MSG_WAITALL, clientAddr, &len);
            /*printf("%s\n", buffer);
            int bufferlenght = strlen(buffer);
            printf("Lunghezza buffer: %d\n", bufferlenght);*/
            if(!started){
                if(strcmp(buffer,"INIZIO") == 0){
                    started = 1;
                    danger = 0;
                }
            }
            else if(strcmp(buffer, "PARCHEGGIO") == 0){
                printf("PARCHEGGIO\n");
                slowingToPark = 1;
                char stroutput[MAXLINE];
                strcpy(stroutput, "FRENO 0");
                printf("Scrivo FRENO 0 a bbw\n");
                write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
            }
            else if(strcmp(buffer,"FINE") != 0){
                char code;
                code = buffer[0];
                char *command;
                command = substring(buffer, 1);
                char stroutput[MAXLINE];
                switch(code){
                    case FRONT_CAMERA_CODE:
                        if(!slowingToPark){
                            if(command[0] > '0' && command[0] < '9' ){ //There is a number to process
                                int readSpeed = atoi(command);
                                /*
                                    Command handled if speed is not being updated yet
                                */
                                if(!updatingSpeed && readSpeed != speed){
                                        updatingSpeed = 1;
                                        newSpeed = readSpeed;
                                        //printf("updating speed from %d to %d\n", speed, newSpeed);
                                        char valueBuffer[5];

                                    if(readSpeed < speed ){
                                        sprintf(valueBuffer, "%d", speed - newSpeed);
                                        strcpy(stroutput, "FRENO ");
                                        strcat(stroutput, valueBuffer); //currentspeed-speed that we have to reach
                                        strcat(stroutput, "\0");
                                        write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                    }
                                    else{

                                        sprintf(valueBuffer, "%d", newSpeed - speed);
                                        strcpy(stroutput, "INCREMENTO ");
                                        strcat(stroutput, valueBuffer); //currentspeed-speed that we have to reach
                                        strcat(stroutput, "\0");
                                        write(tc_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                    }
                                }
                            }
                            else{
                                printf("command %s\n", command);
                                if(strcmp(command, "PERICOLO") == 0){
                                    danger = 1;
                                    strcpy(stroutput, "FRENO -1");
                                    write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                }
                                else if(steering == 0 && (strcmp(command, "DESTRA") == 0 || strcmp(command, "SINISTRA") == 0)){
                                    steering = 1;
                                    strcpy(stroutput, command);
                                    write(sbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                }
                            }
                            logOutput("camera.log", command);
                        }
                        break;
                    case BRAKE_BY_WIRE_CODE:
                            speed -= 5;
                            if(speed == newSpeed)
                                updatingSpeed = 0;
                            if(speed == 0){
                                if((sbw_pid = fork()) == 0){
                                    socketpair(AF_UNIX, SOCK_STREAM, 0, pa_sock_pair); //TODO va bene fatto qui ?
                                    parking = 1;
                                    printf("Park assist %d\n", getpid());
                                    close(pa_sock_pair[0]);
                                    runParkAssist(pa_sock_pair[1]);
                                }
                            }
                            printf("ack ricevuto nuova vel %d\n", speed);

                            break;
                    case THROTTLE_CONTROL_CODE:
                        speed += 5;
                        if(speed == newSpeed)
                            updatingSpeed = 0;
                        printf("ack ricevuto nuova vel %d\n", speed);
                        break;
                    case HALT_CODE:
                        speed = 0;
                        started = 0;
                        printf("HALT ricevuto nuova vel %d\n", speed);
                        break;
                    case STEER_BY_WIRE_CODE:
                        steering = 0;
                        printf("Ack ricevuto fine sterzata\n");
                        break;
                    case PARK_ASSIST_CODE:
                            logOutput("assist.log", command);
                            countParking++;
                            //Qui bisogna leggere i dati dagli altri sensori e vedere se vanno bene e vedere se il
                            if(countParking == 30){
                                printf("parcheggio finito\n");
                                parking = 0;
                            }
                        break;
                    default: printf("Unknown command. %s\n",command);
                }
                logOutput("ECU.log", stroutput);
            }
        }while(strcmp(buffer,"FINE") != 0);
    }
    close(cen_ecu_sockUDPFd);
    kill(fwc_pid, SIGKILL);
    kill(tc_pid, SIGKILL);
    kill(bbw_pid, SIGKILL);
    kill(sbw_pid, SIGKILL);
    printf("FINE\n");
    exit(0);
    return 0;
}


