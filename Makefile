.POSIX:
.SUFFIXES:
CC=gcc
CFLAGS=-O2 -Wall -Wextra -Wpedantic -std=c99

all: st0 st1 st2 st3 st4
clean:; rm -f st0 st1 st2 st3 st4

st0: src/st0.jpg; @echo "if only"
st1: src/st1.c; $(CC) $(CFLAGS) -o $@ src/st1.c -Wno-implicit-fallthrough -Wno-unused-parameter
st2: src/st2.c; $(CC) $(CFLAGS) -o $@ src/st2.c -Wno-implicit-fallthrough
st3: src/st3.c; $(CC) $(CFLAGS) -o $@ src/st3.c -Wno-implicit-fallthrough -Wno-sign-compare -Wno-string-plus-int
st4: src/st4.c; $(CC) $(CFLAGS) -o $@ src/st4.c -Wno-implicit-fallthrough -Wno-sign-compare -Wno-string-plus-int -Wno-format-zero-length -Wno-bool-operation
