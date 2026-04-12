APP = littleOS

all: pre_build
	$(MAKE) -C ../.. APP=$(APP)

pre_build:
	python3 build_tasks.py

clear: clean
	clear

clean:
	rm -f src/autotasks.c include/autotasks.h
	$(MAKE) -C ../.. APP=$(APP) clean

usb_pendrive_install: pre_build
	$(MAKE) -C ../.. APP=$(APP) usb_pendrive_install

usb_serial_install: pre_build
	$(MAKE) -C ../.. APP=$(APP) usb_serial_install

usb_ums_install: pre_build
	$(MAKE) -C ../.. APP=$(APP) usb_ums_install

.venv:
	python3 -m venv python_server_and_webapp/.venv

initpy: .venv
	python_server_and_webapp/.venv/bin/pip install -r python_server_and_webapp/requirements.txt

runpy: initpy
	python_server_and_webapp/.venv/bin/python python_server_and_webapp/app.py 

.PHONY: all pre_build clear clean usb_pendrive_install usb_serial_install usb_ums_install initpy runpy