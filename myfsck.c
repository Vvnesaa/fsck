#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <endian.h>
#include <stdint.h>

#include "myfsck.h"
#include "printPartitionTable.h"

int theParNum = 0; // Partition Number
char* diskFileName = 0; // Disk Image File Name
int device; // Disk Image File Handler

void readDisk(unsigned int startPoint, unsigned int readByteNum, void *buf) {
	int ret;
	int64_t lret;

	if ((lret = lseek64(device, startPoint, SEEK_SET)) != startPoint) {
		printf("Seek to position %d failed at %lld, device:%d\n", startPoint, lret, device);
		perror("");
		exit(-1);
	}
	if ((ret = read(device, buf, readByteNum)) != readByteNum) {
		printf("Read disk error at %d length %d\n", startPoint, readByteNum);
		exit(-1);
	}
}

int getIntFromBuf(unsigned char* buf, int start) {
	uint32_t x = *(uint32_t*)(buf + start);
	return (int) le32toh(x);
}

int main(int argc, char *argv[]) {
	int opt;
	opterr = 0;
	while ((opt = getopt(argc, argv, "p:i:")) != -1) {
		switch (opt) {
			case 'p':
				theParNum = atoi(optarg);
				break;
			case 'i':
				diskFileName = optarg;
				break;
			default:
				break;
		}
	}
	if (diskFileName == 0) {
		printf("Argument error\n");
		return 0;
	}
	//printf("%d %s\n", pNum, diskFileName);
	
	printPartitionInfo();
	return 0;
}
