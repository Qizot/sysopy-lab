CC = gcc -Wall -std=c11 -ggdb


all: static shared test clean

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
	$(CC) -o test main.c library.a -I . -O$(Olevel)
	# works :)

test_dynamic:
	make shared
	$(CC) -o test dynamic_main.c -ldl library.a -I . -O$(Olevel)

test_shared:
	make shared
	$(CC) -o test main.c -L. library.so -I . -O$(Olevel)

test:

	echo 'OPTIMIZATION LEVEL', '$(Olevel)\n\n' >> results3b.txt

	make test_static
	echo '======== TEST STATIC\n\n' >> results3b.txt
	echo 'SMALL TEST\n' >> results3b.txt
	./test < test_files/small_test.txt >> results3b.txt
	echo 'MEDIUM TEST\n' >> results3b.txt
	./test < test_files/medium_test.txt >> results3b.txt
	echo 'BIG TEST\n' >> results3b.txt
	./test < test_files/big_test.txt >> results3b.txt
	
	echo '\n\n======== TEST DYNAMIC\n\n' >> results3b.txt
	make test_dynamic
	echo 'SMALL TEST\n' >> results3b.txt
	./test < test_files/small_test.txt >> results3b.txt
	echo 'MEDIUM TEST\n' >> results3b.txt
	./test < test_files/medium_test.txt >> results3b.txt
	echo 'BIG TEST\n' >> results3b.txt
	./test < test_files/big_test.txt >> results3b.txt
	
	echo '\n\n======== TEST SHARED\n\n' >> results3b.txt
	make test_shared
	export LD_LIBRARY_PATH=./
	echo 'SMALL TEST\n' >> results3b.txt
	./test < test_files/small_test.txt >> results3b.txt
	echo 'MEDIUM TEST\n' >> results3b.txt
	./test < test_files/medium_test.txt >> results3b.txt
	echo 'BIG TEST\n' >> results3b.txt
	./test < test_files/big_test.txt >> results3b.txt

