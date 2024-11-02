/**
 * @file context.c
 * @brief Context switching implementation for RTOS on STM32 Cortex-M4
 * 
 * This file implements the context switching mechanisms for an RTOS
 * targeting the ARM Cortex-M4 processor. It handles:
 * - Task context saving and restoration
 * - PendSV exception handling for context switching
 * - Initial task context setup
 */

#include <stdint.h>
#include "context.h"

/* Assembly functions declared in context_asm.s */
extern void PendSV_Handler(void);
extern void SVC_Handler(void);

/* Current and next task control blocks */
TCB_t *current_task = NULL;
TCB_t *next_task = NULL;

/**
 * @brief Initialize task stack frame
 * 
 * Sets up initial stack frame for a task, including:
 * - CPU registers (R0-R12)
 * - Link Register (LR)
 * - Program Counter (PC)
 * - Program Status Register (PSR)
 * 
 * @param task_func Pointer to task function
 * @param stack_ptr Pointer to top of task's stack
 * @return uint32_t* New stack pointer after context setup
 */
uint32_t* task_stack_init(task_func_t task_func, uint32_t *stack_ptr) {
    /* Stack frame follows Cortex-M4 exception entry layout */
    stack_ptr--;
    *stack_ptr = 0x01000000;    /* PSR: Set T-bit for Thumb mode */
    stack_ptr--;
    *stack_ptr = (uint32_t)task_func;  /* PC: Task entry point */
    stack_ptr--;
    *stack_ptr = 0xFFFFFFFD;    /* LR: Return to Thread mode using PSP */
    
    /* R12, R3-R0 */
    for (int i = 0; i < 5; i++) {
        stack_ptr--;
        *stack_ptr = 0;
    }
    
    /* R11-R4 */
    for (int i = 0; i < 8; i++) {
        stack_ptr--;
        *stack_ptr = 0;
    }

    return stack_ptr;
}

/**
 * @brief Start the first task
 * 
 * Initializes system for first context switch:
 * 1. Sets PendSV exception priority to lowest
 * 2. Sets up PSP for first task
 * 3. Switches to use PSP
 * 4. Starts first task
 */
void start_first_task(void) {
    /* Set PendSV to lowest priority */
    *(uint32_t volatile *)0xE000ED20 |= (0xFF << 16);
    
    /* Set PSP to first task's stack */
    __asm volatile (
        "ldr r0, =current_task\n"
        "ldr r0, [r0]\n"
        "ldr r0, [r0]\n"
        "msr psp, r0\n"
        "mov r0, #2\n"
        "msr control, r0\n"
        "isb\n"
        "pop {r0-r11}\n"
        "pop {r12}\n"
        "pop {lr}\n"
        "pop {pc}\n"
        "bx lr\n"
    );
}

/**
 * @brief Trigger context switch
 * 
 * Sets PendSV pending bit to trigger context switch
 * at next opportunity
 */
void trigger_context_switch(void) {
    /* Set PendSV pending bit */
    *(uint32_t volatile *)0xE000ED04 = 0x10000000;
}

/**
 * @brief Initialize context switching system
 * 
 * Sets up necessary system configurations for context switching:
 * - Exception priorities
 * - System timer (if used)
 * - Initial task setup
 */
void context_init(void) {
    /* Disable all interrupts */
    __asm volatile ("cpsid i");
    
    /* Initialize system timer for tick interrupts */
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;  /* 1ms tick */
    SysTick->VAL = 0;
    SysTick->CTRL = 0x07;  /* Enable, interrupt, use CPU clock */
    
    /* Set PendSV to lowest priority */
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    
    /* Enable interrupts */
    __asm volatile ("cpsie i");
}

/**
 * @brief Save current task context
 * @param psp Current process stack pointer
 * @return uint32_t* Updated stack pointer
 */
uint32_t* save_context(uint32_t *psp) {
    /* Save remaining registers */
    current_task->sp = psp;
    return psp;
}

/**
 * @brief Restore next task context
 * @param psp Stack pointer to restore
 * @return uint32_t* Updated stack pointer
 */
uint32_t* restore_context(uint32_t *psp) {
    /* Update current task */
    current_task = next_task;
    psp = current_task->sp;
    return psp;
}