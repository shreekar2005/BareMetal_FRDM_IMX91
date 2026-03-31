// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
#include "include/autotasks.h"
#include "include/multitasking.h"
#include <stddef.h>

#include "../tasks/print100o.c"
#include "../tasks/echo.c"
#include "../tasks/ledblink.c"
#include "../tasks/race_test.c"
#include "../tasks/print100X.c"
#include "../tasks/aprint100A.c"

int print100o_thread_id = -1;
int echo_thread_id = -1;
int ledblink_thread_id = -1;
int race_test_thread_id = -1;
int print100X_thread_id = -1;
int aprint100A_thread_id = -1;

TaskRegistry autotasks[6] = {
    {"print100o", "Print 100 o's", print100o_thread, &print100o_thread_id},
    {"echo", "Console Echo", echo_thread, &echo_thread_id},
    {"ledblink", "LED Blink", ledblink_thread, &ledblink_thread_id},
    {"race_test", "Race Condition", race_test_thread, &race_test_thread_id},
    {"print100X", "Print 100 X's", print100X_thread, &print100X_thread_id},
    {"aprint100A", "Atomic Print 10", aprint100A_thread, &aprint100A_thread_id},
};
const int num_autotasks = 6;

void init_all_tasks(void) {
    print100o_thread_id = os_create_thread("Print 100 o's", print100o_thread, NULL);
    echo_thread_id = os_create_thread("Console Echo", echo_thread, NULL);
    ledblink_thread_id = os_create_thread("LED Blink", ledblink_thread, NULL);
    race_test_thread_id = os_create_thread("Race Condition", race_test_thread, NULL);
    print100X_thread_id = os_create_thread("Print 100 X's", print100X_thread, NULL);
    aprint100A_thread_id = os_create_thread("Atomic Print 10", aprint100A_thread, NULL);
}
