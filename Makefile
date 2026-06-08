.POSIX:
.SUFFIXES:
CC=gcc
CFLAGS=-O2 -Wall -Wextra -Wpedantic -std=c99

all: bin/st0 bin/st1 bin/st2 bin/st3 bin/st4
bin/:; mkdir bin/
clean:; rm -rf bin/

bin/st0: src/st0.jpg;    @echo "if only"
bin/st1: bin/ src/st1.c; $(CC) $(CFLAGS) -o $@ src/st1.c -Wno-implicit-fallthrough -Wno-unused-parameter
bin/st2: bin/ src/st2.c; $(CC) $(CFLAGS) -o $@ src/st2.c -Wno-implicit-fallthrough
bin/st3: bin/ src/st3.c; $(CC) $(CFLAGS) -o $@ src/st3.c -Wno-implicit-fallthrough -Wno-sign-compare -Wno-string-plus-int
bin/st4: bin/ src/st4.c; $(CC) $(CFLAGS) -o $@ src/st4.c -Wno-implicit-fallthrough -Wno-sign-compare -Wno-string-plus-int -Wno-format-zero-length -Wno-bool-operation