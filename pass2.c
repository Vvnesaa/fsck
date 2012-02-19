#include "pass2.h"
#include "init.h"
#include "ext2fsutil.h"
#include "stdio.h"

int findDirInode(int localId, const char *dirName) {
	int result = 0;
	struct ext2_dir_entry_2 *dir = 0;
	struct ext2_inode *root = inodeTable + localId;
	if (!isDirectory(root)) {
		printf("inode %d is not a directory\n", localId + 1);
		return result;
	}
	unsigned char *buf = malloc(root->i_size);
	getData(start, root, blockSize, buf);
	int tempSize = 0;
	int totalSize = root->i_size;
	while (tempSize < totalSize) {
		dir = (struct ext2_dir_entry_2 *)(buf + tempSize);
		if (strcmp(strndupa(dir->name, dir->name_len), dirName) == 0) {
			result = dir->inode;
			break;
		}
		tempSize += dir->rec_len;
	}
	free(buf);
	return result;
}

int setFileType(int localId) {
	struct ext2_inode *inode = inodeTable + localId;
	if (isDirectory(inode)) return EXT2_FT_DIR;
	if (isRegFile(inode)) return EXT2_FT_REG_FILE;
	if (isSymbolicLink(inode))return EXT2_FT_SYMLINK;
	return EXT2_FT_UNKNOWN;
}

void addToLost(struct ext2_inode *lost, int ID) {
	int totalSize = lost->i_size;
	unsigned char *buf = malloc(lost->i_size);
	getData(start, lost, blockSize, buf);
	int len = 2 + (int) floor(log10(ID));
	char *fileName = malloc(len);
	sprintf(fileName, "#%d", ID);
	int tempSize = 0;
	int n = 0;
	struct ext2_dir_entry_2 *temp;
	struct ext2_dir_entry_2 *other;
	int change = 0;
	while (tempSize < totalSize) {
		temp = (struct ext2_dir_entry_2 *)(buf + tempSize);
		++n;
		if (n > 2) { // skip . and ..
			if (temp->inode == 0 && temp->rec_len >= EXT2_DIR_REC_LEN(len)) {
				temp->inode = ID;
				temp->file_type = setFileType(localNo(ID));
				printf("%d %d\n", ID, temp->file_type);
				temp->name_len = len;
				strcpy(temp->name, fileName);
				change = 1;
				break;
			} else if (temp->rec_len - EXT2_DIR_REC_LEN(temp->name_len) >= EXT2_DIR_REC_LEN(len)) {
				other = (struct ext2_dir_entry_2 *)(buf + tempSize + EXT2_DIR_REC_LEN(temp->name_len));
				other->rec_len = temp->rec_len - EXT2_DIR_REC_LEN(temp->name_len);
				other->inode = ID;
				other->file_type = setFileType(localNo(ID));
				printf("%d %d\n", ID, other->file_type);
				other->name_len = len;
				strcpy(other->name, fileName);
				temp->rec_len = EXT2_DIR_REC_LEN(temp->name_len);
				change = 1;
				break;
			}
		}
		tempSize += temp->rec_len;
	}

	//Write Back
	//Assume only use first 12
	int i;
	tempSize = 0;
	for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) {
		writeDisk(start * SECTOR_SIZE + blockSize * lost->i_block[i], blockSize, buf + tempSize);
		tempSize += blockSize;
		if (tempSize >= totalSize) break;
	}

	free(buf);
	free(fileName);
}

void updateParent(struct ext2_inode *inode, int parent) {
	if (!isDirectory(inode)) return;
	unsigned char *buf = malloc(inode->i_size);
	getData(start, inode, blockSize, buf);
	int tempSize = 0;
	int n = 0;
	int totalSize = inode->i_size;
	struct ext2_dir_entry_2 *dir;
	while (tempSize < totalSize) {
		++n;
		dir = (struct ext2_dir_entry_2 *)(buf + tempSize);
		if (n == 2) { //..
			printf("change inode's parent to lost+found\n");
			dir->inode = parent;
		} else if (n > 2 && dir->inode != 0 && isDirectory(inodeTable + localNo(dir->inode))) {
			dir->inode = 0;
		}
		tempSize += dir->rec_len;
	}
	tempSize = 0;
	int i;
	int currentTemp;
	for (i = 0; i < 12; ++i) {
		if (totalSize - tempSize < blockSize)
			currentTemp = totalSize - tempSize;
		else
			currentTemp = blockSize;
		writeDisk(start * SECTOR_SIZE + inode->i_block[i] * blockSize, currentTemp, buf + tempSize);
		tempSize += blockSize;
		if (tempSize >= totalSize) break;
	}
}

void pass2() {
	// Found /lost+found
	int lostInodeId = findDirInode(localNo(EXT2_ROOT_INO), "lost+found");
	if (lostInodeId == 0) {
		printf("/lost+found is not found\n");
		return;
	}
	// Link every 0-reference-inode with /lost+found
	int i;
	struct ext2_inode *inode;
	for (i = localNo(EXT2_FIRST_INO(&x)); i < groupNum * EXT2_INODES_PER_GROUP(&x); ++i) {
		inode = inodeTable + i;
		if (inode->i_links_count > 0 && inodeLink[i] == 0) {
			printf("partition %d inode %d should be put into lost+found\n", partitionNumber, i + 1);
			addToLost(inodeTable + localNo(lostInodeId), i + 1);
			updateParent(inode, lostInodeId);
		}
	}
	for (i = 0; i < groupNum * EXT2_INODES_PER_GROUP(&x); ++i)
		inodeLink[i] = 0;
	pass1();
}
