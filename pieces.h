#ifndef __PIECES_H_
#define __PIECES_H_

enum Piece {
  P_O = 0,
  P_L,
  P_J,
  P_S,
  P_Z,
  P_I,
  P_T,
  P_END,
};

void chk_piece(enum Piece p) {
  chk(p >= 0);
  chk(p < P_END);
}

char piece_names[] = {'O', 'L', 'J', 'S', 'Z', 'I', 'T'};

#define init8(dst, x0,y0, x1,y1, x2,y2, x3,y3) {	\
    dst[0] = (y0)*w+(x0);				\
    dst[1] = (y1)*w+(x1);				\
    dst[2] = (y2)*w+(x2);				\
    dst[3] = (y3)*w+(x3);				\
  }

/* Returns buffer offsets for each segment of a piece in a board of
   width `w` */
void getOffsets(int offsets[4], enum Piece p, int variant, int w) {
  chk(w > 2);

  switch (p) {
  case P_END:
    chk(0);
    break;
  case P_O:
    init8(offsets, 0,0, 1,0, 0,1, 1,1); break;
  case P_L:
    switch (variant % 4) {
    case 0: init8(offsets, 0,0, 0,1, 0,2, 1,2); break;
    case 1: init8(offsets, 0,1, 1,1, 2,1, 0,2); break;
    case 2: init8(offsets, 0,1, 1,1, 1,2, 1,3); break;
    case 3: init8(offsets, 1,1, 1,2, 0,2, -1,2); break;
    }
    break;
  case P_J:
    switch (variant % 4) {
    case 0: init8(offsets, 1,-1, 1,0, 0,1, 1,1); break;
    case 1: init8(offsets, 0,0, 0,1, 1,1, 2,1); break;
    case 2: init8(offsets, 0,0, 1,0, 0,1, 0,2); break;
    case 3: init8(offsets, -1,0, 0,0, 1,0, 1,1); break;
    }
    break;
  case P_T:
    switch (variant % 4) {
    case 0: init8(offsets, 0,-1, -1,0, 0,0, 1,0); break;
    case 1: init8(offsets, 0,-1, 0,0, 1,0, 0,1); break;
    case 2: init8(offsets, -1,0, 0,0, 1,0, 0,1); break;
    case 3: init8(offsets, 0,-1, 0,0, -1,0, 0,1); break;
    }
    break;
  case P_S:
    switch (variant % 2) {
    case 0: init8(offsets, 1,1, 2,1, 0,2, 1,2 ); break;
    case 1: init8(offsets, 0,0, 0,1, 1,1, 1,2 ); break;
    }
    break;
  case P_Z:
    switch (variant % 2) {
    case 0: init8(offsets,  0,1, 1,1, 1,2, 2,2 ); break;
    case 1: init8(offsets,  1,0, 0,1, 1,1, 0,2 ); break;
    }
    break;
  case P_I:
    switch (variant % 2) {
    case 0: init8(offsets, 0,0, 1,0, 2,0, 3,0 ); break;
    case 1: init8(offsets, 1,-2, 1,-1, 1,0, 1,1 ); break;
    }
    break;
  }
}

#endif
