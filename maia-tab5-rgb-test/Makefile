# Standalone RGB Diagnostic Test for M5Stack Tab5
# Uses verified hardware drivers from M5Tab5 BSP (ZERO MaiaRTOS)

TAB5_PLATFORM_DIR := /Users/roberto/M5Tab5-UserDemo/platforms/tab5
IDF_PATH := /Users/roberto/esp-idf
PYTHON := /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python
PORT := /dev/cu.usbmodem1301

.PHONY: all build flash monitor

all: build

build:
	source $(IDF_PATH)/export.sh && cd $(TAB5_PLATFORM_DIR) && idf.py build
	cp $(TAB5_PLATFORM_DIR)/build/m5stack_tab5.bin ./tab5_rgb_test.bin

flash:
	source $(IDF_PATH)/export.sh && cd $(TAB5_PLATFORM_DIR) && idf.py -p $(PORT) flash

monitor:
	$(PYTHON) -c 'import serial, time; s = serial.Serial("$(PORT)", 115200, timeout=1); s.setDTR(False); s.setRTS(True); time.sleep(0.1); s.setRTS(False); time.sleep(1.5); print(s.read(4096).decode(errors="replace")); s.close()'
