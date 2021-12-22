CC=clang
CFLAGS=-std=c99 -Wall -Werror -g
LIBS=`sdl2-config --cflags --libs` -lSDL2_ttf

all: main

cpu.o: cpu.c
	$(CC) $(CFLAGS) -c cpu.c -o cpu.o

disassembler.o: disassembler.c
	$(CC) $(CFLAGS) -c disassembler.c -o disassembler.o

ports.o: ports.c
	$(CC) $(CFLAGS) -c ports.c -o ports.o

main: main.c cpu.o disassembler.o ports.o
	$(CC) $(CFLAGS) -o main main.c cpu.o disassembler.o ports.o $(LIBS)

clean:
	rm main *.o

run: main
	./main rom/invaders
