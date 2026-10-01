#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <iostream>
#include <string>

// กำหนดประเภทคำสั่ง (Command Types) ตามโจทย์โครงงาน
enum CommandType {
    CMD_LIST = 1,
    CMD_STATUS,
    CMD_RESERVE,
    CMD_CANCEL,
    CMD_QUIT
};

// โครงสร้างข้อความสำหรับส่งผ่าน Message Queue (System V Message Queue)
struct Message {
    long mtype;          // Message type (จำเป็นต้องมากกว่า 0 สำหรับ System V)
    int client_id;       // รหัสหรือ ID ของ Client ผู้ส่ง
    int command;         // คำสั่งที่ส่งมา (จาก CommandType)
    int resource_id;     // หมายเลขทรัพยากร (เช่น 1-20)[cite: 5]
    char data[256];      // ข้อความเพิ่มเติมหรือผลลัพธ์ที่ Server ตอบกลับ
};

// โครงสร้างข้อมูลทรัพยากรในตารางส่วนกลาง (Shared Reservation Table)[cite: 5]
struct ResourceItem {
    int resource_id;     // หมายเลขทรัพยากร (1-20)[cite: 5]
    std::string status;  // สถานะ: "AVAILABLE" หรือ "RESERVED"[cite: 5]
    std::string owner;   // ชื่อผู้ครอบครอง เช่น "Client-1" (ว่างไว้ถ้ายังไม่มีใครจอง)[cite: 5]
};

// กำหนดจำนวนทรัพยากรทั้งหมด (อย่างน้อย 20 รายการตามข้อกำหนดโครงงาน)[cite: 5]
#define TOTAL_RESOURCES 20

// ค่า Key กลางสำหรับการสร้าง Message Queue
#define MQ_KEY_PATH "."
#define MQ_PROJECT_ID 'M'

#endif