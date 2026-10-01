# ใช้ Ubuntu เป็นสภาพแวดล้อมพื้นฐาน
FROM ubuntu:22.04

# อัปเดตแพ็กเกจและติดตั้งคอมไพล์เลอร์ C++ และเครื่องมือพัฒนา
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# กำหนด Working Directory ภายใน Container
WORKDIR /app

# คัดลอกไฟล์ทั้งหมดจากเครื่องจริงเข้าไปใน Container
COPY . /app/
