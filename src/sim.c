#include "aprsim.h"
#include "argparser.h"
#include "cpu.h"
#include "gui.h"
#ifdef PORT_EMU
#include "ports.h"
#endif
#include <unistd.h>

Settings settings;

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

void loadSymbolsFile(char* filename) {
    FILE* f = fopen(filename, "r");

    if(f == NULL)
        return;

    char lineBuff[512];
    char symbolNameBuff[256];
    int symbolLocation;

    while(fgets(lineBuff, sizeof(lineBuff), f)) {
        sscanf(lineBuff, "%d:%[^\n]", &symbolLocation, symbolNameBuff);

        int symbolNameLen = strlen(symbolNameBuff);
        char* symbolName = (char*)malloc(sizeof(char) * (symbolNameLen + 1));
        memcpy(symbolName, symbolNameBuff, symbolNameLen + 1);

        if(symbols[symbolLocation])
            free(symbols[symbolLocation]);

        symbols[symbolLocation] = symbolName;
    }

    fclose(f);
}

void freeSymbolTable() {
    int i;
    for(i = 0; i < 65536; i++) {
        if(symbols[i])
            free(symbols[i]);
    }
}

static void finish(int sig) {
    destroygui();
    freeSettings(settings);
    freeSymbolTable();
    exit(0);
}

void mainloop() {
    char docycle = 0;
    char running = 0; /* Needed so we can do a full GUI refresh if we break out of a run */
    int c;
    MEVENT event;

    /* Set up curses for mouse handling (only tested in xterm) */
    mousemask(BUTTON1_PRESSED|BUTTON4_PRESSED|BUTTON5_PRESSED|REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0); /* Apparently fixes some bugs in some terminals */

    for(;;) {
        if(c = getch(), c == ERR && !docycle) {
            /* Prevent this busy wait from consuming too much CPU time */
            usleep(10);
            continue;
        }

        switch(c) {
            case KEY_MOUSE:
                if(getmouse(&event) == OK) {
                    if(event.bstate & BUTTON1_PRESSED) { /* Left click */
                        handleLeftClick(event.y, event.x);
                    } else if (event.bstate & BUTTON4_PRESSED) { /* Scroll up */
                        scrollSelectedDisplayUp(event.y, event.x,
                                (event.bstate & BUTTON_SHIFT) ? MOUSE_SCROLL_LINES * 2 : MOUSE_SCROLL_LINES);
                    } else if (event.bstate & BUTTON5_PRESSED) { /* Scroll down */
                        scrollSelectedDisplayDown(event.y, event.x,
                                (event.bstate & BUTTON_SHIFT) ? MOUSE_SCROLL_LINES * 2 : MOUSE_SCROLL_LINES);
                    }
                }
                break;
            case ' ':
                docycle = !docycle;
                break;
            case 'j':
                scrollMemoryDisplayDown(1);
                break;
            case 'k':
                scrollMemoryDisplayUp(1);
                break;
            case 'J':
                scrollStackDisplayDown(1);
                break;
            case 'K':
                scrollStackDisplayUp(1);
                break;
            case KEY_NPAGE: /* page down */
                memdisplay += 0x0100;
                if(memdisplay > 0xFFFF)
                    memdisplay = 0xFFFF;
                refreshMemoryDisplay();
                break;
            case KEY_PPAGE:
                if(memdisplay > 0x0100)
                    memdisplay -= 0x0100;
                else
                    memdisplay = 0;
                refreshMemoryDisplay();
                break;
            case 'b':
                breakpoints[memdisplay] = (breakpoints[memdisplay] + 1) & 0x01;
                refreshMemoryDisplay();
                break;
            case 'm':
                cyclemode = (cyclemode + 1) % 3;
                refreshStatusDisplay();
                break;
            case 'r':
                rdisplaymode = !rdisplaymode;
                refreshRegisterDisplay();
                break;
            case 'p':
                memdisplay = pc;
                refreshMemoryDisplay();
                break;
            case 'q':
            case 'Q':
            case KEY_F(1):
                return;
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
}

int main(int argc, char** argv) {
    int temp = 0;
    int i;

    settings = getSettingsFromArgs(argc, argv);

#ifdef TTY_EMU
    if(settings.fifoFile && settings.serialFile) {
        fprintf(stderr, "ERROR: Fifo file and serial file cannot be both set!\n");
        exit(EXIT_FAILURE);
    }
#endif /* TTY_EMU */

    signal(SIGINT, finish);
    cyclemode = 0;

    resetMachine();

#ifdef PORT_EMU
    initPorts(settings);
#endif

    for(i = 0; i < settings.binFileCount; i++)
        loadProgram(settings.binFiles[i]);

    if(settings.symbolsFile != NULL) {
        loadSymbolsFile(settings.symbolsFile);
    }

    initgui();
    refreshAll();
    refresh();

    mainloop();

    finish(0);

    return 0;
}
