// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
#include "include/autotasks.h"
#include "include/multitasking.h"
#include <stddef.h>

#include "../tasks/print100o.c"
#include "../tasks/echo.c"
#include "../tasks/ledblink.c"
#include "../tasks/print100X.c"
#include "../tasks/aprint100A.c"

int print100o_thread_id = -1;
int echo_thread_id = -1;
int ledblink_thread_id = -1;
int print100X_thread_id = -1;
int aprint100A_thread_id = -1;

TaskRegistry autotasks[5] = {
    {"print100o", "Print 100 o's", print100o_thread, &print100o_thread_id},
    {"echo", "Console Echo", echo_thread, &echo_thread_id},
    {"ledblink", "LED Blink", ledblink_thread, &ledblink_thread_id},
    {"print100X", "Print 100 X's", print100X_thread, &print100X_thread_id},
    {"aprint100A", "Atomic Print A", aprint100A_thread, &aprint100A_thread_id},
};
const int num_autotasks = 5;

void init_all_tasks(void) {
    print100o_thread_id = os_create_thread("Print 100 o's", print100o_thread, NULL);
    echo_thread_id = os_create_thread("Console Echo", echo_thread, NULL);
    ledblink_thread_id = os_create_thread("LED Blink", ledblink_thread, NULL);
    print100X_thread_id = os_create_thread("Print 100 X's", print100X_thread, NULL);
    aprint100A_thread_id = os_create_thread("Atomic Print A", aprint100A_thread, NULL);
}
