CC:=clang
CFLAGS:=-std=c99 -Wall -Werror -g `sdl2-config --cflags`
LDFLAGS:=`sdl2-config --libs` -lSDL2_ttf -lSDL2_mixer
OUT:=main

all: main

main: main.c cpu.o disassembler.o audio.o ports.o
	$(CC) $(CFLAGS) -o $(OUT) main.c cpu.o disassembler.o audio.o ports.o $(LDFLAGS)

web: CC:=emcc
web: CFLAGS:=-O2 -DWASM
web: LDFLAGS:=-s EXPORTED_FUNCTIONS="['_wasm_main']"
web: LDFLAGS+=-s EXPORTED_RUNTIME_METHODS=['ccall']
web: LDFLAGS+=-s USE_SDL=2 -s USE_SDL_MIXER=2 -s USE_SDL_TTF=2
web: LDFLAGS+=-s MODULARIZE=1
web: LDFLAGS+= --preload-file res/
web: OUT:=./www/main.mjs
web: main

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

disassembler.o: disassembler.c
	$(CC) $(CFLAGS) -c disassembler.c -o disassembler.o

audio.o: audio.c
	$(CC) $(CFLAGS) -c audio.c -o audio.o

ports.o: ports.c
	$(CC) $(CFLAGS) -c ports.c -o ports.o

clean:
	rm -f main *.o 
	rm -rf ./www/wasm/*

run: main
	./main res/rom/invaders

cpudiag:
	$(CC) $(CFLAGS) -DCPUDIAG -c cpu.c -o cpu.o
	$(CC) $(CFLAGS) -DCPUDIAG -c disassembler.c -o disassembler.o
	$(CC) $(CFLAGS) -DCPUDIAG -c audio.c -o audio.o
	$(CC) $(CFLAGS) -DCPUDIAG -c ports.c -o ports.o
	$(CC) $(CFLAGS) -DCPUDIAG -o main main.c cpu.o disassembler.o audio.o ports.o $(LIBS)
	./main res/rom/cpudiag.bin
