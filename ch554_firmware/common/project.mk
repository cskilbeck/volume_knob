# HID Makefile

XRAM_SIZE = 0x0400
CODE_SIZE = 0x3800
XRAM_LOC = 0x0100
FREQ_SYS = 24000000

# pass in DEVICE=TYPE
#
# where type is one of:
#
# 	ORIGINAL
# 	DIRECT
# 	DEVKIT

EXTRA_FLAGS += --std-c23 --opt-code-speed -DDEVICE=DEVICE_$(DEVICE_TYPE)

PROJECT = $(PROJECT_NAME)_$(DEVICE_TYPE)

SRC = src/ $(COMMON)src/ 
INC = include/ $(COMMON)include

OBJ = obj/$(DEVICE_TYPE)/
BIN = bin/$(DEVICE_TYPE)/

.PHONY: check-env

build: check-env

flash: check-env

clean: check-env

check-env:
ifndef DEVICE_TYPE
	$(error DEVICE_TYPE is undefined)
endif

include $(COMMON)common.mk
