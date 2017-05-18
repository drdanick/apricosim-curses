#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

typedef struct {
    unsigned char* memory;
    char** symbols;
} disassembler_t;

void initDisassembler(unsigned char* memory, char** symbols);
char* disassembleAddress(unsigned int address);

#endif /* DISASSEMBLER_H */
