#ifndef __IO_H__
#define __IO_H__

#define P1_LEFT 'j'
#define P1_RIGHT 'k'
#define P1_ROTL 'u'
#define P1_ROTR 'i'
#define P1_SDROP 'n'
#define P1_HDROP 'h'
#define P1_QUIT 'q'

#define P2_LEFT 'd'
#define P2_RIGHT 'f'
#define P2_ROTL 'e'
#define P2_ROTR 'r'
#define P2_SDROP 'v'
#define P2_HDROP 'g'
#define P2_QUIT 'q'

enum Event {
  E_TIMER = 1<<0,
  E_QUIT = 1<<1,
  E_LEFT = 1<<2,
  E_RIGHT = 1<<3,
  E_ROTL = 1<<4,
  E_ROTR = 1<<5,
  E_HDROP = 1<<6,
  E_SDROP = 1<<7,
};

#define E_MOVEMENT_KEY (E_LEFT|E_RIGHT)

unsigned short getEvents(int delay) {
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

  unsigned char buf = 0;
  unsigned char k1 = 0;
  unsigned char k2 = 0;
  
  if ( read( 0, &buf, 1 ) > 0 ) {

#define KEY_CASE(var, k, evt) case k: var |= evt; break
    switch ( buf ) {
      KEY_CASE(k1, P1_QUIT, E_QUIT);
      KEY_CASE(k1, P1_LEFT, E_LEFT);
      KEY_CASE(k1, P1_RIGHT, E_RIGHT);
      KEY_CASE(k1, P1_ROTL, E_ROTL);
      KEY_CASE(k1, P1_ROTR, E_ROTR);
      KEY_CASE(k1, P1_SDROP, E_SDROP);
      KEY_CASE(k1, P1_HDROP, E_HDROP);
    }

    switch ( buf ) {
      KEY_CASE(k2, P2_QUIT, E_QUIT);
      KEY_CASE(k2, P2_LEFT, E_LEFT);
      KEY_CASE(k2, P2_RIGHT, E_RIGHT);
      KEY_CASE(k2, P2_ROTL, E_ROTL);
      KEY_CASE(k2, P2_ROTR, E_ROTR);
      KEY_CASE(k2, P2_SDROP, E_SDROP);
      KEY_CASE(k2, P2_HDROP, E_HDROP);
    }
#undef KEY_CASE
    
  } else {
    k1 = k2 = E_TIMER;
  }

  attr.c_lflag |= ICANON;
  attr.c_lflag |= ECHO;

  if ( tcsetattr( 0, TCSADRAIN, &attr ) ) {
    perror( "tcsetattr (reset)" );
  }

  return (k2 << 8) + k1;
}

void cursorOn() {
  printf("\033[?25h");
}

void cursorOff() {
  printf("\033[?25l");
}

void cursorRt(int n) {
  printf("\033[1C");
}

void cursorUp(int n) {
  printf("\033[%dA", n);
}

void cursorDn(int n) {
  printf("\033[%dB", n);
}

#endif
