#include "ext2fsutil.h"
#include <math.h>

void getSuperBlock(int parStart, struct ext2_super_block *buf) {
	readDisk(parStart * SECTOR_SIZE + SUPER_BLOCK_OFFSET, SUPER_BLOCK_SIZE , buf);
}

void getGroupDescs(int parStart, struct ext2_group_desc buf[MAX_GROUP_NUM], int groupNum) {
	readDisk(parStart * SECTOR_SIZE + GROUP_DESC_OFFSET, ONE_GROUP_DESC_SIZE * groupNum, buf);
}

inline int roundUp(int a, int b) {
	return (int) ceil(1.0 * a / b);
}

int getGroupNumber(struct ext2_super_block *superBlock) {
	return roundUp(superBlock->s_blocks_count, superBlock->s_blocks_per_group);
}

// Read All Block Bitmap From All Group
void getBlockBitmap(int parStart, struct ext2_group_desc groupDescs[], int groupNum,  unsigned char *blockBitmap, int blockSize) {
	int i;
	for (i = 0; i < groupNum; ++i)
		readDisk(parStart * SECTOR_SIZE + groupDescs[i].bg_block_bitmap * blockSize, blockSize, blockBitmap + i * blockSize);
}

// Read All Inode Bitmap From All Group
void getInodeBitmap(int parStart, struct ext2_group_desc groupDescs[], int groupNum, unsigned char *inodeBitmap, int blockSize) {
	int i;
	for (i = 0; i < groupNum; ++i)
		readDisk(parStart * SECTOR_SIZE + groupDescs[i].bg_inode_bitmap * blockSize, blockSize, inodeBitmap + i * blockSize);
}

void getGroupBlock(int blockNo, int *groupNo, int *groupBlockNo, struct ext2_super_block *superBlock) {
	*groupNo = (blockNo - 1) / superBlock->s_blocks_per_group;
	*groupBlockNo = (blockNo - 1) % superBlock->s_blocks_per_group;
}

void getInodeGroupBlock(int inodeNo, int *groupNo, int *groupInodeNo, struct ext2_super_block *superBlock) {
	*groupNo = (inodeNo - 1) / superBlock->s_inodes_per_group;
	*groupInodeNo = (inodeNo - 1) % superBlock->s_inodes_per_group;
}

/* Read All Inodes From All Groups */
void getInodeTable(int parStart, struct ext2_super_block *superBlock, struct ext2_group_desc groupDescs[], int groupNum, struct ext2_inode *inodeTable, int blockSize) {
	int i;
	int last = 0;
	for (i = 0; i < groupNum; ++i) {
		readDisk(parStart * SECTOR_SIZE + groupDescs[i].bg_inode_table * blockSize, INODE_SIZE * superBlock->s_inodes_per_group, inodeTable + last);
		last += superBlock->s_inodes_per_group;
	}
}

void getIndexBit(int Number, int *index, int *bit) {
	*index = Number / 8;
	*bit = Number % 8;
}

/* ret 1 if in inode bitmap inodeNo is 1, else ret 0 */
int isInodeBitmapSet(int inodeNo, unsigned char *inodeBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock) {
	int groupNo;
	int groupInodeNo;
	getInodeGroupBlock(inodeNo, &groupNo, &groupInodeNo, superBlock);
	int index;
	int bit;
	getIndexBit(groupInodeNo, &index, &bit);
	return (inodeBitmap[groupNo * blockSize + index] & (1 << bit)) >> bit;
}

/* ret 1 if in block bitmap blockno is 1, else ret 0 */
int isBlockBitmapSet(int blockNo, unsigned char *blockBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock) {
	int groupNo;
	int groupBlockNo;
	getGroupBlock(blockNo, &groupNo, &groupBlockNo, superBlock);
	int index;
	int bit;
	getIndexBit(groupBlockNo, &index, &bit);
	return (blockBitmap[groupNo * blockSize + index] & (1 << bit)) >> bit;
}

void ext2fsutilTest(int start, int length) {
	struct ext2_super_block x;
	getSuperBlock(start, &x);
	int groupNum = getGroupNumber(&x);
	int blockSize = EXT2_BLOCK_SIZE(&x);
	struct ext2_group_desc groupDescs[MAX_GROUP_NUM];
	getGroupDescs(start, groupDescs, groupNum);
	unsigned char *blockBitmap = malloc(blockSize * groupNum + 1);
	getBlockBitmap(start, groupDescs, groupNum, blockBitmap, blockSize);
	unsigned char *inodeBitmap = malloc(blockSize * groupNum + 1);
	getInodeBitmap(start, groupDescs, groupNum, inodeBitmap, blockSize);
	
	struct ext2_inode *inodeTable = malloc(sizeof(struct ext2_inode) * x.s_inodes_per_group * groupNum);
	getInodeTable(start, &x, groupDescs, groupNum, inodeTable, blockSize);
	//printf("%d\n", inodeTable[1].i_mode);
	
	//printf("%d\n", isInodeBitmapSet(2, inodeBitmap, groupNum, blockSize, &x));
	printf("%d\n", isBlockBitmapSet(5000, blockBitmap, groupNum, blockSize, &x));
	free(blockBitmap);
	free(inodeBitmap);
}
