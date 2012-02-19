#include "pass4.h"
#include "init.h"
#include "ext2fsutil.h"

int *inodeFlag;
unsigned char *newBitmap;
int change;

void checkDataBlock(int localId) {
	if (inodeFlag[localId]) return;
	inodeFlag[localId] = 1;

	struct ext2_inode *inode;
	inode = inodeTable + localId;
	int tempSize;
	int totalSize = inode->i_size;
	if (isDirectory(inode)) {
		unsigned char *buf = malloc(inode->i_size);
		getData(start, inode, blockSize, buf);
		tempSize = 0;
		struct ext2_dir_entry_2 *dir;
		while (tempSize < totalSize) {
			dir = (struct ext2_dir_entry_2 *)(buf + tempSize);
			if (dir->inode != 0)
				checkDataBlock(localNo(dir->inode));
			tempSize += dir->rec_len;
		}
		free(buf);
	}
	if (isSymbolicLink(inode)) return;
	// set data block
	tempSize = 0;
	int i, j, k;
	int len = blockSize / sizeof(int);
	int* indblock = malloc(blockSize);
	int* dindblock = malloc(blockSize);
	int* tindblock = malloc(blockSize);

	// Direct Blocks
	for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) {
		// inode->i_block[i]
		setBlockBitmapSet(localNo(inode->i_block[i]), newBitmap, groupNum, blockSize, &x, 1);
		tempSize += blockSize;
		if (tempSize >= totalSize) goto FREE_MEMORY;
	}
	
	// Indirect Blocks
	readDisk(start * SECTOR_SIZE + blockSize * inode->i_block[EXT2_IND_BLOCK], blockSize, indblock);
	setBlockBitmapSet(localNo(inode->i_block[EXT2_IND_BLOCK]), newBitmap, groupNum, blockSize, &x, 1);
	for (i = 0; i < len;  ++i) {
		setBlockBitmapSet(localNo(indblock[i]), newBitmap, groupNum, blockSize, &x, 1);
		tempSize += blockSize;
		if (tempSize >= totalSize) goto FREE_MEMORY;
	}
	
	// Doubly-indirect Blocks
	readDisk(start * SECTOR_SIZE + blockSize * inode->i_block[EXT2_DIND_BLOCK], blockSize, dindblock);
	setBlockBitmapSet(localNo(inode->i_block[EXT2_DIND_BLOCK]), newBitmap, groupNum, blockSize, &x, 1);
	for (i = 0; i < len; ++i) {
		readDisk(start * SECTOR_SIZE + blockSize * dindblock[i], blockSize, indblock);
		setBlockBitmapSet(localNo(dindblock[i]), newBitmap, groupNum, blockSize, &x, 1);
		for (j = 0; j < len; ++j) {
			setBlockBitmapSet(localNo(indblock[j]), newBitmap, groupNum, blockSize, &x, 1);
			tempSize += blockSize;
			if (tempSize >= totalSize) goto FREE_MEMORY;
		}
	}

	// Thriply-indirect Blocks
	readDisk(start * SECTOR_SIZE + blockSize * inode->i_block[EXT2_TIND_BLOCK], blockSize, tindblock);
	setBlockBitmapSet(localNo(inode->i_block[EXT2_TIND_BLOCK]), newBitmap, groupNum, blockSize, &x, 1);
	for (i = 0; i < len; ++i) {
		readDisk(start * SECTOR_SIZE + blockSize * tindblock[i], blockSize, dindblock);
		setBlockBitmapSet(localNo(tindblock[i]), newBitmap, groupNum, blockSize, &x, 1);
		for (j = 0; j < len; ++j) {
			readDisk(start * SECTOR_SIZE + blockSize * dindblock[j], blockSize, indblock);
			setBlockBitmapSet(localNo(dindblock[j]), newBitmap, groupNum, blockSize, &x, 1);
			for (k = 0; k < len; ++k) {
				setBlockBitmapSet(localNo(indblock[k]), newBitmap, groupNum, blockSize, &x, 1);
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

void checkBlockBitmap() {
	int i, j, temp;
	int blockPerGroup = EXT2_BLOCKS_PER_GROUP(&x);
	int bitmap;
	int groupBlock = roundUp(ONE_GROUP_DESC_SIZE * groupNum, blockSize);
	int inodeTableBlock = roundUp(INODE_SIZE * EXT2_INODES_PER_GROUP(&x), blockSize);
	int used = 0;
	//check superBlock & groupDescriptors & block bitmap & inode bitmap & inode table
	for (i = 0; i < groupNum; ++i) {
		//superBlock 0
		++used;
		bitmap = isBlockBitmapSet(i * blockPerGroup + 0, blockBitmap, groupNum, blockSize, &x);
		if (!bitmap) {
			printf("partition %d block %d : bitmap wrong: %d Vs %d\n", partitionNumber, i * blockPerGroup + 1, 0, 1);
			change = 1;
		}
		setBlockBitmapSet(i * blockPerGroup + 0, newBitmap, groupNum, blockSize, &x, 1);
		
		//groupDescriptors 1 .. groupBlock
		for (j = 1; j <= groupBlock; ++j) {
			++used;
			bitmap = isBlockBitmapSet(i * blockPerGroup + j, blockBitmap, groupNum, blockSize, &x);
			if (!bitmap) {
				printf("partition %d block %d : bitmap wrong: %d Vs %d\n", partitionNumber, i * blockPerGroup + j + 1, 0, 1);
				change = 1;
			}
			setBlockBitmapSet(i * blockPerGroup + j, newBitmap, groupNum, blockSize, &x, 1);
		}

		//block bitmap
		++used;
		bitmap = isBlockBitmapSet(localNo(groupDescs[i].bg_block_bitmap), blockBitmap, groupNum, blockSize, &x);
		if (!bitmap) {
			printf("partition %d block %d : bitmap wrong: %d Vs %d\n", partitionNumber, groupDescs[i].bg_block_bitmap, 0, 1);
			change = 1;
		}
		setBlockBitmapSet(localNo(groupDescs[i].bg_block_bitmap), newBitmap, groupNum, blockSize, &x, 1);

		//inode bitmap
		++used;
		bitmap = isBlockBitmapSet(localNo(groupDescs[i].bg_inode_bitmap), blockBitmap, groupNum, blockSize, &x);
		if (!bitmap) {
			printf("partition %d block %d : bitmap wrong: %d Vs %d\n", partitionNumber, groupDescs[i].bg_inode_bitmap, 0, 1);
			change = 1;
		}
		setBlockBitmapSet(localNo(groupDescs[i].bg_inode_bitmap), newBitmap, groupNum, blockSize, &x, 1);
		
		//inode table
		temp = groupDescs[i].bg_inode_table;
		for (j = temp - 1; j < temp + inodeTableBlock; ++j) {
			++used;
			bitmap = isBlockBitmapSet(j, blockBitmap, groupNum, blockSize, &x);
			if (!bitmap) {
				printf("partition %d block %d : bitmap wrong: %d Vs %d\n", partitionNumber, j + 1, 0, 1);
				change = 1;
			}
			setBlockBitmapSet(j, newBitmap, groupNum, blockSize, &x, 1);
		}
	}

	// check data block
	inodeFlag = malloc(sizeof(int) * groupNum * EXT2_INODES_PER_GROUP(&x));
	for (i = 0; i < groupNum * EXT2_INODES_PER_GROUP(&x); ++i)
		inodeFlag[i] = 0;
	checkDataBlock(localNo(EXT2_ROOT_INO));
	free(inodeFlag);
}

void pass4() {
	int i;
	int bitmap;
	change = 0;
	// check Inode Bitmap
	for (i = localNo(EXT2_ROOT_INO); i < groupNum * EXT2_INODES_PER_GROUP(&x); ++i) {
		if (i >= 2 && i <= 9) continue; // special
		bitmap = isInodeBitmapSet(i, inodeBitmap, groupNum, blockSize, &x);
		if (bitmap && !inodeLink[i]) { // set to 0
			printf("partition %d inode %d : bitmap is inconsistent with inode table: %d Vs %d\n", partitionNumber, i + 1, bitmap, inodeLink[i]);
			setInodeBitmapSet(i, inodeBitmap, groupNum, blockSize, &x, 0);
			change = 1;
		} else if (!bitmap && inodeLink[i]) { // set to 1
			printf("partition %d inode %d : bitmap is inconsistent with inode table: %d Vs %d\n", partitionNumber, i + 1, bitmap, inodeLink[i]);
			setInodeBitmapSet(i, inodeBitmap, groupNum, blockSize, &x, 1);
			change = 1;
		}
	}
	if (change) {
		for (i = 0; i < groupNum; ++i)
			writeDisk(start * SECTOR_SIZE + groupDescs[i].bg_inode_bitmap * blockSize, blockSize, inodeBitmap + i * blockSize);
	}
	// check Block Bitmap
	newBitmap = malloc(blockSize * groupNum + 1);
	for (i = 0; i < blockSize * groupNum + 1; ++i)
		newBitmap[i] = 0;
	checkBlockBitmap();
	// replace blockBitmap with newBitmap
	int oldbit;
	int newbit;
//	printf("%d\n", x.s_blocks_count);
	for (i = 0; i < x.s_blocks_count - 1; ++i) {
		oldbit = isBlockBitmapSet(i, blockBitmap, groupNum, blockSize, &x);
		newbit = isBlockBitmapSet(i, newBitmap, groupNum, blockSize, &x);
		if (oldbit != newbit) {
			printf("partition %d block bitmap: block %d: %d Vs %d\n", partitionNumber, i + 1, oldbit, newbit);
			setBlockBitmapSet(i, blockBitmap, groupNum, blockSize, &x, newbit);
		}
	}
	free(newBitmap);
	for (i = 0; i < groupNum; ++i)
		writeDisk(start * SECTOR_SIZE + groupDescs[i].bg_block_bitmap * blockSize, blockSize, blockBitmap + i * blockSize);
}
