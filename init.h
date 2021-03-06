#include "printPartitionTable.h"
#include "ext2fsutil.h"

extern struct ext2_super_block x;
extern int groupNum;
extern int blockSize;
extern struct ext2_group_desc *groupDescs;
extern unsigned char *blockBitmap;
extern unsigned char *inodeBitmap;
extern struct ext2_inode *inodeTable;
extern int *inodeLink;
extern int partitionNumber;
extern int start;

void init(int pN, int parStart);
void cleanup();
