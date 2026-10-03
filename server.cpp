#include <iostream>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <cstdlib>
#include <ctime>
#include "protocol.h"

// 1. ตารางทรัพยากรส่วนกลาง (Shared Reservation Table) 20 รายการตามโจทย์[cite: 5]
ResourceItem reservation_table[TOTAL_RESOURCES];
std::mutex mtx; // Mutex สำหรับใช้เปิด/ปิด Synchronization (ทดลอง Experiment 2 และ 3)

// ฟังก์ชันเริ่มต้นกำหนดสถานะทรัพยากรทั้งหมดให้ว่าง (AVAILABLE)
void init_resources() {
    for (int i = 0; i < TOTAL_RESOURCES; ++i) {
        reservation_table[i].resource_id = i + 1;
        reservation_table[i].status = "AVAILABLE";
        reservation_table[i].owner = "-";
    }
}

// ฟังก์ชันสำหรับสร้าง Random Delay (1000 - 3000 มิลลิวินาที) ตามข้อกำหนดโครงงาน
void random_delay() {
    int delay_ms = (rand() % 2001) + 1000; // สุ่มค่าระหว่าง 1000 ถึง 3000 ms
    usleep(delay_ms * 1000);             // แปลงเป็นไมโครวินาที
}

// 2. ฟังก์ชันการทำงานของ Worker Thread (อย่างน้อย 3 ตัว)[cite: 5]
void worker_task(int worker_id, int msqid) {
    Message msg;
    while (true) {
        // ดึงข้อความจาก Message Queue
        if (msgrcv(msqid, &msg, sizeof(Message) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv error");
            break;
        }

        std::cout << "[Worker-" << worker_id << "] received command " << msg.command 
                  << " for Resource " << msg.resource_id << " from Client-" << msg.client_id << "\n";

        // =========================================================================
        // 📌 จุดสลับการทดลอง (Experiment 2 vs Experiment 3):
        // - Experiment 2 (ไม่มี Synchronization): ให้คง comment บรรทัด lock นี้ไว้
        // - Experiment 3 (มี Synchronization): ให้ลบเครื่องหมาย // ออกเพื่อเปิดใช้งาน Mutex
        // =========================================================================
        std::lock_guard<std::mutex> lock(mtx); 

        std::cout << "[Worker-" << worker_id << "] entering critical section\n";

        // ประมวลผลคำสั่งตามประเภท (Switch-Case)[cite: 5]
        switch (msg.command) {
            case CMD_LIST: {
                std::cout << "[Worker-" << worker_id << "] --- Resource List (1-20) ---\n";
                for (int i = 0; i < TOTAL_RESOURCES; ++i) {
                    std::cout << "Resource " << reservation_table[i].resource_id 
                              << " | " << reservation_table[i].status 
                              << " | Owner: " << reservation_table[i].owner << "\n";
                }
                break;
            }
            case CMD_STATUS: {
                int idx = msg.resource_id - 1;
                if (idx >= 0 && idx < TOTAL_RESOURCES) {
                    std::cout << "[Worker-" << worker_id << "] Status Resource " << msg.resource_id 
                              << ": " << reservation_table[idx].status 
                              << " (Owner: " << reservation_table[idx].owner << ")\n";
                }
                break;
            }
            case CMD_RESERVE: {
                int idx = msg.resource_id - 1;
                if (idx >= 0 && idx < TOTAL_RESOURCES) {
                    // 1. ตรวจสอบสถานะทรัพยากร (Check)
                    std::cout << "[Worker-" << worker_id << "] check Resource " << msg.resource_id 
                              << ": " << reservation_table[idx].status << "\n";

                    if (reservation_table[idx].status == "AVAILABLE") {
                        
                        // 2. ใส่ Random Delay ระหว่าง check และ update ตามโจทย์โครงงาน
                        std::cout << "[Worker-" << worker_id << "] -> hitting random delay...\n";
                        random_delay(); 

                        // 3. อัปเดตข้อมูล (Update)
                        reservation_table[idx].status = "RESERVED";
                        reservation_table[idx].owner = "Client-" + std::to_string(msg.client_id);
                        std::cout << "[Worker-" << worker_id << "] Resource " << msg.resource_id 
                                  << " successfully reserved by Client-" << msg.client_id << "\n";
                    } else {
                        std::cout << "[Worker-" << worker_id << "] Resource " << msg.resource_id 
                                  << " already reserved by " << reservation_table[idx].owner << " (FAILED)\n";
                    }
                }
                break;
            }
            case CMD_CANCEL: {
                int idx = msg.resource_id - 1;
                if (idx >= 0 && idx < TOTAL_RESOURCES) {
                    reservation_table[idx].status = "AVAILABLE";
                    reservation_table[idx].owner = "-";
                    std::cout << "[Worker-" << worker_id << "] Resource " << msg.resource_id 
                              << " reservation cancelled.\n";
                }
                break;
            }
            case CMD_QUIT:
                std::cout << "[Worker-" << worker_id << "] Client-" << msg.client_id << " requested QUIT.\n";
                break;
            default:
                std::cout << "[Worker-" << worker_id << "] Unknown command type.\n";
                break;
        }

        std::cout << "[Worker-" << worker_id << "] leaving critical section\n\n";
    }
}

int main() {
    // กำหนด Seed สำหรับการสุ่มตัวเลข
    srand(time(nullptr));

    // 1. สร้าง Key สำหรับ Message Queue
    key_t key = ftok(MQ_KEY_PATH, MQ_PROJECT_ID);
    if (key == -1) {
        perror("ftok error");
        exit(1);
    }

    // 2. สร้างหรือเข้าถึง Message Queue
    int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid == -1) {
        perror("msgget error");
        exit(1);
    }

    // 3. เริ่มต้นตารางทรัพยากรส่วนกลาง (20 รายการ)[cite: 5]
    init_resources();
    std::cout << "[Server] Started. Shared Reservation Table initialized (20 items).\n";

    // 4. สร้าง Worker Threads จำนวน 3 ตัว (ตามข้อกำหนดโครงงาน)[cite: 5]
    int num_workers = 3;
    std::vector<std::thread> workers;
    for (int i = 1; i <= num_workers; ++i) {
        workers.emplace_back(worker_task, i, msqid);
    }

    // รอให้ Worker Threads ทำงานวนลูปไปเรื่อยๆ
    for (auto& t : workers) {
        t.join();
    }

    // ลบ Message Queue เมื่อเลิกใช้งาน
    msgctl(msqid, IPC_RMID, NULL);
    return 0;
}