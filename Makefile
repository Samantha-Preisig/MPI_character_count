CC = gcc
CFLAGS = -Wpedantic -std=gnu99

mpi_cc:
	mpi_cc -g -Wall -std=c99 -o mpi_cc mpi_cc.c

clean:
	rm -i *.o mpi_cc
