#include "ext2fsutil.h"
#include "init.h"

void pass1();
void checkDirInode(int localId, int parent);
void checkDir(struct ext2_inode *inode, unsigned char *buf, int localId, int parent);
