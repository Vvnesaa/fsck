#include "pass1.h"

void checkDir(struct ext2_inode *inode, unsigned char *buf, int localId, int parent) {
	int changeDir = 0;
	int n = 0;
	int tempSize = 0;
	int totalSize = inode->i_size;
	struct ext2_dir_entry_2 *dir;
	while (tempSize < totalSize) {
		changeDir = 0;
		dir = (struct ext2_dir_entry_2 *)(buf + tempSize);
		++n;
		if (n == 1) { // .
			if (dir->name_len == 1 && strcmp(strndupa(dir->name, dir->name_len), ".") == 0) {
				if (dir->inode != localId + 1) {
					printf("Error: Partition %d Dir Inode %d: . has wrong dir->inode %d, change to itself\n", partitionNumber, localId + 1, dir->inode);
					dir->inode = localId + 1;
					changeDir = 1;
				}
			} else {
				printf("Error: Partition %d Dir Inode %d: first dir is not ., change to .\n", partitionNumber, localId + 1);
				dir->name_len = 1;
				dir->name[0] = '.';
				dir->inode = localId + 1;
				changeDir = 1;
			}
			checkDirInode(localNo(dir->inode), localId + 1);
		} else if (n == 2) { // ..
			if (dir->name_len == 2 && strcmp(strndupa(dir->name, dir->name_len), "..") == 0) {
				if (dir->inode != parent) {
					printf("Error: Partition %d Dir Inode %d: .. has wrong dir->inode %d, change to its parent\n", partitionNumber, localId + 1, dir->inode);
					dir->inode = parent;
					changeDir = 1;
				}
			} else {
				printf("Error: Partition %d Dir Inode %d: second dir is not .., change to ..\n", partitionNumber, localId + 1);
				dir->name_len = 2;
				dir->name[0] = '.';
				dir->name[1] = '.';
				dir->inode = localId + 1;
				changeDir = 1;
			}
			checkDirInode(localNo(dir->inode), localId + 1);
		} else {
			if (dir->inode != 0)
				checkDirInode(localNo(dir->inode), localId + 1);
		}
		if (changeDir)
			writeDisk(start * SECTOR_SIZE + inode->i_block[0] * blockSize + tempSize, dir->rec_len, dir);
		tempSize += dir->rec_len;
	}
}

// localId has local No., parent has global No.
void checkDirInode(int localId, int parent) {
	++inodeLink[localId];
	if (inodeLink[localId] > 1) return;
	struct ext2_inode *inode;
	inode = inodeTable + localId;
	if (!isDirectory(inode)) return;
	unsigned char *buf = malloc(inode->i_size);
	getData(start, inode, blockSize, buf);
	checkDir(inode, buf, localId, parent);

	free(buf);
}

void pass1() {
	checkDirInode(localNo(EXT2_ROOT_INO), EXT2_ROOT_INO);
	inodeLink[localNo(EXT2_ROOT_INO)]--;
}
