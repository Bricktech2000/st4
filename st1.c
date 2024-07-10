#include <stdio.h>
#include <string.h>

unsigned nlines = 0;
unsigned curr = 0;
char lines[65536][256] = {0};

int main(int argc, char **argv) {
  FILE *fp = fopen(argv[1], "r+");

  ungetc('r', stdin);
  while(!feof(stdin)) {
    unsigned cnt;
    if (scanf("%u", &cnt) != 1)
      cnt = 1;

    char cmd;
    if (scanf("%c", &cmd) != 1 || !strchr("gjkaicdnprwq", cmd)) {
      printf("?\n");
      continue;
    }

    switch (cmd) {
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
      break;
    case 'r':
    case 'w':
      rewind(fp);
      break;
    case 'q':
      goto exit;
    }

    for (unsigned n = 0; n < cnt; n++) {
      switch (cmd) {
      case 'a':
      case 'i':
      case 'c':
        fgets(lines[curr + n], sizeof(*lines), stdin);
        break;
      case 'n':
        printf("%u:", curr + n + 1);
      case 'p':
        fputs(lines[curr + n], stdout);
        break;
      case 'r':
        if (fgets(lines[n], sizeof(*lines), fp) != NULL)  
          cnt++;
        nlines = cnt;
        break;
      case 'w':
        cnt = nlines;
        fputs(lines[n], fp);
        break;
      }
    }
  }

exit:
  fclose(fp);
}











































































