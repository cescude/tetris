#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "chk.h"

#include "io.h"			/* Cursor & input functions  */
#include "pieces.h"		/* Piece definitions */

struct Player {
  int x, y, dir;
  enum Piece p, pn;
  int score;
  int lines;
  int active;
  int ghost_on;
};

void chk_player(struct Player *pl) {
  chk(pl != NULL);
  chk_piece(pl->p);
  chk_piece(pl->pn);
  chk(pl->score >= 0);
  chk(pl->lines >= 0);
  chk_bool(pl->active);
  chk_bool(pl->ghost_on);

  /* Note about checking x & y: because x & y are used as a starting
     point, and the piece offsets are relative to that, there's the
     potential for x/y to be offscreen but--because we're drawing four
     points--parts (or all) of the piece end up in a valid
     position. */
}

struct Board {
  char* background;
  char* foreground;
  int width, height;
};

void chk_board(struct Board *b) {
  chk(b != NULL);
  chk(b->background != NULL);
  chk(b->foreground != NULL);
  chk(b->width >= 2);
  chk(b->height >= 1);
}

void printLayer(char *buffer, int width, int height, char opaque) {
  chk(buffer != NULL);
  chk(width >=0 );
  chk(height >=0 );
  chk_bool(opaque);
  
  for ( int i=0; i<height; i++ ) {
    for ( int j=0; j<width; j++ ) {
      switch ( buffer[i*width+j] ) {
      case 0: if (opaque) putstr("  "); else cursorRt(2); break;
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
  chk_board(board);
  printLayer(board->foreground, board->width, board->height, 0);
}

void printBackground(struct Board *board) {
  chk_board(board);
  printLayer(board->background, board->width, board->height, 1);
}

/* return 1 if `p` can be placed at this location */
int testPiece(struct Board *board, enum Piece p, int rotation, int x, int y) {
  chk_board(board);
  chk_piece(p);
  
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
  chk_board(board);
  chk_player(st);
  
  int y = st->y;
  
  /* go down until we collide */
  while (testPiece(board, st->p, st->dir, st->x, y)) {
    y++;
  }

  y--;	       /* We only got here after a collision, so backup one */

  chk(y < board->height);
  return y;
}

void placePiece(char *buffer, int width, int height, enum Piece p, int rotation, int x, int y, char ch) {
  chk(buffer != NULL);
  chk(width >= 0);
  chk(height >= 0);
  chk_piece(p);
  
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
  chk_board(board);
  chk_player(st);
  
  int ghost_point = landingPoint(board, st);
  placePiece(board->foreground, board->width, board->height, st->p, st->dir, st->x, ghost_point, '.');
}

void placePlayer(struct Board *board, struct Player *st, char id) {
  chk_board(board);
  chk_player(st);
  
  placePiece(board->foreground, board->width, board->height, st->p, st->dir, st->x, st->y, id);
}

void stampPlayer(struct Board *board, struct Player *st, int stamp_point) {
  chk_board(board);
  chk_player(st);
  
  placePiece(board->background, board->width, board->height, st->p, st->dir, st->x, stamp_point, '^');
}

void printStats(struct Board *board, struct Player *st, int offset) {
  chk_board(board);
  chk_player(st);
  chk(st->pn < sizeof(piece_names) / sizeof(piece_names[0]));
  chk(offset >= 0);
  
  cursorDn(board->height + offset);
  if ( st->active ) {
    char buffer[100] = {0};
    snprintf(buffer, sizeof(buffer), "P%d SCORE=%d, LINES=%d, NEXT=%c\n", offset+1, st->score, st->lines, piece_names[st->pn]);
    putstr(buffer);
  } else {
    putstr("PLAYER 2?\n");
  }
  cursorUp(board->height + offset + 1);
}

void drawFrame(struct Board *board, struct Player *pl1, struct Player *pl2) {
  chk_board(board);
  chk_player(pl1);
  chk_player(pl2);
  
  printBackground(board);

  memset(board->foreground, 0, board->width*board->height);

  if ( pl1->active && pl1->ghost_on ) {
    placeGhost(board, pl1);
  }

  if ( pl2->active && pl2->ghost_on ) {
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

  blip();
}

void initBoard(struct Board *board) {
  chk_board(board);
  
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
  chk_board(board);
  chk(y >= 0);
  chk(y < board->height);
  
  int offset = y * board->width;
  for (int i=0; i<board->width; i++) {
    if ( !board->background[offset+i] ) {
      return 0;
    }
  }
  
  return 1;
}

void squashLine(struct Board *board, int y) {
  chk_board(board);
  chk(y >= 0);
  chk(y < board->height);
  
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
  chk_board(board);
  
  int num_lines = 0;
  for ( int i=0; i<board->height-1; i++ ) {
    num_lines += testLine(board, i);
  }

  chk(num_lines >= 0);
  return num_lines;
}

void squashAllLines(struct Board *board) {
  chk_board(board);
  
  for (int i=0; i<board->height-1; i++) {
    if (testLine(board, i)) {
      squashLine(board, i);
    }
  }
}

/* return 0 if there can't be another round... */
int nextRound(struct Board *board, struct Player *st) {
  chk_board(board);
  chk_player(st);

  /* Draw the piece onto the background, whereever it nestles in */
  int stamp_point = landingPoint(board, st);
  stampPlayer(board, st, stamp_point);

  int num_lines = countAllLines(board);

  /* Compute score for this round */
  st->score += num_lines*num_lines*100 + (stamp_point - st->y);

  if ( num_lines ) {
    printBackground(board);
    blip();
    /* Longer delay for more lines*/
    for ( int i=0; i<num_lines; i++ ) {
      tick();
    }
    squashAllLines(board);
  }

  st->p = st->pn;
  while ( (st->pn = rand()%P_END) == st->p ); /* Don't do two-in-a-rows */
  st->dir = 0;
  st->x = board->width/2-1;
  st->y = 0;
  st->lines += num_lines;

  chk_player(st);

  return testPiece(board, st->p, st->dir, st->x, st->y);
}

int computeDelay(int lines) {
  chk(lines >= 0);
  
  int delay = 10 - lines/10;
  int clamped_delay = delay < 0 ? 0 : delay;

  chk(clamped_delay >= 0);
  return clamped_delay;
}


/* return 0 if end-of-game, 1 otherwise */
int playerLogic(struct Board *board, struct Player *st, unsigned char btns, char force_drop) {
  chk_board(board);
  chk_player(st);
  chk_bool(force_drop);

  /* Set player to active if a movement key was detected */
  if ( btns & B_MOVEMENT_KEY ) {
    st->active = 1;
  }

  /* Set player to inactive if the player quit */
  if ( btns & B_QUIT ) {
    st->active = 0;
  }

  /* When player isn't active, don't do anything? */
  if (!st->active) return 1;

  if ( btns & B_GHOST ) {
    st->ghost_on = !st->ghost_on;
  }

  int nx = st->x;
  int nd = st->dir;
  
  /* Try moving left/right or rotating left/right... */
  if ( btns & B_LEFT ) nx--;
  if ( btns & B_RIGHT ) nx++;
  if ( btns & B_ROTL ) nd = (nd+3) % 4;
  if ( btns & B_ROTR ) nd = (nd+5) % 4;

  if ( testPiece(board, st->p, nd, nx, st->y) ) {
    st->dir = nd;
    st->x = nx;
    chk_player(st);
  }

  int ny = st->y;

  if ( force_drop ) {
    ny++;
  } else if (btns & B_SDROP) {
    ny++;
  }
  
  if ( testPiece(board, st->p, st->dir, st->x, ny) ) {
    st->y = ny;
    chk_player(st);
  } else {
    return nextRound(board, st);
  }
    
  if ( btns & B_HDROP ) {
    return nextRound(board, st);
  }

  return 1;
}

void quitting_animation(struct Board *board) {
  chk_board(board);
  
  memcpy(board->foreground, board->background, board->width * board->height);
  
  for (int limit=board->height-2; limit>=0; limit-=1) {
    for (int i=board->height-2; i>=limit; i--) {
      memset(board->foreground + i*board->width + 1, 'X', board->width - 2);
    }

    printBackground(board);
    printForeground(board);
    blip();
    tick();
  }

  printBackground(board);
}

int max(int a, int b) {
  return a > b ? a : b;
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
  chk_board(&board);

  initBoard(&board);

  struct Player pl1 = {
    .x = WIDTH/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .lines = 0,
    .p = rand()%P_END,
    .pn = rand()%P_END,
    .active = 1,
    .ghost_on = 0,
  };
  chk_player(&pl1);

  struct Player pl2 = {
    .x = WIDTH/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .lines = 0,
    .p = rand()%P_END,
    .pn = rand()%P_END,
    .active = 0,
    .ghost_on = 0,
  };
  chk_player(&pl2);

  cursorOff();
    
  drawFrame(&board, &pl1, &pl2);

  int ticks = computeDelay(pl1.lines + pl2.lines);
  chk(ticks >= 0);

  int num_players = 0;
  
  while (1) {
    unsigned short btns = getButtons();

    /* Player 1 buttons are in the lower byte */
#define P1(b) (b & 0xFF)

    /* Player 2 buttons are in the upper byte */
#define P2(b) ((b >> 8) & 0xFF)
    
    char force_drop = 0;
    if ( --ticks == 0 ) {
      force_drop = 1;
      ticks = computeDelay(pl1.lines + pl2.lines);
      chk(ticks >= 0);
    }

    if (!playerLogic(&board, &pl1, P1(btns), force_drop)) {
      break;
    }
    
    if (!playerLogic(&board, &pl2, P2(btns), force_drop)) {
      break;
    }

    num_players = max(num_players, pl1.active + pl2.active);

    if ( !pl1.active && !pl2.active ) break;

    drawFrame(&board, &pl1, &pl2);
  }

  quitting_animation(&board);

  pl1.active = pl2.active = 1;
  printStats(&board, &pl1, 0);

  if ( num_players > 1 ) {
    printStats(&board, &pl2, 1);
  }

  free(buffer);

  cursorDn(HEIGHT + num_players);
  cursorOn();
  
  blip();

  return 0;
}
