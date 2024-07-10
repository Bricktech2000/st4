#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

unsigned curr = 0;
unsigned nlines = 0;
char lines[65536][256] = {0};
unsigned ncut = 0;
char cut[256][256] = {0};

int main(int argc, char **argv) {
  if (argc != 2)
    fputs("Usage: st2 <filename>\n", stdout), exit(EXIT_FAILURE);

  char buf[64] = "e\n";
  char *bufp = buf;

  while(!feof(stdin)) {
    if (*bufp == '\n') {
      putchar(':');
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

    FILE *fp = NULL;

    switch (*bufp) {
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
      break;
    case 'y':
      memcpy(*cut, lines[curr], (ncut = cnt) * sizeof(*cut));
      break;
    case 'x':
      memmove(lines[curr + cnt * ncut], lines[curr], (nlines - curr) * sizeof(*lines));
      nlines += cnt * ncut;
    case 'e':
      fp = fopen(argv[1], "r");
      if (fp == NULL)
        perror("fopen"), exit(EXIT_FAILURE);
      break;
    case 'w':
      fp = fopen(argv[1], "w");
      if (fp == NULL)
        perror("fopen"), exit(EXIT_FAILURE);
      break;
    case 'q':
      exit(EXIT_SUCCESS);
    }

    for (unsigned n = 0; n < cnt; n++) {
      switch (*bufp) {
      case 'a':
      case 'i':
      case 'c':
        fgets(lines[curr + n], sizeof(*lines), stdin);
        break;
      case 'n':
        if (*lines[curr + n])
          printf("%5u ", curr + n + 1); // 65536 is 5 chars long
      case 'p':
        fputs(lines[curr + n], stdout);
        break;
      case 'x':
        memcpy(lines[curr + n * ncut], *cut, ncut * sizeof(*cut));
        break;
      case 'e':
        if (fgets(lines[n], sizeof(*lines), fp) != NULL)  
          nlines = ++cnt;
        break;
      case 'w':
        cnt = nlines;
        fputs(lines[n], fp);
        break;
      }
    }

    switch (*bufp) {
    case 'g':
      curr = cnt - 1;
      break;
    case 'j':
    case 'a':
    case 'c':
    case 'n':
    case 'p':
      curr += cnt;
      break;
    case 'k':
      curr -= cnt;
      break;
    case 'e':
    case 'w':
      if (fclose(fp) == EOF)
        perror("fclose"), exit(EXIT_FAILURE);
    }

    bufp++;
  }
}
