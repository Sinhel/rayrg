# Compiler settings
CC_LINUX = gcc
CC_WIN = x86_64-w64-mingw32-gcc

# Flags
CFLAGS_LINUX = -ggdb -Wextra -Wall -Wpedantic
CFLAGS_WIN = -Wall -Wextra -Wpedantic

# Libraries
LIBS_COMMON = -Wl,-Bstatic -lraylib -lpcre2-8 -Wl,-Bdynamic
LIBS_LINUX = -lm -lpthread -ldl -lrt -lGL -lX11
LIBS_WIN = -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32

# Files
SRC = gui.c regexsearcher.c
HDR = regexsearcher.h
TARGET_LINUX = rayrg
TARGET_WIN = rayrg.exe

.PHONY: all linux windows clean

all: linux windows

linux: $(TARGET_LINUX)

$(TARGET_LINUX): $(SRC) $(HDR)
	$(CC_LINUX) $(SRC) -o $(TARGET_LINUX) $(CFLAGS_LINUX) \
		$(LIBS_COMMON) $(LIBS_LINUX)

windows: $(TARGET_WIN)

$(TARGET_WIN): $(SRC) $(HDR)
	$(CC_WIN) -DPCRE2_STATIC $(SRC) -o $(TARGET_WIN) $(CFLAGS_WIN)  \
		$(LIBS_COMMON) $(LIBS_WIN)

clean:
	rm -f $(TARGET_LINUX) $(TARGET_WIN)

