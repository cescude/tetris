#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void cursor(char show) {
  printf("\033[?25%c", show ? 'h' : 'l');
}

int width = 0;
int height = 0;

enum Piece {
  P_O = 0,
  P_L,
  P_J,
  P_S,
  P_Z,
  P_I,
  P_T,
  NUM_PIECES,
};

char piece_names[] = {'O', 'L', 'J', 'S', 'Z', 'I', 'T'};

struct State {
  int x, y, dir;
  enum Piece p, pn;
  int score;
};

#define init4(dst, a, b, c, d) {			\
    dst[0] = a; dst[1] = b; dst[2] = c; dst[3] = d;	\
  }

void getOffsets(int offsets[4], enum Piece p, int variant) {
  switch (p) {
  case NUM_PIECES:
    printf("Bad times!\n");
    exit(1);
    break;
  case P_O:
    init4(offsets, 0, 1, width, width+1); break;
  case P_L:
    switch (variant % 4) {
    case 0: init4(offsets, 0, width, 2*width, 2*width+1); break;
    case 1: init4(offsets, width, width+1, width+2, 2*width); break;
    case 2: init4(offsets, width, width+1, 2*width+1, 3*width+1); break;
    case 3: init4(offsets, width+1, 2*width+1, 2*width, 2*width-1); break;
    }
    break;
  case P_J:
    switch (variant % 4) {
    case 0: init4(offsets, -width+1, 1, width, width+1); break;
    case 1: init4(offsets, 0, width, width+1, width+2); break;
    case 2: init4(offsets, 0, 1, 1*width, 2*width); break;
    case 3: init4(offsets, -1, 0, 1, width+1); break;
    }
    break;
  case P_T:
    switch (variant % 4) {
    case 0: init4(offsets, -width, -1, 0, 1); break;
    case 1: init4(offsets, -width, 0, 1, width); break;
    case 2: init4(offsets, -1, 0, 1, width); break;
    case 3: init4(offsets, -width, 0, -1, width); break;
    }
    break;
  case P_S:
    switch (variant % 2) {
    case 0: init4(offsets,  width+1, width+2, 2*width, 2*width+1 ); break;
    case 1: init4(offsets,  0, width, width+1, 2*width+1 ); break;
    }
    break;
  case P_Z:
    switch (variant % 2) {
    case 0: init4(offsets,  width, width+1, 2*width+1, 2*width+2 ); break;
    case 1: init4(offsets,  1, width, width+1, 2*width ); break;
    }
    break;
  case P_I:
    switch (variant % 2) {
    case 0: init4(offsets, 0, 1, 2, 3 ); break;
    case 1: init4(offsets, -width+1, 1, width+1, 2*width+1 ); break;
    }
    break;
  }
}

void printLayer(char *buffer, char clear) {
  for ( int i=0; i<height; i++ ) {
    for ( int j=0; j<width; j++ ) {
      if ( buffer[i*width+j] ) {
	putchar(buffer[i*width+j]);
      } else if (clear) {
	putchar(' ');
      } else {
	printf("\033[1C"); /* Move to the right w/out printing anything */
      }
    }
    putchar('\n');
  }

  printf("\033[%dA", height);
}

void placePiece(char *buffer, enum Piece p, int rotation, int x, int y, char ch) {
  int offsets[4] = {0};
  getOffsets(offsets, p, rotation);

  int start = y*width+x;
  for ( int i=0; i<4; i++ ) {
    int offset = start + offsets[i];
    if ( 0 <= offset && offset < width*height ) {
      buffer[offset] = ch;
    }
  }
}

/* return 1 if `p` can be placed at this location */
int testPiece(char *buffer, enum Piece p, int rotation, int x, int y) {
  int offsets[4] = {0};
  getOffsets(offsets, p, rotation);

  int start = y*width+x;
  for ( int i=0; i<4; i++ ) {
    int offset = start + offsets[i];
    if ( 0 <= offset && offset < width*height && buffer[offset] ) {
      return 0;
    }
  }

  return 1;
}

#include <unistd.h>
#include <termios.h>

