CC = gcc -Wall -std=c11 -ggdb


all: test clean

compile:
	$(CC) -o main main.c

clean:
	rm -f *.o


 # (25%) Dla obu wariantów implementacji przeprowadź pomiary czasu użytkownika i czasu
 # systemowego operacji sortowania i kopiowania. Testy wykonaj dla następujących
 # rozmiarów rekordu: 1, 4, 512, 1024, 4096 i 8192 bajty.
 # Dla każdego rozmiaru rekordu wykonaj dwa testy różniące się liczbą rekordów w sortowanym pliku.
 # Liczby rekordów dobierz tak, by czas sortowania mieścił się w przedziale od kilku do kilkudziesięciu sekund.
 # Porównując dwa warianty implementacji, należy korzystać z identycznego pliku do sortowania (po wygenerowaniu, a przed sortowaniem, utwórz jego kopię).
 # Zmierzone czasy zestaw w pliku wyniki.txt. Do pliku dodaj komentarz podsumowujący wnioski z testów.


test:
	make compile
	bash test.sh
