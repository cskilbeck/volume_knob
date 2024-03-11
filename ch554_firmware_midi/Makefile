XRAM_SIZE = 0x0400
CODE_SIZE = 0x3800
XRAM_LOC = 0x0100
FREQ_SYS = 24000000

PROJECT = midi_knob

SRC = src/
INC = include/
OBJ = obj/
BIN = bin/

DEVICE_ORIGINAL = 0
DEVICE_DIRECT = 1
DEVICE_DEVKIT = 2

DEVICE_TYPE ?= $(DEVICE_DEVKIT)

EXTRA_FLAGS += --std-c23 -DDEVICE=$(DEVICE_TYPE)

.PHONY build:
	@echo Device type $(DEVICE_TYPE) is up to date

include Makefile.include
