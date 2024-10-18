# HID/MIDI Makefile

# pass in DEVICE=TYPE
#
# where type is one of:
#
# 	ORIGINAL		- CH554G USB Micro-B Female
# 	DIRECT			- CH554G USB A Male
# 	DEVKIT			- Devkit
#	ORIGINAL_V2		- CH554E USB Micro-B Female

ifndef DEVICE_TYPE
$(error DEVICE_TYPE is undefined)
endif

ifeq ($(DEVICE_TYPE), DEVKIT)
DEBUG = -DDEBUG=1
else
DEBUG = -DNDEBUG=1
endif

XRAM_SIZE = 0x0400
CODE_SIZE = 0x3800
XRAM_LOC = 0x0100
FREQ_SYS = 24000000

PROJECT = $(PROJECT_NAME)_$(DEVICE_TYPE)

SRC = src/ $(COMMON)src/ 
INC = include/ $(COMMON)include
OBJ = obj/$(DEVICE_TYPE)/
BIN = bin/$(DEVICE_TYPE)/

EXTRA_FLAGS = --std-c23 --opt-code-speed -DDEVICE=DEVICE_$(DEVICE_TYPE) $(DEBUG) --disable-warning 110 --disable-warning 190

include $(COMMON)common.mk
