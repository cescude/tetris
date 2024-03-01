#ifndef __CHK_H__
#define __CHK_H__

#include <execinfo.h>

void die(char *file, int line) {
  void *buffer[5] = {0};
  int sz = backtrace(buffer, sizeof(buffer)/sizeof(buffer[0]));
  backtrace_symbols_fd(buffer, sz, 1);
  
  fprintf(stderr, "FAILURE: %s:%d\n", file, line);
  exit(99);
}

#define chk(blk) {				\
    if (!(blk)) die(__FILE__, __LINE__);	\
  }

void chk_bool(int b) {
  chk(b == 0 || b == 1);
}

#endif
