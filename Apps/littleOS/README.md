# littleOS

this is my littleos bare-metal project for the nxp i.mx91 board. it features a preemptive multitasking rtos kernel, a periodic/one-shot job dispatch system, multiple scheduling algorithms, custom hardware abstractions, and an interactive command line interface that can now be accessed remotely over Wi-Fi. 

the os uses the arm generic timer to time-slice threads every 20ms, supports true voluntary blocking/sleeping, and allows "atomic" execution to temporarily lock the cpu for critical tasks.

## features
* **preemptive & dynamic scheduling**: swap between round-robin (rr), fixed priority (pri), and earliest deadline first (edf) algorithms on the fly.
* **wi-fi & tcp networking**: full hardware abstraction layer for the esp8266 module. supports station and access point modes, tcp server hosting, and bidirectional over-the-air cli execution via `nc` (netcat). includes a custom `printesp` with variadic argument formatting tailored for network payloads.
* **true rtos blocking**: threads can voluntarily yield the cpu using `os_sleep_ms()`, completely eliminating busy-wait cpu starvation.
* **modular task registry**: add new os commands simply by dropping a `.c` file into the `tasks/` directory. a python pre-build script automatically generates the c headers, task arrays, and links them to the cli parser.
* **advanced job dispatch**: background worker threads can be configured with specific execution targets, repetition periods, priorities, and deadlines via terminal flags.
* **atomic control**: threads can call `os_stop_scheduling()` to block hardware timer preemptions and own the cpu. the os protects itself by auto-downgrading sleep requests to busy-waits during atomic blocks. it also uses hard irq masking (`msr daifset, #2`) for critical hardware rx sections to prevent uart buffer overruns.
* **rtos task manager**: real-time profiling of thread states, completion targets, and turnaround times via the `stat` command.
* **hardware power control**: true hard reboot using the lpwdog1 watchdog timer and physical poweroff via the snvs block.
* **custom stdio**: full `printdbg` implementation over lpuart1 using gcc built-in variable arguments (no stdlib required). I have intentionally kept printdbg non-atomic, so we can feel preemption (e.g., thread changed when printing was happening).
* **crash decoder**: catches synchronous exceptions, unhandled irqs, and data aborts, printing the exact faulting memory addresses.

## available cli commands
* `help` or `?` - list available commands
* `stat` - view the rtos task manager and thread turnaround times
* `clear` - clear the terminal screen
* `reboot` - trigger a hardware watchdog reset
* `shutdown` - send power-down signal to the pmic
* `sched [rr|pri|edf]` - change rtos scheduler algorithm
* `Ctrl+C` - safely and forcefully kill all active background tasks

**task dispatching:**
you can start background tasks by typing their name, optionally followed by rtos parameter flags.
syntax: `<taskname> -n <executions> -per <period_ms> -pri <priority> -d <deadline_ms>`
*(defaults: -n 1, -per 0, -pri 128, -d -1)*
*(note: when executing via remote tcp/wi-fi, prefix your command with `exec `, e.g., `exec stat`)*

