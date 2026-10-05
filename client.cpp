#include "common.h"
#include <fcntl.h>
#include <sys/stat.h>

// ฟังก์ชันแสดงหน้าต่าง Default Guide / Menu
void print_menu(int client_id) {
    std::cout << "\n======================================================\n";
    std::cout << "          CLIENT-" << client_id << " RESERVATION SYSTEM\n";
    std::cout << "======================================================\n";
    std::cout << " Available Commands:\n";
    std::cout << "  1. LIST              : View all seats status (1-" << NUM_RESOURCES << ")\n";
    std::cout << "  2. STATUS <seat_id>  : Check status of a specific seat\n";
    std::cout << "  3. RESERVE <seat_id> : Reserve a specific seat\n";
    std::cout << "  4. CANCEL <seat_id>  : Cancel your reserved seat\n";
    std::cout << "  5. QUIT              : Exit the client program\n";
    std::cout << "======================================================\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./client <client_id>\n";
        return 1;
    }

    int client_id = std::stoi(argv[1]);

    // 1. สร้าง Response Queue สำหรับ Client ตัวนี้โดยเฉพาะ
    std::string resp_queue_name = "/resp_client_" + std::to_string(client_id);
    mq_unlink(resp_queue_name.c_str()); // ลบคิวเดิมทิ้งก่อน (ถ้ามีค้างอยู่)

    struct mq_attr resp_attr;
    resp_attr.mq_flags = 0;
    resp_attr.mq_maxmsg = 10;
    resp_attr.mq_msgsize = sizeof(Request);
    resp_attr.mq_curmsgs = 0;

    mqd_t resp_mq = mq_open(resp_queue_name.c_str(), O_CREAT | O_RDWR, 0666, &resp_attr);
    if (resp_mq == (mqd_t)-1) {
        perror("mq_open response queue error");
        return 1;
    }

    // 2. เชื่อมต่อไปยัง Server Request Queue
    mqd_t message_queue = mq_open(QUEUE_NAME, O_WRONLY);
    if (message_queue == (mqd_t)-1) {
        perror("mq_open server queue error");
        mq_close(resp_mq);
        mq_unlink(resp_queue_name.c_str());
        return 1;
    }

    // แสดงหน้าจอ Default Menu ครั้งแรกหลัง Login เข้ามา
    print_menu(client_id);

    std::string client_command;
    while (std::cout << "Client-" << client_id << "> " && std::cin >> client_command) {
        if (client_command == "QUIT") {
            std::cout << "Exiting...\n";
            break;
        }

        if (client_command != "LIST" && client_command != "STATUS" && 
            client_command != "RESERVE" && client_command != "CANCEL") {
            std::cout << ">> [Error]: We don't have this function.\n";
            print_menu(client_id); // แสดงเมนูซ้ำเมื่อพิมพ์ผิด
            continue;
        }

        struct Request request;
        memset(&request, 0, sizeof(request));
        request.client_id = client_id;
        strcpy(request.command, client_command.c_str());
        request.seat_id = 0;

        if (client_command == "STATUS" || client_command == "RESERVE" || client_command == "CANCEL") {
            if (!(std::cin >> request.seat_id)) {
                std::cout << ">> [Error]: Invalid seat number input.\n";
                std::cin.clear();
                std::cin.ignore(10000, '\n');
                print_menu(client_id);
                continue;
            }
        }

        // ส่ง Request ไปยังเซิร์ฟเวอร์
        if (mq_send(message_queue, (const char*)&request, sizeof(request), 0) == -1) {
            perror("mq_send error.");
        } else {
            // รอรับ Response ตอบกลับจากเซิร์ฟเวอร์
            struct Request response;
            if (mq_receive(resp_mq, (char*)&response, sizeof(response), nullptr) == -1) {
                perror("mq_receive response error.");
            } else {
                std::cout << "\n------------------------------------------------------\n";
                std::cout << "[Server Response]:\n" << response.server_response;
                std::cout << "------------------------------------------------------\n";
            }
        }

        // แสดงหน้าจอ Menu อีกครั้งหลังทำคำสั่งเสร็จสิ้น
        print_menu(client_id);
    }

    // ปิดและทำลาย Message Queue เมื่อออกจากโปรแกรม
    mq_close(message_queue);
    mq_close(resp_mq);
    mq_unlink(resp_queue_name.c_str());

    return 0;
}