#ifndef ARGPARSER_H
#define ARGPARSER_H

#include <unistd.h>
#include <getopt.h>

typedef struct {
    char* fifoFile;
    char* serialFile;
    char* symbolsFile;
    int binFileCount;
    char** binFiles;
} Settings;

const static struct option long_options[] = {
    {"version", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"fifo_tty_file", required_argument, 0, 'f'},
    {"serial_tty_file", required_argument, 0, 'u'},
    {"symbols", required_argument, 0, 's'},
    {0, 0, 0, 0}
};

const static char* option_descriptions[] = {
    "Print version information and exit",
    "Print this message and exit",
    "Set fifo tty file",
    "Set serial tty file",
    "Load symbols file"
};

Settings getSettingsFromArgs(int argc, char** argv);

void freeSettings(Settings settings);

#endif /* ARGPARSER_H */
