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
#include "sensors.h"

char randomFile[50];
char urandomFile[50];
int updatingSpeed = 0; // semaphore to prevent cen ecu to send data to break by wire
int dangerDetected = 0;

//Generic functions that can be in a util file



int setUpFiles(char *mode){
    if (mode == NULL) return 0;
    if(strcmp(mode, "ARTIFICIALE") == 0){
        strcpy(randomFile, "input/randomARTIFICIALE.binary");
        strcpy(urandomFile, "input/urandomARTIFICIALE.binary");
        return 1;
    }
    else if(strcmp(mode, "NORMALE") == 0){
        strcpy(randomFile, "/dev/random");
        strcpy(urandomFile, "/dev/urandom");
        return 1;
    }
    return 0;
}



void sig_handler(int sig){
    if(sig == SIGUSR1){
        updatingSpeed = 0;
        //TODO kill everuthing e send to human interface output messaggio terminazione
    }
    else if(sig == SIGUSR2){
        char s2[] = "SIGUSR2\n";
        write(STDOUT_FILENO, s2, sizeof(s2));

        dangerDetected = 1;
    }
    signal(sig, sig_handler);
}



//Actuators functionz
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





int main(int argc, char **argv){
    if(!setUpFiles(argv[1])){
        printf("Missing or wrong argument\n");
        exit(1);
    }

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
    pid_t ffr_pid; //PID for forward facing radar
    pid_t svc_pid; //PID for surround view cameras
    pid_t bs_pid; //PID for blind spot
    int speed = 50; //Car speed
    int steering = 0;
    int newSpeed = speed;
    char buffer[MAXLINE]; //Buffer to store UDP messages
    int started = 0; //Boolean variable to tell if process has received INIZIO
    int danger = 0;
    int parking = 0;
    int countParking = 0;
    char parkingFailValues[NUM_PARKING_VALUES] = {0x17, 0x2A, 0xD6, 0x93, 0xBD, 0xD8, 0xFA, 0xEE, 0x43, 0x00};
    char facingFailValues[NUM_FACING_VALUES] = {0xa0, 0x0f, 0xb0, 0x72, 0x2f, 0xa8, 0x83, 0x59, 0xce, 0x23};
    char steeringFailValues[NUM_STEERING_VALUES] = {0x41, 0x4e, 0x44, 0x52, 0x4c, 0x41, 0x42, 0x4f, 0x52, 0x41, 0x54, 0x4f, 0x83, 0x59,  0xa0, 0x1f, 0x52, 0x49, 0x51, 0x00};
    char parkBuffer[NUM_PARKING_BYTES+2]={0, 0, 0, 0, 0, 0};
    char cameraBuffer[NUM_SURROUND_CAMERA_BYTES+2]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
    char facingBuffer[NUM_FORWARD_FACING_RADAR_BYTES+2]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  
    char steerBuffer[NUM_BLIND_SPOT_BYTES + 2]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};     
    

    char stroutput[MAXLINE];

    createUDPSocket(&cen_ecu_sockUDPFd, UDP_CENECU_PORT); //Generating socket for human interface
    socketpair(AF_UNIX, SOCK_STREAM, 0, tc_sock_pair); //Handshaking of TCP sockets now in tc_sock_pair i have 2 connected sockets
    socketpair(AF_UNIX, SOCK_STREAM, 0, bbw_sock_pair);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sbw_sock_pair);

   
    printf("sono il padre %d\n", getpid());

    signal(SIGUSR1, sig_handler);

    if((hum_int_pid = fork()) == 0){ //I'm the child humaninterface
        execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL); // execute human interface in a forked process on a new terminal
        exit(0);
    } 
    else if((tc_pid = fork()) == 0){//Throttle control
        printf("  trottol da %d\n", getpid());
        close(tc_sock_pair[0]); //I'm the child and i close father socket
        runThrottleControl(tc_sock_pair[1]); //funzione per controllare la velocita TODO
    }
    else if((fwc_pid = fork()) == 0 ) {//front windshield
        printf("  camera da %d\n", getpid());
        runFrontWindshieldCamera("input/frontCamera.data");
    }
    else if((bbw_pid = fork()) == 0){//break by wire
        printf("Break by wire %d\n", getpid());
        close(bbw_sock_pair[0]); 
        runBrakeByWire(bbw_sock_pair[1]);
    }
    else if((sbw_pid = fork()) == 0){//steer by wire
        printf("Steer by wire %d\n", getpid());
        close(sbw_sock_pair[0]);
        runSteerByWire(sbw_sock_pair[1]);
    }
    else if((ffr_pid = fork()) == 0){//Forward facing radar
        printf("Forward facing radar %d\n", getpid());
        runForwardFacingRadar(randomFile);
    }
    else {//In this else i'm the father
        close(tc_sock_pair[1]); //I'm the father and i close child socket
        close(bbw_sock_pair[1]);
        close(sbw_sock_pair[1]);
        do{
            int n, len;
            n = recvfrom(cen_ecu_sockUDPFd,(char *)buffer, MAXLINE, MSG_WAITALL, clientAddr, &len);
            if(!started){
                if(strcmp(buffer,"INIZIO") == 0){
                    started = 1;
                    if(!danger){
                        
                    }
                    danger = 0;
                }
            }
            else if(strcmp(buffer, "PARCHEGGIO") == 0 && started){
                parking = 1;
                printf("PROCEDURA DI PARCHEGGIO INIZIATA\n");
                logOutput("ECU.log", "PROCEDURA DI PARCHEGGIO INIZIATA",0);
                char tmp[10];
                strcpy(stroutput, "FRENO ");
                sprintf(tmp, "%d", speed);
                strcat(stroutput, tmp);
                write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
            }
            else if(strcmp(buffer,"FINE") != 0){
                char code;
                code = buffer[0];
                char *command;
                command = substring(buffer, 1);
                switch(code){
                    case FRONT_CAMERA_CODE:
                        if(command[0] >= '0' && command[0] <='9' ){ //There is a number to process
                            int readSpeed = atoi(command);
                            /*
                                Command handled if speed is not being updated yet
                            */
                            if(!updatingSpeed && !parking && readSpeed != speed){
                                    updatingSpeed = 1;
                                    newSpeed = readSpeed;
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
                                logOutput("ECU.log", stroutput, 0);
                            }
                        }
                        else{
                            if(strcmp(command, "PERICOLO") == 0){
                                danger = 1;
                                strcpy(stroutput, "PERICOLO - ARRESTO AUTO");
                                parking = 0;
                                updatingSpeed = 0;
                                steering = 0;
                                logOutput("ECU.log", stroutput, 0);
                                kill(bbw_pid, SIGUSR2);
                            }
                            else if(!steering && !parking && (strcmp(command, "DESTRA") == 0 || strcmp(command, "SINISTRA") == 0)){
                                steering = 1;
                                strcpy(stroutput, command);
                                write(sbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                logOutput("ECU.log", stroutput, 0);
                                printf("giro a %s", command);
                                if((bs_pid = fork()) == 0){
                                    printf("esegui blid %d", getpid());
                                    runBlindSpot(urandomFile);
                                }
                            }
                        }
                        logOutput("camera.log", command, 0);
                        break;
                    case BRAKE_BY_WIRE_CODE:
                            speed -= 5;
                            if(speed == newSpeed)
                                updatingSpeed = 0;
                            if(speed == 0 && parking){
                                strcpy(stroutput, "PARCHEGGIO");
                                logOutput("ECU.log", stroutput,0);
                                socketpair(AF_UNIX, SOCK_STREAM, 0, pa_sock_pair);
                                if((pa_pid = fork()) == 0){
                                    printf("Park assist %d\n", getpid());
                                    close(pa_sock_pair[0]);
                                    runParkAssist(pa_sock_pair[1], urandomFile);
                                }else if((svc_pid = fork()) == 0){
                                    runSurroundViewCameras(urandomFile);
                                }
                                else{
                                    close(pa_sock_pair[1]);
                                }
                            }
                            char logstr[50];
                            sprintf(logstr,"NUOVA VELOCITA %d", speed);
                            printf("%s\n", logstr);
                            logOutput("ECU.log", logstr,0);
                            break;
                    case THROTTLE_CONTROL_CODE:
                        if(!parking){
                                speed += 5;
                            if(speed == newSpeed)
                                updatingSpeed = 0;
                               // char logstr[50];
                                sprintf(logstr,"NUOVA VELOCITA %d", speed);
                                printf("%s\n", logstr);
                                logOutput("ECU.log", logstr,0);
                        }
                        break;
                    case HALT_CODE:
                        speed = 0;
                        started = 0;
                        updatingSpeed = 0;
                        parking = 0;
                        countParking = 0;
                        //printf("HALT ricevuto nuova vel %d\n", speed);
                        break;
                    case STEER_BY_WIRE_CODE:
                        steering = 0;
                        printf("Ack ricevuto steer fine sterzata, uccido %d\n", bs_pid);
                        logOutput("ECU.log", "FINE STERZATA",0);
                        kill(bs_pid, SIGKILL);
                        break;
                    case BLIND_SPOT_CODE:

                        strcpy(steerBuffer + 2, command);

                         if(searchForBytes(steerBuffer, NUM_BLIND_SPOT_BYTES+2, steeringFailValues, NUM_STEERING_VALUES)){
                            logOutput("ECU.log", "SEQUENZA TROVATA DA BLIND SPOT- STERZATA FALLITA", 0);
                            countParking = 0;
                            steering = 0;
                            kill(bbw_pid,SIGUSR2);
                            kill(sbw_pid, SIGUSR2);
                            kill(bs_pid, SIGKILL);
                        }
                        steerBuffer[0] = command[NUM_BLIND_SPOT_BYTES-2];
                        steerBuffer[1] = command[NUM_BLIND_SPOT_BYTES-1];
                    break;
                    case PARK_ASSIST_CODE:
                        countParking++;
                        for(int i = 0; i < NUM_PARKING_BYTES; i++){
                            parkBuffer[i+2] = command[i];
                        }
                        if(searchForBytes(parkBuffer, NUM_PARKING_BYTES+2, parkingFailValues, NUM_PARKING_VALUES)){
                            logOutput("ECU.log", "SEQUENZA TROVATA DA PARK ASSIST- RICOMINCIO PARCHEGGIO", 0);
                            countParking = 0;
                            write(pa_sock_pair[0], "P", 1);
                        }
                        parkBuffer[0] = command[NUM_PARKING_BYTES-2];
                        parkBuffer[1] = command[NUM_PARKING_BYTES-1];
                        if(countParking == PARKING_TIME){
                            logOutput("ECU.log", "PARCHEGGIO COMPLETATO", 0);
                            kill(svc_pid, SIGKILL);
                            started = 0;
                            parking = 0;
                            countParking = 0;
                            updatingSpeed = 0;
                        }
                        break;
                    case FORWARD_FACING_RADAR_CODE:

                        strcpy(facingBuffer + 2, command);

                        if(searchForBytes(facingBuffer, NUM_FACING_VALUES+2, facingFailValues, NUM_FACING_VALUES)){
                            danger = 1;
                            printf("SEQUENZA TROVATA DA FACING");
                            logOutput("ECU.log", "SEQUENZA TROVATA DA FORWARD FACING RADAR - ARRESTO AUTO",0);
                            kill(bbw_pid, SIGUSR2);
                        }
                        facingBuffer[0] = command[NUM_FORWARD_FACING_RADAR_BYTES-2];
                        facingBuffer[1] = command[NUM_FORWARD_FACING_RADAR_BYTES-1];
                        break;
                     case SURROUND_CAMERA_CODE:
                        for(int i = 0; i < NUM_PARKING_BYTES; i++){
                                cameraBuffer[i+2] = command[i];
                            }
                            if(searchForBytes(cameraBuffer, NUM_SURROUND_CAMERA_BYTES+2, parkingFailValues, NUM_PARKING_VALUES)){
                                logOutput("ECU.log", "SEQUENZA TROVATA - RICOMINCIO PARCHEGGIO", 0);
                                countParking = 0;
                                write(pa_sock_pair[0], "P", 1);
                            }
                            cameraBuffer[0] = command[NUM_SURROUND_CAMERA_BYTES-2];
                            cameraBuffer[1] = command[NUM_SURROUND_CAMERA_BYTES-1];
                        break;
                    default: printf("Unknown command. %s\n",command);
                }
            }
        }while(strcmp(buffer,"FINE") != 0);
    }
    close(cen_ecu_sockUDPFd);
    //killProcesses(4, tc_sock_pair[0], bbw_sock_pair[0],sbw_sock_pair[0],pa_sock_pair[0]);
    printf("bs %d\n", bs_pid);
    printf("fwc_pid %d\n", fwc_pid);
    printf("tc_pid %d\n", tc_pid);
    printf("hum_int_pid %d\n", hum_int_pid);
    printf("sbw_pid %d\n", sbw_pid);
    printf("bbw_pid %d\n", bbw_pid);
    printf("pa_pid %d\n", pa_pid);
    printf("ffr_pid %d\n", ffr_pid);
    printf("bs %d\n", bs_pid);
    kill(pa_pid, SIGKILL);
    kill(fwc_pid, SIGKILL);
    kill(tc_pid, SIGKILL);
    kill(bbw_pid, SIGKILL);
    kill(sbw_pid, SIGKILL);
    kill(ffr_pid, SIGKILL);
    kill(bs_pid, SIGKILL);
    
    /*
    pid_t pa_pid; //PID for park assist
    pid_t fwc_pid; //PID front wide camera process
    pid_t tc_pid; //PID throttle control process
    pid_t bbw_pid; //PID for break by wire
    pid_t sbw_pid; //PID for steer by wire
    pid_t svc_pid; //PID for surround view cameras
    pid_t ffr_pid; //PID for forward facing radar
    pid_t bs_pid; //PID for blind spot
    pid_t hum_int_pid; //PID for human interface
    */
    printf("FINE\n");
    exit(0);
    return 0;

}


