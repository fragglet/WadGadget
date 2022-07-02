#include <stdio.h>

#ifndef UNIMP
#define UNIMP \
  fprintf(stderr, "%s: Call to unimplemented function!\n", \
          __func__)
#endif

