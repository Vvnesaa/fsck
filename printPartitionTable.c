#include <fcntl.h>
#include <stdio.h>
#include "myfsck.h"
#include "printPartitionTable.h"

struct Partition par[MAX_PARTITION_NUMBER]; // Records of all Partitions
int pNum; // Max Partition Number

void printPartitionInfo() {
	// Open Disk Image
	if ((device = open(diskFileName, O_RDWR)) == -1) {
		perror("Open disk image error");
		return;
	}

	int i;
	unsigned char buf[BUF_LEN];
	readDisk(MBR_PAR_OFFSET, PAR_TAB_LEN, buf);
	//for (i = 0; i < PAR_TAB_LEN; ++i)
	//	printf("%02x ", buf[i]);
	
	for (i = 0; i < MAX_PAR_NUM_MBR; ++i) {
		par[i + 1].type = buf[i*ONE_PAR_LEN + PAR_TYPE_OFFSET];
		par[i + 1].start = getIntFromBuf(buf, i*ONE_PAR_LEN + PAR_START_OFFSET);
		par[i + 1].length = getIntFromBuf(buf, i*ONE_PAR_LEN + PAR_LEN_OFFSET);
	}
	//for (i = 1; i <= MAX_PAR_NUM_MBR; ++i)
	//	printf("0x%02X %d %d\n", par[i].type, par[i].start, par[i].length);

	pNum = MAX_PAR_NUM_MBR;
	int extendBase = -1;
	int extendStart = -1;
	int extendLength = 0;
	for (i = 1; i <= pNum; ++i)
		if (par[i].type == EXTEND_TYPE) {
			extendBase = par[i].start;
			extendStart = par[i].start;
			extendLength = par[i].length;
		}
	//printf("%d %d\n", extendStart, extendLength);
	while (extendStart != -1) {
		readDisk(MBR_PAR_OFFSET + extendStart * SECTOR_SIZE, PAR_TAB_LEN, buf);
		//for (i = 0; i < PAR_TAB_LEN; ++i)
		//	printf("%02x ", buf[i]);
		//printf("\n");
		++pNum;
		par[pNum].type = buf[PAR_TYPE_OFFSET];
		par[pNum].start = extendStart + getIntFromBuf(buf, PAR_START_OFFSET);
		par[pNum].length = getIntFromBuf(buf, PAR_LEN_OFFSET);
		if (buf[ONE_PAR_LEN + PAR_TYPE_OFFSET] != EXTEND_TYPE) {
			extendStart = -1;
		} else {
			extendStart = extendBase + getIntFromBuf(buf, ONE_PAR_LEN + PAR_START_OFFSET);
			extendLength = getIntFromBuf(buf, ONE_PAR_LEN + PAR_LEN_OFFSET);
		}
	}
	//for (i = 1; i <= pNum; ++i)
	//	printf("0x%02X %d %d\n", par[i].type, par[i].start, par[i].length);
	if (theParNum > 0 && theParNum <= pNum)
		printf("0x%02X %d %d\n", par[theParNum].type, par[theParNum].start, par[theParNum].length);
	else
		printf("-1\n");
	return;
}
