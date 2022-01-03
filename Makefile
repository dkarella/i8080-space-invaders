CC:=clang
CFLAGS:=-std=c99 -Wall -Werror `sdl2-config --cflags`
LDFLAGS:=`sdl2-config --libs` -lSDL2_mixer
ENTRYPOINT:=main.c
OUT:=main

all: main

main: main.c invaders.o cpu.o disassembler.o audio.o ports.o
	$(CC) $(CFLAGS) -o $(OUT) $(ENTRYPOINT) invaders.o cpu.o disassembler.o audio.o ports.o $(LDFLAGS)

web: CC:=emcc
web: CFLAGS:=-O2
web: LDFLAGS:=-s EXPORTED_FUNCTIONS="['_start']"
web: LDFLAGS+=-s EXPORTED_RUNTIME_METHODS=['ccall']
web: LDFLAGS+=-s USE_SDL=2 -s USE_SDL_MIXER=2
web: LDFLAGS+=-s FORCE_FILESYSTEM=1
web: LDFLAGS+=-s MODULARIZE=1
web: ENTRYPOINT:= main_wasm.c 
web: OUT:=./www/main.mjs
web: main

invaders.o: invaders.c
	$(CC) $(CFLAGS) -c invaders.c -o invaders.o

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

disassembler.o: disassembler.c
	$(CC) $(CFLAGS) -c disassembler.c -o disassembler.o

audio.o: audio.c
	$(CC) $(CFLAGS) -c audio.c -o audio.o

ports.o: ports.c
	$(CC) $(CFLAGS) -c ports.c -o ports.o

clean:
	rm -f main *.o www/main.* 

run: main
	./main res/rom/invaders
