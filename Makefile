CC=gcc

all:
	$(CC) main.c -std=c89 -lm -Wall -Wextra -pedantic -O3 -o nbc
	$(CC) calcnew.c -std=c89 -lm -Wall -Wextra -pedantic -O3
