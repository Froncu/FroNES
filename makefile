# Variables
TGT_NAME := FroNES
CFG_NAME := frones

CFG_DIR := configuration
SRC_DIR := source
OUT_DIR := build
OBJ_DIR := temporary

CA65 := ca65
LD65 := ld65

#----------------------------------------------------------------

# Create the output and object directory if they don't exist yet
ifeq ($(OS),Windows_NT)
	RM = rmdir /S /Q
	MKDIR = mkdir
else
	RM = rm -rf
	MKDIR = mkdir -p
endif

$(OBJ_DIR):
	$(MKDIR) $(OBJ_DIR)

$(OUT_DIR):
	$(MKDIR) $(OUT_DIR)

#----------------------------------------------------------------

# Assemble .s files to objects
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s $(OBJ_DIR)
	$(CA65) -t nes -o $@ $<

# Link the object files into an NES file
OBJ_FILES := $(patsubst $(SRC_DIR)/%.s,$(OBJ_DIR)/%.o,$(wildcard $(SRC_DIR)/*.s))
BASE_TGT := $(OUT_DIR)/$(TGT_NAME)
TGT := $(BASE_TGT).nes
$(TGT): $(OBJ_FILES) $(OUT_DIR)
	$(LD65) -C ${CFG_DIR}/${CFG_NAME}.cfg -o $@ $(OBJ_FILES)

#----------------------------------------------------------------

# Clean target
clean:
	$(RM) $(OBJ_DIR) $(OUT_DIR)

# Default target
all: $(TGT)