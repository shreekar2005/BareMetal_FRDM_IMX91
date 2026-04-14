#include "include/shared_locks.h"
#include "include/multitasking.h"

void os_mutex_init(os_mutex_t* mutex) {
    // __ATOMIC_RELEASE ensures any previous memory operations are finished 
    // before we officially mark the lock as free (1).
    __atomic_store_n(&mutex->value, 1, __ATOMIC_RELEASE);
}

void os_mutex_lock(os_mutex_t* mutex) {
    int expected;
    
    while (1) {
        expected = 1; // We expect the lock to be free (1)
        
        // HARDWARE INSTRUCTION
        // This compiles down to an atomic Compare-And-Swap (CAS) or LDXR/STXR loop.
        // It asks the hardware: "Is the value currently 'expected' (1)? If yes, change it to 0."
        // All of this happens in a single, uninterruptible hardware step.
        if (__atomic_compare_exchange_n(&mutex->value, &expected, 0, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            // The hardware successfully swapped the value. We own the lock!
            return; 
        }
        
        // If we get here, the hardware saw that the value was already 0 (locked).
        // Yield the CPU to let the thread holding the lock finish its work.
        os_yield(); 
    }
}

void os_mutex_unlock(os_mutex_t* mutex) {
    // Atomically push a 1 back into memory to free the lock.
    __atomic_store_n(&mutex->value, 1, __ATOMIC_RELEASE);
}