st2:
	mkdir -p bin
	gcc -O2 -Wall -Werror -pedantic -std=c99 st2.c -o bin/st2

st1:
	mkdir -p bin
	gcc -O2 -Wall -Werror -pedantic -std=c99 st1.c -o bin/st1

clean:
	rm -rf bin
