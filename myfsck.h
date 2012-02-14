#ifndef MYFSCK
#define MYFSCK

extern int theParNum; // Partition Number
extern char* diskFileName; // Disk Image File Name
extern int device; // Disk Image File Handler

void readDisk(unsigned int startPoint, unsigned int readByteNum, void *buf);
int getIntFromBuf(unsigned char* buf, int start);

#endif
