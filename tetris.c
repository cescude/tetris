#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "io.h"			/* Cursor & input functions  */
#include "pieces.h"		/* Piece definitions */

struct State {
  int x, y, dir;
  enum Piece p, pn;
  int score;
};

struct Board {
  char* background;
  char* foreground;
  int width, height;
};

void printLayer(char *buffer, int width, int height, char clear) {
  for ( int i=0; i<height; i++ ) {
    for ( int j=0; j<width; j++ ) {
      if ( buffer[i*width+j] ) {
	putchar(buffer[i*width+j]);
      } else if (clear) {
	putchar(' ');
      } else {
	cursorRt(1);   /* Move to the right w/out printing anything */
      }
    }
    putchar('\n');
  }

  cursorUp(height);
}

void printForeground(struct Board *board) {
  printLayer(board->foreground, board->width, board->height, 0);
}

void printBackground(struct Board *board) {
  printLayer(board->background, board->width, board->height, 1);
}

/* return 1 if `p` can be placed at this location */
int testPiece(struct Board *board, enum Piece p, int rotation, int x, int y) {
  int offsets[4] = {0};
  getOffsets(offsets, p, rotation, board->width);

  int start = y*board->width+x;
  for ( int i=0; i<4; i++ ) {
    int offset = start + offsets[i];
    if ( 0 <= offset && offset < board->width*board->height && board->background[offset] ) {
      return 0;
    }
  }

  return 1;
}

int landingPoint(struct Board *board, struct State *st) {
  int y = st->y;
  
  /* go down until we collide */
  while (testPiece(board, st->p, st->dir, st->x, y)) {
    y++;
  }

  return y-1; 			/* Need to backup one */
}

void placePiece(char *buffer, int width, int height, enum Piece p, int rotation, int x, int y, char ch) {
  int offsets[4] = {0};
  getOffsets(offsets, p, rotation, width);

  int start = y*width+x;
  for ( int i=0; i<4; i++ ) {
    int offset = start + offsets[i];
    if ( 0 <= offset && offset < width*height ) {
      buffer[offset] = ch;
    }
  }
}

void placeGhost(struct Board *board, struct State *st) {
  int ghost_point = landingPoint(board, st);
  placePiece(board->foreground, board->width, board->height, st->p, st->dir, st->x, ghost_point, '.');
}

void placePlayer(struct Board *board, struct State *st) {
  placePiece(board->foreground, board->width, board->height, st->p, st->dir, st->x, st->y, '#');
}

void stampPlayer(struct Board *board, struct State *st) {
  int stamp_point = landingPoint(board, st);
  placePiece(board->background, board->width, board->height, st->p, st->dir, st->x, stamp_point, '#');
}

void printStats(struct Board *board, struct State *st) {
  cursorDn(board->height);
  printf("SCORE=%d, NEXT=%c\n", st->score, piece_names[st->pn]);
  cursorUp(board->height + 1);
}

void drawFrame(struct Board *board, struct State *st) {
  printBackground(board);

  memset(board->foreground, 0, board->width*board->height);
  
  placeGhost(board, st);
  placePlayer(board, st);
  
  printForeground(board);

  printStats(board, st);
}

void initBoard(struct Board *board) {
  int width = board->width;
  int height = board->height;
  
  memset(board->background, 0, width*height);
  memset(board->foreground, 0, width*height);
  
  for (int i=0; i<height-1; i++) {
    board->background[i*width] = board->background[i*width+width-1] = '|';
  }

  for (int i=0; i<width; i++) {
    board->background[(height-1)*width+i] = '-';
  }
}

int testLine(struct Board *board, int y) {
  int offset = y * board->width;
  for (int i=0; i<board->width; i++) {
    if ( !board->background[offset+i] ) {
      return 0;
    }
  }
  return 1;
}

void squashLine(struct Board *board, int y) {
  for (; y>0; y--) {
    int offset = y * board->width;
    for (int i=1; i<board->width-1; i++) {
      board->background[offset+i] = board->background[offset-board->width+i];
    }
  }
  for (int i=1; i<board->width-1; i++) {
    board->background[i] = 0;
  }
}

int countAllLines(struct Board *board) {
  int num_lines = 0;
  for ( int i=0; i<board->height-1; i++ ) {
    num_lines += testLine(board, i);
  }
  return num_lines;
}

void squashAllLines(struct Board *board) {
  for (int i=0; i<board->height-1; i++) {
    if (testLine(board, i)) {
      squashLine(board, i);
    }
  }
}

/* return 0 if there can't be another round... */
int nextRound(struct Board *board, struct State *st) {

  stampPlayer(board, st);

  int num_lines = countAllLines(board);

  if ( num_lines ) {
    printBackground(board);
    getEvents(num_lines);	/* Just need a simple delay */
    squashAllLines(board);
  }

  st->p = st->pn;
  while ( (st->pn = rand()%NUM_PIECES) == st->p ); /* Don't do two-in-a-rows */
  st->dir = 0;
  st->x = board->width/2-1;
  st->y = 0;
  st->score += num_lines;

  return testPiece(board, st->p, st->dir, st->x, st->y);
}

int computeDelay(int score) {
  int delay = 10 - score/10;
  return delay < 0 ? 0 : delay;
}

int main(int argc, char** argv) {
  int WIDTH = argc > 1 ? atoi(argv[1]) : 12;
  int HEIGHT = argc > 2 ? atoi(argv[2]) : 11;

  srand(time(0));

  char* buffer = (char*)malloc(sizeof(char)*WIDTH*2*HEIGHT);
  if ( !buffer ) {
    printf("Too large!\n");
    return 1;
  }

  struct Board board = {
    .background = buffer,
    .foreground = buffer + WIDTH*HEIGHT,
    .width = WIDTH,
    .height = HEIGHT,
  };

  initBoard(&board);

  struct State st = {
    .x = WIDTH/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .p = rand()%NUM_PIECES,
    .pn = rand()%NUM_PIECES,
  };

  drawFrame(&board, &st);

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
      if ( testPiece(&board, st.p, st.dir, st.x, st.y+1) ) {
	st.y++;
      } else {
	end_round = 1;
      }
    }
    
    if ( evt & E_HDROP ) {
      /* Find the lowest spot and force a new round */
      st.y = landingPoint(&board, &st);
      end_round = 1;
    }


    if ( end_round ) {
      if ( !nextRound(&board, &st) ) {
	break;
      }
    } else if ( testPiece(&board, st.p, nd, nx, st.y) ) {
      st.dir = nd;
      st.x = nx;
    } 
    
    drawFrame(&board, &st);
  }

  memcpy(board.foreground, board.background, WIDTH*HEIGHT);
  
  for (int limit=HEIGHT-2; limit>=0; limit-=1) {
    for (int i=HEIGHT-2; i>=limit; i--) {
      memset(board.foreground + i*WIDTH + 1, 'X', WIDTH - 2);
    }
    printBackground(&board);
    printForeground(&board);
    getEvents(1);
  }

  printBackground(&board);

  free(buffer);

  cursorDn(HEIGHT + 2);
  cursorOn(1);
  
  return 0;
}
