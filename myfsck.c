#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <endian.h>
#include <stdint.h>

#include "myfsck.h"
#include "printPartitionTable.h"
#include "ext2fsutil.h"
#include "pass3.h"
#include "pass4.h"

int theParNum = 0; // Partition Number
char* diskFileName = 0; // Disk Image File Name
int device; // Disk Image File Handler

void readDisk(unsigned int startPoint, unsigned int readByteNum, void *buf) {
	int ret;
	int64_t lret;

	if ((lret = lseek64(device, startPoint, SEEK_SET)) != startPoint) {
		printf("Seek to position %d failed at %lld, device:%d\n", startPoint, lret, device);
		exit(-1);
	}
	if ((ret = read(device, buf, readByteNum)) != readByteNum) {
		printf("Read disk error at %d length %d\n", startPoint, readByteNum);
		exit(-1);
	}
}

void writeDisk(unsigned int startPoint, unsigned int writeByteNum, void *buf) {
	int ret;
        int64_t lret;
	if ((lret = lseek64(device, startPoint, SEEK_SET)) != startPoint) {
		printf("Seek to position %d failed at %lld, device:%d\n", startPoint, lret, device);
                exit(-1);
	}
	if ((ret = write(device, buf, writeByteNum)) != writeByteNum) {
		printf("Write disk error at %d length %d\n", startPoint, writeByteNum);
                exit(-1);
	}
}

int getIntFromBuf(unsigned char* buf, int start) {
	uint32_t x = *(uint32_t*)(buf + start);
	return (int) le32toh(x);
}

int main(int argc, char *argv[]) {
	int fsck = 0;
	int opt;
	opterr = 0;
	while ((opt = getopt(argc, argv, "f:p:i:")) != -1) {
		switch (opt) {
			case 'f':
				fsck = 1;
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
//	printf("%d %d %s\n", fsck, pNum, diskFileName);
	
	printPartitionInfo(fsck);
	if (fsck) {
		int i;
//		printf("%d\n", pNum);
		for (i = 1; i <= pNum; ++i)
			if (par[i].type == EXT2_TYPE && (theParNum == 0 || theParNum == i)) {
			init(i, par[i].start);
			pass1();
			pass2();
			pass3();
			pass4();
			cleanup();
		}
	}
//	ext2fsutilTest(par[1].start, par[1].length);
	
	return 0;
}
