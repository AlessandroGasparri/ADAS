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

char* substring(char *src, int from){
    
    int len = 0, i = 0;
    while(*(src + from + len) != '\0'){
        len++;
    }
    
    char *dest = (char*) malloc(sizeof(char) * (len + 1));
    
    for (i = from; i< (from + len); i++ ){
        *dest = *(src + i);
        dest ++;
    }
    dest -= len;
    return dest;

}


void main(){
	printf("%s", substring("ciao", 1));
}