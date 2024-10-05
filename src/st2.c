#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// g - go to line number N
// j - move down N lines
// k - move up N lines
// a - append N new lines below
// i - insert N new lines above
// c - rewrite next N lines
// d - delete next N lines
// y - yank (copy) next N lines
// x - paste last yank N times
// n - print next N lines numbered
// p - print next N lines
// e - read file from disk
// w - write file to disk
// q - quit without confirmation

#define HI_DEFAULT "\033[m" // (reset)
#define HI_LINENO "\033[;2;3m" // dim italic

#define HI_KEYWORD "\033[;1m" // bold
#define HI_COMMENT "\033[;2;3m" // dim italic
#define HI_OPERATOR "\033[;2m" // dim
#define HI_LITERAL "\033[;3m" // italic

int isident(int c) { return c == '_' || isalnum(c); }
char *hi_c(char **src) {
  // ISO/IEC 9899:TC3, $6.4.1 'Keywords'
  static char *kws[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "inline", "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while", "_Bool", "_Complex", "_Imaginary",
    /* additional */ "bool", "true", "false", NULL,
  };
  // ISO/IEC 9899:TC3, $6.10 'Preprocessing directives'
  static char *pps[] = {
    "if", "ifdef", "ifndef", "elif", "else", "endif", "include", "define",
    "undef", "line", "error", "pragma", NULL,
  };
  // ISO/IEC 9899:TC3, $6.4.6 'Punctuators'
  static char *ops[] = {
    "[", "]", "(", ")", /* "{", "}", */ ".", "->",
    "++", "--", "&", "*", "+", "-", "~", "!",
    "/", "%", "<<", ">>", "<", ">", "<=", ">=", "==", "!=", "^", "|", "&&", "||",
    "?", ":", /* ";", */ "...",
    "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=",
    ",", "#", "##",
    "<:", ":>", "<%", "%>", "%:", "%:%:", NULL,
  };

  if (isspace(**src))
    return ++*src, HI_DEFAULT;

  if (strncmp(*src, "//", 2) == 0) {
    while (*++*src && **src != '\n');
    return HI_COMMENT;
  }

  if (strncmp(*src, "/*", 2) == 0) {
    *src += 1;
    while (*++*src && strncmp(*src, "*/", 2) != 0);
    if (**src)
      *src += 2;
    return HI_COMMENT;
  }

  for (char **kw = kws; *kw; kw++)
    if (strncmp(*src, *kw, strlen(*kw)) == 0)
      if (!isident((*src)[strlen(*kw)]))
        return *src += strlen(*kw), HI_KEYWORD;

  if (isdigit(**src)) {
    while (isident(*++*src));
    return HI_LITERAL;
  }

  if (isident(**src)) {
    while (isident(*++*src));
    return HI_DEFAULT;
  }

  if (**src == '#') {
    char *last = *src;
    while (isspace(*++*src));
    for (char **pp = pps; *pp; pp++)
      if (strncmp(*src, *pp, strlen(*pp)) == 0)
        if (!isident((*src)[strlen(*pp)]))
          return *src += strlen(*pp), HI_KEYWORD;
    *src = last;
  }

  for (char **op = ops; *op; op++)
    if (strncmp(*src, *op, strlen(*op)) == 0)
      return *src += strlen(*op), HI_OPERATOR;

  if (**src == '"' || **src == '\'') {
    char quote = **src;
    while (*++*src && **src != '\n' && **src != quote)
      if (**src == '\\')
        ++*src;
    if (**src)
      *src += 1;
    return HI_LITERAL;
  }

  return ++*src, HI_DEFAULT;
}

unsigned nlines = 0;
char lines[65536][256] = {0};
unsigned ncut = 0;
char cut[256][256] = {0};

int main(int argc, char **argv) {
  if (argc != 2)
    fputs("Usage: st2 <filename>\n", stdout), exit(EXIT_FAILURE);

  unsigned curr = 0;
  char buf[64] = "e\n";
  char *bufp = buf;

  while (!feof(stdin)) {
    if (*bufp == '\n') {
      fputs(HI_DEFAULT ":", stdout);
      bufp = fgets(buf, sizeof(buf), stdin);
      continue;
    }

    int nread;
    unsigned cnt;
    if (sscanf(bufp, "%u%n", &cnt, &nread) != 1)
      nread = 0, cnt = 1;
    bufp += nread;

    if (!strchr("gjkaicdyxnpewq", *bufp)) {
      printf("?\n");
      if (*bufp != '\n')
        bufp++;
      continue;
    }

    switch (*bufp) {
    case 'g':
      curr = cnt - 1;
      break;
    case 'j':
      curr += cnt;
      break;
    case 'k':
      curr -= cnt;
      break;
    case 'd':
      nlines -= cnt;
      memmove(lines[curr], lines[curr + cnt], (nlines - curr) * sizeof(*lines));
      memset(lines[nlines], 0, cnt * sizeof(*lines));
      break;
    case 'a':
      curr++;
    case 'i':
      memmove(lines[curr + cnt], lines[curr], (nlines - curr) * sizeof(*lines));
      nlines += cnt;
    case 'c':
      for (unsigned last = curr + cnt; curr < last; curr++)
        fgets(lines[curr], sizeof(*lines), stdin);
      break;
    case 'y':
      memcpy(*cut, lines[curr], (ncut = cnt) * sizeof(*cut));
      break;
    case 'x':
      memmove(lines[curr + cnt * ncut], lines[curr], (nlines - curr) * sizeof(*lines));
      nlines += cnt * ncut;
      for (unsigned last = curr + cnt * ncut; curr < last; curr += ncut)
        memcpy(lines[curr], *cut, ncut * sizeof(*cut));
      break;
    case 'n':
    case 'p':
      for (unsigned last = curr + cnt; curr < last; curr++) {
        if (*bufp == 'n' && *lines[curr])
          printf(HI_LINENO "%5u ", curr + 1); // 65536 is 5 chars long
        char *end = lines[curr];
        for (char *start = end; *start; start = end) {
          fputs(hi_c(&end), stdout);
          printf("%.*s", (int)(end - start), start);
        }
      }
      break;
    case 'e':
    case 'w': {
      FILE *fp = fopen(argv[1], *bufp == 'e' ? "r" : "w");
      if (fp == NULL)
        perror("fopen"), exit(EXIT_FAILURE);
      if (*bufp == 'e')
        for (nlines = 0; fgets(lines[nlines], sizeof(*lines), fp); nlines++);
      else
        for (unsigned n = 0; n < nlines; fputs(lines[n++], fp));
      if (fclose(fp) == EOF)
        perror("fclose"), exit(EXIT_FAILURE);
    } break;
    case 'q':
      exit(EXIT_SUCCESS);
    }

    bufp++;
  }
}
