# Bare-Metal Framework for FRDM-i.MX91

this project provides a **minimal freestanding bare-metal build system** for the NXP FRDM-i.MX91 board.
it allows you to:

* write pure C / assembly (no libc, no OS)
* link with a custom per-app `linker.ld`
* generate `.elf` and raw `.bin`
* load and execute via U-Boot from USB
* manage multiple independent applications inside one framework safely

---

## directory structure
```text
.
├── Apps/                         # generated bare-metal apps
│   ├── blink_led/                # example bare-metal app
│   │   ├── build/                # generated objects & binaries (ignored by git)
│   │   ├── include/              # app-specific headers
│   │   ├── linker.ld             # app-specific memory layout
│   │   ├── Makefile              # child build script
│   │   └── src/
│   │       ├── main.c            # application logic
│   │       └── start.S           # reset vector / low-level entry
│   └── sonar_proximity/          # another independent app
├── include/                      # global headers
│   ├── nxp_frdm_imx91.h          # register mappings
│   └── uart.h                    # uart interface
├── lib/
│   └── uart.c                    # uart implementation
├── templates/                    # automated project templates
│   ├── template_linker.ld
│   ├── template_main.c
│   ├── template_Makefile
│   └── template_start.S
├── Makefile                      # top-level master build system
├── LICENSE
└── README.md
```

note: the `build/` directory is generated automatically inside each app when compiled.

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
* **custom linker script**: `linker.ld` (generated inside each app)
* outputs:
* `.elf` → debugging / symbol analysis
* `.bin` → raw binary for u-boot execution



multiple apps are managed through the root makefile using:
```bash
make APP=<app_name>
```

---

## creating a new project

initialize a new application. this pulls from the `templates/` folder and dynamically generates your project inside `Apps/`.
```bash
make init APP=my_app
```

this creates:
```text
Apps/my_app/
├── src/main.c
├── src/start.S
├── include/
├── linker.ld
└── Makefile
```

after initialization, you can build directly:
```bash
cd Apps/my_app
make
```

you no longer need to pass `APP=` manually inside the specific project folder.

---

## building an existing app

from root directory:
```bash
make APP=blink_led
```

from inside the app directory:
```bash
cd Apps/blink_led
make
```

output:
```text
Apps/blink_led/build/blink_led.elf
Apps/blink_led/build/blink_led.bin
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
```makefile
USB_DRIVE ?= /media/$(USER)/FRDM_IMX91
```

if your path differs:
```bash
make usb APP=blink_led USB_DRIVE=/media/youruser/USBNAME
```

copy binary to usb:
```bash
make usb APP=blink_led
```

---

## stopping at U-Boot

1. connect the **usb host** port on the board to your pendrive.
2. connect the **debug/uart** port on the board to your host machine.
3. open a serial console (e.g., `sudo picocom -b 115200 /dev/ttyUSB0`).
4. power the board.
5. press any key immediately when prompted to stop autoboot.

you should see the U-Boot prompt:
```u-boot
=> 
```

---

## loading and running binary in U-Boot

use the following commands in the U-Boot console to load the binary into DDR RAM (`0x80000000`) and execute it.

to get the exact, safe single-line command for your specific app, run this on your host machine:
```bash
make cmd APP=blink_led
```

output to copy/paste:
```u-boot
usb stop && usb start && fatload usb 0:1 0x80000000 blink_led.bin && dcache flush && icache flush && go 0x80000000
```

---

## serial console

connect micro-usb to host machine.

find device (usually `ttyUSB0` or `ttyACM0`):
```bash
ls /dev/ttyUSB*
```

start serial console:
```bash
sudo picocom -b 115200 /dev/ttyUSB0
```

exit picocom:
```text
Ctrl + A, then Ctrl + X
```

---

## how execution works

1. board powers up.
2. u-boot initializes DDR RAM, basic clocks, and the LPUART interface.
3. binary is loaded from the FAT32 USB to `0x80000000`.
4. the `go 0x80000000` command hands control to `start.S`.
5. `start.S` safely backs up U-Boot's stack pointer (`sp`) and link register (`lr`) using AAPCS64 standard `stp` instructions.
6. `start.S` zeroes out the `.bss` section to initialize global C variables.
7. control jumps to `main()`.
8. your bare-metal code runs (no OS, no scheduler).
9. when `main()` returns `0`, `start.S` restores U-Boot's stack and executes `ret`, gracefully handing control back to the U-Boot prompt.

---

## important files

### `linker.ld` (inside app)

* defines memory regions (`0x80000000`).
* forces strict 16-byte memory alignment to prevent AArch64 synchronous aborts.
* places `.text`, `.rodata`, `.data`, and `.bss`.

### `start.S` (inside app)

* reset handler.
* stack initialization and U-Boot state preservation.
* `.bss` zeroing loop.
* branch to `main`.

### `nxp_frdm_imx91.h` (global)

* memory-mapped register addresses mapped via C `struct` pointers (STM32 HAL style).
* base addresses for peripherals.

### `uart.c` (global)

* minimal LPUART driver hijacking U-Boot's pre-configured baud rate.
* provides blocking and non-blocking I/O.
* includes smart `\n` to `\r\n` conversion for proper terminal formatting.

---

## cleaning

remove build directory from root:
```bash
make clean APP=blink_led
```

remove build and clear terminal:
```bash
make clear APP=blink_led
```

or inside app:
```bash
make clean
```