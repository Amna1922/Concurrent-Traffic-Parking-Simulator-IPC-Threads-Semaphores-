# 🚦 Multi-Intersection Traffic Simulation with Priority Control & Parking System

## 📌 Overview

This project simulates two neighboring intersections (**F10** and **F11**) with concurrent vehicles, independent traffic controllers communicating via pipes, priority handling for emergency vehicles, and an integrated **parking lot system** attached to each intersection. The simulation demonstrates real-world traffic coordination, emergency preemption, and bounded waiting queues using **semaphores** — all while ensuring no deadlock or intersection blocking.

---

## 🎯 High-Level Scenario

- Two intersections operate independently but coordinate during emergencies.
- Vehicles (15 by default, configurable) are implemented as **pthreads**.
- Each intersection has its own **controller process** (`fork()`), communicating via **pipes**.
- Emergency vehicles (Ambulance, Firetruck) can **preempt** normal signal cycles.
- Parking lots (10 spots each) use **semaphores** for capacity and bounded waiting queues.
- Parking-bound vehicles never block the intersection.

---

## 🧩 Key Features

### 🚗 Vehicle Types & Behavior
| Category    | Priority   | Parking Attempt | Emergency Preemption |
|-------------|------------|----------------|----------------------|
| Ambulance   | Highest    | ❌ No           | ✅ Yes               |
| Firetruck   | Highest    | ❌ No           | ✅ Yes               |
| Bus         | Medium     | ✅ Yes          | ❌ No                |
| Car         | Normal     | ✅ Yes          | ❌ No                |
| Bike        | Normal     | ✅ Yes          | ❌ No                |
| Tractor     | Normal     | ✅ Yes          | ❌ No                |

### 🚦 Traffic Controllers (IPC via Pipes)
- Each controller runs as a separate process (`fork()`).
- Controllers exchange coordination messages (e.g., emergency approaching).
- F10 can inform F11 to clear the path for an incoming ambulance.

### 🚨 Priority & Preemption
- Emergency vehicles interrupt normal green/red cycles.
- Path is cleared in advance across both intersections.
- Buses receive medium priority (optional extension).

### 🅿️ Parking Lot System (Semaphore-Based)
- **10 parking spots** → counting semaphore.
- **Bounded waiting queue** → second semaphore prevents unbounded waits.
- Vehicles reserve a spot or wait-slot **before** entering intersection.
- Parking logic never blocks emergency preemption.

### ⚙️ Safe Crossing Constraint
- Only non-conflicting movements proceed concurrently.
- Intersection logic ensures no collisions.

---

## 🧵 Concurrency Model

| Component          | Implementation          |
|--------------------|-------------------------|
| Vehicles           | `pthread_t` threads     |
| Intersection Ctrl  | Separate processes (`fork()`) |
| IPC                | Pipes (between parent & controllers) |
| Parking spots      | Semaphore (POSIX unnamed) |
| Waiting queue slots| Semaphore (bounded)      |
| Intersection access| Mutex / semaphore        |

---

## 🛠️ Build & Run

### Prerequisites
- Linux / Unix environment
- GCC with pthread support
- POSIX semaphores (`-lpthread`)

### Compilation
```bash
make clean && make && ./traffic_sim
