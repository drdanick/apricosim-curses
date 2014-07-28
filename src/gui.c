#include "gui.h"
#include "cpu.h"

void initgui() {
    int maxX, maxY;
    initscr();
    nonl();
    cbreak();
    noecho();
    typeahead(-1);

    getmaxyx(stdscr, maxY, maxX);
    initDimensions(maxX, maxY);

    registers_b = *subwin(stdscr, rh, rw, ry, rx);
    mainmem_b = *subwin(stdscr, mh, mw, my, mx);
    stack_b = *subwin(stdscr, sh, sw, sy, sx);

    registers = *subwin(&registers_b, rh-2, rw-2, ry+1, rx+1);
    mainmem = *subwin(&mainmem_b, mh-2, mw-2, my+1, mx+1);
    stack = *subwin(&stack_b, sh-2, sw-2, sy+1, sx+1);


    scrollok(&mainmem, 1);
    scrollok(&stack, 1);
    
    idlok(stdscr, 1);
    idcok(stdscr, 1);

    leaveok(stdscr, 1);
    leaveok(&stack, 1);
    leaveok(&mainmem, 1);

    box(&registers_b,0,0);
    box(&mainmem_b,0,0);
    box(&stack_b,0,0);
    mvwprintw(&registers_b, 0, 2, " Registers ");
    mvwprintw(&stack_b, 0, 2, " Stack ");
    mvwprintw(&mainmem_b, 0, 2, " Memory ");
    wrefresh(&registers_b);
    wrefresh(&mainmem_b);
    wrefresh(&registers);
    wrefresh(&mainmem);
    wrefresh(&stack);

}

void initDimensions(int maxX, int maxY) {
    rx = 0;
    ry = 0;
    rw = maxX;
    rh = 13;
    
    mx = 0;
    my = rh;
    mw = maxX / 2;
    mh = maxY - my;

    sx = mw + 1;
    sy = my;
    sw = maxX - sx;
    sh = mh;
}


void refreshRegisterDisplay() {
    werase(&registers);
    printRegister(&registers, "Accumulator 0     ", "\n", accumulator[0], 8, (amux == 0));
    printRegister(&registers, "Accumulator 1     ", "\n", accumulator[1], 8, (amux == 1));
    printRegister(&registers, "Accumulator 2     ", "\n", accumulator[2], 8, (amux == 2));
    printRegister(&registers, "Accumulator 3     ", "\n", accumulator[3], 8, (amux == 3));
    printRegister(&registers, "Stack Pointer     ", "\n", stackpt, 8, 0);
    printRegister(&registers, "CPU Flags         ", "\n", flags, 8, 0);
    waddstr(&registers, "\n");
    printRegister(&registers, "MDR               ", "\n", mdr, 8, 0);
    printRegister(&registers, "IR                ", "\n", ir, 8, 0);
    printRegister(&registers, "Program Counter   ", "\n", pc, 16, 0);
    printRegister(&registers, "MAR               ", "\n", mar, 16, 0);


    wnoutrefresh(&registers);

}

void printMemory(WINDOW* win, int y, int x, int address, int value, int is_breakpoint, int is_pointed_to) {
    wmove(win, y, x);

    waddstr(win, is_pointed_to ? ">" : " ");
    waddstr(win, is_breakpoint ? "   @0x" : "    0x");
    
    printHexString(win, address, 16);
    wprintw(win, ":\t");
    printHexString(win, value, 8);
    wprintw(win, "\t");

    printBinaryString(win, value, 8);
}

void refreshMemoryDisplay() {
    int i = 0;
    werase(&mainmem);

    for(; i < mh - 2 && i + memdisplay < 65536; i++) {
        printMemory(&mainmem, i, 0, i + memdisplay, memory[memdisplay + i], breakpoints[memdisplay + i], pc == (memdisplay + i));

    }
    
    /* TODO: move status to a seperate window */
    mvwaddstr(&mainmem_b, mh - 1,4, "Current state: 0x");
    printHexString(&mainmem_b, currentState, 8);
    wprintw(&mainmem_b, "  Instructions executed: %d  Cycles Executed: %d  Cycle Mode: %s",instructionCount, cycleCount,cyclemode ? "C MODE" : "I MODE");
    wnoutrefresh(&mainmem);
    wnoutrefresh(&mainmem_b);
}

void refreshStackDisplay() {
    int i = 0;
    werase(&stack);

    for(; i < sh - 2 && i + stackdisplay < 256; i++) {
        printMemory(&stack, i, 0, i + stackdisplay, stackmem[stackdisplay + i], 0, stackpt == (stackdisplay + i));
    }
    wnoutrefresh(&stack);
    
}

void printRegister(WINDOW* win, char* name, char* suffix, int value, int size, char mark) {
    wprintw(win, "[%s:  %d\t:: 0x", name, value);
    printHexString(win, value, size);
    wprintw(win, " :: ");
    printBinaryString(win, value, size);
    if(mark)
        wprintw(win, "b\t*]%s",suffix);
    else
        wprintw(win, "b\t ]%s",suffix);
}

void printBinaryString(WINDOW* win, unsigned int num, unsigned int size) {
    while(size > 0) {
        waddch(win, (num >> --size) & 0x01 ? '1' : '0');
    }
}

void printHexString(WINDOW* win, unsigned int num, unsigned int size) {
    while(size > 0) {
        waddch(win, HEX[(num >> size - 4) & 0x0F]);

        size -= 4;
    }
}
