#include "cpu.h"
#include "gui.h"
#ifdef PORT_EMU
#include "ports.h"
#endif

#include <unistd.h>

static void finish(int sig) {
    destroygui();
    exit(0);
}

void loadProgram(char* filename) {
    FILE* f = fopen(filename, "rb");
    if(f != NULL) {
        short magicnum = 0;
        short origin = 0;
        short size;

        fread(&magicnum, 2, 1, f);

        if(magicnum != 0x4250) {
            fclose(f);
            return;
        }

        fread(&size, 2, 1, f);

        fread(&origin, 2, 1, f);

        int i = 0;
        for(; i < size; i++) {
            fread(&memory[i + origin], 1, 1, f);
        }

        fclose(f);
        refreshMemoryDisplay();
    }
}

int main(int argc, char** argv) {
    int temp = 0;
    char docycle = 0;
    char running = 0; /* Needed so we can do a full GUI refresh if we break out of a run */
    int c;

    signal(SIGINT, finish);
    cyclemode = 0;

    initgui();
    resetMachine();

#ifdef PORT_EMU
    initPorts();
#endif

    refreshAll();

    for(; argc > 1; argc--)
        loadProgram(argv[argc-1]);

    refresh();

    for(;;) {
        if(c = getch(), c == ERR && !docycle) {
            /* Prevent this busy wait from consuming too much CPU time */
            usleep(10);
            continue;
        }

        /* This should be a switch-case block. if-elseif blocks probably won't generate optimal assembly */
        if(c == ' ') {
            docycle = !docycle;
        }else if(c == 'j' && memdisplay < 65535) {
            memdisplay++;
            scroll(mainmem);
            if(memdisplay < 65536 - (mh - 3))
                printMemory(mainmem, mh-3, 0, memdisplay + (mh - 3), memory[memdisplay + (mh - 3)], breakpoints[memdisplay + (mh - 3)], pc == (memdisplay + (mh - 3)));
            else
                mvwaddch(mainmem, mh - 3, 0, '~');
            wnoutrefresh(mainmem);
        } else if(c == 'k' && memdisplay > 0) {
            memdisplay--;
            wscrl(mainmem, -1);
            printMemory(mainmem, 0, 0, memdisplay, memory[memdisplay], breakpoints[memdisplay], pc == memdisplay);
            wnoutrefresh(mainmem);
        } else if(c == 'J' && stackdisplay < 255) {
            stackdisplay++;
            scroll(stack);
            if(stackdisplay < 256 - (sh -3))
                printMemory(stack, sh-3, 0, stackdisplay + (sh - 3), stackmem[stackdisplay + (sh - 3)], 0, stackpt == (stackdisplay + (sh - 3)));
            else
                mvwaddch(stack, sh - 3, 0, '~');
            wnoutrefresh(stack);

        } else if(c == 'K' && stackdisplay > 0) {
            stackdisplay--;
            wscrl(stack, -1);
            printMemory(stack, 0, 0, stackdisplay, stackmem[stackdisplay], 0, stackpt == stackdisplay);

            wnoutrefresh(stack);
        } else if(c == KEY_NPAGE) { /* page down */
            memdisplay += 0x0100;
            if(memdisplay > 0xFFFF)
                memdisplay = 0xFFFF;
            refreshMemoryDisplay();
        } else if(c == KEY_PPAGE) { /* page up */
            if(memdisplay > 0x0100)
                memdisplay -= 0x0100;
            else
                memdisplay = 0;
            refreshMemoryDisplay();
        } else if(c == 'b') {
            breakpoints[memdisplay] = (breakpoints[memdisplay] + 1) & 0x01;
            refreshMemoryDisplay();
        } else if(c == 'm') {
            cyclemode = (cyclemode + 1) % 3;
            refreshStatusDisplay();
        } else if(c == 'r'){
            rdisplaymode = !rdisplaymode;
            refreshRegisterDisplay();
        }else if(c == 'q' || c == 'Q') {
            break;
        }

        /* Perform a cycle if required */
        if(docycle) {
            do {
                cycle();
            } while(cyclemode && nextState != &x00);

            if(cyclemode != 2) {
                refreshMemoryDisplay();
                refreshStackDisplay();
                refreshRegisterDisplay();
            }
            refreshStatusDisplay();

            if(cyclemode != 2 || breakpoints[pc])
                docycle = 0;
            else
                running = 1;
        }

        if(!docycle && running) {
            running = 0;
            refreshAll();
        }

        refresh();
    }

    finish(0);
}

