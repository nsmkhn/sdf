.PHONY: all run animate-only animate clean

SRC = src/main.c

ifeq ($(OS),Windows_NT)
SHELL     := cmd.exe
CC        = cl
CFLAGS    = /O2 /std:clatest
BIN       = bin\sdf.exe
MKDIR     = if not exist bin mkdir bin & if not exist output mkdir output
CLEAN_CMD = if exist bin rmdir /s /q bin & if exist output rmdir /s /q output
COMPILE   = $(CC) $(CFLAGS) $(SRC) /Fe:$(BIN)
else
CC        = cc
CFLAGS    = -Wall -Wextra -O3 -std=c2x -Wpedantic
LDFLAGS   = -lm
BIN       = bin/sdf
MKDIR     = mkdir -p bin output
CLEAN_CMD = rm -rf bin output
COMPILE   = $(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)
endif

all: $(BIN)

$(BIN): $(SRC)
	$(MKDIR)
	$(COMPILE)
	@echo Built $(BIN)

run: $(BIN)
	$(BIN)

animate-only:
	ffmpeg -y -framerate 60 -i output/metaballs_%04d.ppm \
	-c:v libx264 -pix_fmt yuv420p -crf 18 -preset slow \
	-movflags faststart output/metaballs.mp4

animate: run animate-only

clean:
	$(CLEAN_CMD)
	@echo Cleaned
