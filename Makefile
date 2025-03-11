CC=gcc
CFLAGS=-O2 -Wall -Wextra -Wpedantic -std=c99

all: bin/st0 bin/st1 bin/st2 bin/st3 bin/st4

bin/st0: src/st0.jpg
	@echo "if only"

bin/st1: src/st1.c | bin/
	$(CC) $(CFLAGS) -Wno-implicit-fallthrough -Wno-unused-parameter $^ -o $@

bin/st2: src/st2.c | bin/
	$(CC) $(CFLAGS) -Wno-implicit-fallthrough $^ -o $@

bin/st3: src/st3.c | bin/
	$(CC) $(CFLAGS) -Wno-implicit-fallthrough -Wno-sign-compare -Wno-string-plus-int $^ -o $@

bin/st4: src/st4.c | bin/
	$(CC) $(CFLAGS) -Wno-implicit-fallthrough -Wno-sign-compare -Wno-string-plus-int -Wno-format-zero-length $^ -o $@

bin/:
	mkdir bin/

clean:
	rm -rf bin/
