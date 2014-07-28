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

void loadProgram(char* filename) {
    FILE* f = fopen(filename, "rb");
    if(f != NULL) {
        short magicnum = 0;
        short size;

        fread(&magicnum, 2, 1, f);

        if(magicnum != 0x4150) {
            fclose(f);
            return;
        }

        fread(&size, 2, 1, f);

        int i = 0;
        for(; i < size; i++) {
            fread(&memory[i], 1, 1, f);
        }

        fclose(f);
        refreshMemoryDisplay();
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, finish);
    cyclemode = 1;

    initgui();
    resetMachine();

    if(argc > 1)
        loadProgram(argv[1]);

    refresh();


    int temp = 0;
    for(;;) {
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
            do {
                cycle();
                refreshMemoryDisplay();
                refreshStackDisplay();
                refreshRegisterDisplay();
            } while (!cyclemode && nextState != &x00);

        } else if(c == 'b') {
            breakpoints[memdisplay] = (breakpoints[memdisplay] + 1) & 0x01;
            refreshMemoryDisplay();
        } else if(c == 'm') {
            cyclemode ^= 0x01;
            refreshMemoryDisplay();
        } else if(c == 'q' || c == 'Q') {
            break;
        }
        refresh();
    }

    finish(0);
}

