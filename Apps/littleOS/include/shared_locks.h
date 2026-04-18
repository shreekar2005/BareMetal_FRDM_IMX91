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

// THE STATIC INITIALIZER
#define OS_MUTEX_INITIALIZER { 1 }

/**
 * @brief Attempts to take the lock. If taken, yields the CPU until it becomes available.
 */
void mutex_lock(os_mutex_t* mutex);

/**
 * @brief Releases the lock (sets value back to 1)
 */
void mutex_unlock(os_mutex_t* mutex);

#endif // SHARED_LOCKS_H