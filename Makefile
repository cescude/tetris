all: tetris

tetris: tetris.c
	gcc -Wall tetris.c -o tetris