## directory structure
```text
.
├── build/
├── build_tasks.py
├── ESP8266_PinDiagram.png
├── ESP8266_README.md
├── FRDMboard20x2Pins.png
├── include/
│   ├── autotasks.h
│   ├── cli.h
│   ├── cli_utility.h
│   ├── esp8266.h
│   ├── gic.h
│   ├── multitasking.h
│   ├── stdio.h
│   └── string.h
├── linker.ld
├── littleOS_Task_Studio.py
├── Makefile
├── README.md
├── src/
│   ├── cli.c
│   ├── cli_utility.c
│   ├── esp8266.c
│   ├── gic.c
│   ├── irq.c
│   ├── main.c
│   ├── multitasking.c
│   ├── start.S
│   ├── stdio.c
│   ├── string.c
│   ├── timer.c
│   └── vector.S
└── tasks/
    ├── aprint100A.c
    ├── echo.c
    ├── echoesp.c
    ├── ledblink.c
    ├── print100o.c
    ├── print100X.c
    └── race.c
```
* **include/**: contains all core os header files with doxygen style comments.
* **src/**: contains the core kernel c and assembly source code.
* **tasks/**: drop `.c` files here to automatically generate new terminal commands!

## file details
* **cli.c / cli.h**: runs the main terminal thread. handles backspaces/ctrl+c, and dynamically dispatches tasks by reading the auto-generated registry.
* **build_tasks.py**: pre-build script that scans the `tasks/` directory and writes `src/autotasks.c` to link your custom tasks to the kernel.
* **cli_utility.c / cli_utility.h**: hardware abstractions for the cli (watchdog, power control, rtos table generation). also houses the `wifi_listener_forCLI_thread` which safely captures raw network packets in an interrupt-masked critical section.
* **esp8266.c / esp8266.h**: wi-fi network driver. implements at-command parsing, hardware uart overrun protection, and acts as a transparent network socket to pass remote `nc` (netcat) commands directly into the os task dispatcher. includes `printesp()` for wireless data transmission.
* **gic.c / gic.h**: driver for the arm generic interrupt controller.
* **irq.c**: central interrupt dispatcher. reads the fired hardware id and jumps to the correct driver.
* **main.c**: bootloader and kernel init. sets up threads, initializes hardware (including wi-fi config), and starts the scheduler.
* **multitasking.c / multitasking.h**: the core scheduler. handles context switching, true sleeping/blocking, graveyard revival, and toggling atomic execution.
* **timer.c**: configures the arm generic timer to fire an interrupt every 20ms to drive the time slicer.
* **start.S**: early boot assembly to clear `.bss` before jumping into c code.
* **vector.S**: the arm64 exception vector table. pushes/pops physical registers during context switches.
* **littleOS_Task_Studio.py**: cross-platform visual IDE built with python for managing, editing, and compiling tasks.

---

## steps to add a new task to the cli

because littleos uses a python-powered modular task registry, adding a new command is incredibly easy. you **do not** need to touch `cli.c`, `cli.h`, or the core scheduler!

### step 1: create your task file
create a new `.c` file inside the `tasks/` directory. the name of the file will automatically become the command you type in the terminal.
*example: `tasks/mycmd.c`*

```c
// Task_Name : My Custom Task
#include "include/multitasking.h"
#include "include/stdio.h"

// the function name MUST be the filename + "_thread"
void mycmd_thread(void* arg) {
    printdbg("\r\n[MYCMD] doing work...");
    os_sleep_ms(500); // use os_sleep_ms to yield the cpu!
    printdbg("\r\n[MYCMD] finished!");
}
```
*note: the `// Task_Name :` comment on the very first line is required. the python script reads this to name your task in the `stat` menu!*

### step 2: hardware initialization (if needed)
hardware configuration (like setting gpio pin directions) belongs to the os, not the individual tasks! if your task requires a physical silicon pin to be configured, add it to the `hardware_init()` function inside `src/main.c`.

```c
// inside src/main.c
void hardware_init(void) {
    // LED output mode
    GPIO2->PDDR |= (1 << 4);
    
    // add your custom hardware config here!
}
```

### step 3: build!
just run `make clear && make` (or use `littleOS_Task_Studio.py`). the python script will automatically find your file, generate an id, link it to the scheduler, add it to the `help` menu, and map it to the `mycmd` terminal string!

---

## steps to run and deploy

this project compiles down to a raw binary that is loaded directly into ram by u-boot.

**build the os:**
```bash
make clear
make
```

**deploy via u-boot mass storage:**
drop into your u-boot serial console and mount the emmc to your laptop via usb. (command may vary)
#### Note: This ums command may be different for you, plase read more about ums command.
```text
u-boot=> ums 0 mmc 1:3
```
the board will pop up as a usb drive on your linux machine. copy `littleOS.bin` to this drive, safely eject it, and hit `Ctrl+C` in the u-boot console to stop the ums server.

**boot the os:**
```text
u-boot=> boot
```

### u-boot environment setup
*NOTE: you only need to do this setup once! u-boot saves these variables to non-volatile memory. if your board is already configured, skip straight to the deployment steps above.*

if your board doesn't boot automatically, ensure these environment variables are set in u-boot. this configures the fatload command to push our binary exactly to `0x80000000`.
#### Note: "mmc 1:3" this may vary, please consider reading more about fatload command
```text
u-boot=> setenv app littleOS.bin
u-boot=> setenv bootcmd_baremetal 'fatload mmc 1:3 0x80000000 ${app} && dcache flush && icache flush && go 0x80000000'
u-boot=> setenv bootcmd_linux 'run sr_ir_v2_cmd;run distro_bootcmd;run bsp_bootcmd'
```

**to set default boot behaviors:**

if you want to auto-boot into the bare-metal os on power up:
```text
u-boot=> setenv bootcmd 'run bootcmd_baremetal'
u-boot=> saveenv
```

if you want to switch back to auto-booting linux:
```text
u-boot=> setenv bootcmd 'run bootcmd_linux'
u-boot=> saveenv
```

if you just want to run one of them manually once without changing the default:
```text
u-boot=> run bootcmd_baremetal
// or
u-boot=> run bootcmd_linux
```