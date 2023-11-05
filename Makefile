CC=gcc

all:
	$(CC) calcnew.c -std=c89 -lc -lm -Wall -Wextra -pedantic -O3
