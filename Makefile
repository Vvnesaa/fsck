P = 1
CC=gcc
OBJ = myfsck.o printPartitionTable.o
CFLAGS = -Wall -g -D_LARGEFILE64_SOURCE -c

myfsck: $(OBJ)
	$(CC) -o myfsck $(OBJ)
myfsck.o: myfsck.c myfsck.h
	$(CC) $(CFLAGS) myfsck.c
printPartitionTable.o: printPartitionTable.c printPartitionTable.h
	$(CC) $(CFLAGS) printPartitionTable.c

run: myfsck
	./myfsck -p $(P) -i disk
