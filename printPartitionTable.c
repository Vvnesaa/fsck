#include <fcntl.h>
#include <stdio.h>
#include "myfsck.h"
#include "printPartitionTable.h"

struct Partition par[maxPNum]; // Records of all Partitions
int pNum; // Max Partition Number

void printPartitionInfo() {
	// Open Disk Image
	if ((device = open(diskFileName, O_RDWR)) == -1) {
		perror("Open disk image error");
		return;
	}

	int i;
	unsigned char buf[bfLen];
	readDisk(mPToffset, PTableLen, buf);
	//for (i = 0; i < PTableLen; ++i)
	//	printf("%02x ", buf[i]);
	
	for (i = 0; i < maxPNumMBR; ++i) {
		par[i + 1].type = buf[i*oneParLen + parTypeOffset];
		par[i + 1].start = getIntFromBuf(buf, i*oneParLen + parStartOffset);
		par[i + 1].length = getIntFromBuf(buf, i*oneParLen + parLenOffset);
	}
	//for (i = 1; i <= maxPNumMBR; ++i)
	//	printf("0x%02X %d %d\n", par[i].type, par[i].start, par[i].length);

	pNum = maxPNumMBR;
	int extendBase = -1;
	int extendStart = -1;
	int extendLength = 0;
	for (i = 1; i <= pNum; ++i)
		if (par[i].type == ExtendType) {
			extendBase = par[i].start;
			extendStart = par[i].start;
			extendLength = par[i].length;
		}
	//printf("%d %d\n", extendStart, extendLength);
	while (extendStart != -1) {
		readDisk(mPToffset + extendStart * sectorSize, PTableLen, buf);
		//for (i = 0; i < PTableLen; ++i)
		//	printf("%02x ", buf[i]);
		//printf("\n");
		++pNum;
		par[pNum].type = buf[parTypeOffset];
		par[pNum].start = extendStart + getIntFromBuf(buf, parStartOffset);
		par[pNum].length = getIntFromBuf(buf, parLenOffset);
		if (buf[oneParLen + parTypeOffset] != ExtendType) {
			extendStart = -1;
		} else {
			extendStart = extendBase + getIntFromBuf(buf, oneParLen + parStartOffset);
			extendLength = getIntFromBuf(buf, oneParLen + parLenOffset);
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
