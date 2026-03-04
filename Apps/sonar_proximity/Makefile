APP = sonar_proximity

all:
	$(MAKE) -C ../.. APP=$(APP)

clear: clean
	clear

clean:
	$(MAKE) -C ../.. APP=$(APP) clean

usb:
	$(MAKE) -C ../.. APP=$(APP) usb

cmd:
	$(MAKE) -C ../.. APP=$(APP) cmd

.PHONY: all clear clean usb cmd