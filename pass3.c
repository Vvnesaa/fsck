#include "pass3.h"
#include "init.h"
#include "ext2fsutil.h"

void pass3() {
	int i;
	struct ext2_inode *inode;
	for (i = localNo(EXT2_ROOT_INO); i< groupNum * EXT2_INODES_PER_GROUP(&x); ++i) {
		inode = inodeTable + i;
		if (inode->i_links_count != inodeLink[i]) {
			printf("partition %d inode %d inconsistent link count: %d Vs %d\n", partitionNumber, i + 1, inode->i_links_count, inodeLink[i]);
			inode->i_links_count = inodeLink[i];
			int groupNo;
			int groupInodeNo;
			getInodeGroupBlock(i, &groupNo, &groupInodeNo, &x);
			writeDisk(start * SECTOR_SIZE + groupDescs[groupNo].bg_inode_table * blockSize + INODE_SIZE * groupInodeNo, INODE_SIZE, inodeTable + i);
		}
	}
}
