#include "ext2fsutil.h"

void getSuperBlock(int parStart, struct ext2_super_block *buf) {
	readDisk(parStart * SECTOR_SIZE + SUPER_BLOCK_OFFSET, SUPER_BLOCK_SIZE , buf);
}

void getGroupDescs(int parStart, struct ext2_group_desc *buf, int groupNum) {
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
/* local inode number */
int isInodeBitmapSet(int inodeNo, unsigned char *inodeBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock) {
	int groupNo;
	int groupInodeNo;
	getInodeGroupBlock(inodeNo, &groupNo, &groupInodeNo, superBlock);
	int index;
	int bit;
	getIndexBit(groupInodeNo, &index, &bit);
	return (inodeBitmap[groupNo * blockSize + index] & (1 << bit)) >> bit;
}

void setInodeBitmapSet(int inodeNo, unsigned char *inodeBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock, int setbit) {
	int groupNo;
	int groupInodeNo;
	getInodeGroupBlock(inodeNo, &groupNo, &groupInodeNo, superBlock);
	int index;
	int bit;
	getIndexBit(groupInodeNo, &index, &bit);
	if (setbit)
		inodeBitmap[groupNo * blockSize + index] |= (1 << bit);
	else
		inodeBitmap[groupNo * blockSize + index] &= (~(1 << bit));
}

void setBlockBitmapSet(int blockNo, unsigned char *blockBitmap, int groupNum, int blockSize, struct ext2_super_block *superBlock, int setbit) {
	//printf("%d\n", blockNo);
	int groupNo;
	int groupBlockNo;
	getGroupBlock(blockNo, &groupNo, &groupBlockNo, superBlock);
	int index;
	int bit;
	getIndexBit(groupBlockNo, &index, &bit);
	if (setbit) 
		blockBitmap[groupNo * blockSize + index] |= (1 << bit);
	else
		blockBitmap[groupNo * blockSize + index] &= (~(1 << bit));
}

/* ret 1 if in block bitmap blockno is 1, else ret 0 */
/* local block Number */
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
	int temp = inode->i_mode & 0xF000;
	return temp == EXT2_S_IFDIR;
}

inline int isSymbolicLink(struct ext2_inode *inode) {
	int temp = inode->i_mode & 0xF000;
	return temp == EXT2_S_IFLNK;
}

inline int isRegFile(struct ext2_inode *inode) {
	int temp = inode->i_mode & 0xF000;
	return temp == EXT2_S_IFREG;
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
	
/*	struct ext2_inode *inode;
	//we already know /lion is on inode 4017
	inode = inodeTable + 4016;
	printf("%d\n", isDirectory(inode));
	unsigned char *lionbuf = malloc(inode->i_size);
	getData(start, inode, blockSize, lionbuf);
	AnalyzeDir(inode, lionbuf);

	//we already know /lion/tigers is on inode 4018
	inode = inodeTable + 4017;
	printf("%d\n", isDirectory(inode));
	unsigned char *tigersbuf= malloc(inode->i_size);
	getData(start, inode, blockSize, tigersbuf);
	AnalyzeDir(inode, tigersbuf);

	//we already know /lion/tigers/bears is on inode 4019
	inode = inodeTable + 4018;
	printf("%d\n", isDirectory(inode));
	unsigned char *bearsbuf= malloc(inode->i_size);
	getData(start, inode, blockSize, bearsbuf);
	AnalyzeDir(inode, bearsbuf);
	
	//we already know /lion/tigers/bears/ohmy.txt is on inode 4021
	inode = inodeTable + 4020;
	printf("%d\n", isDirectory(inode)); 
	unsigned char *txtbuf= malloc(inode->i_size);
	getData(start, inode, blockSize, txtbuf);
	//printf("%s", txtbuf);
	printf("%d\n", isBlockBitmapSet(localNo(inode->i_block[0]), blockBitmap, groupNum, blockSize, &x));

	free(txtbuf); 
	free(bearsbuf);	
	free(tigersbuf);
	free(lionbuf); */

/*	struct ext2_inode *inode;
	//we already know /oz/tornado/glinda is on inode 30 
	inode = inodeTable + 29;
	printf("%d\n", isSymbolicLink(inode));
	printf("%d %d %d %d\n", inode->i_size, inode->i_blocks, inode->i_block[0], inode->i_block[1]);
	int i;
	for (i = 0; i < inode->i_size; ++i)
		printf("%c", ((unsigned char *)inode->i_block)[i]);
	printf("\n");
//	unsigned char *ozbuf = malloc(inode->i_size);
//	getData(start, inode, blockSize, ozbuf);
//	printf("%s\n", ozbuf);
//	AnalyzeDir(inode, ozbuf);
	
//	free(ozbuf);
*/
	free(buf);
	free(blockBitmap);
	free(inodeBitmap);
}
