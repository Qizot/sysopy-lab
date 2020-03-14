CC = gcc -Wall -std=c11 -ggdb

all: static test clean

static:
	$(CC) -c lib.c
	ar rcs lib.a lib.o

main:
	$(CC) -o main main.c lib.a -I .

test:
	make main
	echo $(PATH)
	./main --type readdir --path $(PWD)/directory --atime=+1 --mtime=+1 --maxdepth 10 > readdir.txt
	./main --type nftw --path $(PWD)/directory --atime=+1 --mtime=+1 --maxdepth 10 > nftw.txt

clean:
	rm lib.a
	rm -f *.o
