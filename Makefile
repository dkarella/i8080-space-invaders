CC:=clang
CFLAGS:=-std=c99 -Wall -Werror -g `sdl2-config --cflags`
LDFLAGS:=`sdl2-config --libs` -lSDL2_ttf -lSDL2_mixer

all: main

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

disassembler.o: disassembler.c
	$(CC) $(CFLAGS) -c disassembler.c -o disassembler.o

audio.o: audio.c
	$(CC) $(CFLAGS) -c audio.c -o audio.o

ports.o: ports.c
	$(CC) $(CFLAGS) -c ports.c -o ports.o

main: main.c cpu.o disassembler.o audio.o ports.o 
	$(CC) $(CFLAGS) -o main main.c cpu.o disassembler.o audio.o ports.o $(LDFLAGS)

clean:
	rm -f main *.o

run: main
	./main rom/invaders

cpudiag:
	$(CC) $(CFLAGS) -DCPUDIAG -c cpu.c -o cpu.o
	$(CC) $(CFLAGS) -DCPUDIAG -c disassembler.c -o disassembler.o
	$(CC) $(CFLAGS) -DCPUDIAG -c audio.c -o audio.o
	$(CC) $(CFLAGS) -DCPUDIAG -c ports.c -o ports.o
	$(CC) $(CFLAGS) -DCPUDIAG -o main main.c cpu.o disassembler.o audio.o ports.o $(LIBS)
	./main rom/cpudiag.bin
