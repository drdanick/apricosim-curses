#include "gui.h"
#include "cpu.h"
#include "signal.h"
#include "aprsim.h"
#include "disassembler.h"

/* TODO: This code is a hack job, and needs a rewrite */
unsigned char guiLock = 0;
unsigned char windowShouldResize = 0;

void doResize() {
    destroygui();
    refresh(); /* This forces ncurses to pick up the new term size */

    clear();

    initgui();
    refreshRegisterDisplay();
    refreshMemoryDisplay();
    refreshStackDisplay();
    refreshStatusDisplay();
    refresh();
}

void handle_winch(int sig) {
    if(!guiLock)
        doResize();
    else
        windowShouldResize = 1;

    signal(SIGWINCH, handle_winch); /* Required under c89 */
}

/* This should be called BEFORE any other GUI operation is called to
 * prevent corruption on resize.
 */
void lockGui() {
    guiLock = 1;
}

/* This should be called AFTER any other GUI operation is called to
 * prevent corruption on resize.
 */
void unlockGui() {
    if(windowShouldResize) {
        windowShouldResize = 0;
        doResize();
    }
    guiLock = 0;
}

void initgui() {
    int maxX, maxY;

    if(winchHandler != &handle_winch) {
        initscr();
        winchHandler = &handle_winch;
        signal(SIGWINCH, handle_winch);
        registerPage = 0;
        printDisassembly = 0;
    }

    curs_set(0);
    nonl();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    cbreak();
    noecho();
    typeahead(-1);

    getmaxyx(stdscr, maxY, maxX);
    initDimensions(maxX, maxY);
    initRegisterPageLayout();

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


    wrefresh(registers);
    wrefresh(info);
    wrefresh(mainmem);
    wrefresh(stack);
    refreshBorders();
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
    if(maxY < MINIMUM_WINDOW_HEIGHT) {
        maxY = MINIMUM_WINDOW_HEIGHT;
    }

    if(maxX < MINIMUM_WINDOW_WIDTH) {
        maxX = MINIMUM_WINDOW_WIDTH;
    }

    if(maxY > ALTERNATE_LAYOUT_WINDOW_HEIGHT_THRESHOLD) {
        registerDisplayLayout = 0;
        rh = 12;
    } else {
        registerDisplayLayout = 1;
        rh = 8;
    }

    rx = 0;
    ry = 0;
    rw = maxX;

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

void initRegisterPageLayout() {
    switch(registerDisplayLayout) {
        default:
        case 0:
            numberOfRegisterPages = 2;
            registerPage %= numberOfRegisterPages;
            break;
        case 1:
            numberOfRegisterPages = 4;
            registerPage %= numberOfRegisterPages;
            break;
    }
}

void displayNextRegisterPage() {
    lockGui();
    registerPage = (registerPage + 1) % numberOfRegisterPages;
    refreshRegisterDisplay();
    refreshBorders();
}

void displayPreviousRegisterPage() {
    lockGui();
    if(--registerPage < 0) registerPage = numberOfRegisterPages - 1;
    refreshRegisterDisplay();
    refreshBorders();
}

void refreshBorders() {
    lockGui();
    box(registers_b,0,0);
    box(mainmem_b,0,0);
    box(stack_b,0,0);
    box(info_b,0,0);
    mvwprintw(registers_b, 0, rw - sizeof(PROGRAM_VERSION_STRING) - 3, " " PROGRAM_VERSION_STRING " ");
    mvwprintw(registers_b, 0, 2, " Registers (%d/%d)", registerPage+1, numberOfRegisterPages);
    mvwprintw(stack_b, 0, 2, " Stack ");
    mvwprintw(mainmem_b, 0, 2, " Memory ");

    wrefresh(registers_b);
    wrefresh(info_b);
    wrefresh(mainmem_b);
    wrefresh(stack_b);
}

void refreshRegisterDisplay() {
    lockGui();
    werase(registers);

    switch(registerDisplayLayout) {
        default:
        case 0:
            switch(registerPage) {
                default:
                case 0:
                    printRegister(&registers, "A0", "", accumulator[0], 8, (amux == 0), 0);
                    printRegister(&registers, "A8", "\n", accumulator[8], 8, (amux == 8), 0);

                    printRegister(&registers, "A1", "", accumulator[1], 8, (amux == 1), 0);
                    printRegister(&registers, "A9", "\n", accumulator[9], 8, (amux == 9), 0);

                    printRegister(&registers, "A2", "", accumulator[2], 8, (amux == 2), 0);
                    printRegister(&registers, "A10", "\n", accumulator[10], 8, (amux == 10), 0);

                    printRegister(&registers, "A3", "", accumulator[3], 8, (amux == 3), 0);
                    printRegister(&registers, "A11", "\n", accumulator[11], 8, (amux == 11), 0);

                    printRegister(&registers, "A4", "", accumulator[4], 8, (amux == 4), 0);
                    printRegister(&registers, "A12", "\n", accumulator[12], 8, (amux == 12), 0);

                    printRegister(&registers, "A5", "", accumulator[5], 8, (amux == 5), 0);
                    printRegister(&registers, "A13", "\n", accumulator[13], 8, (amux == 13), 0);

                    printRegister(&registers, "A6", "", accumulator[6], 8, (amux == 6), 0);
                    printRegister(&registers, "A14", "\n", accumulator[14], 8, (amux == 14), 0);

                    printRegister(&registers, "A7", "", accumulator[7], 8, (amux == 7), 0);
                    printRegister(&registers, "A15", "\n", accumulator[15], 8, (amux == 15), 0);

                    waddstr(registers, "\n");
                    printRegister(&registers, "Program Counter ", "\n", pc, 16, 0, 1);


                    break;
                case 1:
                    printRegister(&registers, "CPU Flags       ", "\n", flags, 8, 0, 1);
                    printRegister(&registers, "MDR             ", "\n", mdr, 8, 0, 1);
                    printRegister(&registers, "IA              ", "\n", ia, 8, 0, 1);
                    printRegister(&registers, "Stack Pointer   ", "\n", stackpt, 8, 0, 1);
                    printRegister(&registers, "IR              ", "\n", ir, 16, 0, 1);
                    printRegister(&registers, "MAR             ", "\n", mar, 16, 0, 1);
                    waddstr(registers, "\n");
                    waddstr(registers, "\n");
                    waddstr(registers, "\n");
                    printRegister(&registers, "Program Counter ", "\n", pc, 16, 0, 1);
                    break;
            }
            break;
        case 1:
            switch(registerPage) {
                default:
                case 0:
                    printRegister(&registers, "A0", "", accumulator[0], 8, (amux == 0), 0);
                    printRegister(&registers, "A4", "\n", accumulator[4], 8, (amux == 4), 0);

                    printRegister(&registers, "A1", "", accumulator[1], 8, (amux == 1), 0);
                    printRegister(&registers, "A5", "\n", accumulator[5], 8, (amux == 5), 0);

                    printRegister(&registers, "A2", "", accumulator[2], 8, (amux == 2), 0);
                    printRegister(&registers, "A6", "\n", accumulator[6], 8, (amux == 6), 0);

                    printRegister(&registers, "A3", "", accumulator[3], 8, (amux == 3), 0);
                    printRegister(&registers, "A7", "\n", accumulator[7], 8, (amux == 7), 0);

                    waddstr(registers, "\n");
                    printRegister(&registers, "Program Counter ", "\n", pc, 16, 0, 1);
                    break;
                case 1:
                    printRegister(&registers, "A8", "", accumulator[8], 8, (amux == 8), 0);
                    printRegister(&registers, "A12", "\n", accumulator[12], 8, (amux == 12), 0);

                    printRegister(&registers, "A9", "", accumulator[9], 8, (amux == 9), 0);
                    printRegister(&registers, "A13", "\n", accumulator[13], 8, (amux == 13), 0);

                    printRegister(&registers, "A10", "", accumulator[10], 8, (amux == 10), 0);
                    printRegister(&registers, "A14", "\n", accumulator[14], 8, (amux == 14), 0);

                    printRegister(&registers, "A11", "", accumulator[11], 8, (amux == 11), 0);
                    printRegister(&registers, "A15", "\n", accumulator[15], 8, (amux == 15), 0);

                    waddstr(registers, "\n");
                    printRegister(&registers, "Program Counter ", "\n", pc, 16, 0, 1);
                    break;
                case 2:
                    printRegister(&registers, "CPU Flags       ", "\n", flags, 8, 0, 1);
                    printRegister(&registers, "MDR             ", "\n", mdr, 8, 0, 1);
                    printRegister(&registers, "IA              ", "\n", ia, 8, 0, 1);

                    waddstr(registers, "\n");
                    waddstr(registers, "\n");
                    printRegister(&registers, "Program Counter ", "\n", pc, 16, 0, 1);
                    break;
                case 3:
                    printRegister(&registers, "Stack Pointer   ", "\n", stackpt, 8, 0, 1);
                    printRegister(&registers, "IR              ", "\n", ir, 16, 0, 1);
                    printRegister(&registers, "MAR             ", "\n", mar, 16, 0, 1);

                    waddstr(registers, "\n");
                    waddstr(registers, "\n");
                    printRegister(&registers, "Program Counter ", "\n", pc, 16, 0, 1);
                    break;
            }
            break;
    }

    wnoutrefresh(registers);
}

void printMemory(WINDOW** win, int y, int x, int address, int addressSize, int value, int is_breakpoint, int is_pointed_to, int printDisassembly, char* symbol) {
    lockGui();
    static char suffixBuffer[64];
    wmove(*win, y, x);

    waddstr(*win, is_pointed_to ? ">" : " ");
    waddstr(*win, is_breakpoint ? "   @0x" : "    0x");

    printHexString(win, address, addressSize);
    wprintw(*win, ":    ");
    printHexString(win, value, 8);

    if(printDisassembly) {
        wprintw(*win, "  %-20.20s  ", disassembleAddress(address));
    } else {
        wprintw(*win, "     ");
    }

    printBinaryString(win, value, 8);

    if(symbol) {
        wprintw(*win, "  ");
        waddstr(*win, symbol);
    }
}

void clearStatusDisplay() {
    lockGui();
    int i = 0;
    wmove(info, 0, 0);
    for(; i < iw - 2; i++) {
        wprintw(info, " ");
    }

}

void refreshStatusDisplay() {
    lockGui();
    clearStatusDisplay();
    mvwaddstr(info, 0,3, "State: 0x");
    printHexString(&info, currentState, 8);
    wprintw(info, "  Instruction Count: %d  Cycle Count: %d  Mode: %s",instructionCount, cycleCount,cyclemode ? ((cyclemode == 1) ? "INSTRUCTION" : "RUN") : "STATE");

    mvhline(iy, 0, 0, iw);
    mvvline(iy, 0, 0, 1);
    mvvline(iy, iw - 1, 0, 1);

    wnoutrefresh(info);
}

void refreshMemoryDisplay() {
    lockGui();
    int i = 0;
    werase(mainmem);

    for(; i < mh - 2; i++) {
        if(i + memdisplay < 65536) {
            printMemory(
                    &mainmem,
                    i,
                    0,
                    i + memdisplay,
                    16,
                    memory[memdisplay + i],
                    breakpoints[memdisplay + i],
                    pc == (memdisplay + i),
                    printDisassembly,
                    symbols[memdisplay + i]);
        } else {
            wmove(mainmem, i, 0);
            waddch(mainmem, '~');
        }
    }

    wnoutrefresh(mainmem);
}

void refreshStackDisplay() {
    lockGui();
    int i = 0;
    werase(stack);

    for(; i < sh - 2; i++) {
        if(i + stackdisplay < 256) {
            printMemory(
                    &stack,
                    i,
                    0,
                    i + stackdisplay,
                    8,
                    stackmem[stackdisplay + i],
                    0,
                    stackpt == (stackdisplay + i),
                    0,
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

void handleLeftClick(int mouseY, int mouseX) {
    lockGui();
    if(wmouse_trafo(mainmem, &mouseY, &mouseX, 0) && memdisplay + mouseY < 65536) {
        breakpoints[memdisplay + mouseY] = (breakpoints[memdisplay + mouseY] + 1) & 0x01;
        refreshMemoryDisplay();
    }
}

void scrollSelectedDisplayUp(int mouseY, int mouseX, int lines) {
    lockGui();
    if(wenclose(stack_b, mouseY, mouseX)) {
        scrollStackDisplayUp(lines);
    } else {
        scrollMemoryDisplayUp(lines);
    }
}

void scrollSelectedDisplayDown(int mouseY, int mouseX, int lines) {
    lockGui();
    if(wenclose(stack_b, mouseY, mouseX)) {
        scrollStackDisplayDown(lines);
    } else {
        scrollMemoryDisplayDown(lines);
    }
}

void scrollMemoryDisplayUp(int lines){
    lockGui();
    int oldmemdisplay = memdisplay;
    int i;
    memdisplay -= lines;

    if(memdisplay < 0)
        memdisplay = 0;

    /* Scroll */
    wscrl(mainmem, memdisplay - oldmemdisplay);

    /* Draw new lines */
    for(i = oldmemdisplay - 1; i >= memdisplay; i--)
        printMemory(&mainmem, i - memdisplay, 0, i, 16, memory[i], breakpoints[i], pc == i, printDisassembly, symbols[i]);
    wnoutrefresh(mainmem);
}

void scrollMemoryDisplayDown(int lines){
    lockGui();
    int oldmemdisplay = memdisplay;
    int drawOffset = mh - 3;
    int i;
    int actualLines;

    memdisplay += lines;
    if(memdisplay > 65535)
        memdisplay = 65535;
    actualLines = memdisplay - oldmemdisplay;

    /* Scroll */
    wscrl(mainmem, actualLines);

    /* Draw new lines */
    for(i = oldmemdisplay; i <= memdisplay; i++) {
        if(i < (65536 - drawOffset))
            printMemory(&mainmem, drawOffset - actualLines--, 0, i + drawOffset, 16, memory[i + drawOffset], breakpoints[i + drawOffset], pc == (i + drawOffset), printDisassembly, symbols[i + drawOffset]);
        else
            mvwaddch(mainmem, drawOffset - actualLines--, 0, '~');
    }
    wnoutrefresh(mainmem);
}

void scrollStackDisplayUp(int lines){
    lockGui();
    int oldstackdisplay = stackdisplay;
    int i;

    stackdisplay -= lines;
    if(stackdisplay < 0)
        stackdisplay = 0;

    /* Scroll */
    wscrl(stack, stackdisplay - oldstackdisplay);

    /* Draw new lines */
    for(i = oldstackdisplay - 1; i >= stackdisplay; i--)
        printMemory(&stack, i - stackdisplay, 0, i, 8, stackmem[i], 0, stackpt == i, 0, NULL);
    wnoutrefresh(stack);
}

void scrollStackDisplayDown(int lines) {
    lockGui();
    int oldstackdisplay = stackdisplay;
    int drawOffset = sh - 3;
    int actualLines;
    int i;

    stackdisplay += lines;
    if(stackdisplay > 255)
        stackdisplay = 255;
    actualLines = stackdisplay - oldstackdisplay;

    /* Scroll */
    wscrl(stack, actualLines);

    /* Draw new lines */
    for(i = oldstackdisplay; i <= stackdisplay; i++) {
        if(i < (256 - drawOffset))
            printMemory(&stack, drawOffset - actualLines--, 0, i + drawOffset, 8, stackmem[i + drawOffset], 0, stackpt == (i + drawOffset), 0, NULL);
        else
            mvwaddch(stack, drawOffset - actualLines--, 0, '~');
    }
    wnoutrefresh(stack);
}

void printRegister(WINDOW** win, char* name, char* suffix, int value, int size, char mark, char doubleWordAlignment) {
    lockGui();
    if(doubleWordAlignment) {
        wprintw(*win, "[%-4s: %-5d :: 0x%-4s :: %16sb %c]%s",
                name,
                value,
                getHexString(value, size),
                getBinaryString(value, size),
                mark ? '<' : ' ',
                suffix);
    } else {
        wprintw(*win, "[%-4s: %-3d :: 0x%-2s :: %8sb %c]%s",
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

void printBinaryString(WINDOW** win, unsigned int num, unsigned int size) {
    lockGui();
    wprintw(*win, "%s", getBinaryString(num, size));
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

void printHexString(WINDOW** win, unsigned int num, unsigned int size) {
    lockGui();
    wprintw(*win, "%s", getHexString(num, size));
}
