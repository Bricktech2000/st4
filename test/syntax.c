// C syntax highlighting

#include <stdio.h>

size_t strlen(char *s) {
	char *p = s;
	while (*p)
		p++;
	return p - s;
}

int main(void) {
	printf("%d\n", strlen("strlen"));
}
