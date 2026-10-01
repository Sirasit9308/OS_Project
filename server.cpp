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
#include "common.h"

// 1. ตารางทรัพยากรส่วนกลาง (Shared Reservation Table) 20 รายการตามโจทย์
Seat reservation_table[NUM_RESOURCES];
std::mutex mtx; // Mutex สำหรับสลับทดสอบ Experiment 2 และ 3

// ฟังก์ชันเริ่มต้นกำหนดสถานะทรัพยากรทั้งหมดให้ว่าง (AVAILABLE)
void init_resources() {
    for (int i = 0; i < NUM_RESOURCES; ++i) {
        reservation_table[i].id = i + 1;
        reservation_table[i].status = AVAILABLE;
        reservation_table[i].owner_client_id = -1;
    }
}

// ฟังก์ชันสำหรับสร้าง Random Delay (50 - 500 มิลลิวินาที)
void random_delay() {
    int delay_ms = (rand() % 451) + 50; // สุ่มค่าระหว่าง 50 ถึง 500 ms
    usleep(delay_ms * 1000);            // แปลงเป็นไมโครวินาที
}

// 2. ฟังก์ชันการทำงานของ Worker Thread
void worker_task(int worker_id, mqd_t mqdes) {
    Request req;
    while (true) {
        // ดึง Request จาก POSIX Message Queue
        ssize_t bytes_read = mq_receive(mqdes, (char*)&req, sizeof(req), nullptr);
        if (bytes_read == -1) {
            perror("mq_receive error");
            break;
        }

        std::string cmd = req.command;
        std::cout << "[Worker-" << worker_id << "] received command " << cmd 
                  << " for Seat " << req.seat_id << " from Client-" << req.client_id << "\n";

        // =========================================================================
        // 📌 จุดสลับการทดลอง (Experiment 2 vs Experiment 3):
        // - Experiment 2 (ไม่มี Synchronization): ให้คง comment บรรทัด lock นี้ไว้
        // - Experiment 3 (มี Synchronization): ลบ comment ออกเพื่อเปิดใช้งาน Mutex
        // =========================================================================
        // std::lock_guard<std::mutex> lock(mtx);

        std::cout << "[Worker-" << worker_id << "] entering critical section\n";

        if (cmd == "LIST") {
            std::cout << "[Worker-" << worker_id << "] --- Resource List (1-20) ---\n";
            for (int i = 0; i < NUM_RESOURCES; ++i) {
                std::string status_str = (reservation_table[i].status == AVAILABLE) ? "AVAILABLE" : "RESERVED";
                std::string owner_str = (reservation_table[i].owner_client_id == -1) 
                                        ? "-" 
                                        : "Client-" + std::to_string(reservation_table[i].owner_client_id);
                std::cout << "Resource " << reservation_table[i].id 
                          << " | " << status_str 
                          << " | Owner: " << owner_str << "\n";
            }
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
            } else {
                std::cout << "[Worker-" << worker_id << "] Invalid Seat ID: " << req.seat_id << "\n";
            }
        } 
        else if (cmd == "RESERVE") {
            int idx = req.seat_id - 1;
            if (idx >= 0 && idx < NUM_RESOURCES) {
                // 1. ตรวจสอบสถานะทรัพยากร (Check)
                std::string status_str = (reservation_table[idx].status == AVAILABLE) ? "AVAILABLE" : "RESERVED";
                std::cout << "[Worker-" << worker_id << "] check Resource " << req.seat_id 
                          << ": " << status_str << "\n";

                if (reservation_table[idx].status == AVAILABLE) {
                    // 2. จำลอง Race Condition ด้วย Random Delay ระหว่าง Check กับ Update
                    std::cout << "[Worker-" << worker_id << "] -> hitting random delay...\n";
                    random_delay();

                    // 3. อัปเดตข้อมูล (Update)
                    reservation_table[idx].status = RESERVED;
                    reservation_table[idx].owner_client_id = req.client_id;
                    std::cout << "[Worker-" << worker_id << "] Resource " << req.seat_id 
                              << " successfully reserved by Client-" << req.client_id << "\n";
                } else {
                    std::cout << "[Worker-" << worker_id << "] Resource " << req.seat_id 
                              << " already reserved by Client-" << reservation_table[idx].owner_client_id 
                              << " (FAILED)\n";
                }
            } else {
                std::cout << "[Worker-" << worker_id << "] Invalid Seat ID: " << req.seat_id << "\n";
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
                } else {
                    std::cout << "[Worker-" << worker_id << "] Cancel failed: Not reserved by Client-" 
                              << req.client_id << "\n";
                }
            }
        } 
        else {
            std::cout << "[Worker-" << worker_id << "] Unknown command: " << cmd << "\n";
        }

        std::cout << "[Worker-" << worker_id << "] leaving critical section\n\n";
    }
}

int main() {
    srand(time(nullptr));

    // ลบคิวเดิมทิ้งก่อน (ถ้ามีค้างอยู่)
    mq_unlink(QUEUE_NAME);

    // กำหนดแอตทริบิวต์ของ POSIX Message Queue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Request);
    attr.mq_curmsgs = 0;

    // สร้าง Message Queue ในโหมด Read-Write
    mqd_t mqdes = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mqdes == (mqd_t)-1) {
        perror("mq_open error");
        return 1;
    }

    // เริ่มต้น Resource Table
    init_resources();
    std::cout << "[Server] Started. Shared Reservation Table initialized (" 
              << NUM_RESOURCES << " items).\n";

    // สร้าง Worker Threads 3 ตัว
    int num_workers = 3;
    std::vector<std::thread> workers;
    for (int i = 1; i <= num_workers; ++i) {
        workers.emplace_back(worker_task, i, mqdes);
    }

    // รอ Worker Threads
    for (auto& t : workers) {
        t.join();
    }

    // ปิดและลบ Message Queue
    mq_close(mqdes);
    mq_unlink(QUEUE_NAME);

    return 0;
}