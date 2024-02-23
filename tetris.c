#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "io.h"			/* Cursor & input functions  */
#include "pieces.h"		/* Piece definitions */

int WIDTH = 0;
int HEIGHT = 0;

struct State {
  int x, y, dir;
  enum Piece p, pn;
  int score;
};

void printLayer(char *buffer, char clear) {
  for ( int i=0; i<HEIGHT; i++ ) {
    for ( int j=0; j<WIDTH; j++ ) {
      if ( buffer[i*WIDTH+j] ) {
	putchar(buffer[i*WIDTH+j]);
      } else if (clear) {
	putchar(' ');
      } else {
	cursorRt(1);   /* Move to the right w/out printing anything */
      }
    }
    putchar('\n');
  }

  cursorUp(HEIGHT);
}

void placePiece(char *buffer, enum Piece p, int rotation, int x, int y, char ch) {
  int offsets[4] = {0};
  getOffsets(offsets, p, rotation, WIDTH);

  int start = y*WIDTH+x;
  for ( int i=0; i<4; i++ ) {
    int offset = start + offsets[i];
    if ( 0 <= offset && offset < WIDTH*HEIGHT ) {
      buffer[offset] = ch;
    }
  }
}

/* return 1 if `p` can be placed at this location */
int testPiece(char *buffer, enum Piece p, int rotation, int x, int y) {
  int offsets[4] = {0};
  getOffsets(offsets, p, rotation, WIDTH);

  int start = y*WIDTH+x;
  for ( int i=0; i<4; i++ ) {
    int offset = start + offsets[i];
    if ( 0 <= offset && offset < WIDTH*HEIGHT && buffer[offset] ) {
      return 0;
    }
  }

  return 1;
}

int landingPoint(char *background, enum Piece p, int dir, int x, int y) {
  /* go down until we collide */
  while (testPiece(background, p, dir, x, y)) {
    y++;
  }

  return y-1; 			/* Need to backup one */
}

void drawFrame(char *background, struct State *st, char include_ghost) {
  char layer[10000] = {0};
  printLayer(background, 1);
  
  if ( include_ghost ) {
    placePiece(layer, st->p, st->dir, st->x, landingPoint(background, st->p, st->dir, st->x, st->y), '.');
  }
  placePiece(layer, st->p, st->dir, st->x, st->y, '#');

  printLayer(layer, 0);

  cursorDn(HEIGHT);
  printf("SCORE=%d, NEXT=%c\n", st->score, piece_names[st->pn]);
  cursorUp(HEIGHT + 1);
}

void initBoard(char *layer) {
  memset(layer, 0, WIDTH*HEIGHT);
  for (int i=0; i<HEIGHT-1; i++) {
    layer[i*WIDTH] = layer[i*WIDTH+WIDTH-1] = '|';
  }

  for (int i=0; i<WIDTH; i++) {
    layer[(HEIGHT-1)*WIDTH+i] = '-';
  }
}

int testLine(char *background, int y) {
  int offset = y * WIDTH;
  for (int i=0; i<WIDTH; i++) {
    if ( !background[offset+i] ) {
      return 0;
    }
  }
  return 1;
}

void squashLine(char *background, int y) {
  for (; y>0; y--) {
    int offset = y * WIDTH;
    for (int i=0; i<WIDTH; i++) {
      background[offset+i] = background[offset-WIDTH+i];
    }
  }
  for (int i=1; i<WIDTH-2; i++) {
    background[i] = 0;
  }
}

int countAllLines(char *background) {
  int num_lines = 0;
  for ( int i=0; i<HEIGHT-1; i++ ) {
    num_lines += testLine(background, i);
  }
  return num_lines;
}

void squashAllLines(char *background) {
  for (int i=0; i<HEIGHT-1; i++) {
    if (testLine(background, i)) {
      squashLine(background, i);
    }
  }
}

/* return 0 if there can't be another round... */
int nextRound(char *background, struct State *st) {
  st->y = landingPoint(background, st->p, st->dir, st->x, st->y);
  
  placePiece(background, st->p, st->dir, st->x, st->y, '#');

  /* Check to see if we've cleared any lines */
  int num_lines = countAllLines(background);

  if ( num_lines ) {
    drawFrame(background, st, 0);
    getEvents(num_lines);	/* Just need a simple delay */
    squashAllLines(background);
  }

  st->p = st->pn;
  while ( (st->pn = rand()%NUM_PIECES) == st->p ); /* Don't do two-in-a-rows */
  st->dir = 0;
  st->x = WIDTH/2-1;
  st->y = 0;
  st->score += num_lines;

  return testPiece(background, st->p, st->dir, st->x, st->y);
}

int computeDelay(int score) {
  int delay = 10 - score/10;
  return delay < 0 ? 0 : delay;
}

int main(int argc, char** argv) {
  WIDTH = argc > 1 ? atoi(argv[1]) : 12;
  HEIGHT = argc > 2 ? atoi(argv[2]) : 11;

  srand(time(0));

  char *board = (char*)malloc(sizeof(char)*WIDTH*HEIGHT);

  if ( board ) {
    initBoard(board);
  } else {
    printf( "Too large!\n" );
    return 1;
  }

  struct State st = {
    .x = WIDTH/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .p = rand()%NUM_PIECES,
    .pn = rand()%NUM_PIECES,
  };

  drawFrame(board, &st, 1);

  cursorOn(0);
    
  while (1) {
    int evt = getEvents(computeDelay(st.score));

    int nx = st.x, nd = st.dir;
    
    if ( evt & E_QUIT ) break;

    /* Try moving left/right or rotating left/right... */
    if ( evt & E_LEFT ) nx--;
    if ( evt & E_RIGHT ) nx++;
    if ( evt & E_ROTL ) nd = (nd+NUM_PIECES) % 4;
    if ( evt & E_ROTR ) nd = (nd+9) % 4;

    int end_round = 0;

    if ( (evt & E_TIMER) || (evt & E_SDROP) ) {
      if ( testPiece(board, st.p, st.dir, st.x, st.y+1) ) {
	st.y++;
      } else {
	end_round = 1;
      }
    }
    
    if ( evt & E_HDROP ) {
      /* Find the lowest spot and force a new round */
      st.y = landingPoint(board, st.p, st.dir, st.x, st.y);
      end_round = 1;
    }


    if ( end_round ) {
      if ( !nextRound(board, &st) ) {
	break;
      }
    } else if ( testPiece(board, st.p, nd, nx, st.y) ) {
      st.dir = nd;
      st.x = nx;
    } 
    
    drawFrame(board, &st, 1);
  }

  char overlay[10000] = {0};
  memcpy(overlay, board, WIDTH*HEIGHT);
  
  for (int limit=HEIGHT-2; limit>=0; limit-=1) {
    for (int i=HEIGHT-2; i>=limit; i--) {
      memset(overlay + i*WIDTH + 1, 'X', WIDTH - 2);
    }
    printLayer(overlay, 1);
    getEvents(1);
  }

  drawFrame(board, &st, 1);

  free(board);

  cursorDn(HEIGHT + 2);
  cursorOn(1);
  
  return 0;
}
