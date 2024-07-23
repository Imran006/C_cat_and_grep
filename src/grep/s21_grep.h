#ifndef S21_GREP_H

typedef struct {
  int e, i, v, c, l, n, h, s, f, o, error;
} opt_t;

typedef struct {
  char **data;
  unsigned long size, capacity;
} templates_t;

void getFlags(int, char **, opt_t *, templates_t *);
void readFiles(int, char **, templates_t *, opt_t *);
int getFlag(const opt_t *);
void parseTemplate(templates_t *, opt_t *);
void parseTemplateFile(templates_t *, opt_t *);
int processLine(char *, char *, opt_t *, templates_t *, int, int, int);

#endif