#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>

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
  "<home>\tGo to beginning of line.\n"                                         \
  "<end>\tGo to end of line.\n"                                                \
  "<bs>\tDelete character left of cursor.\n"                                   \
  "<del>\tDelete character under cursor.\n"                                    \
  "<c-x>\tInsert character from two hex digits.\n"                             \
  "\n"                                                                         \
  "<c-q>\tQuit without confirmation.\n"                                        \
  "<c-h>\tDisplay this help page.\n"                                           \
  "<c-l>\tReinitialize terminal and redraw.\n"                                 \
  "<c-o>\tRead current file from disk into buffer.\n"                          \
  "<c-s>\tWrite buffer into current file on disk.\n"                           \
  "\n"                                                                         \
  "<c-t>\tCycle through tab widths.\n"                                         \
  "<c-n>\tToggle line numbers.\n"                                              \
  "\n"                                                                         \
  "Keystrokes not listed above are inserted into\n"                            \
  "the buffer left of the cursor.\n"                                           \

char *hi_txt(char **src) { return ++*src, HI_DEFAULT; }

struct ext2hi {
  char *ext;
  char *(*hi)(char **);
} ext2hi[] = {
  { .ext = ".c", .hi = hi_c },
  { .ext = ".txt", .hi = hi_txt },
};

struct opts {
  unsigned rows, cols;
  char *(*hi)(char **);
  unsigned tabstop;
  bool number;
};

void render(char *screen, size_t size, unsigned lineno, char *cursor,
            struct opts opts) {
  unsigned row = 0, col = 0;
  unsigned crow = opts.rows, ccol = opts.cols;
  fputs("\033[?25l", stdout); // hide cursor
  fputs("\033[H\033[K" HI_LINENO, stdout); // home cursor and erase line
  if (opts.number)
    col = screen[-1] == '\n' ? printf("%5u ", ++lineno) : printf("%5s ", "");

  char *last_hi, *line = screen, *end = screen;
  for (char *start = end; start < screen + size; start = end) {
    fputs(last_hi = opts.hi(&end), stdout);

    for (; start < end; start++) {
      int len = *start == '\n'  ? 1
              : *start == '\t'  ? opts.tabstop - (start - line) % opts.tabstop
              : isprint(*start) ? 1 : 4; // "\\xHH"

      if (col + len > opts.cols) {
        if (++row >= opts.rows)
          goto brk;
        fputs("\n\033[K" HI_LINENO, stdout);
        col = printf(opts.number ? "%5s " : "", "");
        fputs(last_hi, stdout); // to preserve highlight across softwraps
      }

      if (start == cursor)
        crow = row, ccol = col;

      if (*start == '\n') {
        if (++row >= opts.rows)
          goto brk;
        fputs("\n\033[K" HI_LINENO, stdout);
        col = printf(opts.number ? "%5u " : "", ++lineno);
        fputs(last_hi, stdout); // for '\n' within a highlight region
        line = start + 1;
      } else if (*start == '\t') {
        col += printf("%*s", len, "");
        line = start + 1;
      } else if (isprint(*start)) {
        putchar(*start), col++;
      } else {
        fputs(HI_LINENO, stdout);
        col += printf("\\x%02hhx", *start);
        fputs(last_hi, stdout); // to restore highlight after nonprintable
      }
    }
  }
brk:

  if (end == cursor)
    crow = row, ccol = col;

  while (++row < opts.rows)
    fputs("\n\033[K" HI_LINENO, stdout), col = printf("~     ");

  printf("\033[%u;%uH", crow + 1, ccol + 1); // move cursor
  fputs("\033[?25h", stdout); // unhide cursor
}

enum key {
  KEY_UNK,
  CTRL_C = 3,
  CTRL_H = 8,
  KEY_TAB = 9,
  CTRL_L = 12,
  KEY_ENTER = 13,
  CTRL_N = 14,
  CTRL_O = 15,
  CTRL_Q = 17,
  CTRL_S = 19,
  CTRL_T = 20,
  CTRL_W = 23,
  CTRL_X = 24,
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

enum key seq[8] = {260, 259, 260, 259, 258, 258, 257, 257};

struct termios reinit(unsigned *rows, unsigned *cols) {
  struct termios orig;
  if (tcgetattr(STDIN_FILENO, &orig) != 0)
    perror("tcgetattr"), exit(EXIT_FAILURE);

