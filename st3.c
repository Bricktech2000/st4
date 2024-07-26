#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HI_DEFAULT "\033[m" // (reset)
#define HI_KEYWORD "\033[;1m" // bold
#define HI_COMMENT "\033[;2;3m" // dim italic
#define HI_OPERATOR "\033[;2m" // dim
#define HI_LITERAL "\033[;3m" // italic
#define HI_LINENO "\033[;2;3m" // dim italic

int isident(int c) { return c == '_' || isalnum(c); }
char *hi_c(char **src) {
  static char *kws[] = {
    "auto", "break", "case", "char", "const", "continue",
    "default", "do", "double", "else", "enum", "extern",
    "float", "for", "goto", "if", "inline", "int",
    "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while", "_Bool", "_Complex",
    "_Imaginary", NULL,
  };
  static char *pps[] = {
    "if", "elif", "else", "endif", "ifdef", "ifndef",
    "define", "undef", "include", "line", "error", "pragma",
    NULL,
  };
  static char *ops[] = {
    "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=",
    "^=", "<<=", ">>=", "++", "--", "+", "-", "*",
    "/", "%", "~", "&", "|", "^", "<<", ">>",
    "!", "&&", "||", "==", "!=", "<", ">", "<=",
    ">=", "[", "]", "->", ".", "(", ")", ",",
    "?", ":", "sizeof", NULL,
  };

  if (isspace(**src))
    return ++*src, HI_DEFAULT;

  if (strncmp(*src, "//", 2) == 0) {
    while (**src && *++*src != '\n');
    return HI_COMMENT;
  }

  if (strncmp(*src, "/*", 2) == 0) {
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

  for (char **op = ops; *op; op++)
    if (strncmp(*src, *op, strlen(*op)) == 0)
      return *src += strlen(*op), HI_OPERATOR;

  if (**src == '"' || **src == '\'') {
    char quote = **src;
    while (**src && **src != '\n' && *++*src != quote)
      if (**src == '\\')
        ++*src;
    if (**src)
      *src += 1;
    return HI_LITERAL;
  }

  if (**src == '#') {
    while (isspace(*++*src));
    for (char **pp = pps; *pp; pp++)
      if (strncmp(*src, *pp, strlen(*pp)) == 0)
        if (!isident((*src)[strlen(*pp)]))
          return *src += strlen(*pp), HI_KEYWORD;
    return HI_DEFAULT;
  }

  return ++*src, HI_DEFAULT;
}

char *hi_help(char **src) {
  if (**src == '<') {
    while (**src && !isspace(**src) && *++*src != '>');
    if (**src == '>')
      return ++*src, HI_KEYWORD;
    return HI_DEFAULT;
  }

  return ++*src, HI_DEFAULT;
}

#define HELP "\n"                                                              \
  "<up>\tGo to previous line.\n"                                               \
  "<down>\tGo to next line.\n"                                                 \
  "<left>\tGo back one character.\n"                                           \
  "<right>\tGo forward one character.\n"                                       \
  "<pgup>\tScroll up one line.\n"                                              \
  "<pgdn>\tScroll down one line.\n"                                            \
  "<bs>\tDelete character left of cursor.\n"                                   \
  "<del>\tDelete character under cursor.\n"                                    \
  "\n"                                                                         \
  "<c-q>\tQuit without confirmation.\n"                                        \
  "<c-h>\tDisplay this help page.\n"                                           \
  "<c-l>\tReinitialize terminal and redraw.\n"                                 \
  "<c-o>\tRead current file from disk into buffer.\n"                          \
  "<c-s>\tWrite buffer into current file on disk.\n"                           \
  "\n"                                                                         \
  "Keystrokes not listed above are inserted into\n"                            \
  "the buffer left of the cursor.\n"                                           \

void render(char *screen, unsigned lineno, char *cursor, char *(*hi)(char **),
            unsigned rows, unsigned cols) {
  unsigned row = 0, col = 0;
  unsigned crow = 0, ccol = 0;
  fputs("\033[H\033[K" HI_LINENO, stdout); // home cursor and erase line
  col = printf(screen[-1] == '\n' ? "%5u " : "      ", ++lineno);

  char *last_hi, *line = screen;
  for (char *start = screen; *start; start = screen) {
    fputs(last_hi = hi(&screen), stdout);

    for (; start < screen; start++) {
      int niter = *start == '\t' ? 8 - (start - line) % 8 : 1;
      for (; niter > 0; niter--) {
        if (col >= cols) {
          if (++row >= rows)
            goto brk;
          fputs("\n\033[K" HI_LINENO, stdout), col = printf("      ");
          fputs(last_hi, stdout); // to preserve highlight across line wraps
        }

        if (start == cursor)
          crow = row, ccol = col;

        if (*start == '\n') {
          if (++row >= rows)
            goto brk;
          fputs("\n\033[K" HI_LINENO, stdout), col = printf("%5u ", ++lineno);
          fputs(last_hi, stdout); // for '\n' within a highlight region
          line = start + 1;
        } else if (*start == '\t')
          putchar(' '), col++;
        else
          putchar(*start), col++;
      }
    }
  }
brk:

  if (screen == cursor)
    crow = row, ccol = col;

  while (++row < rows)
    fputs("\n\033[K" HI_LINENO, stdout), col = printf("~     ");

  printf("\033[%u;%uH", crow + 1, ccol + 1); // move cursor
}

enum key {
  KEY_UNK,
  CTRL_A = 1,
  CTRL_C = 3,
  CTRL_H = 8,
  KEY_TAB = 9,
  CTRL_L = 12,
  KEY_ENTER = 13,
  CTRL_O = 15,
  CTRL_Q = 17,
  CTRL_S = 19,
  CTRL_V = 22,
  CTRL_X = 24,
  CTRL_Z = 26,
  KEY_BS = 127,
  KEY_DEL = 256,
  ARR_UP,
  ARR_DOWN,
  ARR_LEFT,
  ARR_RIGHT,
  KEY_PGUP,
  KEY_PGDN,
  KEY_HOME,
  KEY_END,
};

enum key getkey(void) {
  char c;

  if ((c = getchar()) != '\033')
    return c;

  if ((c = getchar()) != '[')
    return c;

  switch (c = getchar()) {
  case 'A':
    return ARR_UP;
  case 'B':
    return ARR_DOWN;
  case 'C':
    return ARR_RIGHT;
  case 'D':
    return ARR_LEFT;
  case 'H':
    return KEY_HOME;
  case 'F':
    return KEY_END;
  }

  if (c < '0' || c > '9')
    return KEY_UNK;

  if (getchar() != '~')
    return KEY_UNK;

  switch (c) {
  case '3':
    return KEY_DEL;
  case '5':
    return KEY_PGUP;
  case '6':
    return KEY_PGDN;
  }

  return KEY_UNK;
}

void reinit(unsigned *rows, unsigned *cols) {
  // XXX stty -icanon -echo -nl
  unsigned row = 0, col = 0;
  do {
    *rows = row, *cols = col;
    fputs("\033[99B\033[99C", stdout); // move down then right
    fputs("\033[6n", stdout); // request cursor position
    if (scanf("\033[%u;%uR", &row, &col) != 2) // parse response
      fputs("Could not query cursor position\n", stdout), exit(EXIT_FAILURE);
  } while (row != *rows || col != *cols);
}

enum key seq[8] = {260, 259, 260, 259, 258, 258, 257, 257};

void *memrchr(void *s, int c, size_t n) {
  unsigned char *cp = (unsigned char *)s + n;
  for (; n > 0; n--)
    if (*--cp == (unsigned char)c)
      return cp;
  return NULL;
}

void *clamp_memchr(void *s, int c, size_t n) {
  void *p = memchr(s, c, n);
  return p ? p : (unsigned char *)s + n;
}

void *clamp_memrchr(void *s, int c, size_t n) {
  void *p = memrchr(s, c, n);
  return p ? p : (unsigned char *)s - 1;
}

int main(int argc, char **argv) {
  if (argc != 2)
    fputs("Usage: st3 <filename>\n", stdout), exit(EXIT_FAILURE);

  unsigned rows = 0, cols = 0;
  reinit(&rows, &cols);

  size_t cursor = 0, virtual = cursor;
  size_t screen = 0;
  char *buf = (char *)1, *line, *other;
  long size = 0;
  unsigned lineno = 0;
  enum key key = CTRL_O, keys[8] = {0};

  while (1) {
    switch (key) {
    case CTRL_Q:
      goto brk;
    case CTRL_H:
      render(HELP + 1, 0, NULL, hi_help, rows, cols);
      getkey();
      break;
    case CTRL_L:
      reinit(&rows, &cols);
      break;
    case CTRL_O: {
      FILE *fp = fopen(argv[1], "r");
      if (fp == NULL)
        perror("fopen"), exit(EXIT_FAILURE);
      if (fseek(fp, 0, SEEK_END) == -1)
        perror("fseek"), exit(EXIT_FAILURE);
      if ((size = ftell(fp)) == -1)
        perror("ftell"), exit(EXIT_FAILURE);
      rewind(fp);

      free(--buf), buf = malloc(size + 2), *buf++ = '\n';
      if (fread(buf, sizeof(*buf), size, fp) != size)
        perror("fread"), exit(EXIT_FAILURE);
      buf[size] = '\0';
      if (fclose(fp) == EOF)
        perror("fclose"), exit(EXIT_FAILURE);
    } break;
    case CTRL_S: {
      FILE *fp = fopen(argv[1], "w");
      if (fp == NULL)
        perror("fopen"), exit(EXIT_FAILURE);
      if (fwrite(buf, sizeof(*buf), size, fp) != size)
        perror("fwrite"), exit(EXIT_FAILURE);
      if (fclose(fp) == EOF)
        perror("fclose"), exit(EXIT_FAILURE);
    } break;
    case ARR_UP:
      line = (char *)clamp_memrchr(buf, '\n', cursor) + 1;
      if (line == buf)
        break;
      other = (char *)clamp_memrchr(buf, '\n', line - 1 - buf) + 1;
      goto virtual;
    case ARR_DOWN:
      line = (char *)clamp_memrchr(buf, '\n', cursor) + 1;
      other = (char *)clamp_memchr(buf + cursor, '\n', size - cursor) + 1;
      if (other == buf + size + 1)
        break;
    virtual:
      virtual += other - line; // not UB because `virtual` is not a pointer
      cursor = (char *)clamp_memchr(other, '\n', buf + size - other) - buf;
      cursor = cursor < virtual ? cursor : virtual;
      break;
    case ARR_LEFT:
      if (cursor == 0)
        break;
      virtual = --cursor;
      break;
    case ARR_RIGHT:
      if (cursor == size)
        break;
      virtual = ++cursor;
      break;
    case KEY_PGUP:
      if (screen == 0)
        break;
      screen = (char *)clamp_memrchr(buf, '\n', screen - 1) + 1 - buf;
      lineno--;
      break;
    case KEY_PGDN:
      if (screen == size + 1)
        break;
      screen = (char *)clamp_memchr(buf + screen, '\n', size - screen) + 1 - buf;
      lineno++;
      break;
    case KEY_DEL:
      if (cursor == size)
        break;
      cursor++;
    case KEY_BS:
      if (cursor == 0)
        break;
      memmove(buf + cursor - 1, buf + cursor, size - cursor + 1);
      buf = realloc(--buf, --size + 2), buf++;
      virtual = --cursor;
      break;
    case KEY_ENTER:
      key = '\n';
    default:
      buf = realloc(--buf, ++size + 2), buf++;
      memmove(buf + cursor + 1, buf + cursor, size - cursor);
      buf[cursor] = key, virtual = ++cursor;
    }

    render(buf + screen, lineno, buf + cursor, hi_c, rows, cols);

    memmove(keys + 1, keys, sizeof(keys) - sizeof(*keys));
    *keys = key = getkey();
    if (memcmp(seq, keys, sizeof(keys)) == 0)
      key = '\b'; // XXX nonportable
  }
brk:

  fputs("\033[2J\033[H", stdout); // clear screen and home cursor
  free(--buf);
}
