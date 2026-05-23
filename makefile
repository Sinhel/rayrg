# Compiler settings
CC_LINUX = gcc
CC_WIN = x86_64-w64-mingw32-gcc

# Flags
CFLAGS_LINUX = -ggdb -Wextra -Wall -Wpedantic
CFLAGS_WIN = -Wall -Wextra -Wpedantic

# Paths
INC_PATH ?= .
LIB_PATH ?= .

# Libraries
LIBS_COMMON = -lraylib -lm
LIBS_LINUX = -lpcre2-8
LIBS_WIN = -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 \
		   -Wl,-Bstatic -lpcre2-8 -Wl,-Bdynamic

# Files
SRC = regexsearcher.c
TARGET_LINUX = rayrg
TARGET_WIN = rayrg.exe
TARGET_WIN_GITHUB = $(TARGET_WIN)

.PHONY: all linux windows windows_github clean

all: linux windows

linux: $(TARGET_LINUX)

$(TARGET_LINUX): $(SRC)
	$(CC_LINUX) $(SRC) -o $(TARGET_LINUX) $(CFLAGS_LINUX) $(LIBS_COMMON) $(LIBS_LINUX) 

windows: $(TARGET_WIN)

windows_github: INC_PATH = deps/include
windows_github: LIB_PATH = deps/lib
windows_github: $(TARGET_WIN)

$(TARGET_WIN): $(SRC)
	$(CC_WIN) $(SRC) -o $(TARGET_WIN) $(CFLAGS_WIN) $(LIBS_COMMON) $(LIBS_WIN) \
		$(if $(INC_PATH),-I$(INC_PATH)) \
		$(if $(LIB_PATH),-L$(LIB_PATH))

clean:
	rm -f $(TARGET_LINUX) $(TARGET_WIN)

