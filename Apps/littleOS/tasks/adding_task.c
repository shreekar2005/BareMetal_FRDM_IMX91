// Task_Name : my task
#include "include/multitasking.h"
#include "include/stdio.h"

void adding_task_thread(void* arg) {
    printf("\r\n[ADDING_TASK] Task started!\r\n");
    
    // TODO: Add your logic here
    os_sleep_ms(500); 
    
    printf("\r\n[ADDING_TASK] Task finffishedd!\r\n");
}
