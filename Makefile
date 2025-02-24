# Detect operating system
ifeq ($(OS),Windows_NT)
    detected_OS := Windows
    # Windows-specific settings
    MKDIR_CMD = mkdir -p $1
    RM_CMD = rm -rf $1
    EXE_EXT = .exe
    PATH_SEP = \\
else
    detected_OS := $(shell uname -s)
    # Unix-like settings
    MKDIR_CMD = mkdir -p $1
    RM_CMD = rm -rf $1
    EXE_EXT =
    PATH_SEP = /
endif

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g -O2

# SDL2 settings based on OS
ifeq ($(detected_OS),Windows)
    # MinGW settings
    ifdef SDL_INCLUDE_PATH
        CFLAGS += -I$(SDL_INCLUDE_PATH) -Dmain=SDL_main
    else
        CFLAGS += -I$(SDL2_DIR)/include -Dmain=SDL_main
    endif
    
    # Check for different possible lib paths
    ifneq (,$(wildcard $(SDL2_DIR)/lib/x64/SDL2.dll))
        LDFLAGS = -L$(SDL2_DIR)/lib/x64 -lmingw32 -lSDL2main -lSDL2 -mwindows
    else ifneq (,$(wildcard $(SDL2_DIR)/lib/SDL2.dll))
        LDFLAGS = -L$(SDL2_DIR)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
    else ifneq (,$(wildcard $(SDL2_DIR)/x86_64-w64-mingw32/lib/libSDL2.a))
        LDFLAGS = -L$(SDL2_DIR)/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
    else
        LDFLAGS = -L$(SDL2_DIR)/lib -lmingw32 -lSDL2main -lSDL2 -mwindows
    endif
else
    # Linux/macOS settings
    LDFLAGS = -lSDL2
endif

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
DATA_DIR = data

# Files
SRCS = $(filter-out $(SRC_DIR)/test_rom.c, $(wildcard $(SRC_DIR)/*.c))
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Target executable
TARGET = $(BIN_DIR)/pacman-emu$(EXE_EXT)
TEST_ROM_GEN = $(BIN_DIR)/test_rom_gen$(EXE_EXT)
TEST_ROM = $(DATA_DIR)/test.rom

# Default target
all: dirs $(TARGET) $(TEST_ROM)

# Create necessary directories
dirs:
	$(call MKDIR_CMD,$(OBJ_DIR))
	$(call MKDIR_CMD,$(BIN_DIR))
	$(call MKDIR_CMD,$(DATA_DIR))

# Compile the emulator
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile test ROM generator
$(TEST_ROM_GEN): $(SRC_DIR)/test_rom.c
	$(CC) -Wall -Wextra -g -O2 -o $@ $<

# Generate test ROM
$(TEST_ROM): $(TEST_ROM_GEN)
	$< $@

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target
clean:
	$(call RM_CMD,$(OBJ_DIR))
	$(call RM_CMD,$(BIN_DIR))
	$(call RM_CMD,$(DATA_DIR))

# Run target with test ROM
run: all
	$(TARGET) --test

# Windows-specific help target
winhelp:
	@echo "MinGW64 Build Instructions:"
	@echo "1. Set SDL2_DIR environment variable to your SDL2 installation path"
	@echo "   Example: set SDL2_DIR=C:\SDL2-2.0.14"
	@echo "2. Run mingw32-make or make"
	@echo ""
	@echo "Note: You need to have SDL2.dll in your PATH or copy it to the same directory"
	@echo "as the executable after building."

.PHONY: all dirs clean run winhelp