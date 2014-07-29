#include "cpu.h"
#include "gui.h"


void resetMachine() { /* Clear the registers */
    int i;

    flags = 0;
    mdr = 0;
    ir = 0;
    mar = 0;
    pc = 0;
    amux = 0;
    stackpt = 255;

    for(i = 0; i < ACCUMULATOR_COUNT; i++)
        accumulator[i] = 0;


    nextState = &x00;
    currentState = 0;

    cycleCount = 0;
    instructionCount = 0;

    halted = 0;

    /* Clear any other variables */
    memdisplay = 0;
    stackdisplay = 0;

    /* Clear the main memory block */
    memset(memory, 0, 65536);
    memset(breakpoints, 0, 65536);

    stackmem = &memory[65536 - 256];

    refreshMemoryDisplay();
    refreshStackDisplay();
    refreshRegisterDisplay();
}

void cycle() {
    nextState();
    cycleCount++;
    if(nextState == &x00)
        instructionCount++;
}

void setflags() {
    /* Clear arithmetic flags */
    flags &= ~(NEG_FLAG | POS_FLAG | ZER_FLAG);

    if(accumulator[amux] & 0x80) /* Mimics hardware implementation */
        flags |= NEG_FLAG;
    else if(accumulator[amux])
        flags |= POS_FLAG;
    else
        flags |= ZER_FLAG;

}


void x00() {
    currentState = 0x00;
    mdr = memory[pc];
    /*mar = pc;*/
    nextState = &x01;
}

void x01() {
    currentState = 0x01;
    pc++;
    nextState = &x03;
}

void x02() {
    /* This state is no longer used */
    currentState = 0x02;
    /*mdr = memory[mar];*/
    nextState = &x03;
}

char imm4;
char imm3;
char imm2;
char f1;
char f2;
char f3;

void x03() {
    currentState = 0x03;
    ir = mdr;

    imm4 = ir & 0x0F;
    imm3 = ir & 0x07;
    imm2 = ir & 0x03;
    f1 = (ir & 0x08) != 0;
    f2 = (ir & 0x04) != 0;
    f3 = (ir & 0x02) != 0;

    switch((ir & 0xF0) >> 4) {
    case ADD:
        nextState = &x04;
        break;
    case AND:
        nextState = &x06;
        break;
    case OR:
        nextState = &x07;
        break;
    case XOR:
        nextState = &x08;
        break;
    case NOT:
        nextState = &x05;
        break;
    case SHF:
        nextState = &x09;
        break;
    case LD:
        nextState = &x0A;
        break;
    case LDI:
        nextState = &x0D;
        break;
    case ST:
        nextState = &x10;
        break;
    case STI:
        nextState = &x12;
        break;
    case STK:
        nextState = &x15;
        break;
    case LEA:
        nextState = &x1C;
        break;
    case BR:
        nextState = &x1F;
        break;
    case PRT:
        nextState = &x20;
        break;
    case ASET:
        nextState = &x22;
        break;
    default:
        nextState = &exception;
    }
}

void x04() {
    currentState = 0x04;
    accumulator[amux] += imm4;
    setflags();
    nextState = &x00;
}

void x05() {
    currentState = 0x05;
    accumulator[amux] = ~accumulator[amux];
    setflags();
    nextState = &x00;
}

void x06() {
    currentState = 0x06;
    accumulator[amux] &= (imm4 & 0x0F);
    setflags();
    nextState = &x00;
}

void x07() {
    currentState = 0x07;
    accumulator[amux] |= (imm4 & 0x0F);
    setflags();
    nextState = &x00;
}

void x08() {
    currentState = 0x08;
    accumulator[amux] ^= (imm4 & 0x0F);
    setflags();
    nextState = &x00;
}

void x09() {
    currentState = 0x09;
    if(f1)
        accumulator[amux] >>= 1;
    else
        accumulator[amux] <<= 1;
    setflags();
    nextState = &x00;
}

