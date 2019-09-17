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

void runBlindSpot(char file[]){
    int bs_sockFd;
    FILE *fptr;
    bs_sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    fptr = fopen(file, "rb");
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;
    int res;
    while (1){
        char randomBytes[NUM_BLIND_SPOT_BYTES+2];
        randomBytes[NUM_BLIND_SPOT_BYTES+1] = '\0';
        randomBytes[0] = BLIND_SPOT_CODE;
        ssize_t result = fread(randomBytes+1, 1, NUM_BLIND_SPOT_BYTES, fptr);
        if (result == NUM_BLIND_SPOT_BYTES){
           // printf("sono svc invio %s\n", randomBytes);
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
    while (1){
        char randomBytes[NUM_SURROUND_CAMERA_BYTES+2];
        randomBytes[NUM_SURROUND_CAMERA_BYTES+1] = '\0';
        randomBytes[0] = SURROUND_CAMERA_CODE;
        ssize_t result = fread(randomBytes+1, 1, NUM_SURROUND_CAMERA_BYTES, fptr);
        if (result == NUM_SURROUND_CAMERA_BYTES){
           // printf("sono svc invio %s\n", randomBytes);
            logOutput("cameras.log", randomBytes+1, 1);
            sendToCenEcu(svc_sockFd, randomBytes);
        }
        sleep(1);
    }

}