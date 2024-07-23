#include "s21_cat.h"

#include <ctype.h>
#include <getopt.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  opt_t opt = parse_arguments(argc, argv);
  if (!opt.error) {
    for (int i = optind; i < argc; i++) {
      FILE* file = fopen(argv[i], "r");
      if (file == NULL) {
        perror(argv[i]);
      } else {
        printFile(file, &opt);
        fclose(file);
      }
    }
  }
  return 0;
}

int getLine(unsigned char** line, unsigned long* len, FILE* fp) {
  unsigned char c;
  unsigned long capacity = 4, size = 0;

  if (*line != NULL) free(*line);
  *line = (unsigned char*)calloc(capacity, sizeof(unsigned char));

  while ((c = fgetc(fp)) != '\n' && !feof(fp) && !ferror(fp)) {
    if (size + 2 == capacity) {
      capacity *= 2;
      unsigned char* tmp = calloc(capacity, sizeof(unsigned char));

      for (unsigned long i = 0; i < size; i++) {
        tmp[i] = (*line)[i];
      }
      free(*line);
      *line = tmp;
    }
    (*line)[size++] = c;
  }
  if (c == '\n') {
    (*line)[size++] = c;
  }
  int result = 0;
  if (size == 0 && (feof(fp) || ferror(fp))) {
    result = -1;
  }
  *len = size;
  return result;
}

void print_visible(unsigned char c) {
  if (c == 127) {
    printf("^?");
  } else if (c == 255) {
    printf("M-^?");
  } else if (iscntrl(c)) {
    printf("^%c", c + 64);
  } else if (c > 127 && c < 160) {
    printf("M-^%c", c - 64);
  } else if (c > 159) {
    printf("M-%c", c - 128);
  } else
    printf("%c", c);
}

void printLine(unsigned char* line, unsigned long len, opt_t* opt) {
  for (unsigned long i = 0; i < len; i++) {
    if (opt->e && line[i] == '\n') {
      printf("$");
    }
    if (line[i] == '\t') {
      if (opt->t)
        printf("^I");
      else
        printf("\t");
    } else if (opt->v && (!isprint(line[i])) && line[i] != '\n') {
      print_visible(line[i]);
    } else
      printf("%c", line[i]);
  }
}

void printFile(FILE* fp, opt_t* opt) {
  opt->error = 0;
  unsigned long len = 0;
  unsigned char* line = NULL;
  int line_number = 1, skip_line = 0;

  while (getLine(&line, &len, fp) != -1) {
    if (opt->s && line[0] == '\n' && skip_line) {
      continue;
    }
    if (opt->n || (opt->b && line[0] != '\n')) {
      printf("%6.d\t", line_number);
      line_number++;
    }
    printLine(line, len, opt);
    if (line[0] == '\n') {
      skip_line = 1;
    } else
      skip_line = 0;
  }
  if (line) free(line);
}

opt_t parse_arguments(int argc, char** argv) {
  opt_t opt = {0};

  const char short_options[] = "bevnstET";
  struct option long_options[] = {{"number-nonblank", 0, NULL, 'b'},
                                  {"number", 0, NULL, 'n'},
                                  {"squeeze-blank", 0, NULL, 's'},
                                  {0, 0, 0, 0}};

  int op = 0;

  while ((op = getopt_long(argc, argv, short_options, long_options, NULL)) !=
         -1) {
    switch (op) {
      case 'b':
        opt.b = 1;
        opt.n = 0;
        break;
      case 's':
        opt.s = 1;
        break;
      case 'v':
        opt.v = 1;
        break;
      case 'n':
        opt.n = !opt.b;
        break;
      case 'e':
        opt.e = 1;
        opt.v = 1;
        break;
      case 't':
        opt.t = 1;
        opt.v = 1;
        break;
      case 'E':
        opt.e = 1;
        break;
      case 'T':
        opt.t = 1;
        break;
      default:
        opt.error = 1;
        break;
    }
  }

  return opt;
}