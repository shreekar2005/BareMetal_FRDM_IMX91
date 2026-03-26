# littleOS

this is my littleos bare-metal project for the nxp i.mx91 board. it features a preemptive multitasking kernel, a one-shot job dispatch system, custom hardware abstractions, and an interactive command line interface. 

the os uses the arm generic timer to time-slice threads every 20ms, but supports "atomic" execution to temporarily lock the cpu for critical tasks.

## features
* **preemptive scheduling**: round-robin time slicing using arm64 exceptions and hardware timers.
* **job dispatch model**: background worker threads (like led blinking and printing) act as one-shot jobs. when they return, they die, and the scheduler rebuilds their physical stack the next time they are dispatched.
* **atomic control**: threads can call `os_stop_scheduling()` to block hardware timer preemptions and own the cpu.
* **hardware power control**: true hard reboot using the lpwdog1 watchdog timer and physical poweroff via the snvs block.
* **custom stdio**: full `printf` implementation over lpuart1 using gcc built-in variable arguments (no stdlib required). I have intentionally kept printf non atomic, so we can feel preemption e.g. thread changed when priting was happening.
* **crash decoder**: catches synchronous exceptions, unhandled irqs, and data aborts, printing the exact faulting memory addresses.

## available cli commands
* `help` or `?` - list available commands
* `reboot` - trigger a hardware watchdog reset
* `shutdown` - send power-down signal to the pmic
* `ledblink` - dispatch a background job to blink the hardware led twice
* `print "text" n` - dispatch a background job to print text n times (time-sliced)
* `printa "text" n` - dispatch an atomic print job (locks the cpu until finished)
* `Ctrl+C` - safely cancel active background print jobs

## directory structure
* **include/**: contains all header files with doxygen style comments.
* **src/**: contains the c and assembly source code.

## file details
* **cli.c / cli.h**: runs the main terminal thread. parses input, handles backspaces/ctrl+c, and dispatches one-shot jobs to worker threads.
* **cli_utility.c / cli_utility.h**: contains hardware abstractions specifically for the cli, like triggering the watchdog and snvs power control.
* **gic.c / gic.h**: driver for the arm generic interrupt controller. routes hardware signals to the cpu.
* **irq.c**: central interrupt dispatcher. reads the fired hardware id from the gic and jumps to the correct driver.
* **main.c**: bootloader and kernel init. sets up threads, initializes the scheduler, holds the crash logger, and defines the one-shot worker thread logic (`led_blink_thread`, `print_thread`, `atomic_print_thread`).
* **multitasking.c / multitasking.h**: the core scheduler. handles posix style thread creation, yielding, context switching, stack-rebuilding for dead threads, and toggling atomic execution.
* **stdio.c / stdio.h**: custom printf implementation for serial output.
* **string.c / string.h**: custom string utilities (strcmp, strncmp, atoi) for parsing terminal input without standard libraries.
* **timer.c**: configures the arm generic timer to fire an interrupt every 20ms to drive the time slicer.
* **start.S**: early boot assembly to clear the `.bss` segment before jumping into c code.
* **vector.S**: the arm64 exception vector table. handles pushing and popping the 31 physical registers during context switches and hardware faults.

---

## steps to add a new task to the cli

to add a new background command to the operating system, you just need to wire up a new thread and link it to the cli parser. 

**in `src/main.c`:**
create your one-shot worker function and declare a global id for it.
```c
int my_new_thread_id;

void my_new_thread(void* arg) {
    // do work here
    printf("\r\n[MYCMD] task finished!\r\n> ");
    // returning kills the thread until the cli wakes it up again
}
```

inside `main()`, register it with the scheduler before calling `os_start()`:
```c
my_new_thread_id = os_create_thread(my_new_thread, NULL);
```

**in `include/cli.h`:**
export the id so the terminal can see it.
```c
extern int my_new_thread_id;
```

**in `src/cli.c` (update parser):**
add your command trigger inside the `input_thread` parser loop.
```c
else if (my_strcmp(cmd, "mycmd") == 0) {
    os_thread_start(my_new_thread_id); 
    printf("\r\n[System] my_task job dispatched.");
}
```

**in `src/cli.c` (update help menu):**
don't forget to add your new command to the help text so users know it exists!
```c
else if (my_strcmp(cmd, "help") == 0 || my_strcmp(cmd, "?") == 0) {
    // ... existing prints ...
    printf("  mycmd             - does my_task\r\n");
}
```

---

## steps to run and deploy

this project compiles down to a raw binary that is loaded directly into ram by u-boot.

**build the os:**
```bash
make clear
make
```

**deploy via u-boot mass storage:**
drop into your u-boot serial console and mount the emmc to your laptop via usb.
```text
u-boot=> ums 0 mmc 1:3
```
the board will pop up as a usb drive on your ubuntu machine. copy `littleOS.bin` to this drive, safely eject it in ubuntu, and hit `Ctrl+C` in the u-boot console to stop the ums server.

**boot the os:**
```text
u-boot=> boot
```

### u-boot environment setup
*NOTE: you only need to do this setup once! u-boot saves these variables to non-volatile memory. if your board is already configured, skip straight to the deployment steps above.*

if your board doesn't boot automatically, ensure these environment variables are set in u-boot. this configures the fatload command to push our binary exactly to `0x80000000`.

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