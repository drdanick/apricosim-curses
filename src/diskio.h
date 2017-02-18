#ifndef DISKIO_H
#define DISKIO_H

void selectDiskFile(char* fileName, int id);

void selectDiskTrack(int t);

void writeToSector(unsigned char* data, int size, int sector);

void readFromSector(unsigned char* data, int size, int sector);

int getDiskStatus();

#endif /*DISKIO_H*/
