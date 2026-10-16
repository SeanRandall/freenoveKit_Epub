#ifndef PARSE_H
#define PARSE_H

static char attr[17] = {0};
static char comm[9]  = {0};

static int  s2n(const char *s, const char *e);
static void s2s(const char *s, const char *e);
static void constructCommand(char l, int num);
static void writeCommand(char* MARKER, char* CURSOR);
void parse(char* YYCURSOR);

#endif
