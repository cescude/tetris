#ifndef __IO_H__
#define __IO_H__

#define P1_LEFT 'h'
#define P1_RIGHT 'l'
#define P1_ROTL 'j'
#define P1_ROTR 'k'
#define P1_SDROP 'u'
#define P1_HDROP 'n'
#define P1_QUIT 'q'

#define P2_LEFT 's'
#define P2_RIGHT 'g'
#define P2_ROTL 'd'
#define P2_ROTR 'f'
#define P2_SDROP 'r'
#define P2_HDROP 'v'
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

static int buffer_len = 0;
static char buffer[10000] = {0};

void putstr(char* s) {
  for ( int i=0; s[i]; i++ ) {
    buffer[buffer_len++] = s[i];
  }
}

void putchr(char c) {
  buffer[buffer_len++] = c;
}

void flip() {
  int bytes_written = 0;
  while (bytes_written < buffer_len) {
    int result = write(1, buffer + bytes_written, buffer_len - bytes_written);
    if ( result < 0 ) {
      perror("flip/write");
    }
    bytes_written += result;
  }

  buffer_len = 0;
  buffer[0] = 0;
}

void cursorOn() {
  putstr("\033[?25h");
}

void cursorOff() {
  putstr("\033[?25l");
}

void cursorRt(int n) {
  snprintf(buffer+buffer_len, sizeof(buffer)-buffer_len, "\033[%dC", n);
  buffer_len += strlen(buffer+buffer_len);
}

void cursorUp(int n) {
  snprintf(buffer+buffer_len, sizeof(buffer)-buffer_len, "\033[%dA", n);
  buffer_len += strlen(buffer+buffer_len);
}

void cursorDn(int n) {
  snprintf(buffer+buffer_len, sizeof(buffer)-buffer_len, "\033[%dB", n);
  buffer_len += strlen(buffer+buffer_len);
}

#endif
