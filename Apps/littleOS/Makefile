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
	python3 -m venv flask_webapp_esp_bridge/.venv

initflask: .venv
	flask_webapp_esp_bridge/.venv/bin/pip install -r flask_webapp_esp_bridge/requirements.txt

runflask: initflask
	flask_webapp_esp_bridge/.venv/bin/python flask_webapp_esp_bridge/app.py 

.PHONY: all pre_build clear clean usb_pendrive_install usb_serial_install usb_ums_install initflask runflask