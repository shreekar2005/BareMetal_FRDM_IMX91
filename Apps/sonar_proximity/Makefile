APP = sonar_proximity

all:
	$(MAKE) -C ../.. APP=$(APP)

clear: clean
	clear

clean:
	$(MAKE) -C ../.. APP=$(APP) clean

usb_install:
	$(MAKE) -C ../.. APP=$(APP) usb_install

serial_install_steps:
	$(MAKE) -C ../.. APP=$(APP) serial_install_steps

cmd:
	$(MAKE) -C ../.. APP=$(APP) cmd

.PHONY: all clear clean usb_install serial_install_steps cmd