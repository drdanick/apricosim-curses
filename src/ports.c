#ifdef PORT_EMU
#include "ports.h"
#include "cpu.h"
#include "gui.h"
#include "argparser.h"

#ifdef DISK_EMU
#include "diskio.h"
#endif

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char diskFileName[] = "x.dsk";
int inbuff = -1;

#ifdef TTY_EMU
int ttyHandle = 0;
int fd;
#endif

Settings settings;

void initPorts(Settings s) {
    settings = s;
#ifdef TTY_EMU
    /*Set up tty*/
    if(settings.fifoFile) {
        fd = open(settings.fifoFile, O_WRONLY);
    } else if(settings.serialFile) {
        struct termios options;

        ttyHandle = open(settings.serialFile, O_RDWR | O_NOCTTY);

        /* Set serial settings
         * (values are hard-coded for my uVGA board, which 
         * receives data at 9600 baud over USB. These can be reconfigured 
         * for other devices as necessary) */
        if(ttyHandle != -1) {
            tcgetattr(ttyHandle, &options);

            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);

            cfmakeraw(&options);
            options.c_cflag |= (CLOCAL | CREAD);   /* Enable the receiver and set local mode */
            options.c_cflag &= ~CSTOPB;            /* 1 stop bit */
            options.c_cc[VMIN]  = 1;
            options.c_cc[VTIME] = 2;

            tcsetattr(ttyHandle, TCSANOW, &options);
        } else {
            ttyHandle = 0;
        }
    }
#endif /*TTY_EMU*/
}

void portIO(unsigned int portId, unsigned int writeMode) {
    int diskID;
    int charin;
    if(!writeMode) {
        /* Read */
        switch(portId) {
            case 0:    /* NOP */
                break;
            case 1:    /* Keyboard Status Register */
                inbuff = getch();

                if(inbuff == KEY_F(1)) {
                    ungetch(inbuff);
                }
                

                if(inbuff > 0xFF)
                    inbuff = 0;

                if(inbuff == ERR) {
                    accumulator[amux] = 0;
                    inbuff = -1;
                } else {
                    accumulator[amux] = 1;
                }

                

                break;
            case 2:    /* Keyboard Input */
                /* Block and poll for keyboard input unless a chaaracter was buffered by a status check */
                for(;;) {
                    charin = getch();

                    if(charin == KEY_F(1)) {
                        ungetch(charin);
                        break;
                    }

                    if(charin > 0xFF)
                        charin = 0;

                    accumulator[amux] = (inbuff != -1) ? inbuff : charin;
                    if(charin != ERR)
                        break;
                    usleep(10);
                }
                if(accumulator[amux] == 13) accumulator[amux] = 10;
                inbuff = -1;
                break;
            case 3:    /* Disk Status */
#ifdef DISK_EMU
                accumulator[amux] = getDiskStatus();
#else
                accumulator[amux] = 0;
#endif /* DISK_EMU */
                break;
            case 4:    /* NOP */
                break;
            case 5:    /* NOP */
                break;
            case 6:    /* NOP */
                break;
            case 7:    /* NOP */
                break;
            default:   /* NOP */
                break;
        }
    } else {
        /* Write */
        switch(portId) {
            case 0:    /* Sleep Request */
                usleep(accumulator[amux] * 10000); /* argument translates to 10 * msec */
                break;
            case 1:    /* Keyboard Mode Select */
                /* Do nothing */
                break;
            case 2:    /* NUL */
                break;
#ifdef DISK_EMU
            case 3:    /* Disk Select */
                diskID = accumulator[amux];
                if(diskID >= 0 && diskID < 9) {
                    diskFileName[0] = diskID + '0';
                    selectDiskFile(diskFileName, diskID);
                }
                break;
            case 4:    /* Disk Track Select */
                selectDiskTrack(accumulator[amux]);

                break;
            case 5:    /* Disk Read Sector */
                readFromSector(&memory[DISK_PAGE_SECTOR * BLOCK_SIZE], BLOCK_SIZE, accumulator[amux]);

                break;
            case 6:    /* Disk Write Sector */
                writeToSector(&memory[DISK_PAGE_SECTOR * BLOCK_SIZE], BLOCK_SIZE, accumulator[amux]);
#else
            case 3:
            case 4:
            case 5:
            case 6:
#endif /*DISK_EMU*/

                break;
            case 7:    /* TTY Write */
#ifdef TTY_EMU
                if(settings.fifoFile) {
                    write(fd, &accumulator[amux], 1);
                } else if(settings.serialFile) {
                    write(ttyHandle, &accumulator[amux], 1);
                    ioctl(ttyHandle, TCIOFLUSH);
                }
#endif /* TTY_EMU */
                break;
            default:   /* NOP */
                break;
        }
    }
}

#endif /*PORT_EMU*/
