#ifndef __SCHED__
#define __SCHED__

void tm_startTask(uint32_t taskIndex, void (*taskFunction)(void));
void tm_tick_handler(void);
void tm_startSched(void);


#endif
