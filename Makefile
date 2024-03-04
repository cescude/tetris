all: tetris

tetris.rel: tetris.c io.h pieces.h chk.h
	gcc -Wall tetris.c -o tetris.rel

tetris: tetris.c io.h pieces.h chk.h
	gcc -fsanitize=address,undefined -rdynamic -Wall tetris.c -o tetris

clean:
	rm tetris tetris.rel
