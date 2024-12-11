
all: main.c
	clang -o notify main.c -static -Wall -Wextra -Werror -pedantic -O2
