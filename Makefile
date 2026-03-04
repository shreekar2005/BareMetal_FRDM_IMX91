# Cross-compiler definitions
CC = aarch64-linux-gnu-gcc
LD = aarch64-linux-gnu-ld
OBJCOPY = aarch64-linux-gnu-objcopy

# Skip "APP check" for "make help"
ifeq ($(filter help,$(MAKECMDGOALS)),)
ifndef APP
$(error ERROR: You must provide an APP folder name. Example: make APP=directory_name)
endif
endif

BUILD_DIR = $(APP)/build
TARGET = $(BUILD_DIR)/$(APP).bin
ELF = $(BUILD_DIR)/$(APP).elf

CFLAGS = -g -c -nostdlib -ffreestanding -O0 -Iinclude -I$(APP)/include
LDFLAGS = -T linker.ld

APP_SRCS = $(wildcard $(APP)/src/*.c)
APP_ASMS = $(wildcard $(APP)/src/*.S)
LIB_SRCS = $(wildcard lib/*.c)

APP_OBJS = $(patsubst $(APP)/src/%.c, $(BUILD_DIR)/%.o, $(APP_SRCS))
APP_OBJS += $(patsubst $(APP)/src/%.S, $(BUILD_DIR)/%.o, $(APP_ASMS))
LIB_OBJS = $(patsubst lib/%.c, $(BUILD_DIR)/lib_%.o, $(LIB_SRCS))

USB_DRIVE ?= /media/$(USER)/FRDM_IMX91

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(APP)/src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(APP)/src/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/lib_%.o: lib/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(ELF): $(APP_OBJS) $(LIB_OBJS)
	$(LD) $(LDFLAGS) $(filter %start.o, $(APP_OBJS)) $(filter-out %start.o, $(APP_OBJS)) $(LIB_OBJS) -o $@

$(TARGET): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "\n>>> Successfully built $@ <<<\n"

init:
	@mkdir -p $(APP)/src $(APP)/include
	@echo "Creating project: $(APP)..."
	
	@# Create main.c if it doesn't exist
	@if [ ! -f $(APP)/src/main.c ]; then \
		echo "$$MAIN_C_TEMPLATE" > $(APP)/src/main.c; \
		echo "  + Created $(APP)/src/main.c"; \
	fi
	
	@# Create the child Makefile if it doesn't exist
	@if [ ! -f $(APP)/Makefile ]; then \
		echo "$$CHILD_MAKEFILE_TEMPLATE" > $(APP)/Makefile; \
		echo "  + Created $(APP)/Makefile"; \
	fi
	
	@echo ">>> Project $(APP) initialized. You can now: cd $(APP) && make"

clear: clean
	clear

clean:
	rm -rf $(BUILD_DIR)
	@echo "\n>>> Cleaned $(APP) <<<\n"

usb: $(TARGET)
	@if [ -d "$(USB_DRIVE)" ]; then \
		cp $(TARGET) $(USB_DRIVE)/; \
		sync; \
		echo "\n>>> Successfully copied to $(USB_DRIVE) <<<\n"; \
	else \
		echo "\n>>> ERROR: USB drive not found at $(USB_DRIVE). Pass USB_DRIVE=/your/path <<<\n"; \
	fi

uboot_cmds:
	@echo "======================================================================"
	@echo "        U-BOOT COMMANDS FOR APP: $(APP)"
	@echo "======================================================================"
	@echo "usb start"
	@echo "fatload usb 0:1 0x80000000 $(APP).bin"
	@echo "dcache flush"
	@echo "icache flush"
	@echo "go 0x80000000"
	@echo "======================================================================"

help:
	@echo "======================================================================"
	@echo "              NXP FRDM-i.MX91 Bare-Metal Build System                 "
	@echo "======================================================================"
	@echo "Project Management:"
	@echo "  make init APP=<name>          - Create a new project directory structure"
	@echo ""
	@echo "Building & Cleaning:"
	@echo "  make APP=<name>               - Build the application binary (.bin & .elf)"
	@echo "  make clean APP=<name>         - Remove the build directory for the app"
	@echo "  make clear APP=<name>         - Clean the app and clear the terminal"
	@echo ""
	@echo "Deployment & Interaction:"
	@echo "  make usb APP=<name>           - Build and copy binary to $(USB_DRIVE)"
	@echo "  make uboot_cmds APP=<name>    - Show commands to run in the U-Boot console"
	@echo ""
	@echo "General:"
	@echo "  make help                     - Show this help message"
	@echo "======================================================================"
	@echo "Tip: Once you run 'make init', you can just 'cd' into the project folder"
	@echo "     and run 'make' or 'make usb' without typing APP=<name> every time."
	@echo "======================================================================"

.PHONY: all clear clean usb help uboot_cmds init


# --- TEMPLATE DEFINITIONS ---

# This defines the content for the new project's main.c
define MAIN_C_TEMPLATE
#include <stdint.h>
#include "nxp_frdm_imx91.h"
#include "uart.h"

/**
 * @brief Main entry point for $(APP)
 */
int main() {
    /* Initialize your hardware here */
    
    uart_print_string(LPUART1, "Hello from $(APP) bare-metal!\\n");

    while(1) {
        /* Application Loop */
    }
    return 0;
}
endef

# This defines the content for the child Makefile
define CHILD_MAKEFILE_TEMPLATE
APP = $(APP)
USB_DRIVE = $(USB_DRIVE)

# Default target: go up to parent and build this app
all:
	$$(MAKE) -C .. APP=$$(APP)

# Clean this specific app's build
clean:
	$$(MAKE) -C .. APP=$$(APP) clean

# Copy this app's binary to USB
usb:
	$$(MAKE) -C .. APP=$$(APP) USB_DRIVE=$$(USB_DRIVE) usb

# Show U-Boot commands for this app
run:
	$$(MAKE) -C .. APP=$$(APP) uboot_cmds

.PHONY: all clean usb run
endef

export MAIN_C_TEMPLATE
export CHILD_MAKEFILE_TEMPLATE