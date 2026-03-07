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

# --- Route everything through the Apps directory ---
APP_DIR = Apps/$(APP)

BUILD_DIR = $(APP_DIR)/build
TARGET = $(BUILD_DIR)/$(APP).bin
ELF = $(BUILD_DIR)/$(APP).elf

CFLAGS = -g -c -nostdlib -ffreestanding -O0 -Iinclude -I$(APP_DIR)/include
LDFLAGS = -T $(APP_DIR)/linker.ld

APP_SRCS = $(wildcard $(APP_DIR)/src/*.c)
APP_ASMS = $(wildcard $(APP_DIR)/src/*.S)
LIB_SRCS = $(wildcard lib/*.c)

APP_OBJS = $(patsubst $(APP_DIR)/src/%.c, $(BUILD_DIR)/%.o, $(APP_SRCS))
APP_OBJS += $(patsubst $(APP_DIR)/src/%.S, $(BUILD_DIR)/%.o, $(APP_ASMS))
LIB_OBJS = $(patsubst lib/%.c, $(BUILD_DIR)/lib_%.o, $(LIB_SRCS))

USB_DRIVE ?= /media/$(USER)/FRDM_IMX91
SD_USB_DRIVE ?= /media/$(USER)/BM_PROGRAMS

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(APP_DIR)/src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(APP_DIR)/src/%.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/lib_%.o: lib/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(ELF): $(APP_OBJS) $(LIB_OBJS)
	$(LD) $(LDFLAGS) $(filter %start.o, $(APP_OBJS)) $(filter-out %start.o, $(APP_OBJS)) $(LIB_OBJS) -o $@

$(TARGET): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "\n>>> Successfully built $@ <<<\n"

init:
	@mkdir -p $(APP_DIR)/src $(APP_DIR)/include
	@echo "Creating/Verifying project: $(APP) inside Apps/..."
	
	@if [ ! -f $(APP_DIR)/src/main.c ]; then \
		sed 's/__APP_NAME__/$(APP)/g' templates/template_main.c > $(APP_DIR)/src/main.c; \
		echo "  + Created $(APP_DIR)/src/main.c"; \
	else \
		echo "  ~ Skipped $(APP_DIR)/src/main.c (already exists)"; \
	fi

	@if [ ! -f $(APP_DIR)/src/start.S ]; then \
		cp templates/template_start.S $(APP_DIR)/src/start.S; \
		echo "  + Created $(APP_DIR)/src/start.S"; \
	else \
		echo "  ~ Skipped $(APP_DIR)/src/start.S (already exists)"; \
	fi
	
	@if [ ! -f $(APP_DIR)/linker.ld ]; then \
		cp templates/template_linker.ld $(APP_DIR)/linker.ld; \
		echo "  + Created $(APP_DIR)/linker.ld"; \
	else \
		echo "  ~ Skipped $(APP_DIR)/linker.ld (already exists)"; \
	fi
	
	@if [ ! -f $(APP_DIR)/Makefile ]; then \
		sed 's/__APP_NAME__/$(APP)/g' templates/template_Makefile > $(APP_DIR)/Makefile; \
		echo "  + Created $(APP_DIR)/Makefile"; \
	else \
		echo "  ~ Skipped $(APP_DIR)/Makefile (already exists)"; \
	fi
	
	@echo ">>> Project $(APP) initialized. You can now: cd $(APP_DIR) && make"

clear: clean
	clear

clean:
	rm -rf $(BUILD_DIR)
	@echo "\n>>> Cleaned $(APP) <<<\n"

usb_pendrive_install: $(TARGET)
	@if [ -d "$(USB_DRIVE)" ]; then \
		cp $(TARGET) $(USB_DRIVE)/; \
		sync; \
		echo "\n>>> Successfully copied to $(USB_DRIVE) <<<\n"; \
	else \
		echo "\n>>> ERROR: USB drive not found at $(USB_DRIVE). Pass USB_DRIVE=/your/path <<<\n"; \
		exit 1; \
	fi
	@echo "======================================================================"
	@echo "             USB PENDRIVE DEPLOYMENT FOR $(APP)"
	@echo "======================================================================"
	@echo "  --- U-BOOT COMMANDS ---"
	@echo "  1. Connect your USB pendrive to the board's USB Host port."
	@echo "  2. Load & Run: usb reset && fatload usb 0:1 0x80000000 $(APP).bin && dcache flush && icache flush && go 0x80000000"
	@echo "======================================================================"

usb_serial_install: $(TARGET)
	@echo "======================================================================"
	@echo "               YMODEM SERIAL DEPLOYMENT FOR $(APP)"
	@echo "======================================================================"
	@echo "  --- U-BOOT COMMANDS & STEPS ---"
	@echo "  1. Start Transfer: loady 0x80000000"
	@echo "  2. In picocom, press Ctrl+A, then Ctrl+S."
	@echo "  3.1. Paste path: build/$(APP).bin"
	@echo "  3.2. Paste path: $(TARGET)"
	@echo "  4. Load & Run: dcache flush && icache flush && go 0x80000000"
	@echo "======================================================================"

usb_ums_install: $(TARGET)
	@echo "======================================================================"
	@echo "                 UMS DEPLOYMENT FOR $(APP)"
	@echo "======================================================================"
	@echo "  --- FOR SD CARD (U-BOOT COMMANDS) ---"
	@echo "  1. Start UMS from U-Boot : 'ums 0 mmc 1:3'"
	@echo "  2.1. 'cp build/$(APP).bin $(SD_USB_DRIVE)/;' "
	@echo "  2.2. 'cp $(TARGET) $(SD_USB_DRIVE)/;' "
	@echo "  3. Load & Run: fatload mmc 1:3 0x80000000 $(APP).bin && dcache flush && icache flush && go 0x80000000"
	@echo ""
	@echo "  --- FOR eMMC (U-BOOT COMMANDS) ---"
	@echo "  1. Start UMS: ums 0 mmc 0"
	@echo "  2. Copy $(APP).bin to the 'boot' drive on PC, then eject and Ctrl+C"
	@echo "  3. Load & Run: fatload mmc 0:1 0x80000000 $(APP).bin && dcache flush && icache flush && go 0x80000000"
	@echo "======================================================================"

help:
	@echo "======================================================================"
	@echo "              NXP FRDM-i.MX91 Bare-Metal Build System                 "
	@echo "======================================================================"
	@echo "Project Management:"
	@echo "  make init APP=<name>                 - Create a new project inside Apps/"
	@echo ""
	@echo "Building & Cleaning:"
	@echo "  make APP=<name>                      - Build the application binary (.bin & .elf)"
	@echo "  make clean APP=<name>                - Remove the build directory for the app"
	@echo "  make clear APP=<name>                - Clean the app and clear the terminal"
	@echo ""
	@echo "Deployment & Interaction:"
	@echo "  make usb_pendrive_install APP=<name> - Copy binary to USB and show U-Boot command"
	@echo "  make usb_serial_install APP=<name>   - Show instructions & path for Ymodem transfer"
	@echo "  make usb_ums_install APP=<name>      - Show UMS (eMMC/SD Card) commands"
	@echo "======================================================================"

.PHONY: all clear clean usb_pendrive_install usb_serial_install usb_ums_install help init