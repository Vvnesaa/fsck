P = 1
D = disk
CC=gcc
OBJ = myfsck.o printPartitionTable.o ext2fsutil.o init.o pass1.o pass2.o pass3.o
CFLAGS = -Wall -g -D_LARGEFILE64_SOURCE -D_GNU_SOURCE -c

myfsck: $(OBJ)
	$(CC) -lm -o myfsck $(OBJ)
myfsck.o: myfsck.c myfsck.h
	$(CC) $(CFLAGS) myfsck.c
printPartitionTable.o: printPartitionTable.c printPartitionTable.h
	$(CC) $(CFLAGS) printPartitionTable.c
ext2fsutil.o: ext2fsutil.c ext2fsutil.h
	$(CC) $(CFLAGS) ext2fsutil.c
init.o: init.c init.h
	$(CC) $(CFLAGS) init.c
pass1.o: pass1.c pass1.h
	$(CC) $(CFLAGS) pass1.c
pass2.o: pass2.c pass2.h
	$(CC) $(CFLAGS) pass2.c
pass3.o: pass3.c pass3.h
	$(CC) $(CFLAGS) pass3.c

runf: myfsck
	./myfsck -f $(P) -i $(D)

runp: myfsck
	./myfsck -p $(P) -i $(D)

tar:
	tar cf myfsck.tar *.c *.h Makefile
