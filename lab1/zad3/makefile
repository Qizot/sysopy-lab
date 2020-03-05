CC = gcc -Wall -std=c11 -ggdb


all: static shared test_dynamic clean

static:
	$(CC) -c library.c
	ar rcs library.a library.o

shared:
	$(CC) -c -fPIC library.c
	$(CC) -shared -fPIC -o library.so library.o

clean:
	rm -f *.o

test_static:
	make static
	$(CC) -o test main.c library.a -I .
	# works :)

test_dynamic:
	make shared
	$(CC) -o test dynamic_main.c -ldl library.a -I .

test_shared:
	make shared
	$(CC) -o test main.c -L. library.so -I .
