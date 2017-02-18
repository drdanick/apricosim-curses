#ifdef DISK_EMU
#include "diskio.h"
#include "cpu.h"

FILE* driveFile = NULL;
int diskID = 0xFFFF;
int track;

/* 8*256*256 disks (TRACKS*SECTORS*SECTOR_SIZE) */
#define SECTOR_SIZE 256
#define TRACK_SIZE  16384  /* (64 * 256) */


#define TRACK_ID_MASK  0x1F
#define SECTOR_ID_MASK 0x3F

/* old values
#define TRACK_ID_MASK  0x07
#define SECTOR_ID_MASK 0xFF
*/

void selectDiskFile(char* fileName, int id) {
    /* Check that the file exists */
    if(driveFile)
        fclose(driveFile);

    driveFile = fopen(fileName, "r+b");
    if(!driveFile)
        return;

    track = 0;
    diskID = id;
}

void selectDiskTrack(int t) {
    t &= TRACK_ID_MASK;
    track = t;
}

void writeToSector(unsigned char* data, int size, int sector) {
    if(!driveFile)
        return;

    sector &= SECTOR_ID_MASK;
    fseek(driveFile, (track * TRACK_SIZE) + (sector * SECTOR_SIZE), SEEK_SET);

    fwrite((char*)data, sizeof(char), size, driveFile);

    fflush(driveFile);
}


void readFromSector(unsigned char* data, int size, int sector) {
    if(!driveFile)
        return;

    int i;
    int c;
    sector &= SECTOR_ID_MASK;
    fseek(driveFile, (track * TRACK_SIZE) + (sector * SECTOR_SIZE), SEEK_SET);

    for(i = 0; i < size; i++) {
        c = fgetc(driveFile);
        if(c == -1) break;

        data[i] = (char)(c & 0xFF);
    }
}

int getDiskStatus() {
    if(driveFile) {
        return diskID;
    } else {
        return 0xFFFF;
    }
}

#endif /*DISK_EMU*/
