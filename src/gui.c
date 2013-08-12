#include "gui.h"
#include "cpu.h"

void initgui() {
    int maxX, maxY;
    initscr();
    /*keypad(stdscr, TRUE);*/
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
    /*rh = maxY / 3;*/
    rh = 10;
    
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
    printRegister(&registers, "Accumulator     ", "\n", accumulator, 8);
    printRegister(&registers, "Stack Pointer   ", "\n", stackpt, 8);
    printRegister(&registers, "CPU Flags       ", "\n", flags, 8);
    waddstr(&registers, "\n");
    printRegister(&registers, "MDR             ", "\n", mdr, 8);
    printRegister(&registers, "IR              ", "\n", ir, 8);
    printRegister(&registers, "Program Counter ", "\n", pc, 16);
    printRegister(&registers, "MAR             ", "\n", mar, 16);

    /*wrefresh(&registers);*/

    wnoutrefresh(&registers);

}

void printMemory(WINDOW* win, int y, int x, int address, int value, int is_breakpoint, int is_pointed_to) {
    wmove(win, y, x);

    waddstr(win, is_pointed_to ? ">" : " ");
    waddstr(win, is_breakpoint ? "   @0x" : "    0x");
    
    /*mvwprintw(win, y, x, "0x");*/
    printHexString(win, address, 16);
    wprintw(win, ":\t");
    printHexString(win, value, 8);
    wprintw(win, "\t");

    printBinaryString(win, value, 8);
    /*wprintw(win, "\n");*/
}

void refreshMemoryDisplay() {
    int i = 0;
    werase(&mainmem);
    int j = 0;
    memory[j++] = 0x0B;
    memory[j++] = 0x50;
    memory[j++] = 0x50;
    memory[j++] = 0x50;
    memory[j++] = 0x50;
    memory[j++] = 0x0A;
    memory[j++] = 0xA0;
    memory[j++] = 0xAF;
    for(; i < mh - 2 && i + memdisplay < 65536; i++) {
	printMemory(&mainmem, i, 0, i + memdisplay, memory[memdisplay + i], breakpoints[memdisplay + i], pc == (memdisplay + i));
	/* TODO: print the binary value too! (And get rid of the test values below! */
	/*wprintw(&mainmem, "0x");
	printHexString(&mainmem, i + memdisplay, 16);
	wprintw(&mainmem, ":\t");
	printHexString(&mainmem, memory[memdisplay + i], 8);
	wprintw(&mainmem, "\t");
	printBinaryString(&mainmem, memory[memdisplay + i++], 8);
	wprintw(&mainmem, "\n");*/

    }
    
    wnoutrefresh(&mainmem);
    /*wrefresh(&mainmem);*/
}

void refreshStackDisplay() {
    int i = 0;
    werase(&stack);

    for(; i < sh - 2 && i + stackdisplay < 256; i++) {
	printMemory(&stack, i, 0, i + stackdisplay, stackmem[stackdisplay + i], 0, stackpt == (stackdisplay + i));
	/*wprintw(&stack, "0x");
	printHexString(&stack, i + stackdisplay, 16);
	wprintw(&stack, ":\t");
	printHexString(&stack, stackmem[stackdisplay + i], 8);
	wprintw(&stack, "\t");
	printBinaryString(&stack, stackmem[stackdisplay + i++], 8);
	wprintw(&stack, "\n");*/

    }
    wnoutrefresh(&stack);
    
}

void printRegister(WINDOW* win, char* name, char* suffix, int value, int size) {
    wprintw(win, "[%s:  %d\t:: 0x", name, value);
    printHexString(win, value, size);
    wprintw(win, " :: ");
    printBinaryString(win, value, size);
    wprintw(win, "b\t]%s",suffix);
}

void printBinaryString(WINDOW* win, unsigned int num, unsigned int size) {
    while(size > 0) {
	waddch(win, (num >> --size) & 0x01 ? '1' : '0');
    }
}

void printHexString(WINDOW* win, unsigned int num, unsigned int size) {
    while(size > 0) {
	waddch(win, HEX[(num >> size - 4) & 0x0F]);

	/*num >>= 4;*/
	size -= 4;
    }
}
