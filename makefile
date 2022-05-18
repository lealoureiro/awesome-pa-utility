CC = gcc
CFLAGS = -g -Wall -std=gnu99
LIBS = -lpulse

main: main.o
	$(CC) $(CFLAGS) -o awesome-pa-utility main.o $(LIBS)

main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c

clean:
	rm -f awesome-pa-utility  main.o