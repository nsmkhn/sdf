.PHONY: all run animate-only animate clean

CC      = cc
CFLAGS  = -Wall -Wextra -O3 -std=c23 -Wpedantic
LDFLAGS = -lm

SRC = src/main.c
BIN = bin/sdf

all: run

$(BIN): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)
	@echo "Built $(BIN)"

run: $(BIN)
	mkdir -p output
	$(BIN)

animate-only:
	@if command -v ffmpeg &> /dev/null; then \
		ffmpeg -y -framerate 60 -i output/frame_%04d.ppm -c:v libx264 -crf 18 -preset slow -movflags faststart output/animation.mp4 && \
		echo "Generated animation.mp4"; \
	else \
		echo "ffmpeg not found. Install with: brew install ffmpeg"; \
	fi

animate: run animate-only

clean:
	rm -rf bin output
	@echo "Cleaned"
