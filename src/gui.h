#ifndef GUI_H
#define GUI_H

#include <curses.h>

/* Functions */
void initgui();
void destroygui();
void initDimensions(int maxX, int maxY);
void refreshRegisterDisplay();
void printMemory(WINDOW* win, int y, int x, int address, int value, int is_breakpoint, int is_pointed_to, char* symbol); 
void refreshStatusDisplay();
void refreshMemoryDisplay();
void refreshStackDisplay();
void refreshAll();
void printRegister(WINDOW* win, char* name, char* suffix, int value, int size, char mark);
void printBinaryString(WINDOW* win, unsigned int num, unsigned int size);
void printHexString(WINDOW* win, unsigned int num, unsigned int size);


/* Constants */
const static char HEX[16] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'A', 'B',
    'C', 'D', 'E', 'F'
};


/* Globals */

void (*winchHandler)();
WINDOW *registers_b;
WINDOW *registers;
WINDOW *mainmem_b;
WINDOW *mainmem;
WINDOW *stack_b;
WINDOW *stack;
WINDOW *info_b;
WINDOW *info;

int rx, ry, rw, rh, mx, my, mw, mh, sx, sy, sw, sh, ix, iy, iw, ih; /* GUI dimensions and coordinates */

unsigned int memdisplay; /* Memory display offset */
unsigned int stackdisplay; /* Stack display offset */

char cyclemode;
char rdisplaymode;

/* Constants */

#define MAIN_DISPLAY_HORIZONTAL_RATIO 1.6

#endif /* GUI_H */
