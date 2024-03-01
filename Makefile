all: tetris

tetris: tetris.c io.h pieces.h chk.h
	gcc -rdynamic -Wall tetris.c -o tetris

clean:
	rm tetris
