#include "pass4.h"
#include "init.h"

void pass4() {
	int i;
	int bitmap;
	int change = 0;
	for (i = localNo(EXT2_ROOT_INO); i < groupNum * EXT2_INODES_PER_GROUP(&x); ++i) {
		if (i >= 2 && i <= 9) continue; // special
		bitmap = isInodeBitmapSet(i, inodeBitmap, groupNum, blockSize, &x);
		printf("%d\n", bitmap);
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
}
