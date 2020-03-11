CC = gcc -Wall -std=c11 -ggdb


all: static main clean

static:
	$(CC) -c lib.c
	ar rcs lib.a lib.o

main:
	$(CC) -o main main.c lib.a -I .

clean:
	rm -f *.o
