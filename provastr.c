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



int searchForBytes(char *toSearchIn, int lenght, char *values, int nValues){
    //0x 17 2A , ii) 0xD693, iiI) 0xBDD8, iv) 0xFAEE, v) 0x4300
    for(int i = 0; i < lenght-1; i++){
        for(int j = 0; j < nValues-1; j+=2){
            if(toSearchIn[i] == values[j] && toSearchIn[i+1] == values[j+1]){
                return 1;
            }
        }
    }
    return 0;
}

void main(){
    char values[10] = {0x17, 0x2A, 0xD6, 0x93, 0xBD, 0xD8, 0xFA, 0xEE, 0x43, 0x00};
    char toSearch[4] = {0x11, 0x23, 0x2A, 0xD6};
    printf("%d\n", searchForBytes(toSearch, 4, values, 10));
    exit(0);
}