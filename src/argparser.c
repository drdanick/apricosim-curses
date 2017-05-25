#include "argparser.h"
#include "aprsim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void printHelp() {
    int i = 0;
    struct option coption;
    printf("%s usage:\n", PROGRAM_NAME);
#ifdef TTY_EMU
    printf("\taprsim [options] [-f file | -u file] [-s file] [-d file] [binfiles...]\n");
#else
    printf("\taprsim [options] [-s file] [-d file] [binfiles...]\n");
#endif /* TTY_EMU */
    printf("Options:\n");
    while(coption = long_options[i],
            !(coption.flag == 0
            && coption.has_arg == 0
            && coption.val == 0
            && coption.name == 0)) {
        printf("\t--%-20s -%-5c %s\n", coption.name, coption.val, option_descriptions[i]);
        i++;
    }
}

Settings getSettingsFromArgs(int argc, char** argv) {
    Settings s;
    int option_index;
    int c, i;

    s.serialFile = NULL;
    s.symbolsFile = NULL;
    s.hintsFile = NULL;
    s.fifoFile = NULL;
    s.binFiles = NULL;
#ifdef TTY_EMU
    while(c = getopt_long(argc, argv, "vhf:u:s:d:", long_options, NULL), c != -1) {
#else
    while(c = getopt_long(argc, argv, "vhs:d:", long_options, NULL), c != -1) {
#endif /* TTY_EMU */
        switch(c) {
            case 'v':
                printf("%s\n", PROGRAM_VERSION_STRING);
                exit(EXIT_SUCCESS);
                break;
            case 'h':
                printHelp();
                exit(EXIT_SUCCESS);
                break;
#ifdef TTY_EMU
            case 'f':
                s.fifoFile = optarg;
                break;
            case 'u':
                s.serialFile = optarg;
                break;
#endif /* TTY_EMU */
            case 's':
                s.symbolsFile = optarg;
                break;
            case 'd':
                s.hintsFile = optarg;
                break;
            case '?':
            default:
                exit(EXIT_FAILURE);
                break;
        }
    }

    /* Populate the file names array if necessary */
    s.binFileCount = argc - optind;
    option_index = optind;
    if(s.binFileCount > 0) {
        s.binFiles = (char**)malloc(sizeof(char*) * s.binFileCount);
    }

    i = 0;
    while(option_index < argc) {
        s.binFiles[i++] = argv[option_index++];
    }

    return s;
}

void freeSettings(Settings s) {
    if(s.binFiles)
        free(s.binFiles);
}
