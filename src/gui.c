#include "gui.h"
#include "cpu.h"
#include "signal.h"

/* TODO: This code is a hack job, and needs a rewrite */

void handle_winch(int sig) {
    destroygui();
    refresh(); /* This forces ncurses to pick up the new term size */

    clear();

    initgui();
    refreshRegisterDisplay();
    refreshMemoryDisplay();
    refreshStackDisplay();
    refreshStatusDisplay();
    refresh();

    signal(SIGWINCH, handle_winch); /* Required under c89 */
}

void initgui() {
    int maxX, maxY;

    if(winchHandler != &handle_winch) {
        initscr();
        winchHandler = &handle_winch;
        signal(SIGWINCH, handle_winch);
        rdisplaymode = 1;
    }

    nonl();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    typeahead(-1);

    getmaxyx(stdscr, maxY, maxX);
    initDimensions(maxX, maxY);

    /* Force main window to update so it is never redrawn */
    refresh();

    registers_b = newwin(rh, rw, ry, rx);
    mainmem_b = newwin(mh, mw, my, mx);
    stack_b = newwin(sh, sw, sy, sx);
    info_b = newwin(ih, iw, iy, ix);

    registers = subwin(registers_b, rh-2, rw-2, ry+1, rx+1);
    mainmem = subwin(mainmem_b, mh-2, mw-2, my+1, mx+1);
    stack = subwin(stack_b, sh-2, sw-2, sy+1, sx+1);
    info = subwin(info_b, ih-2, iw-2, iy+1, ix+1);

    scrollok(mainmem, 1);
    scrollok(stack, 1);

    idlok(stdscr, 1);
    idcok(stdscr, 1);

    leaveok(stdscr, 1);
    leaveok(stack, 1);
    leaveok(mainmem, 1);

    box(registers_b,0,0);
    box(mainmem_b,0,0);
    box(stack_b,0,0);
    box(info_b,0,0);
    mvwprintw(registers_b, 0, 2, " Registers ");
    mvwprintw(stack_b, 0, 2, " Stack ");
    mvwprintw(mainmem_b, 0, 2, " Memory ");

    wrefresh(registers_b);
    wrefresh(info_b);
    wrefresh(mainmem_b);
    wrefresh(stack_b);
    wrefresh(registers);
    wrefresh(info);
    wrefresh(mainmem);
    wrefresh(stack);
}

void destroygui() {
    delwin(registers);
    delwin(registers_b);
    delwin(mainmem);
    delwin(mainmem_b);
    delwin(stack);
    delwin(stack_b);
    delwin(registers);
    delwin(registers_b);
    delwin(info);
    delwin(info_b);
    endwin();
}

void initDimensions(int maxX, int maxY) {
    rx = 0;
    ry = 0;
    rw = maxX;
    rh = 12;

    mx = 0;
    my = rh;
    mw = (int)(maxX / MAIN_DISPLAY_HORIZONTAL_RATIO);
    mh = maxY - my - 2;

    sx = mw;
    sy = my;
    sw = maxX - sx;
    sh = mh;

    ix = 0;
    iy = my + mh - 1;
    iw = maxX;
    ih = 3;
}

void refreshRegisterDisplay() {
    werase(registers);

    if(rdisplaymode) {
        printRegister(registers, "A0", "", accumulator[0], 8, (amux == 0), 0);
        printRegister(registers, "A8", "\n", accumulator[8], 8, (amux == 8), 0);

        printRegister(registers, "A1", "", accumulator[1], 8, (amux == 1), 0);
        printRegister(registers, "A9", "\n", accumulator[9], 8, (amux == 9), 0);

        printRegister(registers, "A2", "", accumulator[2], 8, (amux == 2), 0);
        printRegister(registers, "A10", "\n", accumulator[10], 8, (amux == 10), 0);

        printRegister(registers, "A3", "", accumulator[3], 8, (amux == 3), 0);
        printRegister(registers, "A11", "\n", accumulator[11], 8, (amux == 11), 0);

        printRegister(registers, "A4", "", accumulator[4], 8, (amux == 4), 0);
        printRegister(registers, "A12", "\n", accumulator[12], 8, (amux == 12), 0);

        printRegister(registers, "A5", "", accumulator[5], 8, (amux == 5), 0);
        printRegister(registers, "A13", "\n", accumulator[13], 8, (amux == 13), 0);

        printRegister(registers, "A6", "", accumulator[6], 8, (amux == 6), 0);
        printRegister(registers, "A14", "\n", accumulator[14], 8, (amux == 14), 0);

        printRegister(registers, "A7", "", accumulator[7], 8, (amux == 7), 0);
        printRegister(registers, "A15", "\n", accumulator[15], 8, (amux == 15), 0);

        waddstr(registers, "\n");
        printRegister(registers, "Program Counter   ", "\n", pc, 16, 0, 1);


    } else {
        printRegister(registers, "CPU Flags         ", "\n", flags, 8, 0, 1);
        printRegister(registers, "MDR               ", "\n", mdr, 8, 0, 1);
        printRegister(registers, "IA                ", "\n", ia, 8, 0, 1);
        printRegister(registers, "Stack Pointer     ", "\n", stackpt, 8, 0, 1);
        printRegister(registers, "IR                ", "\n", ir, 16, 0, 1);
        printRegister(registers, "MAR               ", "\n", mar, 16, 0, 1);
        waddstr(registers, "\n");
        waddstr(registers, "\n");
        waddstr(registers, "\n");
        printRegister(registers, "Program Counter   ", "\n", pc, 16, 0, 1);
    }

    wnoutrefresh(registers);
}

