// THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
#include "include/autotasks.h"
#include "include/multitasking.h"
#include <stddef.h>

#include "../tasks/print100o.c"
#include "../tasks/race.c"
#include "../tasks/echo.c"
#include "../tasks/ledblink.c"
#include "../tasks/statistics.c"
#include "../tasks/print100X.c"
#include "../tasks/esp.c"
#include "../tasks/aprint100A.c"
#include "../tasks/sonar.c"
#include "../tasks/datetime.c"

int print100o_thread_id = -1;
int race_thread_id = -1;
int echo_thread_id = -1;
int ledblink_thread_id = -1;
int statistics_thread_id = -1;
int print100X_thread_id = -1;
int esp_thread_id = -1;
int aprint100A_thread_id = -1;
int sonar_thread_id = -1;
int datetime_thread_id = -1;

TaskRegistry autotasks[10] = {
    {"print100o", "Print o's", print100o_thread, &print100o_thread_id},
    {"race", "Race Condition", race_thread, &race_thread_id},
    {"echo", "DBG ECHO", echo_thread, &echo_thread_id},
    {"ledblink", "LED Blink", ledblink_thread, &ledblink_thread_id},
    {"statistics", "Show stats", statistics_thread, &statistics_thread_id},
    {"print100X", "Print X's", print100X_thread, &print100X_thread_id},
    {"esp", "ESP Commands", esp_thread, &esp_thread_id},
    {"aprint100A", "AtomicPrint A's", aprint100A_thread, &aprint100A_thread_id},
    {"sonar", "sonar", sonar_thread, &sonar_thread_id},
    {"datetime", "datetime", datetime_thread, &datetime_thread_id},
};
const int numAutotasks = 10;

void init_all_tasks(void) {
    print100o_thread_id = os_create_thread("Print o's", print100o_thread, NULL);
    race_thread_id = os_create_thread("Race Condition", race_thread, NULL);
    echo_thread_id = os_create_thread("DBG ECHO", echo_thread, NULL);
    ledblink_thread_id = os_create_thread("LED Blink", ledblink_thread, NULL);
    statistics_thread_id = os_create_thread("Show stats", statistics_thread, NULL);
    print100X_thread_id = os_create_thread("Print X's", print100X_thread, NULL);
    esp_thread_id = os_create_thread("ESP Commands", esp_thread, NULL);
    aprint100A_thread_id = os_create_thread("AtomicPrint A's", aprint100A_thread, NULL);
    sonar_thread_id = os_create_thread("sonar", sonar_thread, NULL);
    datetime_thread_id = os_create_thread("datetime", datetime_thread, NULL);
}
