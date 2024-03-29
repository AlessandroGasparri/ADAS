#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> /* For AFINET sockets */
#include <time.h>
#include "constants.h"
#include "util.h"

void runFrontWindshieldCamera(char file[]){
    printf("Front camera is running\n");
    int fc_sockFd;
    FILE *fptr;
    fptr = fopen(file, "r");
    int speedLimit;
    char line[MAXLINE];
    char output[MAXLINE];
    output[0] = FRONT_CAMERA_CODE;
    fc_sockFd = socket(AF_INET, SOCK_DGRAM, 0);

    /**
        every second a line is read from the file and is sent to central ecu
    **/
    while (fgets(line, sizeof(line), fptr)){
        strcpy(output + 1, line);
        output[strlen(output)-1] = '\0';
        sendToCenEcu(fc_sockFd, output);
        sleep(1);
    }
    fclose(fptr);
    close(fc_sockFd);
    exit(0);
}

void runBlindSpot(char file[]){
    int bs_sockFd;
    FILE *fptr;
    bs_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    fptr = fopen(file, "rb");
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;
    int res;

    /**
        every 0.5 seconds n bytes are read from 'file', depending on NUM_BLIND_SPOT_BYTES, and are sent to central ecu,
        logging them on spot.log
    **/
    while (1){
        char randomBytes[NUM_BLIND_SPOT_BYTES+2];
        randomBytes[NUM_BLIND_SPOT_BYTES+1] = '\0';
        randomBytes[0] = BLIND_SPOT_CODE;
        ssize_t result = fread(randomBytes+1, 1, NUM_BLIND_SPOT_BYTES, fptr);
        if (result == NUM_BLIND_SPOT_BYTES){
            logOutput("spot.log", randomBytes+1, 1);
            sendToCenEcu(bs_sockFd, randomBytes);
        }
        do {
            res = nanosleep(&ts, &ts);
        } while(res && errno == EINTR);
        sleep(1);
    }
}

void runForwardFacingRadar(char file[]){
    int ffr_sockFd;
    FILE *fptr;
    ffr_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    fptr = fopen(file, "rb");
    size_t randomDataLen = 0;
    /**
        every 2 seconds n bytes are read from 'file', depending on NUM_FORWARD_FACING_RADAR_BYTES, and are sent to central ecu,
        logging them on radar.log
    **/
    while (1){
        char randomBytes[NUM_FORWARD_FACING_RADAR_BYTES+2];
        randomBytes[NUM_FORWARD_FACING_RADAR_BYTES+1] = '\0';
        randomBytes[0] = FORWARD_FACING_RADAR_CODE;
        ssize_t result = fread(randomBytes+1, 1, NUM_FORWARD_FACING_RADAR_BYTES, fptr);
        if (result == NUM_FORWARD_FACING_RADAR_BYTES){
            logOutput("radar.log", randomBytes+1, 1);
            sendToCenEcu(ffr_sockFd, randomBytes);
        }
        sleep(2);
    }
}

void runSurroundViewCameras(char file[]){
    int svc_sockFd;
    FILE *fptr;
    svc_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    fptr = fopen(file, "rb");
    /**
        every 1 seconds n bytes are read from 'file', depending on NUM_SURROUND_CAMERA_BYTES, and are sent to central ecu,
        logging them on cameras.log
    **/
    while (1){
        char randomBytes[NUM_SURROUND_CAMERA_BYTES+2];
        randomBytes[NUM_SURROUND_CAMERA_BYTES+1] = '\0';
        randomBytes[0] = SURROUND_CAMERA_CODE;
        ssize_t result = fread(randomBytes+1, 1, NUM_SURROUND_CAMERA_BYTES, fptr);
        if (result == NUM_SURROUND_CAMERA_BYTES){
            logOutput("cameras.log", randomBytes+1, 1);
            sendToCenEcu(svc_sockFd, randomBytes);
        }
        sleep(1);
    }

}

void runParkAssist(int fd, char file[]){
    char str[12];
    int pa_sockFd;
    int i = 0;
    int n = 0;
    pa_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    FILE *fptr;
    fptr = fopen(file, "rb");
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);//non bloccante
    /**
        every 1 seconds n bytes are read from 'file', depending on NUM_PARKING_BYTES, and are sent to central ecu,
        logging them on assist.log with a max of PARKING_TIME times
    **/
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