#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <cstring>
#include "common.h"

Seat reservation_table[NUM_RESOURCES];
std::mutex mtx;

void init_resources() {
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        reservation_table[i].id = i + 1;
        reservation_table[i].status = AVAILABLE;
        reservation_table[i].owner_client_id = -1;
    }
}

void random_delay() {
    int delay_ms = (rand() % 451) + 50;
    usleep(delay_ms * 1000);
}

void worker_task(int worker_id, mqd_t mqdes) {
    Request req;
    while (true) {
        ssize_t bytes_read = mq_receive(mqdes, (char*)&req, sizeof(req), nullptr);
        if (bytes_read == -1) {
            perror("mq_receive error.");
            break;
        }

        std::string cmd = req.command;
        std::cout << "[Worker-" << worker_id << "] received command " << cmd 
                  << " for Seat " << req.seat_id << " from Client-" << req.client_id << "\n";

        {
        // ----------------------------------------------------------------------------------
        // ----------------------------------------------------------------------------------
            // std::lock_guard<std::mutex> lock(mtx);
        // ----------------------------------------------------------------------------------
        // ----------------------------------------------------------------------------------
            std::cout << "[Worker-" << worker_id << "] entering critical section\n";

            if (cmd == "LIST") {
                std::cout << "[Worker-" << worker_id << "] --- Resource List (1-" << NUM_RESOURCES << ") ---\n";
                std::string list_res = "=== Resource List (1-" + std::to_string(NUM_RESOURCES) + ") ===\n";
                for (int i = 0; i < NUM_RESOURCES; ++i) {
                    std::string status_str = (reservation_table[i].status == AVAILABLE) ? "AVAILABLE" : "RESERVED";
                    std::string owner_str = (reservation_table[i].owner_client_id == -1) 
                                            ? "-" 
                                            : "Client-" + std::to_string(reservation_table[i].owner_client_id);
                    
                    std::cout << "Resource " << reservation_table[i].id 
                              << " | " << status_str 
                              << " | Owner: " << owner_str << "\n";

                    list_res += "Seat " + std::to_string(reservation_table[i].id) 
                             + "\t| " + status_str + "\t| Owner: " + owner_str + "\n";
                }
                strncpy(req.server_response, list_res.c_str(), sizeof(req.server_response) - 1);
            } 
            else if (cmd == "STATUS") {
                int idx = req.seat_id - 1;
                if (idx >= 0 && idx < NUM_RESOURCES) {
                    std::string status_str = (reservation_table[idx].status == AVAILABLE) ? "AVAILABLE" : "RESERVED";
                    std::string owner_str = (reservation_table[idx].owner_client_id == -1) 
                                            ? "-" 
                                            : "Client-" + std::to_string(reservation_table[idx].owner_client_id);
                    std::cout << "[Worker-" << worker_id << "] Status Resource " << req.seat_id 
                              << ": " << status_str << " (Owner: " << owner_str << ")\n";

                    std::string res = "Seat " + std::to_string(req.seat_id) + " Status: " + status_str + " (Owner: " + owner_str + ")\n";
                    strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                } else {
                    std::cout << "[Worker-" << worker_id << "] Invalid Seat ID: " << req.seat_id << "\n";
                    std::string res = "Error: Invalid Seat ID " + std::to_string(req.seat_id) + " (Must be 1-" + std::to_string(NUM_RESOURCES) + ")\n";
                    strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                }
            } 
            else if (cmd == "RESERVE") {
                int idx = req.seat_id - 1;
                if (idx >= 0 && idx < NUM_RESOURCES) {
                    std::string status_str = (reservation_table[idx].status == AVAILABLE) ? "AVAILABLE" : "RESERVED";
                    std::cout << "[Worker-" << worker_id << "] check Resource " << req.seat_id 
                              << ": " << status_str << "\n";

                    if (reservation_table[idx].status == AVAILABLE) {
                    // ----------------------------------------------------------------------------------
                    // ----------------------------------------------------------------------------------
                        std::cout << "[Worker-" << worker_id << "] -> hitting random delay...\n";
                        random_delay();
                    // ----------------------------------------------------------------------------------
                    // ----------------------------------------------------------------------------------

                        reservation_table[idx].status = RESERVED;
                        reservation_table[idx].owner_client_id = req.client_id;
                        std::cout << "[Worker-" << worker_id << "] Resource " << req.seat_id 
                                  << " successfully reserved by Client-" << req.client_id << "\n";

                        std::string res = "SUCCESS: Seat " + std::to_string(req.seat_id) + " successfully reserved by Client-" + std::to_string(req.client_id) + "!\n";
                        strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                    } else {
                        std::cout << "[Worker-" << worker_id << "] Resource " << req.seat_id 
                                  << " already reserved by Client-" << reservation_table[idx].owner_client_id 
                                  << " (FAILED)\n";

                        std::string res = "FAILED: Seat " + std::to_string(req.seat_id) + " is already reserved by Client-" + std::to_string(reservation_table[idx].owner_client_id) + "\n";
                        strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                    }
                } else {
                    std::cout << "[Worker-" << worker_id << "] Invalid Seat ID: " << req.seat_id << "\n";
                    std::string res = "Error: Invalid Seat ID " + std::to_string(req.seat_id) + " (Must be 1-" + std::to_string(NUM_RESOURCES) + ")\n";
                    strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                }
            } 
            else if (cmd == "CANCEL") {
                int idx = req.seat_id - 1;
                if (idx >= 0 && idx < NUM_RESOURCES) {
                    if (reservation_table[idx].status == RESERVED && 
                        reservation_table[idx].owner_client_id == req.client_id) {
                        reservation_table[idx].status = AVAILABLE;
                        reservation_table[idx].owner_client_id = -1;
                        std::cout << "[Worker-" << worker_id << "] Resource " << req.seat_id 
                                  << " reservation cancelled.\n";

                        std::string res = "SUCCESS: Seat " + std::to_string(req.seat_id) + " reservation has been cancelled.\n";
                        strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                    } else {
                        std::cout << "[Worker-" << worker_id << "] Cancel failed: Not reserved by Client-" 
                                  << req.client_id << "\n";

                        std::string res = "FAILED: Cannot cancel Seat " + std::to_string(req.seat_id) + " (You are not the owner)\n";
                        strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                    }
                } else {
                    std::string res = "Error: Invalid Seat ID " + std::to_string(req.seat_id) + "\n";
                    strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
                }
            } 
            else {
                std::cout << "[Worker-" << worker_id << "] Unknown command: " << cmd << "\n";
                std::string res = "Error: Unknown command.\n";
                strncpy(req.server_response, res.c_str(), sizeof(req.server_response) - 1);
            }

            std::cout << "[Worker-" << worker_id << "] leaving critical section\n\n";
        }

        std::string client_resp_q_name = "/resp_client_" + std::to_string(req.client_id);
        mqd_t resp_mq = mq_open(client_resp_q_name.c_str(), O_WRONLY);
        if (resp_mq != (mqd_t)-1) {
            mq_send(resp_mq, (const char*)&req, sizeof(req), 0);
            mq_close(resp_mq);
        }
    }
}

int main() {
    srand(time(nullptr));

    mq_unlink(QUEUE_NAME);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Request);
    attr.mq_curmsgs = 0;

    mqd_t mqdes = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mqdes == (mqd_t)-1) {
        perror("mq_open error");
        return 1;
    }

    init_resources();
    std::cout << "[Server] Started. Shared Reservation Table initialized (" 
              << NUM_RESOURCES << " items).\n";
    // ----------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------
    int num_workers = 3;
    // ----------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------
    // ----------------------------------------------------------------------------------
    std::vector<std::thread> workers;
    for (int i = 1; i <= num_workers; ++i) {
        workers.emplace_back(worker_task, i, mqdes);
    }

    for (auto& t : workers) {
        t.join();
    }

    mq_close(mqdes);
    mq_unlink(QUEUE_NAME);

    return 0;
}