#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "io.h"			/* Cursor & input functions  */
#include "pieces.h"		/* Piece definitions */

struct Player {
  int x, y, dir;
  enum Piece p, pn;
  int score;
  int active;
};

struct Board {
  char* background;
  char* foreground;
  int width, height;
};

void printLayer(char *buffer, int width, int height, char clear) {
  for ( int i=0; i<height; i++ ) {
    for ( int j=0; j<width; j++ ) {
      switch ( buffer[i*width+j] ) {
      case 0: if (clear) putstr("  "); else cursorRt(2); break;
      case '#': putstr("##"); break;
      case '@': putstr("@@"); break;
      case '.': putstr(".."); break;
      default:
	putstr("XX"); break;
	break;
      }
    }
    putstr("\n");
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

int landingPoint(struct Board *board, struct Player *st) {
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

void placeGhost(struct Board *board, struct Player *st) {
  int ghost_point = landingPoint(board, st);
  placePiece(board->foreground, board->width, board->height, st->p, st->dir, st->x, ghost_point, '.');
}

void placePlayer(struct Board *board, struct Player *st, char id) {
  placePiece(board->foreground, board->width, board->height, st->p, st->dir, st->x, st->y, id);
}

void stampPlayer(struct Board *board, struct Player *st) {
  int stamp_point = landingPoint(board, st);
  placePiece(board->background, board->width, board->height, st->p, st->dir, st->x, stamp_point, '^');
}

void printStats(struct Board *board, struct Player *st, int offset) {
  cursorDn(board->height + offset);
  if ( st->active ) {
    char buffer[100] = {0};
    snprintf(buffer, sizeof(buffer), "P%d SCORE=%d, NEXT=%c\n", offset+1, st->score, piece_names[st->pn]);
    putstr(buffer);
  } else {
    putstr("PLAYER 2?\n");
  }
  cursorUp(board->height + offset + 1);
}

void drawFrame(struct Board *board, struct Player *pl1, struct Player *pl2) {
  printBackground(board);

  memset(board->foreground, 0, board->width*board->height);

  if ( pl1->active ) {
    placeGhost(board, pl1);
  }

  if ( pl2->active ) {
    placeGhost(board, pl2);
  }

  if ( pl1->active ) {
    placePlayer(board, pl1, '#');
  }

  if ( pl2->active ) {
    placePlayer(board, pl2, '@');
  }
  
  printForeground(board);

  printStats(board, pl1, 0);
  printStats(board, pl2, 1);

  flip();
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
int nextRound(struct Board *board, struct Player *st) {

  stampPlayer(board, st);

  int num_lines = countAllLines(board);

  if ( num_lines ) {
    printBackground(board);
    flip();
    /* Longer delay for more lines*/
    for ( int i=0; i<num_lines; i++ ) {
      tick();
    }
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


/* return 0 if end-of-game, 1 otherwise */
int playerLogic(struct Board *board, struct Player *st, char evt, char force_drop) {

  /* Set player to active if a movement key was detected */
  if ( evt & B_MOVEMENT_KEY ) {
    st->active = 1;
  }

  /* Set player to inactive if the player quit */
  if ( evt & B_QUIT ) {
    st->active = 0;
  }

  /* When player isn't active, don't do anything? */
  if (!st->active) return 1;

  int nx = st->x;
  int nd = st->dir;
  
  /* Try moving left/right or rotating left/right... */
  if ( evt & B_LEFT ) nx--;
  if ( evt & B_RIGHT ) nx++;
  if ( evt & B_ROTL ) nd = (nd+3) % 4;
  if ( evt & B_ROTR ) nd = (nd+5) % 4;

  if ( testPiece(board, st->p, nd, nx, st->y) ) {
    st->dir = nd;
    st->x = nx;
  }

  int ny = st->y;

  if ( force_drop ) {
    ny++;
  } else if (evt & B_SDROP) {
    ny++;
  }
  
  if ( testPiece(board, st->p, st->dir, st->x, ny) ) {
    st->y = ny;
  } else {
    return nextRound(board, st);
  }
    
  if ( evt & B_HDROP ) {
    return nextRound(board, st);
  }

  return 1;
}

int main(int argc, char** argv) {
  int WIDTH = argc > 1 ? atoi(argv[1])+2 : 12;
  int HEIGHT = argc > 2 ? atoi(argv[2])+1 : 21;

  srand(time(0));

  char* buffer = (char*)malloc(sizeof(char)*WIDTH*2*HEIGHT);
  if ( !buffer ) {
    putstr("Too large!\n");
    return 1;
  }

  struct Board board = {
    .background = buffer,
    .foreground = buffer + WIDTH*HEIGHT,
    .width = WIDTH,
    .height = HEIGHT,
  };

  initBoard(&board);

  struct Player pl1 = {
    .x = WIDTH/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .p = rand()%NUM_PIECES,
    .pn = rand()%NUM_PIECES,
    .active = 1,
  };

  struct Player pl2 = {
    .x = WIDTH/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .p = rand()%NUM_PIECES,
    .pn = rand()%NUM_PIECES,
    .active = 0,
  };

  cursorOff();
    
  drawFrame(&board, &pl1, &pl2);

  int ticks = computeDelay(pl1.score + pl2.score);

  while (1) {
    short btns = getButtons();
    char p1btns = btns & 0xFF;
    char p2btns = btns >> 8;

    char force_drop = 0;
    if ( --ticks == 0 ) {
      force_drop = 1;
      ticks = computeDelay(pl1.score + pl2.score);
    }

    if (!playerLogic(&board, &pl1, p1btns, force_drop)) {
      break;
    }
    
    if (!playerLogic(&board, &pl2, p2btns, force_drop)) {
      break;
    }

    if ( !pl1.active && !pl2.active ) break;

    drawFrame(&board, &pl1, &pl2);
  }

  memcpy(board.foreground, board.background, WIDTH*HEIGHT);
  
  for (int limit=HEIGHT-2; limit>=0; limit-=1) {
    for (int i=HEIGHT-2; i>=limit; i--) {
      memset(board.foreground + i*WIDTH + 1, 'X', WIDTH - 2);
    }

    printBackground(&board);
    printForeground(&board);
    flip();
    tick();
  }

  printBackground(&board);

  pl1.active = pl2.active = 1;
  printStats(&board, &pl1, 0);
  printStats(&board, &pl2, 1);

  free(buffer);

  cursorDn(HEIGHT + 2);
  cursorOn();
  
  flip();

  return 0;
}
