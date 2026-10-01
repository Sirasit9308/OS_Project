#include <iostream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <cstring>
#include <cstdlib>
#include "protocol.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./client <client_id> <command> [resource_id]\n";
        std::cout << "Commands Guide:\n";
        std::cout << "  1 (LIST)    : ./client <client_id> 1\n";
        std::cout << "  2 (STATUS)  : ./client <client_id> 2 <resource_id>\n";
        std::cout << "  3 (RESERVE) : ./client <client_id> 3 <resource_id>\n";
        std::cout << "  4 (CANCEL)  : ./client <client_id> 4 <resource_id>\n";
        std::cout << "  5 (QUIT)    : ./client <client_id> 5\n";
        return 1;
    }

    int client_id = std::stoi(argv[1]);
    int command = std::stoi(argv[2]);
    int resource_id = (argc > 3) ? std::stoi(argv[3]) : 0;

    // 1. สร้าง Key ตัวเดียวกับ Server
    key_t key = ftok(MQ_KEY_PATH, MQ_PROJECT_ID);
    if (key == -1) {
        perror("ftok error");
        exit(1);
    }

    // 2. เข้าถึง Message Queue
    int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget error");
        exit(1);
    }

    // 3. เตรียมแพ็กเกจข้อความ (Message Structure)
    Message msg;
    msg.mtype = 1;               // กำหนด Message Type เป็น 1
    msg.client_id = client_id;
    msg.command = command;
    msg.resource_id = resource_id;
    std::strcpy(msg.data, "Request from client");

    // 4. ส่งข้อความเข้า Queue ด้วย msgsnd
    if (msgsnd(msqid, &msg, sizeof(Message) - sizeof(long), 0) == -1) {
        perror("msgsnd error");
    } else {
        std::cout << "[Client-" << client_id << "] Successfully sent command type " << command 
                  << " for Resource ID: " << resource_id << "\n";
    }

    return 0;
}