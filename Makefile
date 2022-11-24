CC = gcc
CFLAGS = -Wpedantic -std=gnu99

A2:
	mpicc -g -Wall -std=c99 -o A2 A2.c

clean:
	rm -i *.o A2