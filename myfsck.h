#ifndef MYFSCK
#define MYFSCK

#include "init.h"
#include "pass1.h"
#define SECTOR_SIZE 512 // Sector Size in Bytes

extern int theParNum; // Partition Number
extern char* diskFileName; // Disk Image File Name
extern int device; // Disk Image File Handler

void readDisk(unsigned int startPoint, unsigned int readByteNum, void *buf);
void writeDisk(unsigned int startPoint, unsigned int writeByteNum, void *buf);
int getIntFromBuf(unsigned char* buf, int start);

#endif
