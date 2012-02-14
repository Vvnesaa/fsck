#ifndef PRINTPARTITIONTABLE
#define PRINTPARTITIONTABLE

#define mPToffset 446 // Master Boot Record Partition Table Offset
#define bfLen 100 // Buffer Length For Partition Table
#define PTableLen 64 // Partition Table Length
#define maxPNum 100 // Max Partition Number ???
#define maxPNumMBR 4 // Max Partition Number in MBR
#define oneParLen 16 // One Partition Record Length in Bytes
#define parTypeOffset 4 // Partition Type Offset in One Partition Record
#define parStartOffset 8 // Partition Start Sector Number Offset in One Partition Record
#define parLenOffset 12 // Partition Total Sector Number Offset in One Partition Record
#define ExtendType 5 // Partition Type Number for Extended Partition
#define sectorSize 512 // Sector Size in Bytes

struct Partition {
	unsigned char type;
	int start;
	int length;
};
extern struct Partition par[maxPNum]; // Records of all Partitions
extern int pNum; // Max Partition Number

void printPartitionInfo();

#endif
