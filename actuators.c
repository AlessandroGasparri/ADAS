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
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include "constants.h"
#include "util.h"
#include "centralecu.h"

void runBrakeByWire(int fd){
    char str[MAXLINE];
    char ack[2];
    int newSpeed;
    int bbw_sockFd; 
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    bbw_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    signal(SIGUSR2, sig_handler);
    while(1){
        int n = 0;
        n = read(fd, str, 1);
        if(n > 0){
            readLine(fd, str+1);
            char *ptr = substring(str, 5); //strlen("FRENO ") = 5
            newSpeed = atoi(ptr);
            while(newSpeed>0 && !dangerDetected){
                n = read(fd, str, 1);
                if(n > 0){ //PARCHEGGIO
                    readLine(fd, str+1);
                    ptr = substring(str, 5);
                    newSpeed = atoi(ptr);
                }
                newSpeed -= 5;
                ack[0] = BRAKE_BY_WIRE_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack);
                logOutput("brake.log", "DECREMENTO 5", 0);
                sleep(1);
            }
            if(dangerDetected){
                printf("PERICOLINO\n");
                ack[0] = HALT_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack);
                logOutput("brake.log", "ARRESTO AUTO", 0);
                dangerDetected = 0;
            }
        }
        else if(dangerDetected){
                printf("PERICOLINO\n");
                ack[0] = HALT_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack);
                logOutput("brake.log", "ARRESTO AUTO", 0);
                dangerDetected = 0;
        }
        else{
            logOutput("brake.log", "NO ACTION", 0);
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
            //printf("throttle %s\n", str);
            //splits into two string by delimiter "-" and puts the current speed integer value in speeds[0] the one to reach in speeds[1]
            char *ptr = substring(str, 10); //strlen("INCREMENTO ") = 10
            newSpeed = atoi(ptr);
            while(newSpeed > 0){
                srand(time(NULL));
                int randomNumber = (int)(((float)rand()/RAND_MAX)*99999);
                if(randomNumber != 50000){
                    newSpeed -= 5;
                    sendToCenEcu(tc_sockFd, ack);
                    logOutput("throttle.log", "AUMENTO 5", 0);        
                    sleep(1);
                }
                else {
                    printf("FALLISCO\n");

                    printf("invio a %d\n", getppid());
                    kill(getppid(), SIGUSR1);
                    logOutput("throttle.log", "ACCELERAZIONE FALLITA", 0);  
                    break; 
                }
            }
        }
        else{
            logOutput("throttle.log", "NO ACTION", 0);        
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
            for(i = 0; i < 4 && !dangerDetected; i++ ){
                logOutput("steer.log", toWrite, 0);
                printf("%s\n", toWrite);
                sleep(1);
            }
            sendToCenEcu(sbw_sockFd, ack);
        }
        else{
            logOutput("steer.log", "NO ACTION", 0);        
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
    fptr = fopen(urandomFile, "rb");
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);//non bloccante
    while(i < PARKING_TIME){ //Do it for 30 seconds
        n = read(fd, str, 1);
        if(n < 0){
            char randomBytes[NUM_PARKING_BYTES];
            fread(randomBytes, NUM_PARKING_BYTES, 1, fptr); //Read 4 times 1 byte of data and puts in a byte array
            char toSend[6];
            toSend[0] = PARK_ASSIST_CODE; //Formatting toSend to conform to standard of a ECU message
            strcpy(toSend+1, randomBytes);
            toSend[5]= '\0';
            logOutput("assist.log", randomBytes, 1);
            sendToCenEcu(pa_sockFd, toSend);
            i++;
        }
        else{
            i = 0; //Restart parking procedure
        }
        sleep(1);
    }
    close(pa_sockFd);
    exit(0);
}