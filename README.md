# Bare-Metal Framework for FRDM-i.MX91

This project provides a **minimal freestanding bare-metal build system** for the NXP FRDM-i.MX91 board. It is designed to be a starting point for developers who want to write performance-critical or low-level software without the overhead of a standard operating system.

It allows you to:
* Write pure C / Assembly (no `libc`, no OS).
* Link with a custom per-app `linker.ld`.
* Generate `.elf` and raw `.bin` binaries.
* Load and execute via U-Boot from USB, Serial (Ymodem), or eMMC/SD.
* Manage multiple independent applications inside one framework safely.
* Utilize a preemptive RTOS kernel (**littleOS**) with Wi-Fi support.

---

## Directory Structure

```text
.
├── Apps/                         # Generated bare-metal apps
│   ├── hello_world/              # Interactive LPUART & LED blink app
│   ├── littleOS/                 # Preemptive RTOS with CLI & Wi-Fi
│   └── sonar_proximity/          # HC-SR04 high-precision radar app
│       ├── build/                # Generated objects & binaries (ignored by git)
│       ├── include/              # App-specific headers
│       ├── linker.ld             # App-specific memory layout
│       ├── Makefile              # Child build script
│       └── src/                  # Application source code
├── include/                      # Global hardware headers
│   ├── GPIO.h                    # 32-bit IOMUXC/GPIO register structs
│   ├── IOMUX.h                   # Pin multiplexing macros and registers
│   ├── LPUART.h                  # Serial interface structs and flags
│   └── SYS_CTR.h                 # 64-bit ARM Generic Timer macros
├── lib/                          # Shared hardware drivers
│   ├── GPIO.c                    # GPIO pin configuration and control
│   ├── IOMUX.c                   # Pad and Mux control logic
│   ├── LPUART.c                  # Blocking/non-blocking LPUART driver
│   └── SYS_CTR.c                 # Microsecond-precision hardware delay driver
├── Manuals/                      # Official NXP hardware documentation
│   ├── IMX91RM.pdf               # i.MX91 Applications Processor Reference Manual
│   └── UM12262.pdf               # FRDM-i.MX91 Board User Manual
├── templates/                    # Automated project templates
│   ├── template_linker.ld
│   ├── template_main.c
│   ├── template_Makefile
│   └── template_start.S
├── Makefile                      # Top-level master build system
├── LICENSE
├── Read_Before_Contribute.txt    # Coding standards and naming conventions
└── README.md
```

---

## Technical Features

### Hardware Drivers Included
* **`GPIO.h/c`**: Maps 32-bit registers (PDOR, PSOR, PCOR, PDIR, PDDR) to C structures.
* **`IOMUX.h/c`**: Handles pin multiplexing and pad settings (Daisy Chain support).
* **`SYS_CTR.h/c`**: Taps into the 64-bit ARM `CNTPCT_EL0` physical timer for zero-latency delays.
* **`LPUART.h/c`**: High-performance serial driver with integer formatting and non-blocking input.

### littleOS RTOS
The repository includes a fully functional RTOS inside `Apps/littleOS/` featuring:
* Preemptive multitasking with Round-Robin, Priority, and EDF scheduling.
* Dynamic task registration (simply drop a `.c` file into `tasks/`).
* Interactive CLI over Serial and Wi-Fi (ESP8266).
* Hardware watchdog and power-off control.

---

## Toolchain Requirements

Install the `aarch64` cross-compiler on Ubuntu:

```bash
sudo apt update
sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
```

Verify:
```bash
aarch64-linux-gnu-gcc --version
```

---

## Build System Overview

* **Cross Compiler**: `aarch64-linux-gnu-gcc`
* **Freestanding Mode**: `-nostdlib -ffreestanding`
* **Custom Linker Script**: `linker.ld` (generated inside each app)
* **Outputs**:
    * `.elf` → Debugging / Symbol analysis
    * `.bin` → Raw binary for U-Boot execution

Multiple apps are managed through the root Makefile:

```bash
make APP=<app_name>
```

---

## Creating a New Project

Initialize a new application using the provided templates:

```bash
make init APP=my_app
```

This creates a self-contained project structure inside `Apps/my_app/`. You can then build directly from that directory or from the root.

---

## Deploying the Code

### Method 1: Serial Transfer (Ymodem)
1. Start serial console: `sudo picocom -b 115200 /dev/ttyUSB0 --send-cmd "sz -vv"`
2. Power board and stop at `=>` prompt.
3. Run: `loady 0x80000000`
4. Press `Ctrl+A` then `Ctrl+S` in picocom to send the `.bin` file.
5. Execute: `dcache flush && icache flush && go 0x80000000`

### Method 2: U-Boot Mass Storage (UMS)
1. Connect USB cable to the board's USB Host port.
2. At U-Boot prompt: `ums 0 mmc 1:3` (for SD card partition 3).
3. The board appears as a flash drive. Copy your `.bin` file.
4. Eject and press `Ctrl+C` in U-Boot.
5. Load and run: `fatload mmc 1:3 0x80000000 <app>.bin && dcache flush && icache flush && go 0x80000000`

### Method 3: USB Pendrive
1. Copy binary to a FAT32 USB drive: `make usb_pendrive_install APP=hello_world USB_DRIVE=/media/$(USER)/DRIVE`
2. Insert into the board.
3. In U-Boot: `usb reset && fatload usb 0:1 0x80000000 hello_world.bin && dcache flush && icache flush && go 0x80000000`

---

## Automated Execution (U-Boot Macros)

To automate the boot process, you can save a macro in U-Boot:

```u-boot
setenv app hello_world.bin
setenv baremetal 'fatload mmc 1:3 0x80000000 ${app} && dcache flush && icache flush && go 0x80000000'
saveenv
```

Now, simply type `run baremetal` to start your app. To hijack the default boot sequence:
```u-boot
setenv bootcmd 'run baremetal'
saveenv
```

---

## Contributing
Please refer to `Read_Before_Contribute.txt` for naming conventions and coding standards. Ensure all new peripheral drivers are placed in `lib/` and generic hardware headers in `include/`.