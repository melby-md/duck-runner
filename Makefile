CC = $(CROSS)cc -std=c99
SDL_CONFIG = sdl2-config
SDL_FLAGS = --cflags --libs
WARNINGS = -Wall -Wextra -Wvla -Wno-unused-parameter -Wno-unused-function

all: duck-runner

duck-runner: main.c stb_image.h
	$(CC) $(WARNINGS) $(CFLAGS) -o $@ $< $$($(SDL_CONFIG) $(SDL_FLAGS))

clean:
	rm -f duck-runner duck-runner.exe

.PHONY: all clean
