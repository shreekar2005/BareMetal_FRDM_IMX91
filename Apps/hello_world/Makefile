APP = hello_world

all:
	$(MAKE) -C ../.. APP=$(APP)

clear: clean
	clear

clean:
	$(MAKE) -C ../.. APP=$(APP) clean

usb_pendrive_install:
	$(MAKE) -C ../.. APP=$(APP) usb_pendrive_install

usb_serial_install:
	$(MAKE) -C ../.. APP=$(APP) usb_serial_install

usb_ums_install:
	$(MAKE) -C ../.. APP=$(APP) usb_ums_install

.PHONY: all clear clean usb_pendrive_install usb_serial_install usb_ums_install