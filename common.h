#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <cstring>
#include <mqueue.h>
#include <pthread.h>

#define QUEUE_NAME "/reservation_mq"
#define NUM_RESOURCES 50 // ปรับเป็น 50 ที่นั่ง
#define RESPONSE_SIZE 2048 // ขยายขนาด buffer สำหรับรับข้อความตอบกลับ

struct Request {
    int client_id;  
    char command[16];   
    int seat_id;       
    char server_response[RESPONSE_SIZE];    
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