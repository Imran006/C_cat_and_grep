#define _GNU_SOURCE
#include "s21_grep.h"

#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  opt_t options = {0};
  templates_t templates;
  templates.capacity = 2;
  templates.size = 0;
  templates.data = malloc(sizeof(char *) * templates.capacity);
  if (templates.data == NULL) {
    printf("error11111");
  } else {
    getFlags(argc, argv, &options, &templates);
    if (!options.error) {
      readFiles(argc, argv, &templates, &options);
    }
    if (templates.data != NULL) {
      for (unsigned i = 0; i < templates.size; i++) {
        if (templates.data[i] != NULL) {
          free(templates.data[i]);
        }
      }
      free(templates.data);
    }
  }
  return 0;
}

int replaceAdd(regmatch_t *matches, unsigned numMatches, regmatch_t match) {
  int needAdd = 1;
  for (unsigned int i = 0; i < numMatches; i++) {
    if (matches[i].rm_so == match.rm_so) {
      needAdd = 0;
      if (matches[i].rm_eo < match.rm_eo) {
        matches[i] = match;
      }
    }
  }
  return needAdd;
}

int compareMatches(const void *l, const void *r) {
  return ((regmatch_t *)l)->rm_so - ((regmatch_t *)r)->rm_eo;
}

void printMatches(char *line, regmatch_t *matches, unsigned numMatches,
                  char *filename, int filesCount, int lineNum, opt_t *options) {
  if (options->o && numMatches > 0) {
    qsort(matches, numMatches, sizeof(regmatch_t), compareMatches);
    for (unsigned int i = 0; i < numMatches; i++) {
      if (!options->h && filesCount > 1) {
        printf("%s:", filename);
      }
      if (options->n) {
        printf("%d:", lineNum);
      }
      for (int j = matches[i].rm_so; j < matches[i].rm_eo; j++) {
        printf("%c", line[j]);
      }
      printf("\n");
    }
  }
}

int processLine(char *line, char *filename, opt_t *options,
                templates_t *templates, int flag, int lineNum, int filesCount) {
  int matched = 0;
  unsigned matchesCapacity = 2, numMatches = 0;
  regmatch_t *matches =
      (regmatch_t *)malloc(matchesCapacity * sizeof(regmatch_t));
  if (matches == NULL) {
    options->error = 1;
  }

  regex_t reg;
  for (unsigned i = 0; i < templates->size; i++) {
    int shift = 0;
    regmatch_t match;
    regcomp(&reg, templates->data[i], flag);
    while (regexec(&reg, line + shift, 1, &match, 0) == 0) {
      matched = 1;
      if (!options->o || (options->v && options->o) || options->l ||
          options->c) {
        break;
      }
      if (numMatches + 1 == matchesCapacity) {
        matchesCapacity *= 2;
        regmatch_t *tmp =
            (regmatch_t *)malloc(sizeof(regmatch_t) * matchesCapacity);
        if (tmp == NULL) {
          free(matches);
          regfree(&reg);
          options->error = 1;
        }
        for (unsigned i = 0; i < numMatches; i++) {
          tmp[i] = matches[i];
        }
        free(matches);
        matches = tmp;
      }
      match.rm_so += shift;
      match.rm_eo += shift;
      shift = match.rm_eo;
      int needAdd = replaceAdd(matches, numMatches, match);
      if (needAdd) {
        matches[numMatches++] = match;
      }
    }
    regfree(&reg);
  }
  if (!options->c) {
    printMatches(line, matches, numMatches, filename, filesCount, lineNum,
                 options);
  }
  if (matches != NULL) free(matches);
  return matched;
}

