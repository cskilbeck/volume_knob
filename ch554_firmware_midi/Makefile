XRAM_SIZE = 0x0400
CODE_SIZE = 0x3800
XRAM_LOC = 0x0100
FREQ_SYS = 24000000

PROJECT = midi_knob

SRC = src/
INC = include/
OBJ = obj/
BIN = bin/

SOURCES = \
	usb.c \
	main.c \
	util.c \
	config.c \
	gpio.c \
	encoder.c \
	debug.c \
	midi.c

EXTRA_FLAGS += --std-c23

include Makefile.include
