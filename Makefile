P = 1
D = disk
CC=gcc
OBJ = myfsck.o printPartitionTable.o ext2fsutil.o
CFLAGS = -Wall -g -D_LARGEFILE64_SOURCE -c

myfsck: $(OBJ)
	$(CC) -lm -o myfsck $(OBJ)
myfsck.o: myfsck.c myfsck.h
	$(CC) $(CFLAGS) myfsck.c
printPartitionTable.o: printPartitionTable.c printPartitionTable.h
	$(CC) $(CFLAGS) printPartitionTable.c
ext2fsutil.o: ext2fsutil.c ext2fsutil.h
	$(CC) $(CFLAGS) ext2fsutil.c

run: myfsck
	./myfsck -p $(P) -i $(D)

tar:
	tar cf myfsck.tar *.c *.h Makefile
