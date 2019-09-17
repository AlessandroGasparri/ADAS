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

char randomFile[50]; //Path for random file
char urandomFile[50]; //Path for urandom file
int updatingSpeed = 0; //Semaphore to prevent cen ecu to send data to break by wire
int throttleFail = 0; //Flag to determine if throttle has failed
int dangerDetected = 0; //Flag to determine if danger is detected


/* Misc functions */

int setUpFiles(char *mode){ //Set up the correct random and urandom file
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



void sig_handler(int sig){ //Signal handler
    if(sig == SIGUSR1){
        updatingSpeed = 0;
        throttleFail = 1;
    }
    else if(sig == SIGUSR2){
        char s2[] = "SIGUSR2\n";
        write(STDOUT_FILENO, s2, sizeof(s2));

        dangerDetected = 1;
    }
    signal(sig, sig_handler);
}


/* Actuator functions */

void runBrakeByWire(int fd){ //break by wire
    char str[MAXLINE]; //Buffer to store message received
    char ack[2]; //Buffer to store the message to send
    int newSpeed; //New speed to reach
    int bbw_sockFd; //Socket to send data to central ECU
    struct timeval tv; //Struct to store timer values
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv); //Not blocking reads on socket fd
    bbw_sockFd = socket(AF_INET, SOCK_DGRAM, 0); //Initialize the socket used to send data to centralECU
    signal(SIGUSR2, sig_handler); //Set signal handler for SIGUSR2
    while(1){
        int n = 0;
        n = read(fd, str, 1); //Read first byte of messaage
        if(n > 0){ //If we read something it means we have to do something
            readLine(fd, str+1); //Read the rest of message
            char *ptr = substring(str, 5); //strlen("FRENO ") = 5
            newSpeed = atoi(ptr); //Integer speed from string
            while(newSpeed>0 && !dangerDetected){ //While speed is not 0 and there is not danger
                n = read(fd, str, 1); //Not blockng read use to know if we are parking or not
                if(n > 0){ //We are parking because we received something
                    readLine(fd, str+1); //Read the rest of message
                    ptr = substring(str, 5);
                    newSpeed = atoi(ptr); //Set newSpeed to 5 so break by wire exits immediatly after this cycle
                }
                newSpeed -= 5; //Decrement the speed
                ack[0] = BRAKE_BY_WIRE_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack); //Send ack to central ECU
                logOutput("brake.log", "DECREMENTO 5", 0); //Log output
                sleep(1);
            }
            if(dangerDetected){ //Danger detected
                printf("PERICOLINO\n");
                ack[0] = HALT_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack); //Send halt code ack
                logOutput("brake.log", "ARRESTO AUTO", 0);
                dangerDetected = 0;
            }
        }
        else if(dangerDetected){ //Danger detected
                printf("PERICOLINO\n");
                ack[0] = HALT_CODE;
                ack[1] = '\0';
                sendToCenEcu(bbw_sockFd, ack); //send halt code ack
                logOutput("brake.log", "ARRESTO AUTO", 0);
                dangerDetected = 0;
        }
        else{
            logOutput("brake.log", "NO ACTION", 0); //NO action
        }
        sleep(1);
    }
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
            char *ptr = substring(str, 10); //strlen("INCREMENTO ") = 10
            newSpeed = atoi(ptr);
            while(newSpeed > 0){
                srand(time(NULL));
                int randomNumber = (int)(((float)rand()/RAND_MAX)*99999); //Random number generation to fail throttle
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
    if(!setUpFiles(argv[1])){ //Set up the files if there is an argument that has been passed
        printf("Missing or wrong argument\n"); //No argument program terminates unsuccessfully
        exit(1);
    }

    int cen_ecu_sockUDPFd; //UDP socket descriptor Central ECU
    struct sockaddr* clientAddr; //client address
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
    int speed = 0; //Car speed
    int steering = 0; //Flag to know if we are steering or not
    int newSpeed = speed; //New speed that the car has to reach initialized to speed
    char buffer[MAXLINE]; //Buffer to store UDP messages
    int started = 0; //Boolean variable to tell if process has received INIZIO
    int danger = 0; //Flag to know if danger is detected in the system
    int parking = 0; //Flag used to determine if the car is parking or not
    int countParking = 0; //Counter used to determine if parking has failed and has to restart
    char parkingFailValues[NUM_PARKING_VALUES] = {0x17, 0x2A, 0xD6, 0x93, 0xBD, 0xD8, 0xFA, 0xEE, 0x43, 0x00}; //Array of fail values for parking
    char facingFailValues[NUM_FACING_VALUES] = {0xa0, 0x0f, 0xb0, 0x72, 0x2f, 0xa8, 0x83, 0x59, 0xce, 0x23}; //Array of fail values for forward facing radar
    char steeringFailValues[NUM_STEERING_VALUES] = {0x41, 0x4e, 0x44, 0x52, 0x4c, 0x41, 0x42, 0x4f, 0x52, 0x41, 0x54, 0x4f, 0x83, 0x59,  0xa0, 0x1f, 0x52, 0x49, 0x51, 0x00}; //Array of fail values for blind spot
    char parkBuffer[NUM_PARKING_BYTES+2]={0, 0, 0, 0, 0, 0}; //Buffer to check fail values for parking
    char cameraBuffer[NUM_SURROUND_CAMERA_BYTES+2]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Buffer to check fail values for camera
    char facingBuffer[NUM_FORWARD_FACING_RADAR_BYTES+2]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Buffer to check fail values for forward facing radar
    char steerBuffer[NUM_BLIND_SPOT_BYTES + 2]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Buffer to check fail values for blind spot
    

    char stroutput[MAXLINE]; //Buffer used to store messages to send to childs

    createUDPSocket(&cen_ecu_sockUDPFd, UDP_CENECU_PORT); //Generating socket for human interface
    socketpair(AF_UNIX, SOCK_STREAM, 0, tc_sock_pair); //Handshaking of TCP sockets now in tc_sock_pair i have 2 connected sockets
    socketpair(AF_UNIX, SOCK_STREAM, 0, bbw_sock_pair);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sbw_sock_pair);

   
    printf("sono il padre %d\n", getpid());

    signal(SIGUSR1, sig_handler); //Set the signal handler for signal SIGUSR1

    if((hum_int_pid = fork()) == 0){ //I'm the child humaninterface
        execl("/usr/bin/xterm", "xterm", "./humaninterface", NULL); // execute human interface in a forked process on a new terminal
        exit(0);
    } 
    else if((tc_pid = fork()) == 0){//Throttle control
        printf("  trottol da %d\n", getpid());
        close(tc_sock_pair[0]); //I'm the child and i close father socket
        runThrottleControl(tc_sock_pair[1]); 
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
        close(bbw_sock_pair[1]); //I'm the father and i close child socket
        close(sbw_sock_pair[1]); //I'm the father and i close child socket
        do{
            int n, len;
            n = recvfrom(cen_ecu_sockUDPFd,(char *)buffer, MAXLINE, MSG_WAITALL, clientAddr, &len); //Receive message from child processes into buffer
            if(throttleFail){//Check every cycle if throttle has failed with signal flag and kill everything if it has
                //Kill all processes and output termination
                kill(pa_pid, SIGKILL);
                kill(fwc_pid, SIGKILL);
                kill(tc_pid, SIGKILL);
                kill(bbw_pid, SIGKILL);
                kill(sbw_pid, SIGKILL);
                kill(ffr_pid, SIGKILL);
                kill(bs_pid, SIGKILL);
                kill(svc_pid, SIGKILL);
                logOutput("ECU.log", "ACCELERAZIONE FALLITA TERMINO SISTEMA.");
                exit(1);
            }
            if(!started){
                if(strcmp(buffer,"INIZIO") == 0){ //System has received INIZIO from humaninterface
                    logOutput("ECU.log", "AVVIO SISTEMA",0);
                    started = 1;
                }
            }
            else if(strcmp(buffer, "PARCHEGGIO") == 0 && started){ //System has received PARCHEGGIO from humaninterface
                parking = 1;
                printf("PROCEDURA DI PARCHEGGIO INIZIATA\n");
                logOutput("ECU.log", "PROCEDURA DI PARCHEGGIO INIZIATA",0);
                char tmp[10];
                strcpy(stroutput, "FRENO ");
                sprintf(tmp, "%d", speed);
                strcat(stroutput, tmp);
                write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1); //Send to bbw the command to stop the car
            }
            else if(strcmp(buffer,"FINE") != 0){ //System received a generic message that has to be handled
                char code; //Code of the task that sent the message
                code = buffer[0];
                char *command; //Message received
                command = substring(buffer, 1);
                switch(code){//Switch on the code of the task to determine what central ECU has to do
                    case FRONT_CAMERA_CODE: //Message received from Front windshield camera
                        if(command[0] >= '0' && command[0] <='9' ){ //There is a number to process
                            int readSpeed = atoi(command); //Conversion of the new speed from string to integer
                            /*
                                Command handled if speed is not being updated yet
                            */
                            if(!updatingSpeed && !parking && readSpeed != speed){
                                    updatingSpeed = 1;
                                    newSpeed = readSpeed;
                                    char valueBuffer[5]; //Buffer used to store the speed in string form

                                if(readSpeed < speed ){ //Action has to be handled from bbw
                                    sprintf(valueBuffer, "%d", speed - newSpeed);
                                    strcpy(stroutput, "FRENO ");
                                    strcat(stroutput, valueBuffer); //currentspeed-speed that we have to reach
                                    strcat(stroutput, "\0");
                                    write(bbw_sock_pair[0], stroutput, strlen(stroutput) + 1); //Write command to bbw
                                }
                                else{ //Action has to be handled from tc
                                    sprintf(valueBuffer, "%d", newSpeed - speed);
                                    strcpy(stroutput, "INCREMENTO ");
                                    strcat(stroutput, valueBuffer); //currentspeed-speed that we have to reach
                                    strcat(stroutput, "\0");
                                    write(tc_sock_pair[0], stroutput, strlen(stroutput) + 1); //Write command to tc
                                }
                                logOutput("ECU.log", stroutput, 0); //Log the command wrote
                            }
                        }
                        else{
                            if(strcmp(command, "PERICOLO") == 0){ //System received PERICOLO
                                danger = 1; //Set flags, we do this to terminate the other operations
                                parking = 0;
                                updatingSpeed = 0;
                                steering = 0;
                                strcpy(stroutput, "PERICOLO - ARRESTO AUTO");
                                logOutput("ECU.log", stroutput, 0);
                                kill(bbw_pid, SIGUSR2); //Send danger signal to break by wire that has to stop the car
                            }
                            else if(!steering && !parking && (strcmp(command, "DESTRA") == 0 || strcmp(command, "SINISTRA") == 0)){ //System received DESTRA or SINISTRA
                                steering = 1; //Set flag 
                                strcpy(stroutput, command); //Instructions to log the command received
                                write(sbw_sock_pair[0], stroutput, strlen(stroutput) + 1);
                                logOutput("ECU.log", stroutput, 0);
                                printf("giro a %s", command);
                                if((bs_pid = fork()) == 0){ //blind spot generated only when needed
                                    printf("esegui blid %d", getpid());
                                    runBlindSpot(urandomFile);
                                }
                            }
                        }
                        logOutput("camera.log", command, 0); //Log the message into camera.log
                        break;
                    case BRAKE_BY_WIRE_CODE: //Message received from brake by wire
                            speed -= 5; //Decrement the speed
                            if(speed == newSpeed) //Check if we have reached the speed that we had to reach
                                updatingSpeed = 0; //Set flag 
                            if(speed == 0 && parking){ //If we are parking and we reached speed 0
                                strcpy(stroutput, "PARCHEGGIO");
                                logOutput("ECU.log", stroutput,0); //Log that we are parking
                                socketpair(AF_UNIX, SOCK_STREAM, 0, pa_sock_pair); //Pair socket for park assist
                                if((pa_pid = fork()) == 0){ //Generate park assist when needed
                                    printf("Park assist %d\n", getpid());
                                    close(pa_sock_pair[0]); //Close the child socket
                                    runParkAssist(pa_sock_pair[1], urandomFile);
                                }else if((svc_pid = fork()) == 0){ //Generate surround view cameras when needed
                                    runSurroundViewCameras(urandomFile);
                                }
                                else{
                                    close(pa_sock_pair[1]); //Close the father socket
                                }
                            }
                            char logstr[50]; //Log of new speed
                            sprintf(logstr,"NUOVA VELOCITA %d", speed);
                            printf("%s\n", logstr);
                            logOutput("ECU.log", logstr,0);
                            break;
                    case THROTTLE_CONTROL_CODE: //Message received from throttle control
                        if(!parking){ //Check if we are parking
                                speed += 5; //Augment the speed
                            if(speed == newSpeed) //Check if we reached the speed that we had to
                                updatingSpeed = 0; //Set flag
                            sprintf(logstr,"NUOVA VELOCITA %d", speed); //Log of new speed
                            printf("%s\n", logstr);
                            logOutput("ECU.log", logstr,0);
                        }
                        break;
                    case HALT_CODE: //Code sent after danger detected to completely stop the car
                        speed = 0;
                        started = 0; //Set flags to match the behaviour the central ECU has to have after a danger signal was received
                        updatingSpeed = 0;
                        parking = 0;
                        countParking = 0;
                        //printf("HALT ricevuto nuova vel %d\n", speed);
                        break;
                    case STEER_BY_WIRE_CODE: //Message received from steer by wire it means that the steering is finished
                        steering = 0; //Steering finished so steering is now 0
                        printf("Ack ricevuto steer fine sterzata, uccido %d\n", bs_pid);
                        logOutput("ECU.log", "FINE STERZATA",0); //Logs the correct finished steering into output
                        kill(bs_pid, SIGKILL); //Blind spot is no more necessary so we kill it
                        break;
                    case BLIND_SPOT_CODE: //Message received from blind spot
                        strcpy(steerBuffer + 2, command); //We keep the last 2 bytes of the prevoius reading to check for sequences
                         if(searchForBytes(steerBuffer, NUM_BLIND_SPOT_BYTES+2, steeringFailValues, NUM_STEERING_VALUES)){ //Search for a sequence
                            logOutput("ECU.log", "SEQUENZA TROVATA DA BLIND SPOT- STERZATA FALLITA", 0);
                            countParking = 0; //Set flags
                            steering = 0;
                            kill(bbw_pid,SIGUSR2); //Send danger signal to bbw and sbw
                            kill(sbw_pid, SIGUSR2);
                            kill(bs_pid, SIGKILL); //Kill processes that are not necessary anymore
                        }
                        steerBuffer[0] = command[NUM_BLIND_SPOT_BYTES-2]; //puts the last 2 bytes into the first and second position of the buffer to check for sequences the next
                        steerBuffer[1] = command[NUM_BLIND_SPOT_BYTES-1]; //time we receive data
                    break;
                    case PARK_ASSIST_CODE: //Message received from park assist
                        countParking++; //Increment counter used to know if parking is finished
                        for(int i = 0; i < NUM_PARKING_BYTES; i++){ //Keep last 2 bytes to check sequence
                            parkBuffer[i+2] = command[i];
                        }
                        if(searchForBytes(parkBuffer, NUM_PARKING_BYTES+2, parkingFailValues, NUM_PARKING_VALUES)){ //Search for a failing sequence
                            logOutput("ECU.log", "SEQUENZA TROVATA DA PARK ASSIST- RICOMINCIO PARCHEGGIO", 0); //Sequence found -> output
                            countParking = 0; //Set counter to 0 to restart
                            write(pa_sock_pair[0], "P", 1); //Send message to restart parking to park assist
                        }
                        parkBuffer[0] = command[NUM_PARKING_BYTES-2]; //Keep last 2 bytes
                        parkBuffer[1] = command[NUM_PARKING_BYTES-1];
                        if(countParking == PARKING_TIME){ //Parking completed
                            logOutput("ECU.log", "PARCHEGGIO COMPLETATO", 0);
                            kill(svc_pid, SIGKILL); //Surround view camera is not necessary anymore so we kill it
                            started = 0; //Sets the flag to atch the behaviour central ECU has to have after a correct parking
                            parking = 0;
                            countParking = 0;
                            updatingSpeed = 0;
                        }
                        break;
                    case FORWARD_FACING_RADAR_CODE: //Message received from forward facing radar
                        strcpy(facingBuffer + 2, command); //Keep 2 bytes
                        if(searchForBytes(facingBuffer, NUM_FACING_VALUES+2, facingFailValues, NUM_FACING_VALUES)){ //Search for sequence
                            danger = 1; //Danger detected
                            printf("SEQUENZA TROVATA DA FACING");
                            logOutput("ECU.log", "SEQUENZA TROVATA DA FORWARD FACING RADAR - ARRESTO AUTO",0); //Sequence found
                            kill(bbw_pid, SIGUSR2); //Send danger signal to break by wire
                        }
                        facingBuffer[0] = command[NUM_FORWARD_FACING_RADAR_BYTES-2]; //Keep 2 bytes
                        facingBuffer[1] = command[NUM_FORWARD_FACING_RADAR_BYTES-1];
                        break;
                     case SURROUND_CAMERA_CODE: //Message received from Surround camera
                        for(int i = 0; i < NUM_PARKING_BYTES; i++){ //keep 2 bytes
                                cameraBuffer[i+2] = command[i];
                            }
                            if(searchForBytes(cameraBuffer, NUM_SURROUND_CAMERA_BYTES+2, parkingFailValues, NUM_PARKING_VALUES)){ //Check for sequence
                                logOutput("ECU.log", "SEQUENZA TROVATA - RICOMINCIO PARCHEGGIO", 0);
                                countParking = 0; //Parking failed
                                write(pa_sock_pair[0], "P", 1); //Tells park assist to restart parking
                            }
                            cameraBuffer[0] = command[NUM_SURROUND_CAMERA_BYTES-2]; //Keep 2 bytes
                            cameraBuffer[1] = command[NUM_SURROUND_CAMERA_BYTES-1];
                        break;
                    default: printf("Unknown command. %s\n",command); //Unknown command
                }
            }
        }while(strcmp(buffer,"FINE") != 0); //System received FINE
    }
    close(cen_ecu_sockUDPFd); //Close UDP socket
    //console prints 
    printf("bs %d\n", bs_pid);
    printf("fwc_pid %d\n", fwc_pid);
    printf("tc_pid %d\n", tc_pid);
    printf("hum_int_pid %d\n", hum_int_pid);
    printf("sbw_pid %d\n", sbw_pid);
    printf("bbw_pid %d\n", bbw_pid);
    printf("pa_pid %d\n", pa_pid);
    printf("ffr_pid %d\n", ffr_pid);
    printf("bs %d\n", bs_pid);
    printf("FINE\n");
    //Kill every process that is running
    kill(pa_pid, SIGKILL);
    kill(fwc_pid, SIGKILL);
    kill(tc_pid, SIGKILL);
    kill(bbw_pid, SIGKILL);
    kill(sbw_pid, SIGKILL);
    kill(ffr_pid, SIGKILL);
    kill(bs_pid, SIGKILL);
    kill(svc_pid, SIGKILL);
    return 0; //Program terminated correctly
}


