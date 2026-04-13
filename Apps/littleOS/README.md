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
* **IoT Edge Integration**: Features a true IoT ecosystem architecture via a "Reverse Callback" TCP mechanism, allowing the bare-metal board to interact with an external Python Flask Command Hub.
* **True RTOS Blocking**: Threads can yield the CPU using `thread_sleep()`, moving to a `BLOCK` state and eliminating busy-wait CPU starvation.
* **Modular Task Registry**: Add new commands by simply dropping a `.c` file into `tasks/`. A Python pre-build script (`build_tasks.py`) automatically generates headers and links them to the kernel.
* **Advanced Job Dispatch**: Background worker threads can be configured with specific repetition counts (`-n`), periods (`-per`), priorities (`-pri`), and deadlines (`-d`).
* **Atomic Control**: Tasks can call `os_stop_scheduling()` to prevent preemption during critical sections (e.g., Wi-Fi initialization).
* **Crash Decoder**: Catches Synchronous Exceptions, Data Aborts, and Unhandled IRQs, printing faulting memory addresses and registers.
* **littleOS Task Studio**: A built-in Python/Tkinter IDE for managing, editing, and building your bare-metal tasks.

## Available CLI Commands
* `help` or `?` - List all available commands (including dynamic tasks).
* `clear` - Clear the terminal screen.
* `reboot` / `restart` - Trigger a hardware watchdog reset.
* `shutdown` / `poweroff` - Send power-down signal to the SNVS block.
* `sched [rr|pri|edf]` - Change the active RTOS scheduling algorithm.
* `Ctrl+C` - Safely and forcefully kill all active background tasks.

**Task Dispatching:**
Start background tasks by typing their name followed by optional flags:
`syntax: <taskname> -n <executions> -per <period_ms> -pri <priority> -d <deadline_ms>`
* `-n`: Number of executions (-1 for infinite). Default: 1.
* `-per`: Period in milliseconds for repetition. Default: 0 (one-shot).
* `-pri`: Fixed priority (0-255). Default: 128.
* `-d`: Deadline in milliseconds relative to start. Default: -1 (none).

*Note: For remote execution via Wi-Fi, prefix commands with `exec` (e.g., `exec ledblink`).*

## IoT Command Hub & RTC Synchronization
littleOS goes beyond basic Wi-Fi by acting as a true Edge Node, bridging bare-metal hardware with a modern web interface.

### The Flask WebApp Bridge (`python_server_and_webapp/`)
A centralized Python Command & Control Hub that runs on your laptop. It utilizes a dual-thread design:
1. **Raw TCP Listener (Port 5555)**: A background thread that listens for asynchronous hardware triggers and status payloads directly from the NXP board.
2. **Web Interface (Port 5000)**: A frontend UI featuring a hacker-style terminal and live status monitor. Any command typed in the browser is dynamically injected into the littleOS Wi-Fi listener.

### Hardware RTC & Time Sync (`datetime` task)
Because the bare-metal OS lacks an internet DNS resolver and native NTP capabilities, it uses the Flask Bridge to synchronize its Real-Time Clock (RTC) using a **Reverse Callback** pattern:
1. You run `datetime --sync <laptop_ip> 5555` on the NXP board.
2. littleOS locks the scheduler, connects as a TCP Client, sends a `GET_TIME` payload, and immediately closes the socket.
3. The Python Hub detects this trigger, formats the current real-world time, and opens a new TCP connection back to the littleOS port 8080 server.
4. Python injects `exec datetime --set hh:mm:ss dd:mm:yyyy` into the OS.
5. The `datetime` task wakes up, parses the arguments, and seamlessly updates the hardware clock.

## Directory Structure
```text
.
├── build/
├── build_tasks.py              # Pre-build registry generator
├── ESP8266_connections.md      # Hardware wiring guide
├── ESP8266_PinDiagram.png
├── python_server_and_webapp/    # IoT Command Hub
│   ├── app.py                  # Dual-thread Flask/TCP Server
│   ├── requirements.txt
│   └── templates/
│       └── index.html          # Web-based terminal GUI
├── FRDMboard20x2Pins.png
├── include/                    # OS Core Headers
│   ├── autotasks.h             # Auto-generated task registry
│   ├── cli.h                   # CLI thread definitions
│   ├── cli_utility.h           # System helpers
│   ├── esp8266.h               # Wi-Fi driver & AT engine
│   ├── gic.h                   # ARM GICv3 driver
│   ├── multitasking.h          # Scheduler & Threading core
│   ├── status.h                # Profiling and metrics
│   ├── stdio.h                 # Custom print_dbg (no stdlib)
│   └── string.h                # Custom string/math helpers
├── linker.ld
├── littleOS_Task_Studio.py     # Python-based IDE
├── Makefile
├── README.md
├── src/                        # Kernel Source
│   ├── autotasks.c             # Task registration mappings
│   ├── cli.c                   # Main input loop
│   ├── cli_utility.c           # Command parsing logic
│   ├── esp8266.c               # TCP Server/Client & Wi-Fi logic
│   ├── gic.c                   # Generic Interrupt Controller
│   ├── irq.c                   # Interrupt Handlers
│   ├── main.c                  # Hardware init & kernel entry
│   ├── multitasking.c          # Context switcher & process states
│   ├── start.S                 # Low-level boot code
│   ├── stdio.c                 # UART printing logic
│   ├── string.c                # String manipulation logic
│   ├── timer.c                 # ARM Generic Timer config
│   └── vector.S                # Exception table
├── tasks/                      # User-defined RTOS tasks
│   ├── aprint100A.c
│   ├── datetime.c              # RTC manager & Reverse Callback sync
│   ├── echo.c
│   ├── esp.c                   # Wi-Fi config and status task
│   ├── ledblink.c              
│   ├── print100o.c
│   ├── print100X.c
│   ├── race.c                  # Critical section testing task
│   └── status.c                # OS Profiler task
└── ThreadStates.png            # Process state documentation
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
    * **UMS**: Use `ums 0 mmc 1:3` in U-Boot to mount the SD card and copy `littleOS.bin`.
    * **Serial**: Use `loady 0x80000000` in U-Boot and send via Ymodem.
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