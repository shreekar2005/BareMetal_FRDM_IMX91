# Bare-Metal Framework for FRDM-i.MX91

this project provides a **minimal freestanding bare-metal build system** for the NXP FRDM-i.MX91 board.
it allows you to:

* write pure C / assembly (no libc, no OS)
* link with a custom per-app `linker.ld`
* generate `.elf` and raw `.bin`
* load and execute via U-Boot from USB or Serial
* manage multiple independent applications inside one framework safely

---

## directory structure
```text
.
├── Apps/                         # generated bare-metal apps
│   ├── blink_led/                # example bare-metal app
│   └── sonar_proximity/          # hc-sr04 high-precision radar app
│       ├── build/                # generated objects & binaries (ignored by git)
│       ├── include/              # app-specific headers
│       ├── linker.ld             # app-specific memory layout
│       ├── Makefile              # child build script
│       └── src/
│           ├── main.c            # application logic
│           └── start.S           # reset vector / low-level entry
├── include/                      # global hardware headers
│   ├── GPIO.h                    # 32-bit iomuxc/gpio register structs
│   ├── LPUART.h                  # serial interface structs and flags
│   └── SYS_CTR.h                 # 64-bit arm generic timer macros
├── lib/
│   ├── LPUART.c                  # blocking/non-blocking uart driver
│   └── SYS_CTR.c                 # microsecond-precision hardware delay driver
├── Manuals/                      # official nxp hardware documentation
│   ├── IMX91RM.pdf               # i.mx91 applications processor reference manual
│   └── UM12262.pdf               # frdm-imx91 board user manual
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

## reference manuals & documentation

this entire framework was built from scratch by cross-referencing the official NXP silicon and board manuals. we have included these inside the `Manuals/` directory for easy access:

* **IMX91RM.pdf**: the massive thousands-of-pages master reference for every register, memory map, and peripheral inside the i.mx91 soc.
* **UM12262.pdf**: the physical board manual detailing the pinouts, switches, and schematics for the frdm-imx91 evaluation board.

if you want to expand this framework (e.g., writing an i2c or spi driver), these two pdfs are your absolute source of truth.

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
make APP=sonar_proximity
```

from inside the app directory:
```bash
cd Apps/sonar_proximity
make
```

output:
```text
Apps/sonar_proximity/build/sonar_proximity.elf
Apps/sonar_proximity/build/sonar_proximity.bin
```

---

## deploying the code

there are three main ways to push the compiled `.bin` file to the bare-metal board:

### method 1: serial transfer (ymodem)

this method requires no usb drives and pushes the code straight down the debug cable.

1. start your serial console: `sudo picocom -b 115200 /dev/ttyUSB0 --send-cmd "sz -vv"`
2. power the board and stop autoboot to get the `=>` prompt.
3. on the board type: `loady 0x80000000`
4. press `Ctrl+A` then `Ctrl+S` in picocom to send the file.
5. type the path to your binary (or run `make usb_serial_install APP=sonar_proximity` on your host to get the exact path).
6. when finished, run: `dcache flush && icache flush && go 0x80000000`

### method 2: u-boot mass storage (ums)

this method temporarily turns the board's internal emmc into a pendrive connected to your laptop.
(you can run `make usb_ums_install APP=sonar_proximity` to see a quick reference for these commands).

1. connect a data cable to the board's usb host port.
2. at the u-boot prompt type: `ums 0 mmc 0` (or `mmc 1` depending on your emmc number).
3. the board will appear as a flash drive on your ubuntu host. drag and drop the `.bin` file into it.
4. **eject** the drive safely in ubuntu.
5. press `Ctrl+C` in u-boot to stop ums mode.
6. load from the emmc into ram: `fatload mmc 0:1 0x80000000 sonar_proximity.bin`
7. run: `dcache flush && icache flush && go 0x80000000`

### method 3: physical usb pendrive

this method uses a standard usb flash drive to move the binary from your laptop to the board.

1. insert a FAT32 formatted usb drive into your laptop.
2. run the automated install command (adjust the path if yours is different). this will copy the file and print the exact u-boot command you need:
```bash
make usb_pendrive_install APP=sonar_proximity USB_DRIVE=/media/$(USER)/FRDM_IMX91
```

3. unplug the usb drive and insert it into the FRDM board's usb host port.
4. copy/paste the output printed by the make command into u-boot:
```u-boot
usb reset && fatload usb 0:1 0x80000000 sonar_proximity.bin && dcache flush && icache flush && go 0x80000000
```

---

## serial console

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

## hardware drivers included

### `GPIO.h`

maps the massive 32-bit registers (PDOR, PSOR, PCOR, PDIR, PDDR) to simple c structures. controls physical pin direction and logic levels (handling both active-high and active-low circuits).

### `SYS_CTR.h` & `SYS_CTR.c`

bypasses software loops by tapping directly into the 64-bit arm `CNTPCT_EL0` physical timer register. provides true, zero-latency microsecond (`_us`) and millisecond (`_ms`) delay functions necessary for ultrasonic acoustics and protocol bit-banging.

### `LPUART.h` & `LPUART.c`

hijacks u-boot's pre-configured baud rate to provide serial output. includes custom `uart_print_dec` for formatting integers, and a `uart_getchar_nonblocking` function to intercept `Ctrl+C` commands mid-execution without freezing the processor.

---

## how execution works

1. board powers up.
2. u-boot initializes DDR RAM, basic clocks, and the LPUART interface.
3. binary is loaded into RAM at `0x80000000`.
4. the `go 0x80000000` command hands control to `start.S`.
5. `start.S` safely backs up U-Boot's stack pointer (`sp`) and link register (`lr`) using AAPCS64 standard `stp` instructions.
6. `start.S` zeroes out the `.bss` section to initialize global C variables.
7. control jumps to `main()`.
8. your bare-metal code runs (no OS, no scheduler).
9. when `main()` returns `0`, `start.S` restores U-Boot's stack and executes `ret`, gracefully handing control back to the U-Boot prompt.

---

## cleaning

remove build directory from root:
```bash
make clean APP=sonar_proximity
```

remove build and clear terminal:
```bash
make clear APP=sonar_proximity
```

or inside app:
```bash
make clean
```