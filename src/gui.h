#ifndef GUI_H
#define GUI_H

#include <curses.h>

/* Functions */
void initgui();
void destroygui();
void initDimensions(int maxX, int maxY);
void initRegisterPageLayout();
void displayNextRegisterPage();
void displayPreviousRegisterPage();
void refreshRegisterDisplay();
void printMemory(WINDOW** win, int y, int x, int address, int addressSize, int value, int is_breakpoint, int is_pointed_to, int printDisassembly, char* symbol);
void refreshBorders();
void refreshStatusDisplay();
void refreshMemoryDisplay();
void refreshStackDisplay();
void refreshAll();
void handleLeftClick(int mouseY, int mouseX);
void scrollSelectedDisplayUp(int mouseY, int mouseX, int lines);
void scrollSelectedDisplayDown(int mouseY, int mouseX, int lines);
void scrollMemoryDisplayUp(int lines);
void scrollMemoryDisplayDown(int lines);
void scrollStackDisplayUp(int lines);
void scrollStackDisplayDown(int lines);
void printRegister(WINDOW** win, char* name, char* suffix, int value, int size, char mark, char doubleWordAlignment);
char* getBinaryString(unsigned int num, unsigned int size);
void printBinaryString(WINDOW** win, unsigned int num, unsigned int size);
char* getHexString(unsigned int num, unsigned int size);
void printHexString(WINDOW** win, unsigned int num, unsigned int size);
void lockGui();
void unlockGui();


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

int memdisplay; /* Memory display offset */
int stackdisplay; /* Stack display offset */
int printDisassembly; /* Non-zero if disassembly should be printed, zero otherwise */

unsigned char cyclemode;
unsigned char numberOfRegisterPages;
unsigned char registerDisplayLayout;
char registerPage;

/* Constants */

#define MAIN_DISPLAY_HORIZONTAL_RATIO 1.8
#define ALTERNATE_LAYOUT_WINDOW_HEIGHT_THRESHOLD 30
#define MINIMUM_WINDOW_HEIGHT 20
#define MINIMUM_WINDOW_WIDTH 75

#endif /* GUI_H */
