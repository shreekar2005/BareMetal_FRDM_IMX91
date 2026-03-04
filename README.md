# Bare-Metal Framework for FRDM-i.MX91

this project provides a **minimal freestanding bare-metal build system** for the NXP FRDM-i.MX91 board.
it allows you to:

* write pure C / assembly (no libc, no OS)
* link with a custom `linker.ld`
* generate `.elf` and raw `.bin`
* load and execute via U-Boot from USB
* manage multiple independent applications inside one framework

---

## directory structure

```text
.
├── blinking_led/                 # example bare-metal app
│   ├── build/                    # generated objects & binaries
│   │   ├── blinking_led.bin
│   │   ├── blinking_led.elf
│   │   ├── lib_uart.o
│   │   ├── main.o
│   │   └── start.o
│   ├── include/                  # app-specific headers
│   └── src/
│       ├── main.c                # application logic
│       └── start.S               # reset vector / low-level entry
│
├── include/                      # global headers
│   ├── nxp_frdm_imx91.h          # register mappings
│   └── uart.h                    # uart interface
│
├── lib/
│   └── uart.c                    # uart implementation
│
├── linker.ld                     # memory layout
├── Makefile                      # top-level build system
├── LICENSE
└── README.md
```

note: the `build/` directory is generated automatically.

---

## toolchain requirements

install the aarch64 cross compiler on ubuntu:

```bash
sudo apt update
sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
```

verify:

```bash
aarch64-linux-gnu-gcc --version
```

---

## build system overview

* **cross compiler**: `aarch64-linux-gnu-gcc`
* **freestanding mode**: `-nostdlib -ffreestanding`
* **custom linker script**: `linker.ld`
* outputs:

  * `.elf` → debugging / symbol analysis
  * `.bin` → raw binary for u-boot execution

multiple apps are supported using:

```bash
make APP=<app_name>
```

---

## creating a new project

initialize a new application:

```bash
make init APP=my_app
```

this creates:

```
my_app/
├── src/main.c
├── include/
└── Makefile
```

after initialization:

```bash
cd my_app
make
```

you no longer need to pass `APP=` manually inside the project folder.

---

## building an existing app

from root directory:

```bash
make APP=blinking_led
```

from inside the app directory:

```bash
cd blinking_led
make
```

output:

```
blinking_led/build/blinking_led.elf
blinking_led/build/blinking_led.bin
```

---

## preparing the USB drive

1. use a USB flash drive
2. format it as **FAT32**
3. mount it (example path):

```bash
/media/<user>/FRDM_IMX91
```

default path inside Makefile:

```
USB_DRIVE ?= /media/$(USER)/FRDM_IMX91
```

if your path differs:

```bash
make usb APP=blinking_led USB_DRIVE=/media/youruser/USBNAME
```

copy binary to usb:

```bash
make usb APP=blinking_led
```

---

## stopping at U-Boot

1. power the board
2. press any key when prompted to stop autoboot

you should see the U-Boot prompt:

```
=> 
```

---

## loading and running binary in U-Boot

use the following commands:

```
usb start
fatload usb 0:1 0x80000000 blinking_led.bin
dcache flush
icache flush
go 0x80000000
```

you can print them automatically with:

```bash
make uboot_cmds APP=blinking_led
```

---

## serial console

connect micro-usb to host machine.

find device:

```bash
ls /dev/ttyACM*
```

start serial console:

```bash
sudo picocom -b 115200 /dev/ttyACM0
```

exit picocom:

```
Ctrl + A, then Ctrl + X
```

---

## how execution works

1. board powers up
2. u-boot initializes DDR and clocks
3. binary is loaded to `0x80000000`
4. `start.S` executes first
5. stack pointer is configured
6. control jumps to `main()`
7. your bare-metal code runs

no kernel. no libc. no scheduler.

---

## important files

### `linker.ld`

* defines memory regions
* sets entry point
* places `.text`, `.data`, `.bss`

### `start.S`

* reset handler
* stack initialization
* branch to `main`

### `nxp_frdm_imx91.h`

* memory-mapped register addresses
* base addresses for peripherals

### `uart.c`

* minimal uart driver
* used for debug output

---

## cleaning

remove build directory:

```bash
make clean APP=blinking_led
```

or inside app:

```bash
make clean
```

---