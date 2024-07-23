#include <stdio.h>

#ifndef S21_CAT_H

typedef struct options_s {
  int b, e, v, n, s, t, error;
} opt_t;

opt_t parse_arguments(int, char**);
void printFile(FILE*, opt_t*);

#endif