# Compiler settings
CC_LINUX = gcc
CC_WIN = x86_64-w64-mingw32-gcc

# Flags
CFLAGS_LINUX = -ggdb -Wextra -Wall -Wpedantic
CFLAGS_WIN = -Wall -Wextra -Wpedantic

# Libraries
LIBS_COMMON = -lraylib -lm
LIBS_LINUX = -lpcre2-8
LIBS_WIN = -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 \
		   -Wl,-Bstatic -lpcre2-8 -Wl,-Bdynamic

# Files
SRC = regexsearcher.c
TARGET_LINUX = rayrg.out
TARGET_WIN = rayrg.exe

.PHONY: all linux windows clean

all: linux windows

linux: $(TARGET_LINUX)

$(TARGET_LINUX): $(SRC)
	$(CC_LINUX) $(SRC) -o $(TARGET_LINUX) $(CFLAGS_LINUX) $(LIBS_COMMON) $(LIBS_LINUX) 

windows: $(TARGET_WIN)

$(TARGET_WIN): $(SRC)
	$(CC_WIN) $(SRC) -o $(TARGET_WIN) $(CFLAGS_WIN) $(LIBS_COMMON) $(LIBS_WIN) 

clean:
	rm -f $(TARGET_LINUX) $(TARGET_WIN)

