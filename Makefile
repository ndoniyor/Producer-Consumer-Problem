CC = gcc
CFLAGS = -g -Wall
default: final

final: auxiliary_functions.o semaphore_ops.o main.o
	$(CC) $(CFLAGS) -o output auxiliary_functions.o semaphore_ops.o main.o
	$(RM) *.o
auxiliary_functions.o:
	$(CC) $(CFLAGS) -c auxiliary_functions.c
semaphore_ops.o:
	$(CC) $(CFLAGS) -c semaphore_ops.c
main.o:
	$(CC) $(CFLAGS) -c main.c

clean:
	$(RM) output *.o file2
