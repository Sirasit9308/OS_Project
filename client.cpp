#include "common.h"
#include <fcntl.h>

int main(int argc, char* argv[]){

    if(argc < 2){
        std::cout << "ใส่ข้อมูลไม่ครบ";
        return 1;
    }
    else if(argc > 2){
        std::cout << "ใส่ข้อมูลเกิน";
        return 1;
    }
    else if(argc == 2){
        int client_id = std::stoi(argv[1]);
        mqd_t message_queue = mq_open(QUEUE_NAME, O_WRONLY);
        if (message_queue == (mqd_t)-1) {
            perror("mq_open error.");
            return 1;
        }

        std::string client_command;
        while(std::cout << "Client-" << client_id << ": " && std::cin >> client_command){
            if(client_command == "QUIT"){
                std::cout << "Exiting...";
                break;
            }
            if(client_command != "LIST" && client_command != "STATUS" && 
                client_command != "RESERVE" && client_command != "CANCEL"){
                std::cout << "We don't have this function.\n";
                continue;
            }

            struct Request request;
            request.client_id = client_id;
            strcpy(request.command, client_command.c_str());
            request.seat_id = 0;

            if(client_command == "STATUS" || client_command == "RESERVE" || client_command == "CANCEL"){
                std::cin >> request.seat_id;
            }
            if(mq_send(message_queue, (const char*)&request, sizeof(request), 0) == -1){
                perror("mq_send error.");
            }
            else{
                std::cout << "Request send:" << request.command << "\n"; 
            }
        }
        mq_close(message_queue);
    }
    return 0;
}