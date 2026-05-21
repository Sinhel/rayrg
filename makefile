# Compiler settings
CC_LINUX = gcc
CC_WIN = x86_64-w64-mingw32-gcc

# Flags
CFLAGS_LINUX = -ggdb -Wextra -Wall -Wpedantic
CFLAGS_WIN = -Wall -Wextra -Wpedantic

# Libraries
LIBS_LINUX = -lraylib -lm
LIBS_WIN = -lraylib -lm -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32

# Files
SRC = regexsearcher.c
TARGET_LINUX = a.out
TARGET_WIN = a.exe

.PHONY: all linux windows clean

all: linux windows

linux: $(TARGET_LINUX)

$(TARGET_LINUX): $(SRC)
	$(CC_LINUX) $(SRC) -o $(TARGET_LINUX) $(CFLAGS_LINUX) $(LIBS_LINUX)

windows: $(TARGET_WIN)

$(TARGET_WIN): $(SRC)
	$(CC_WIN) $(SRC) -o $(TARGET_WIN) $(CFLAGS_WIN) $(LIBS_WIN)

clean:
	rm -f $(TARGET_LINUX) $(TARGET_WIN)

