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

inline int getGroupNumber(struct ext2_super_block *superBlock) {
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

// blockNo is local BlockNo
void getGroupBlock(int blockNo, int *groupNo, int *groupBlockNo, struct ext2_super_block *superBlock) {
	*groupNo = blockNo / superBlock->s_blocks_per_group;
	*groupBlockNo = blockNo % superBlock->s_blocks_per_group;
}

// inodeNo is local inodeNo
void getInodeGroupBlock(int inodeNo, int *groupNo, int *groupInodeNo, struct ext2_super_block *superBlock) {
	*groupNo = inodeNo / superBlock->s_inodes_per_group;
	*groupInodeNo = inodeNo % superBlock->s_inodes_per_group;
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

inline void getIndexBit(int Number, int *index, int *bit) {
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

inline int isDirectory(struct ext2_inode *inode) {
	return inode->i_mode & EXT2_S_IFDIR ? 1 : 0;
}

inline int localNo(int x) {
	return x - 1;
}

// get all data from one inode
void getData(int parStart, struct ext2_inode *inode, int blockSize, unsigned char* buf) {
	int i, j, k;
	int tempSize = 0;
	int totalSize = inode->i_size;
	int len = blockSize / sizeof(int);
	int* indblock = malloc(blockSize);
	int* dindblock = malloc(blockSize);
	int* tindblock = malloc(blockSize);

	// Direct Blocks
	for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) { 
		readDisk(parStart * SECTOR_SIZE + blockSize * inode->i_block[i], blockSize, buf + tempSize);
		tempSize += blockSize;
		if (tempSize >= totalSize) goto FREE_MEMORY;
	}
	
	// Indirect Blocks
	readDisk(parStart * SECTOR_SIZE + blockSize * inode->i_block[EXT2_IND_BLOCK], blockSize, indblock);
	for (i = 0; i < len;  ++i) {
		readDisk(parStart * SECTOR_SIZE + blockSize * indblock[i], blockSize, buf + tempSize);
		tempSize += blockSize;
		if (tempSize >= totalSize) goto FREE_MEMORY;
	}
	
	// Doubly-indirect Blocks
	readDisk(parStart * SECTOR_SIZE + blockSize * inode->i_block[EXT2_DIND_BLOCK], blockSize, dindblock);
	for (i = 0; i < len; ++i) {
		readDisk(parStart * SECTOR_SIZE + blockSize * dindblock[i], blockSize, indblock);
		for (j = 0; j < len; ++j) {
			readDisk(parStart * SECTOR_SIZE + blockSize * indblock[j], blockSize, buf + tempSize);
			tempSize += blockSize;
			if (tempSize >= totalSize) goto FREE_MEMORY;
		}
	}

	// Thriply-indirect Blocks
	readDisk(parStart * SECTOR_SIZE + blockSize * inode->i_block[EXT2_TIND_BLOCK], blockSize, tindblock);
	for (i = 0; i < len; ++i) {
		readDisk(parStart * SECTOR_SIZE + blockSize * tindblock[i], blockSize, dindblock);
		for (j = 0; j < len; ++j) {
			readDisk(parStart * SECTOR_SIZE + blockSize * dindblock[j], blockSize, indblock);
			for (k = 0; k < len; ++k) {
				readDisk(parStart * SECTOR_SIZE + blockSize * indblock[k], blockSize, buf + tempSize);
				tempSize += blockSize;
				if (tempSize >= totalSize) goto FREE_MEMORY;
			}
		}
	}
	
	FREE_MEMORY:
		free(indblock);
		free(dindblock);
		free(tindblock);
}

void AnalyzeDir(struct ext2_inode *inode, unsigned char *buf) {
	int tempSize = 0;
	int totalSize = inode->i_size;
	struct ext2_dir_entry_2 *dir;
	while (tempSize < totalSize) {
		dir = (struct ext2_dir_entry_2 *)(buf + tempSize);
		printf("%d %d %d %d %s\n", dir->inode, dir->rec_len, dir->name_len, dir->file_type, strndupa(dir->name, dir->name_len));
		tempSize += dir->rec_len;
	}
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
	unsigned char *buf = malloc(inodeTable[localNo(EXT2_ROOT_INO)].i_size);
	getData(start, inodeTable + 1, blockSize, buf);
	AnalyzeDir(inodeTable + 1, buf);

	free(buf);
	free(blockBitmap);
	free(inodeBitmap);
}
