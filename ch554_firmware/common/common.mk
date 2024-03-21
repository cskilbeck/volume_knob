CC = sdcc
OBJCOPY = objcopy
PACK_HEX = packihx
WCHISP = wchisp

FREQ_SYS ?= 12000000
XRAM_SIZE ?= 0x0400
XRAM_LOC ?= 0x0000
CODE_SIZE ?= 0x3800

INCLUDE_FOLDERS = $(foreach incdir, $(INC), -I$(incdir))

CFLAGS := -mmcs51 \
		--model-small \
		--xram-size $(XRAM_SIZE) \
		--xram-loc $(XRAM_LOC) \
		--code-size $(CODE_SIZE) \
		$(INCLUDE_FOLDERS)\
		-o$(OBJ) \
		-DFREQ_SYS=$(FREQ_SYS) \
		$(EXTRA_FLAGS)

LFLAGS := $(CFLAGS)
TARGET := $(BIN)$(PROJECT)

SOURCES := $(foreach dir, $(SRC), $(wildcard $(dir)*.c))

SOURCE_FILENAMES := $(foreach src, $(SOURCES), $(notdir $(src)))

DEP_FILES := $(SOURCE_FILENAMES:%.c=$(OBJ)%.d)
REL_FILES := $(SOURCE_FILENAMES:%.c=$(OBJ)%.rel)

VPATH = $(SRC)

$(OBJ)%.rel : %.c
	@echo Compile $(notdir $<)
	@$(CC) -c $(CFLAGS) -Wp,-MMD,-MT$@,-MF$(OBJ)$*.d $<

$(REL_FILES): $(MAKEFILE_LIST) | $(OBJ)

$(TARGET).bin: $(REL_FILES) $(SOURCES) | $(BIN)
	@echo Link
	@$(CC) $(REL_FILES) $(LFLAGS) -o $(TARGET).ihx
	@$(OBJCOPY) -I ihex -O binary $(TARGET).ihx $(TARGET).bin

clean:
	rm -f $(BIN)* $(OBJ)*

-include $(DEP_FILES)

.DEFAULT_GOAL := build

flash: $(TARGET).bin
	$(WCHISP) flash $(TARGET).bin

build: $(TARGET).bin
	@echo $(notdir $<) built

print-% : ; @echo $* = $($*)

$(OBJ):
	mkdir -p $@

$(BIN):
	mkdir -p $@