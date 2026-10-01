#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <cstring>

#include <mqueue.h>

#include <pthread.h>

#define QUEUE_NAME "/reservation_mq"

#define NUM_RESOURCES 20

struct Request {
    int client_id;  
    char command[16];   
    int seat_id;       
    char server_response[128];    
};

enum SeatStatus {
    AVAILABLE = 0,   
    RESERVED = 1   
};

struct Seat {
    int id;                
    SeatStatus status; 
    int owner_client_id;   
};

#endif

