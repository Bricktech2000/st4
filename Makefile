st4:
	mkdir -p bin
	gcc -O2 -Wall -Werror -pedantic -std=c99 src/st4.c -o bin/st4

st3:
	mkdir -p bin
	gcc -O2 -Wall -Werror -pedantic -std=c99 src/st3.c -o bin/st3

st2:
	mkdir -p bin
	gcc -O2 -Wall -Werror -pedantic -std=c99 src/st2.c -o bin/st2

st1:
	mkdir -p bin
	gcc -O2 -Wall -Werror -pedantic -std=c99 src/st1.c -o bin/st1

st0:
	echo "if only"

clean:
	rm -rf bin
