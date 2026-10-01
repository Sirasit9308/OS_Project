#include "common.h"
#include <fcntl.h>
#include <unistd.h>

struct Seat seats[NUM_RESOURCES];
pthread_mutex_t seat_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]){

    for(int i = 0; i < NUM_RESOURCES; i++){
        seats[i].id = i + 1;
        seats[i].status = AVAILABLE;
        seats[i].owner_client_id = -1;
    }

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;                    
    attr.mq_msgsize = sizeof(struct Request);
    attr.mq_curmsgs = 0;

    mq_unlink(QUEUE_NAME);
    mqd_t server_mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (server_mq == (mqd_t)-1) {
        perror("Server mq_open failed.");
        return 1;
    }
    std::cout << "Server is running and." << QUEUE_NAME << "...\n";

    while(true){
        ssize_t bytes_read = mq_receive(server_mq, (char*)&req, sizeof(req), NULL);
        if(bytes_read >= 0){
            pthread_t tid;
            pthread_create(&tid, NULL, handle_request, (void)* );
            pthread_detach(tid);
        }
    }

    return 0;
}

void* handle_request(void* arg) {
    // 1. แปลง arg กลับมาเป็น struct Request ของเรา
    
    // 2. ล็อคห้องก่อนแตะข้อมูลที่นั่ง (Critical Section)
    // ใช้ฟังก์ชัน: pthread_mutex_lock(&seat_mutex);

    // 3. ตรวจสอบคำสั่ง (cmd) ด้วย if / else if:
    // - ถ้าเป็น "LIST" -> วนลูปดูสถานะของ seats ทั้งหมด 20 ที่
    // - ถ้าเป็น "STATUS" -> ดูที่นั่งตาม seat_id ที่ขอมา
    // - ถ้าเป็น "RESERVE" -> เช็คว่าว่างไหม ถ้าว่างให้เปลี่ยนเป็น RESERVED และใส่ owner
    // - ถ้าเป็น "CANCEL" -> เช็คว่าเป็นเจ้าของที่นั่งนี้จริงไหม ถ้าใช่ให้เคลียร์คืนเป็น AVAILABLE

    // 4. ปลดล็อคห้อง
    // ใช้ฟังก์ชัน: pthread_mutex_unlock(&seat_mutex);

    return NULL;
}