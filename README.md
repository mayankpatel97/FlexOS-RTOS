# Task Master

 Overview
Task Master is a lightweight task scheduler designed for microcontrollers. It provides an efficient way to manage and prioritize tasks, ensuring that critical processes meet their deadlines in real-time applications. This scheduler is ideal for embedded systems requiring multitasking capabilities.

 Features
- Priority-Based Scheduling: Assign different priorities to tasks to ensure critical tasks run first.
- Preemptive Scheduling: High-priority tasks can interrupt lower-priority tasks, allowing for responsive real-time performance.
- Time Management: Configure time slices for task execution, supporting round-robin scheduling.
- Easy Integration: Simple API to integrate with existing embedded systems.
- Lightweight: Minimal footprint suitable for resource-constrained environments.

 Getting Started

# Prerequisites
- A compatible microcontroller (e.g., STM32, Arduino, ESP32)
- Basic understanding of C/C++ programming

# Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/mayankpatel97/task-master.git


# Usage

    #include "task_master.h"

    // Define your tasks
    void task1() {
        // Task 1 implementation
    }

    void task2() {
        // Task 2 implementation
    }

    int main() {
        // Initialize the scheduler
        init_scheduler();

        // Create tasks with priority
        create_task(task1, HIGH_PRIORITY);
        create_task(task2, LOW_PRIORITY);

        // Start the scheduler
        start_scheduler();

        return 0;
    }


# API Reference
    - init_scheduler(): Initializes the task scheduler.
    - create_task(void (*task_func)(), int priority): Creates a new task with the specified function and priority.
    - start_scheduler(): Starts the scheduler to begin task execution.

# Contributing
Contributions are welcome! If you'd like to contribute to Task Master, please follow these steps:

    - Fork the repository.
    - Create a new branch (git checkout -b feature/YourFeature).
    - Commit your changes (git commit -m 'Add new feature').
    - Push to the branch (git push origin feature/YourFeature).
    - Open a pull request.
    
# License
This project is licensed under the MIT License - see the LICENSE file for details.