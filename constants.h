#ifndef _CONSTANTS_H
#define _CONSTANTS_H
#define FRONT_CAMERA_CODE '0'
#define BRAKE_BY_WIRE_CODE '1'
#define THROTTLE_CONTROL_CODE '2'
#define HALT_CODE '3'
#define STEER_BY_WIRE_CODE '4'
#define PARK_ASSIST_CODE '5'
#define FORWARD_FACING_RADAR_CODE '6'
#define SURROUND_CAMERA_CODE '7'
#define BLIND_SPOT_CODE '8'
#define PARKING_TIME 5 //Used for debug
#define NUM_PARKING_BYTES 4
#define NUM_FORWARD_FACING_RADAR_BYTES 24
#define NUM_SURROUND_CAMERA_BYTES 16
#define NUM_BLIND_SPOT_BYTES 8
#define NUM_PARKING_VALUES 10
#define NUM_FACING_VALUES 10
#define NUM_STEERING_VALUES 20
#define DEFAULT_PROTOCOL 0
#define HUMAN_INTERFACE_PORT 1025
#define UDP_CENECU_PORT 1026
#define MAXLINE 100
#endif