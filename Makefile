
all: main.c
	$(CC) -o notify main.c -static -Wall -Wextra -Werror -pedantic -O2
