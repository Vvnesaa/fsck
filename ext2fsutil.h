#ifndef EXT2FSUTIL
#define EXT2FSUTIL

#include "ext2_fs.h"
#include "myfsck.h"
#include "printPartitionTable.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SUPER_BLOCK_OFFSET 1024 // Super Block Offset in Bytes
#define SUPER_BLOCK_SIZE 1024 // Super Block Size in Bytes
#define GROUP_DESC_OFFSET 2048 // Group Descriptors Offset in Bytes
#define ONE_GROUP_DESC_SIZE sizeof(struct ext2_group_desc)
#define MAX_GROUP_NUM 100 // Maximum Group Number ???
#define INODE_SIZE sizeof(struct ext2_inode)

void getSuperBlock(int parStart, struct ext2_super_block *buf);
void ext2fsutilTest(int start, int length);
void getGroupDescs(int parStart, struct ext2_group_desc buf[MAX_GROUP_NUM], int groupNum);
int getGroupNumber(struct ext2_super_block *superBlock);
void getBlockBitmap(int parStart, struct ext2_group_desc groupDescs[], int groupNum,  unsigned char *blockBitmap, int blockSize);
void getInodeBitmap(int parStart, struct ext2_group_desc groupDescs[], int groupNum, unsigned char *inodeBitmap, int blockSize);
void getGroupBlock(int blockNo, int *groupNo, int *groupBlockNo, struct ext2_super_block *superBlock);
void getInodeGroupBlock(int inodeNo, int *groupNo, int *groupInodeNo, struct ext2_super_block *superBlock);
void getInodeTable(int parStart, struct ext2_super_block *superBlock, struct ext2_group_desc groupDescs[], int groupNum, struct ext2_inode *inodeTable, int blockSize);


#endif