  // stty -icanon -echo -nl
  struct termios term = orig;
  term.c_lflag &= ~ICANON & ~ECHO;
  term.c_iflag &= ~INLCR & ~IGNCR;
  term.c_oflag &= ~OCRNL & ~ONLRET;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term) != 0)
    perror("tcsetattr"), exit(EXIT_FAILURE);

  fputs("\033[?25l", stdout); // hide cursor
  unsigned row = 0, col = 0;
  do {
    *rows = row, *cols = col;
    fputs("\033[99B\033[99C", stdout); // move down then right
    fputs("\033[6n", stdout); // request cursor position
    if (scanf("\033[%u;%uR", &row, &col) != 2) // parse response
      fputs("Could not query cursor position\n", stdout), exit(EXIT_FAILURE);
  } while (row != *rows || col != *cols);
  fputs("\033[?25h", stdout); // unhide cursor

  return orig;
}

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
    fputs("Usage: st4 <filename>\n", stdout), exit(EXIT_FAILURE);

  struct opts opts = {
    .hi = hi_txt,
    .tabstop = 8,
    .number = true,
  };

  fputs("\033[?1049h", stdout); // switch to alternate buffer
  struct termios orig = reinit(&opts.rows, &opts.cols);

  char *ext = strrchr(argv[1], '.');
  for (int i = 0; i < sizeof(ext2hi) / sizeof(*ext2hi); i++)
    if (ext && strcmp(ext, ext2hi[i].ext) == 0)
      opts.hi = ext2hi[i].hi;

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
      render(HELP + 1, sizeof(HELP) - 2, 0, NULL, (struct opts){
        .rows = opts.rows,
        .cols = opts.cols,
        .hi = hi_help,
        .tabstop = 8,
        .number = false,
      });
      getkey();
      break;
    case CTRL_L:
      reinit(&opts.rows, &opts.cols);
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
    case CTRL_N:
      opts.number = !opts.number;
      break;
    case CTRL_T:
      // 2 -> 4 -> 6 -> 8 -> 10 -> 12
      opts.tabstop = opts.tabstop % 12 + 2;
      break;
    case ARR_UP:
    arr_up:
      line = (char *)clamp_memrchr(buf, '\n', cursor) + 1;
      if (line == buf)
        break;
      other = (char *)clamp_memrchr(buf, '\n', line - 1 - buf) + 1;
      goto virtual;
    case ARR_DOWN:
    arr_down:
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
      goto arr_up;
    case KEY_PGDN:
      if (screen == size + 1)
        break;
      screen = (char *)clamp_memchr(buf + screen, '\n', size - screen) + 1 - buf;
      lineno++;
      goto arr_down;
    case KEY_HOME:
      if (cursor == 0)
        break;
      virtual = cursor = (char *)clamp_memrchr(buf, '\n', cursor) + 1 - buf;
      break;
    case KEY_END:
      if (cursor == size)
        break;
      virtual = cursor = (char *)clamp_memchr(buf + cursor, '\n', size - cursor) - buf;
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
    case CTRL_X:
      key = 0;
      for (int chr, i = 0; i < 2; i++) {
        key <<= 4;
        while (!isxdigit(chr = getchar()))
          putchar('\a');
        if (isdigit(chr))
          key |= chr - '0';
        else
          key |= tolower(chr) - 'a' + 10;
      }
      goto insert;
    default:
      if (isprint(key) || isspace(key))
        goto insert;
      putchar('\a');
      break;
    insert:
      buf = realloc(--buf, ++size + 2), buf++;
      memmove(buf + cursor + 1, buf + cursor, size - cursor);
      buf[cursor] = key, virtual = ++cursor;
    }

    render(buf + screen, size - screen, lineno, buf + cursor, opts);

    memmove(keys + 1, keys, sizeof(keys) - sizeof(*keys));
    *keys = key = getkey();
    if (memcmp(seq, keys, sizeof(keys)) == 0)
      key = '\b'; // too far
  }
brk:

  free(--buf);

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig) != 0)
    perror("tcsetattr"), exit(EXIT_FAILURE);
  fputs("\033[?1049l", stdout); // switch back to main buffer
}
