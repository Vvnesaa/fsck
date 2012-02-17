#ifndef PRINTPARTITIONTABLE
#define PRINTPARTITIONTABLE

#define MAX_PARTITION_NUMBER 100

#define MBR_PAR_OFFSET 446 // Master Boot Record Partition Table Offset
#define BUF_LEN 100 // Buffer Length For Partition Table
#define PAR_TAB_LEN 64 // Partition Table Length
#define MAX_PAR_NUM_MBR 4 // Max Partition Number in MBR
#define ONE_PAR_LEN 16 // One Partition Record Length in Bytes
#define PAR_TYPE_OFFSET 4 // Partition Type Offset in One Partition Record
#define PAR_START_OFFSET 8 // Partition Start Sector Number Offset in One Partition Record
#define PAR_LEN_OFFSET 12 // Partition Total Sector Number Offset in One Partition Record
#define EXTEND_TYPE 5 // Partition Type Number for Extended Partition
#define EXT2_TYPE 0x83

struct Partition {
	unsigned char type;
	int start;
	int length;
};
extern struct Partition par[MAX_PARTITION_NUMBER]; // Records of all Partitions
extern int pNum; // Max Partition Number

void printPartitionInfo(int fsckFlag);

#endif
