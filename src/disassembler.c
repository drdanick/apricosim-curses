/*
 * A simple disassembler for Apricos binaries
 */
#include "disassembler.h"
#include "cpu.h"
#include "gui.h"
#include <stdlib.h>
#include <string.h>

disassembler_t disassembler = {
    NULL,
    NULL
};

void initDisassembler(unsigned char* memory, char** symbols) {
    disassembler.memory = memory;
    disassembler.symbols = symbols;
}

char* getSymbolAtAddress(unsigned int address, unsigned char literalValue, char printLiteralInHex) {
    static char buffer[64];
    if(symbols[address]) {
        if(!printLiteralInHex) {
            sprintf(buffer, "%d (%s)", literalValue, symbols[address]);
        } else {
            sprintf(buffer, "x%s (%s)", getHexString(literalValue, 8), symbols[address]);
        }
    } else {
        if(!printLiteralInHex) {
            sprintf(buffer, "%d", literalValue);
        } else {
            sprintf(buffer, "x%s", getHexString(literalValue, 8));
        }
        return buffer;
    }

    return buffer;
}

char* disassembleInstruction(unsigned int address) {
    static char buffer[32];
    unsigned char data, nextData;
    char imm4;
    char imm3;
    char imm2;
    char f1;
    char f2;
    char f3;
    char f4;
    address &= 0xFFFF;
    buffer[0] = '\0';

    if(!disassembler.memory || !disassembler.symbols) {
        return buffer;
    }

    data = disassembler.memory[address];
    nextData = (address < 65535) ? disassembler.memory[address + 1] : 0;

    imm4 = data & 0x0F;
    imm3 = data & 0x07;
    imm2 = data & 0x03;
    f1 = (data & 0x08) != 0;
    f2 = (data & 0x04) != 0;
    f3 = (data & 0x02) != 0;
    f4 = (data & 0x01) != 0;

    switch(data >> 4) {
        case ADD:
            if(data != 0) {
                sprintf(buffer, "ADD %d", imm4);
            }
            break;
        case AND:
            sprintf(buffer, "AND %d", imm4);
            break;
        case OR:
            sprintf(buffer, "OR %d", imm4);
            break;
        case XOR:
            sprintf(buffer, "XOR %d", imm4);
            break;
        case NOT:
            sprintf(buffer, "NOT");
            break;
        case SHF:
            sprintf(buffer, "SHF%c %d", f1 ? 'r' : 'l', imm3 + 1);
            break;
        case LD:
            if(f1) {
                sprintf(buffer, "LDa%c", f2 ? 'h' : 'l');
            } else {
                sprintf(buffer, "LD");
            }
            break;
        case LDI:
            sprintf(buffer, "LDI %s", getSymbolAtAddress(address + nextData + 2, nextData, 0));
            break;
        case ST:
            if(f1) {
                sprintf(buffer, "STa%c", f2 ? 'h' : 'l');
            } else {
                sprintf(buffer, "ST");
            }
            break;
        case STI:
            sprintf(buffer, "STI %s", getSymbolAtAddress(address + nextData + 2, nextData, 0));
            break;
        case STK:
            if(f2) {
                switch(imm2) {
                    case ADD:
                        sprintf(buffer, "S%s ADD", f1 ? "POP" : "PUSH");
                        break;
                    case AND:
                        sprintf(buffer, "S%s AND", f1 ? "POP" : "PUSH");
                        break;
                    case OR:
                        sprintf(buffer, "S%s OR", f1 ? "POP" : "PUSH");
                        break;
                    case XOR:
                        sprintf(buffer, "S%s XOR", f1 ? "POP" : "PUSH");
                        break;
                }
            } else {
                sprintf(buffer, "S%s", f1 ? "POP" : "PUSH");
            }
            break;
        case LA:
            if(f1) {
                sprintf(buffer, "LA%sh x%s", f2 ? "R" : "", getHexString(nextData, 8));
            } else {
                sprintf(buffer, "LA%sl %s", f2 ? "R" : "", getSymbolAtAddress((address & 0xFF00) | nextData, nextData, 1));
            }
            break;
        case BR:
            if(f1 && f2 && f3) {
                sprintf(buffer, "JMP %s",
                        f4 ? getSymbolAtAddress((address & 0xFF00) | nextData, nextData, 1) : ""
                        );
            } else {
                sprintf(buffer, "BR%s%s%s %s",
                        f1 ? "n" : "",
                        f2 ? "z" : "",
                        f3 ? "p" : "",
                        f4 ? getSymbolAtAddress((address & 0xFF00) | nextData, nextData, 1) : ""
                        );
            }
            break;
        case PRT:
            sprintf(buffer, "PRT%s %d", f1 ? "out" : "in", imm3);
            break;
        case ASET:
            sprintf(buffer, "ASET %d", imm4);
            break;
        default:
            break;
    }

    return buffer;
}

char* disassembleData(unsigned int address, char numeric) {
    static char buffer[8];

    address &= 0xFFFF;
    buffer[0] = '\0';

    if(!disassembler.memory || !disassembler.symbols) {
        return buffer;
    }

    if(numeric) {
        sprintf(buffer, "%u", memory[address]);
    } else {
        if(memory[address] >= ' ') { /* Only print printable characters. (Everything before a space in ASCII is non-printable) */
            sprintf(buffer, "'%c'", (memory[address]));
        } else { /* ...if it's not printable, check if it's a control character */
            switch(memory[address]) { /* We need to specify the actual bytes here. Escape codes may expand to more than one byte under some OS' */
                case 0:
                    sprintf(buffer, "'\\0'");
                    break;
                case 9:
                    sprintf(buffer, "'\\t'");
                    break;
                case 10:
                    sprintf(buffer, "'\\n'");
                    break;
                case 13:
                    sprintf(buffer, "'\\r'");
                    break;
                default:
                    break;
            }
        }
    }

    return buffer;
}

char* disassembleAddress(unsigned int address) {
    static char nothing = '\0';

    switch(hints[address]) {
        case HINT_NDAT:
            return disassembleData(address, 1);
        case HINT_CDAT:
            return disassembleData(address, 0);
        case HINT_IARG:
            return &nothing;
        case HINT_INST:
        default:
            return disassembleInstruction(address);
    }
}