enum Event {
  E_TIMER = 1,
  E_QUIT = 2,
  E_LEFT = 4,
  E_RIGHT = 8,
  E_ROTL = 16,
  E_ROTR = 32,
  E_HDROP = 64,
  E_SDROP = 128,
};

int getEvents(int delay) {
  struct termios attr = {0};
  if ( tcgetattr( 0, &attr ) < 0 ) {
    perror("tcgetattr");
  }

  attr.c_lflag &= ~ICANON;
  attr.c_lflag &= ~ECHO;
  attr.c_cc[VMIN] = 0;
  attr.c_cc[VTIME] = delay; /* tenths of a second */

  if ( tcsetattr( 0, TCSANOW, &attr ) < 0 ) {
    perror( "tcsetattr" );
  }

  int result = 0;
  char buf = 0;
  if ( read( 0, &buf, 1 ) > 0 ) {
    switch ( buf ) {
    case 'q': result |= E_QUIT; break;
    case 'j': case 'h': result |= E_LEFT; break;
    case 'k': case 'l': result |= E_RIGHT; break;
    case 'd': result |= E_ROTL; break;
    case 'f': result |= E_ROTR; break;
    case 'n': result |= E_SDROP; break;
    case ' ': result |= E_HDROP; break;
    }
  } else {
    result = E_TIMER;
  }

  attr.c_lflag |= ICANON;
  attr.c_lflag |= ECHO;

  if ( tcsetattr( 0, TCSADRAIN, &attr ) ) {
    perror( "tcsetattr (reset)" );
  }

  return result;
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

  printf("\033[%dB", height);
  printf("SCORE=%d, NEXT=%c\n", st->score, piece_names[st->pn]);
  printf("\033[%dA", height + 1);
}

void initBoard(char *layer) {
  memset(layer, 0, width*height);
  for (int i=0; i<height-1; i++) {
    layer[i*width] = layer[i*width+width-1] = '|';
  }

  for (int i=0; i<width; i++) {
    layer[(height-1)*width+i] = '-';
  }
}

int testLine(char *background, int y) {
  int offset = y * width;
  for (int i=0; i<width; i++) {
    if ( !background[offset+i] ) {
      return 0;
    }
  }
  return 1;
}

void squashLine(char *background, int y) {
  for (; y>0; y--) {
    int offset = y * width;
    for (int i=0; i<width; i++) {
      background[offset+i] = background[offset-width+i];
    }
  }
  for (int i=1; i<width-2; i++) {
    background[i] = 0;
  }
}

int countAllLines(char *background) {
  int num_lines = 0;
  for ( int i=0; i<height-1; i++ ) {
    num_lines += testLine(background, i);
  }
  return num_lines;
}

void squashAllLines(char *background) {
  for (int i=0; i<height-1; i++) {
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
  st->x = width/2-1;
  st->y = 0;
  st->score += num_lines;

  return testPiece(background, st->p, st->dir, st->x, st->y);
}

int computeDelay(int score) {
  int delay = 10 - score/10;
  return delay < 0 ? 0 : delay;
}

int main(int argc, char** argv) {
  width = argc > 1 ? atoi(argv[1]) : 12;
  height = argc > 2 ? atoi(argv[2]) : 11;

  srand(time(0));

  char board[10000] = {0};

  if ( width * height < sizeof(board) ) {
    initBoard(board);
  } else {
    printf( "Too large!\n" );
    return 1;
  }

  struct State st = {
    .x = width/2-1,
    .y = -1,
    .dir = 0,
    .score = 0,
    .p = rand()%NUM_PIECES,
    .pn = rand()%NUM_PIECES,
  };

  drawFrame(board, &st, 1);

  cursor(0);
    
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
  memcpy(overlay, board, width*height);
  
  for (int limit=height-2; limit>=0; limit-=1) {
    for (int i=height-2; i>=limit; i--) {
      memset(overlay + i*width + 1, 'X', width - 2);
    }
    printLayer(overlay, 1);
    getEvents(1);
  }

  drawFrame(board, &st, 1);

  printf("\033[%dB", height + 2);

  cursor(1);
  
  return 0;
}