void printMemory(WINDOW* win, int y, int x, int address, int value, int is_breakpoint, int is_pointed_to, char* symbol) {
    wmove(win, y, x);

    waddstr(win, is_pointed_to ? ">" : " ");
    waddstr(win, is_breakpoint ? "   @0x" : "    0x");

    printHexString(win, address, 16);
    wprintw(win, ":\t");
    printHexString(win, value, 8);
    wprintw(win, "\t");

    printBinaryString(win, value, 8);

    if(symbol) {
        wprintw(win, "\t");
        waddstr(win, symbol);
    }
}

void clearStatusDisplay() {
    int i = 0;
    wmove(info, 0, 0);
    for(; i < iw - 2; i++) {
        wprintw(info, " ");
    }

}

void refreshStatusDisplay() {
    clearStatusDisplay();
    mvwaddstr(info, 0,4, "Current state: 0x");
    printHexString(info, currentState, 8);
    wprintw(info, "  Instructions executed: %d  Cycles Executed: %d  Cycle Mode: %s",instructionCount, cycleCount,cyclemode ? ((cyclemode == 1) ? "INSTRUCTION" : "RUN") : "STATE");

    mvhline(iy, 0, 0, iw);
    mvvline(iy, 0, 0, 1);
    mvvline(iy, iw - 1, 0, 1);

    wnoutrefresh(info);
}

void refreshMemoryDisplay() {
    int i = 0;
    werase(mainmem);

    for(; i < mh - 2; i++) {
        if(i + memdisplay < 65536) {
            printMemory(
                    mainmem,
                    i,
                    0,
                    i + memdisplay,
                    memory[memdisplay + i],
                    breakpoints[memdisplay + i],
                    pc == (memdisplay + i),
                    symbols[memdisplay + i]);
        } else {
            wmove(mainmem, i, 0);
            waddch(mainmem, '~');
        }
    }

    wnoutrefresh(mainmem);
}

void refreshStackDisplay() {
    int i = 0;
    werase(stack);

    for(; i < sh - 2; i++) {
        if(i + stackdisplay < 256) {
            printMemory(
                    stack,
                    i,
                    0,
                    i + stackdisplay,
                    stackmem[stackdisplay + i],
                    0,
                    stackpt == (stackdisplay + i),
                    NULL);
        } else {
            wmove(stack, i, 0);
            waddch(stack, '~');
        }
    }

    wnoutrefresh(stack);
}

void refreshAll() {
    refreshMemoryDisplay();
    refreshStackDisplay();
    refreshRegisterDisplay();
    refreshStatusDisplay();
}

void printRegister(WINDOW* win, char* name, char* suffix, int value, int size, char mark, char doubleWordAlignment) {
    if(doubleWordAlignment) {
        wprintw(win, "[%-4s: %-5d :: 0x%-4s :: %16sb %c]%s",
                name,
                value,
                getHexString(value, size),
                getBinaryString(value, size),
                mark ? '<' : ' ',
                suffix);
    } else {
        wprintw(win, "[%-4s: %-3d :: 0x%-2s :: %8sb %c]%s",
                name,
                value,
                getHexString(value, size),
                getBinaryString(value, size),
                mark ? '<' : ' ',
                suffix);
    }
}

char* getBinaryString(unsigned int num, unsigned int size) {
    static char buffer[64]; /* Don't bother defining a constant for this size. The size should never change. */
    char* bufferptr = buffer;

    while(size > 0) {
        *bufferptr++ = (num >> --size) & 0x01 ? '1' : '0';
    }
    *bufferptr = '\0';

    return buffer;
}

void printBinaryString(WINDOW* win, unsigned int num, unsigned int size) {
    wprintw(win, "%s", getBinaryString(num, size));
}

char* getHexString(unsigned int num, unsigned int size) {
    static char buffer[16]; /* Don't bother defining a constant for this size. The size should never change. */
    char* bufferptr = buffer;

    while(size > 0) {
        *bufferptr++ = HEX[(num >> (size - 4)) & 0x0F];
        size -= 4;
    }
    *bufferptr = '\0';

    return buffer;
}

void printHexString(WINDOW* win, unsigned int num, unsigned int size) {
    wprintw(win, "%s", getHexString(num, size));
}
