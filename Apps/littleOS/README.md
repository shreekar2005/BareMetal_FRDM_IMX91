# littleOS

this is my littleos bare-metal project for the nxp i.mx91 board. it features a preemptive multitasking kernel, a one-shot job dispatch system, custom hardware abstractions, and an interactive command line interface. 

the os uses the arm generic timer to time-slice threads every 20ms, but supports "atomic" execution to temporarily lock the cpu for critical tasks.

## features
* **preemptive scheduling**: round-robin time slicing using arm64 exceptions and hardware timers.
* **job dispatch model**: background worker threads (like led blinking and printing) act as one-shot jobs. when they return, they die, and the scheduler rebuilds their physical stack the next time they are dispatched.
* **atomic control**: threads can call `os_stop_scheduling()` to block hardware timer preemptions and own the cpu.
* **hardware power control**: true hard reboot using the lpwdog1 watchdog timer and physical poweroff via the snvs block.
* **custom stdio**: full `printf` implementation over lpuart1 using gcc built-in variable arguments (no stdlib required).
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