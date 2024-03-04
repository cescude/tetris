all: tetris

tetris.dbg: tetris.c io.h pieces.h chk.h
	gcc -fsanitize=address,undefined -rdynamic -Wall tetris.c -o tetris.dbg

tetris: tetris.c io.h pieces.h chk.h
	gcc -Wall tetris.c -o tetris

clean:
	rm tetris
