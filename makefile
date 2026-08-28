# Compiler settings
CC_LINUX = gcc
CC_WIN   = x86_64-w64-mingw32-gcc

# Common flags
COMMON_CFLAGS = -Wall -Wextra -Wpedantic

# Linux flags
CFLAGS_LINUX = $(COMMON_CFLAGS) -ggdb \
    $(shell pkg-config --cflags sdl2) \
    -I/usr/include/SDL2

LDFLAGS_LINUX = -static-libgcc \
    $(shell pkg-config --static --libs sdl2) \
    -Wl,-Bstatic -lpcre2-8 -Wl,-Bdynamic \
    -lm -lpthread -ldl -lrt

# Windows flags
CFLAGS_WIN = -Wall -Wextra -Wpedantic \
             -I/usr/x86_64-w64-mingw32/include/SDL2 \
             -DSDL_MAIN_HANDLED -DPCRE2_STATIC

LDFLAGS_WIN = -static -static-libgcc \
              -lmingw32 -lSDL2main -lSDL2 \
              -Wl,-Bstatic -lpcre2-8 \
              -Wl,-Bdynamic -lwinmm -limm32 -lsetupapi -lversion -lole32 \
              -loleaut32 -luuid -lgdi32 -luser32 -lkernel32 -mwindows

# Source files
SRC = nuklear.c regexsearcher.c
HDR = regexsearcher.h

# Build directory
BUILD_DIR = build

# Object files in build directory
OBJ_LINUX = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))
OBJ_WIN   = $(patsubst %.c,$(BUILD_DIR)/%.win.o,$(SRC))

# Targets in build directory
TARGET_LINUX = $(BUILD_DIR)/regexsearcher
TARGET_WIN   = $(BUILD_DIR)/regexsearcher.exe

# Default target
.PHONY: all linux windows clean

all: linux windows

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Linux build
linux: $(TARGET_LINUX)

$(TARGET_LINUX): $(OBJ_LINUX) | $(BUILD_DIR)
	$(CC_LINUX) $(OBJ_LINUX) -o $@ $(CFLAGS_LINUX) $(LDFLAGS_LINUX)

$(BUILD_DIR)/%.o: %.c $(HDR) | $(BUILD_DIR)
	$(CC_LINUX) $(CFLAGS_LINUX) -c $< -o $@

# Windows build
windows: $(TARGET_WIN)

$(TARGET_WIN): $(OBJ_WIN) | $(BUILD_DIR)
	$(CC_WIN) $(OBJ_WIN) -o $@ $(CFLAGS_WIN) $(LDFLAGS_WIN)

$(BUILD_DIR)/%.win.o: %.c $(HDR) | $(BUILD_DIR)
	$(CC_WIN) $(CFLAGS_WIN) -c $< -o $@

# Cleanup
clean:
	rm -r $(BUILD_DIR)

