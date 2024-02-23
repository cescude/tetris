all: tetris

tetris: tetris.c io.h pieces.h
	gcc -Wall tetris.c -o tetris
