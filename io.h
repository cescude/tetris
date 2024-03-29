#ifndef __IO_H__
#define __IO_H__

#define P1_GHOST '`'
#define P1_LEFT 'j'
#define P1_RIGHT 'k'
#define P1_ROTL 'u'
#define P1_ROTR 'i'
#define P1_SDROP 'h'
#define P1_HDROP 'n'
#define P1_QUIT 'q'

#define P2_GHOST '`'
#define P2_LEFT 'd'
#define P2_RIGHT 'f'
#define P2_ROTL 'e'
#define P2_ROTR 'r'
#define P2_SDROP 'g'
#define P2_HDROP 'v'
#define P2_QUIT 'q'

enum Button {
  B_GHOST = 1<<0,
  B_QUIT = 1<<1,
  B_LEFT = 1<<2,
  B_RIGHT = 1<<3,
  B_ROTL = 1<<4,
  B_ROTR = 1<<5,
  B_HDROP = 1<<6,
  B_SDROP = 1<<7,
};

#define B_MOVEMENT_KEY (B_LEFT|B_RIGHT)

/* Wait 1 decisecond */
void tick() {
  usleep(10 * 10000);
}

unsigned short getButtons() {

  struct termios attr = {0};
  if ( tcgetattr( 0, &attr ) < 0 ) {
    perror("tcgetattr");
  }

  attr.c_lflag &= ~ICANON;
  attr.c_lflag &= ~ECHO;
  attr.c_cc[VMIN] = 0;
  attr.c_cc[VTIME] = 0;

  if ( tcsetattr( 0, TCSANOW, &attr ) < 0 ) {
    perror( "tcsetattr" );
  }

  tick();
  
  unsigned char buf = 0;
  unsigned char k1 = 0;
  unsigned char k2 = 0;
  
  while ( read( 0, &buf, 1 ) > 0 ) {

#define KEY_CASE(var, k, evt) case k: var |= evt; break
    switch ( buf ) {
      KEY_CASE(k1, P1_GHOST, B_GHOST);
      KEY_CASE(k1, P1_QUIT, B_QUIT);
      KEY_CASE(k1, P1_LEFT, B_LEFT);
      KEY_CASE(k1, P1_RIGHT, B_RIGHT);
      KEY_CASE(k1, P1_ROTL, B_ROTL);
      KEY_CASE(k1, P1_ROTR, B_ROTR);
      KEY_CASE(k1, P1_SDROP, B_SDROP);
      KEY_CASE(k1, P1_HDROP, B_HDROP);
    }

    switch ( buf ) {
      KEY_CASE(k2, P1_GHOST, B_GHOST);
      KEY_CASE(k2, P2_QUIT, B_QUIT);
      KEY_CASE(k2, P2_LEFT, B_LEFT);
      KEY_CASE(k2, P2_RIGHT, B_RIGHT);
      KEY_CASE(k2, P2_ROTL, B_ROTL);
      KEY_CASE(k2, P2_ROTR, B_ROTR);
      KEY_CASE(k2, P2_SDROP, B_SDROP);
      KEY_CASE(k2, P2_HDROP, B_HDROP);
    }
#undef KEY_CASE
    
  }

  attr.c_lflag |= ICANON;
  attr.c_lflag |= ECHO;

  if ( tcsetattr( 0, TCSADRAIN, &attr ) ) {
    perror( "tcsetattr (reset)" );
  }

  return (k2 << 8) + k1;
}

static size_t buffer_len = 0;
static char buffer[10000] = {0};

/* Write contents of buffer to stdout */
void blip() {
  chk(buffer_len <= sizeof(buffer));
  
  int bytes_written = 0;
  while (bytes_written < buffer_len) {
    int result = write(1, buffer + bytes_written, buffer_len - bytes_written);

    chk(result >= 0);
    bytes_written += result;
  }

  buffer_len = 0;
  buffer[0] = 0;

  chk(buffer_len == 0);
}

void putchr(char c) {
  chk(buffer_len < sizeof(buffer));
  
  buffer[buffer_len++] = c;
  if ( buffer_len == sizeof(buffer) ) {
    blip();
  }
}

void putstr(char* s) {
  chk(s != NULL);
  chk(strlen(s) < 64); /* If we're given a large string, probably an error */
  
  for ( size_t i=0; s[i]; i++ ) {
    putchr(s[i]);
  }
}

void cursorOn() {
  putstr("\033[?25h");
}

void cursorOff() {
  putstr("\033[?25l");
}

void cursorRt(int n) {
  chk(n >= 0);
  chk(buffer_len < sizeof(buffer));
  
  snprintf(buffer+buffer_len, sizeof(buffer)-buffer_len, "\033[%dC", n);
  buffer_len += strlen(buffer+buffer_len);

  chk(buffer_len <= sizeof(buffer));
}

void cursorUp(int n) {
  chk(n >= 0);
  chk(buffer_len < sizeof(buffer));
  
  snprintf(buffer+buffer_len, sizeof(buffer)-buffer_len, "\033[%dA", n);
  buffer_len += strlen(buffer+buffer_len);

  chk(buffer_len <= sizeof(buffer));
}

void cursorDn(int n) {
  chk(n >= 0);
  chk(buffer_len < sizeof(buffer));
  
  snprintf(buffer+buffer_len, sizeof(buffer)-buffer_len, "\033[%dB", n);
  buffer_len += strlen(buffer+buffer_len);

  chk(buffer_len <= sizeof(buffer));
}

#endif
