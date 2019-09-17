# Makefile for Central Ecu and Human Interface
# gcc version 8.2.0 (Debian 8.2.0-14)
# Operating System And Kernel: Linux 4.19.0-kali3-amd64 x86_64


all: clean centralecu humaninterface

centralecu: centralecu.o sensors.o util.o
	gcc -o centralecu centralecu.o sensors.o util.o
#
humaninterface: humaninterface.o util.o
	gcc -o humaninterface humaninterface.o util.o
#
centralecu.o: centralecu.c
	gcc -c centralecu.c
#
sensors.o: sensors.c
	gcc -c sensors.c
#
util.o: util.c
	gcc -c util.c

clean:
	rm -f *.o *.log centralecu humaninterface
