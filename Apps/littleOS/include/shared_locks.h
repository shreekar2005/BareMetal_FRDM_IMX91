#ifndef SHARED_LOCKS_H
#define SHARED_LOCKS_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Mutex structure
 * value = 1 means available (unlocked)
 * value = 0 means taken (locked)
 */
typedef struct {
    volatile int value; 
} os_mutex_t;


// all mutexes that need to be shared across multiple files should be declared here as extern.

extern os_mutex_t print_dbg_mutex;
extern os_mutex_t esp_send_mutex;
extern os_mutex_t esp_print_mutex;
extern os_mutex_t race_mutex;
extern os_mutex_t esp_transaction_mutex;

/**
 * @brief Initializes a mutex to the unlocked state (value = 1)
 */
void os_mutex_init(os_mutex_t* mutex);

/**
 * @brief Attempts to take the lock. If taken, yields the CPU until it becomes available.
 */
void os_mutex_lock(os_mutex_t* mutex);

/**
 * @brief Releases the lock (sets value back to 1)
 */
void os_mutex_unlock(os_mutex_t* mutex);

#endif // SHARED_LOCKS_H