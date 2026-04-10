# littleOS

This is a preemptive RTOS (Real-Time Operating System) kernel developed from scratch for the NXP i.MX91 (ARMv8-A) board. It features a custom context-switching engine, multiple scheduling algorithms, a modular task registry, and an interactive CLI with Wi-Fi remote execution capabilities.

The OS uses the ARM Generic Timer to provide a 20ms time-slice for threads, supports true voluntary blocking/sleeping, and allows "atomic" execution to temporarily lock the CPU for critical hardware bit-banging.

## Features
* **Preemptive Multitasking**: Full context switching (saving/restoring 31 registers + ELR/SPSR) driven by the ARM Generic Timer.
* **Dynamic Scheduling**: Swap between three scheduling algorithms on the fly:
    * **Round Robin (RR)**: Standard time-slicing for equal CPU distribution.
    * **Fixed Priority (PRI)**: Highest priority task (lower value) always wins the CPU.
    * **Earliest Deadline First (EDF)**: Dynamic priority based on the closest absolute deadline.
* **Wi-Fi & Remote CLI**: Full HAL for the ESP8266 module. Supports Station and Access Point modes. Includes a background listener that allows executing CLI commands remotely via `nc` (netcat).
* **True RTOS Blocking**: Threads can yield the CPU using `thread_sleep()`, moving to a `BLOCK` state and eliminating busy-wait CPU starvation.
* **Modular Task Registry**: Add new commands by simply dropping a `.c` file into `tasks/`. A Python pre-build script (`build_tasks.py`) automatically generates headers and links them to the kernel.
* **Advanced Job Dispatch**: Background worker threads can be configured with specific repetition counts (`-n`), periods (`-per`), priorities (`-pri`), and deadlines (`-d`).
* **Atomic Control**: Tasks can call `os_stop_scheduling()` to prevent preemption during critical sections (e.g., Wi-Fi initialization).
* **Crash Decoder**: Catches Synchronous Exceptions, Data Aborts, and Unhandled IRQs, printing faulting memory addresses and registers.
* **littleOS Task Studio**: A built-in Python/Tkinter IDE for managing, editing, and building your bare-metal tasks.

## Available CLI Commands
* `help` or `?` - List all available commands (including dynamic tasks).
* `taskinfo` - View the RTOS Task Manager (states, priorities, deadlines, turnaround times).
* `clear` - Clear the terminal screen.
* `reboot` / `restart` - Trigger a hardware watchdog reset.
* `shutdown` / `poweroff` - Send power-down signal to the SNVS block.
* `sched [rr|pri|edf]` - Change the active RTOS scheduling algorithm.
* `Ctrl+C` - Safely and forcefully kill all active background tasks.

**Task Dispatching:**
Start background tasks by typing their name followed by optional flags:
`syntax: <taskname> -n <executions> -per <period_ms> -pri <priority> -d <deadline_ms>`
*   `-n`: Number of executions (-1 for infinite). Default: 1.
*   `-per`: Period in milliseconds for repetition. Default: 0 (one-shot).
*   `-pri`: Fixed priority (0-255). Default: 128.
*   `-d`: Deadline in milliseconds relative to start. Default: -1 (none).

*Note: For remote execution via Wi-Fi, prefix commands with `exec` (e.g., `exec taskinfo`).*

## Directory Structure
```text
.
├── build/                      # Generated binaries (littleOS.bin, littleOS.elf)
├── build_tasks.py              # Pre-build registry generator
├── include/                    # OS Core Headers
│   ├── autotasks.h             # Auto-generated task registry
│   ├── cli.h                   # CLI thread definitions
│   ├── cli_utility.h           # System helpers (reboot, taskinfo)
│   ├── esp8266.h               # Wi-Fi driver
│   ├── gic.h                   # ARM GICv3 driver
│   ├── multitasking.h          # Scheduler & Threading core
│   ├── stdio.h                 # Custom print_dbg (no stdlib)
│   └── string.h                # Custom string/math helpers
├── src/                        # Kernel Source
│   ├── cli.c                   # Main input loop
│   ├── cli_utility.c           # Command parsing logic
│   ├── esp8266.c               # AT command engine & TCP listener
│   ├── gic.c / irq.c           # Interrupt handling
│   ├── multitasking.c          # Context switcher & schedulers
│   ├── main.c                  # Hardware init & kernel entry
│   ├── timer.c                 # ARM Generic Timer config (20ms)
│   └── vector.S / start.S      # Exception table & low-level entry
├── tasks/                      # User-defined RTOS tasks
│   ├── ledblink.c
│   ├── esp.c                   # Wi-Fi configuration task
│   └── race.c                  # Critical section testing task
└── littleOS_Task_Studio.py     # Python-based IDE
```

## Adding a New Task
littleOS makes it trivial to expand the OS functionality without touching the kernel core.

1.  **Create File**: Add `tasks/my_task.c`.
2.  **Define Meta**: The first line MUST be `// Task_Name : Your Display Name`.
3.  **Implement**: Create a function named `my_task_thread(void* arg)`.
4.  **Build**: Run `make` (or use Task Studio). The script will automatically link your task to the CLI.

Example:
```c
// Task_Name : Hello Task
#include "include/multitasking.h"
#include "include/stdio.h"

void hello_task_thread(void* arg) {
    print_dbg("\nHello from RTOS thread!");
    thread_sleep(1000); // Yield CPU for 1 second
}
```

## Steps to Run and Deploy

1. **Build**: `make APP=littleOS` (from root) or `make` (inside `Apps/littleOS`).
2. **Deploy**:
    *   **UMS**: Use `ums 0 mmc 1:3` in U-Boot to mount the SD card and copy `littleOS.bin`.
    *   **Serial**: Use `loady 0x80000000` in U-Boot and send via Ymodem.
3. **Execute**:
    ```u-boot
    fatload mmc 1:3 0x80000000 littleOS.bin
    dcache flush && icache flush
    go 0x80000000
    ```

### Recommended U-Boot Setup
Save these for one-click booting:
```u-boot
setenv app littleOS.bin
setenv bootcmd_baremetal 'fatload mmc 1:3 0x80000000 ${app} && dcache flush && icache flush && go 0x80000000'
saveenv
```
Now use `run bootcmd_baremetal` to launch the OS.