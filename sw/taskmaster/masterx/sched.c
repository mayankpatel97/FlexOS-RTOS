#include <stdint.h>

#define STACK_SIZE 1024  // Define stack size for each task

uint8_t max_tasks = 0;
typedef struct {
    uint32_t *stackPointer;  // Stack pointer for the task
    void (*taskFunction)(void);  // Pointer to the task function
    uint32_t stack[STACK_SIZE];  // Stack memory for the task
} TaskControlBlock;

// Array of TCBs for each task
TaskControlBlock tcbArray[max_tasks];

// Current task index
volatile uint32_t currentTaskIndex = 0;
void tm_startTask(uint32_t taskIndex, void (*taskFunction)(void));
void tm_tick_handler(void);
void tm_startSched(void);

// Initialize the TCB for each task
void tm_startTask(uint32_t taskIndex, void (*taskFunction)(void)) {
    tcbArray[taskIndex].taskFunction = taskFunction;
    tcbArray[taskIndex].stackPointer = &tcbArray[taskIndex].stack[STACK_SIZE - 1];

    // Set up the stack for this task
    *(--tcbArray[taskIndex].stackPointer) = 0x01000000;  // xPSR
    *(--tcbArray[taskIndex].stackPointer) = (uint32_t)taskFunction;  // PC
    *(--tcbArray[taskIndex].stackPointer) = 0xFFFFFFF9;  // LR
    for (int i = 0; i < 13; i++) {
        *(--tcbArray[taskIndex].stackPointer) = 0;  // R0-R12
    }

    max_tasks++;
}


// Timer interrupt handler (context switch)
void tm_tick_handler(void) {
    // Save context of the current task
    __asm volatile(
        "PUSH {R4-R11} \n"  // Save non-volatile registers
        "LDR R0, =currentTaskIndex \n"
        "LDR R1, [R0] \n"
        "LDR R2, =tcbArray \n"
        "LDR R3, [R2, R1, LSL #2] \n"
        "STR SP, [R3] \n"  // Save stack pointer
    );

    // Select next task
    currentTaskIndex = (currentTaskIndex + 1) % max_tasks;

    // Restore context of the next task
    __asm volatile(
        "LDR R0, =currentTaskIndex \n"
        "LDR R1, [R0] \n"
        "LDR R2, =tcbArray \n"
        "LDR R3, [R2, R1, LSL #2] \n"
        "LDR SP, [R3] \n"  // Restore stack pointer
        "POP {R4-R11} \n"  // Restore non-volatile registers
        "BX LR \n"  // Return from interrupt
    );
}

// Start the scheduler by enabling the SysTick timer
void tm_startSched(void) {
    // Configure SysTick timer for context switch every 1ms
    //SysTick_Config(SystemCoreClock / 1000);

    // Start first task
    currentTaskIndex = 0;
    __asm volatile(
        "LDR R0, =tcbArray \n"
        "LDR R1, [R0] \n"
        "LDR SP, [R1] \n"  // Load the stack pointer of the first task
        "POP {R4-R11} \n"  // Restore the non-volatile registers
        "BX LR \n"  // Branch to task
    );
}