void x0A() {
    currentState = 0x0A;
    if(f1) /* In the hardware implementation, state 0A and 0C will be combined. */
        nextState = &x0C;
    else {
        mdr = memory[mar];
        nextState = &x0B;
    }
}

void x0B() {
    currentState = 0x0B;
    accumulator[amux] = mdr;
    nextState = &x00;
}

void x0C() {
    currentState = 0x0C;
    if(f2) {
        mar |= ((short)accumulator[amux]) << 8;
    } else {
        mar |= ((short)accumulator[amux]) & 0xFF;
    }
    nextState = &x00;
}

void x0D() {
    currentState = 0x0D;
    mar = pc;
    nextState = &x0E;
}

void x0E() {
    currentState = 0x0E;
    mar += imm4;
    nextState = &x0F;
}

void x0F() {
    currentState = 0x0F;
    mdr = memory[mar];
    nextState = &x0B;
}

void x10() {
    currentState = 0x10;
    if(f1) /* In the hardware implementation, state 10 and 0C will be combined */
        nextState = &x0C;
    else {
        mdr = accumulator[amux];
        nextState = &x11;
    }
}

void x11() {
    currentState = 0x11;
    memory[mar] = mdr;
    nextState = &x00;
}

void x12() {
    currentState = 0x12;
    mar = pc;
    nextState = &x13;
}

void x13() {
    currentState = 0x13;
    mar += imm4;
    nextState = &x14;
}

void x14() {
    currentState = 0x14;
    mdr = accumulator[amux];
    nextState = &x11;
}

void x15() {
    currentState = 0x15;
    if(f1)
        nextState = &x19;
    else
        nextState = &x17;
}

/* STK Push */
void x16() {
    currentState = 0x16;
    /* Replacement for x11 used by x1B */
    stackmem[stackpt] = mdr;
    nextState = &x00;
}

void x17() {
    currentState = 0x17;
    stackpt = ++stackpt & 0x00FF;
    nextState = &x18;
}

void x18() {
    currentState = 0x18;
    mdr = accumulator[amux];
    /*nextState = &x11;*/
    nextState = &x16;
}

/* STK Pop */
void x19() {
    currentState = 0x19;
    mdr = stackmem[stackpt];
    nextState = &x1A;
}

void x1A() {
    currentState = 0x1A;
    stackpt = (char)((stackpt - 1) & 0x00FF);
    nextState = &x1B;
}

void x1B() {
    currentState = 0x1B;
    if(f2) {
        switch(imm2) {
        case ADD:
            accumulator[amux] += mdr;
            break;
        case AND:
            accumulator[amux] &= mdr;
            break;
        case OR:
            accumulator[amux] |= mdr;
            break;
        case XOR:
            accumulator[amux] ^= mdr;
            break;
        }
    } else {
        accumulator[amux] = mdr;
    }

    nextState = &x00;
    setflags();
}

void x1C() {
    currentState = 0x1C;
    mar = pc;
    nextState = &x1D;
}

void x1D() {
    currentState = 0x1D;
    mar += imm3;
    nextState = &x1E;
}

void x1E() {
    currentState = 0x1E;
    if(f1)
        accumulator[amux] = (char)((mar >> 8) & 0x00FF);
    else
        accumulator[amux] = (char)(mar & 0x00FF);

    nextState = &x00;
}

void x1F() {
    currentState = 0x1F;
    if((f1 && flags & NEG_FLAG) || (f2 && flags & ZER_FLAG) || (f3 && flags & POS_FLAG))
        pc = mar;

    nextState = &x00;
}

void x20() {
    currentState = 0x20;
    /* NO CONNECTED PERIPHERALS */
    nextState = &x00;
}

/* No longer used */
void x21() {
    currentState = 0x21;
    /* move current accumulator into accumulator buffer */
    /*swapaccum[2] = accumulator;*/
    nextState = &x22;
}

void x22() {
    currentState = 0x22;
    amux = imm4;
    setflags();
    nextState = &x00;
}

void exception() {
    currentState = 0xFF;
    /* Some sort of exception */
    halted = 1;
}
