# Concurrent Reservation System (OS Project 2569)

A concurrent resource reservation system (e.g., movie theater seats, flight seats, or meeting rooms) implemented in C++ using **POSIX Message Queue** for Interprocess Communication (IPC) and **Multithreading (std::thread & std::mutex)** for Concurrency Control.

---

## 📌 Project Overview
- **Scenario:** Resource Reservation System (20 resources, numbered 1–20).
- **Architecture:** Client-Server Architecture via POSIX Message Queue (`/reservation_mq`).
- **Concurrency Model:** Multi-worker server handling concurrent requests with synchronization control.

---

## 🚀 Getting Started & Docker Environment

### 1. Build Docker Image
Open your terminal inside the project directory and build the Docker image:
```bash
docker build -t os-project .
```

### 2. Run Docker Container
Start an interactive container named `os-server-client`:
```bash
docker run -it --name os-server-client os-project
```

---

## 🖥️ How to Run the Server & Clients

### Running the Server (Terminal 1)
Inside the container, compile and run the server:
```bash
g++ -std=c++11 server.cpp -o server -lrt -lpthread
./server
```

### Running the Client (Terminals 2, 3, 4, 5, 6...)
Open new terminal windows on your host machine and attach to the running container:
```bash
docker exec -it os-server-client bash
```
Compile the client program:
```bash
g++ -std=c++11 client.cpp -o client -lrt
```
Run the client with a specific Client ID (e.g., Client 1):
```bash
./client 1
```

---

## 📋 Client Commands Guide

Once running a client (`./client <client_id>`), you can use the following commands:

| Command | Syntax | Description | Example |
| :--- | :--- | :--- | :--- |
| **LIST** | `LIST` | View all 20 resources with their status and owners. | `LIST` |
| **STATUS** | `STATUS <resource_id>` | Check the status of a specific resource. | `STATUS 10` |
| **RESERVE** | `RESERVE <resource_id>` | Request to reserve a specific resource. | `RESERVE 10` |
| **CANCEL** | `CANCEL <resource_id>` | Cancel your reservation for a resource. | `CANCEL 10` |
| **QUIT** | `QUIT` | Exit the client program. | `QUIT` |

---

## 🧪 Race Condition Experiments

This project includes three experimental setups to demonstrate concurrency issues and solutions:

### Experiment 1 — Sequential Baseline
- **Configuration:** Set `num_workers = 1` in `server.cpp`.
- **Purpose:** Verifies correct sequential processing and absence of race conditions with a single worker.

### Experiment 2 — Concurrent without Synchronization
- **Configuration:** 
  - Set `num_workers = 3` in `server.cpp`.
  - **Comment out** the Mutex lock line in `server.cpp`:
    ```cpp
    // std::lock_guard<std::mutex> lock(mtx);
    ```
  - Random delay is active between `check` and `update` to widen the race window.
- **Test:** Have multiple clients send `RESERVE 10` simultaneously. Observe race conditions where multiple workers check `AVAILABLE` before updating.

### Experiment 3 — Concurrent with Synchronization
- **Configuration:** 
  - Keep `num_workers = 3`.
  - **Uncomment** the Mutex lock line in `server.cpp`:
    ```cpp
    std::lock_guard<std::mutex> lock(mtx);
    ```
- **Test:** Run the reservation test again. Only **one** client will successfully reserve the resource while others receive a `(FAILED)` notice.

---

## 📂 File Structure
- `common.h`: Shared data structures (`Request`, `Seat`, etc.) and protocol constants.
- `server.cpp`: Multi-threaded server managing the reservation table and message queue.
- `client.cpp`: Command-line interface client application.
- `Dockerfile`: Container environment configuration.
- `Makefile`: Build automation rules.