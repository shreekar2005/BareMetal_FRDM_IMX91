APP = littleOS
PYTHON_SERVER_AND_WEBAPP_DIR = webapp_and_server

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

setup_venv:
	@if [ ! -d "$(PYTHON_SERVER_AND_WEBAPP_DIR)/.venv" ]; then \
		echo "Creating virtual environment..."; \
		python3 -m venv $(PYTHON_SERVER_AND_WEBAPP_DIR)/.venv; \
	else \
		echo "Virtual environment already exists. Skipping."; \
	fi

initpy: setup_venv
	$(PYTHON_SERVER_AND_WEBAPP_DIR)/.venv/bin/pip install -r $(PYTHON_SERVER_AND_WEBAPP_DIR)/requirements.txt

runpy:
	$(PYTHON_SERVER_AND_WEBAPP_DIR)/.venv/bin/python $(PYTHON_SERVER_AND_WEBAPP_DIR)/app.py 

.PHONY: all pre_build clear clean usb_pendrive_install usb_serial_install usb_ums_install setup_venv initpy runpy