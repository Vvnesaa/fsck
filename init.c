#include "init.h"

struct ext2_super_block x;
int groupNum;
int blockSize;
struct ext2_group_desc *groupDescs;
unsigned char *blockBitmap;
unsigned char *inodeBitmap;
struct ext2_inode *inodeTable;
int *inodeLink;
int partitionNumber;
int start;

void init(int pN, int parStart) {
	partitionNumber = pN;
	start = parStart;

	getSuperBlock(start, &x);
	groupNum = getGroupNumber(&x);
	blockSize = EXT2_BLOCK_SIZE(&x);
	groupDescs = malloc(groupNum * GROUP_SIZE);
	getGroupDescs(start, groupDescs, groupNum);
	blockBitmap = malloc(blockSize * groupNum + 1);
	getBlockBitmap(start, groupDescs, groupNum, blockBitmap, blockSize);
	inodeBitmap = malloc(blockSize * groupNum + 1);
	getInodeBitmap(start, groupDescs, groupNum, inodeBitmap, blockSize);
	inodeTable = malloc(INODE_SIZE * groupNum * EXT2_INODES_PER_GROUP(&x));
	getInodeTable(start, &x, groupDescs, groupNum, inodeTable, blockSize);
	inodeLink = malloc(sizeof(int) * groupNum * EXT2_INODES_PER_GROUP(&x));
	int i;
	for (i = 0; i < groupNum * EXT2_INODES_PER_GROUP(&x); ++i)
		inodeLink[i] = 0;
}

void cleanup() {
	free(groupDescs);
	free(blockBitmap);
	free(inodeBitmap);
	free(inodeTable);
	free(inodeLink);
}
