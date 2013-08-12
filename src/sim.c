#include "cpu.h"
#include "gui.h"



static void finish(int sig) {
    delwin(&registers);
    delwin(&registers_b);
    delwin(&mainmem);
    delwin(&mainmem_b);
    delwin(&stack);
    delwin(&stack_b);
    delwin(&registers);
    delwin(&registers_b);
    endwin();

    exit(0);
}

int main(int argc, char** argv) {
    signal(SIGINT, finish);
    initgui();
    resetMachine();

    refresh();
    

    int temp = 0;
    for(;;) {
	/*usleep(50000);*/
	/*int c;
	if(temp == 0 && stackdisplay < 254)
	    c = 'J';
	else {
	    temp = 1;
	}

	if(temp == 1 && stackdisplay > 0)
	    c = 'K';
	else {
	    c = 'J';
	    temp = 0;
	}*/
	

	int c = getch();
	if(c == 'j' && memdisplay < 65535) {
	    memdisplay++;
	    scroll(&mainmem);
	    if(memdisplay < 65536 - (mh - 3))
		printMemory(&mainmem, mh-3, 0, memdisplay + (mh - 3), memory[memdisplay + (mh - 3)], breakpoints[memdisplay + (mh - 3)], pc == (memdisplay + (mh - 3)));
	    else
		mvwaddch(&mainmem, mh - 3, 0, '~');
	    wnoutrefresh(&mainmem);
	} else if(c == 'k' && memdisplay > 0) {
	    memdisplay--;
	    wscrl(&mainmem, -1);
	    printMemory(&mainmem, 0, 0, memdisplay, memory[memdisplay], breakpoints[memdisplay], pc == memdisplay);
	    wnoutrefresh(&mainmem);
	} else if(c == 'J' && stackdisplay < 255) {
	    stackdisplay++;
	    scroll(&stack);
	    if(stackdisplay < 256 - (sh -3))
		printMemory(&stack, sh-3, 0, stackdisplay + (sh - 3), stackmem[stackdisplay + (sh - 3)], 0, stackpt == (stackdisplay + (sh - 3)));
	    else
		mvwaddch(&stack, sh - 3, 0, '~');
	    wnoutrefresh(&stack);

	} else if(c == 'K' && stackdisplay > 0) {
	    stackdisplay--;
	    wscrl(&stack, -1);
	    printMemory(&stack, 0, 0, stackdisplay, stackmem[stackdisplay], 0, stackpt == stackdisplay);

	    wnoutrefresh(&stack);
	} else if(c == ' ') {
	    cycle();
	    refreshMemoryDisplay();
	    refreshStackDisplay();
	    refreshRegisterDisplay();

	    mvwaddstr(&mainmem_b, mh - 1,4, "Current state: 0x");
	    printHexString(&mainmem_b, currentState, 8);
	    wprintw(&mainmem_b, "  Instructions executed: %d  Cycles Executed: %d",instructionCount, cycleCount);
	    wnoutrefresh(&mainmem_b);

	} else if(c == 'b') {
	    breakpoints[memdisplay] = (breakpoints[memdisplay] + 1) & 0x01;
	    refreshMemoryDisplay();
	}
	refresh();
    }

    finish(0);
}

