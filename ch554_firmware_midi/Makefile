XRAM_SIZE = 0x0400
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
	load_save.c \
	debug.c

#EXTRA_FLAGS += -DDE_PRINTF
EXTRA_FLAGS += --std-c23

pre-flash:

include Makefile.include
