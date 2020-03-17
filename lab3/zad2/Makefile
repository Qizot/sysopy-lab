CC = gcc -Wall -std=c11

all: main clean

generate_matrix:
	$(CC) -o generate_matrix generate_matrix.c

main:
	$(CC) -o main main.c multiplication.c

clean:
	rm -f *.o