void printFile(FILE *fp, char *filename, opt_t *options, templates_t *templates,
               int flag, int filesCount) {
  unsigned long lineNum = 1, count = 0;
  unsigned long len = 0;
  char *line = NULL;

  while (getline(&line, &len, fp) != -1) {
    int matched = processLine(line, filename, options, templates, flag, lineNum,
                              filesCount);
    int trueMatched = (!options->v && matched) || (options->v && !matched);
    if (!options->o && !options->c && !options->l && trueMatched) {
      if (options->n && filesCount > 1) {
        printf("%s:", filename);
      } else if (options->n)
        printf("%lu:", lineNum);
      if (options->i && options->v && filesCount > 1) printf("%s:", filename);
      printf("%s", line);
    }
    count += trueMatched;
    ++lineNum;
  }
  free(line);

  if (options->l && count > 0) {
    printf("%s\n", filename);
  } else if (options->c) {
    if (!options->h && filesCount > 1) {
      printf("%s:", filename);
    }
    printf("%lu\n", count);
  }
}

void readFiles(int argc, char **argv, templates_t *templates, opt_t *options) {
  int flag = getFlag(options);
  if (templates->size == 0 && optind < argc) {
    int len = strlen(argv[optind]);
    templates->data[templates->size] = calloc(len + 1, sizeof(char));
    if (templates->data[templates->size] == NULL) {
      options->error = 1;
    }
    strcpy(templates->data[templates->size], argv[optind]);
    templates->size++;
    optind++;
  }

  for (int i = optind; i < argc; i++) {
    FILE *fp = fopen(argv[i], "r");
    if (fp == NULL) {
      if (!options->s) {
        fprintf(stderr, "grep: %s: No such file or directory\n", argv[i]);
      }
    } else {
      printFile(fp, argv[i], options, templates, flag, argc - optind);
      fclose(fp);
    }
  }
}

int getFlag(const opt_t *options) {
  int flag = 0;
  if (options->e) {
    flag |= REG_EXTENDED;
  }
  if (options->i) {
    flag |= REG_ICASE;
  }
  return flag;
}

void getFlags(int argc, char **argv, opt_t *options, templates_t *templates) {
  char *short_options = "e:f:ivnclhos";
  int op;

  while ((op = getopt(argc, argv, short_options)) != -1) {
    switch (op) {
      case 'e':
        options->e = 1;
        parseTemplate(templates, options);
        break;
      case 'f':
        options->f = 1;
        parseTemplateFile(templates, options);
        break;
      case 'i':
        options->i = 1;
        break;
      case 'v':
        options->v = 1;
        break;
      case 'n':
        options->n = 1;
        break;
      case 'c':
        options->c = 1;
        break;
      case 'l':
        options->l = 1;
        break;
      case 'h':
        options->h = 1;
        break;
      case 's':
        options->s = 1;
        break;
      case 'o':
        options->o = 1;
        break;
      default:
        options->error = 1;
        break;
    }
  }
}

void checkCapacity(templates_t *templates, opt_t *options) {
  if (templates->size + 1 == templates->capacity) {
    templates->capacity *= 2;
    char **tmp = realloc(templates->data, sizeof(char *) * templates->capacity);
    if (tmp == NULL) {
      options->error = 1;
    } else {
      templates->data = tmp;
    }
  }
}

void parseTemplate(templates_t *templates, opt_t *options) {
  checkCapacity(templates, options);
  unsigned long len = strlen(optarg);
  templates->data[templates->size] = calloc(len + 1, sizeof(char));
  if (templates->data[templates->size] == NULL) {
    options->error = 1;
  } else {
    strcpy(templates->data[templates->size], optarg);
    templates->size++;
  }
}

void parseTemplateFile(templates_t *templates, opt_t *options) {
  FILE *fp = fopen(optarg, "r");
  if (fp == NULL) {
    fprintf(stderr, "grep: %s: No such file or directory\n", optarg);
    options->error = 1;
  } else {
    unsigned long len = 0;
    ssize_t size;
    char *line = NULL;
    while ((size = getline(&line, &len, fp)) != -1) {
      if (size > 0 && line[size - 1] == '\n') {
        line[size - 1] = '\0';
        size--;
      }
      checkCapacity(templates, options);
      templates->data[templates->size] = malloc((size + 1) * sizeof(char));
      if (templates->data[templates->size] == NULL) {
        free(line);
        fclose(fp);
        options->error = 1;
        return;
      } else {
        strcpy(templates->data[templates->size], line);
        templates->size++;
      }
    }
    if (line != NULL) {
      free(line);
    }
    fclose(fp);
  }
}
