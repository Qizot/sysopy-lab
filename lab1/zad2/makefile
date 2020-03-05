
CC = gcc -Wall -std=c11 -ggdb


all: static shared zad2_static clean 

static:
	$(CC) -c library.c
	ar rcs library.a library.o

shared:
	$(CC) -c -fPIC library.c
	$(CC) -shared -fPIC -o library.so library.o

zad2_static:
	make static
	$(CC) -o test main.c library.a -I .

clean:
	rm -f *.o