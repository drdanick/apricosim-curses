#ifndef CPU_H
#define CPU_H

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* Functions */
void resetMachine();
void cycle();
void setflags();

/* CPU State functions */
/* Refer to the CPU state diagram for meanings */
void x00();
void x01();
void x02();
void x03();
void x04();
void x05();
void x06();
void x07();
void x08();
void x09();
void x0A();
void x0B();
void x0C();
void x0D();
void x0E();
void x0F();
void x10();
void x11();
void x12();
void x13();
void x14();
void x15();
void x16();
void x17();
void x18();
void x19();
void x1A();
void x1B();
void x1C();
void x1D();
void x1E();
void x1F();

void x20();

/* States required for SWP */
void x21();
void x22();

/* experimental */
void x23();
void x24();
void x25();

void exception();

/* Misc. Constants */
#define ACCUMULATOR_COUNT 16

/* OP Codes */
#define ADD   0
#define AND   1
#define OR    2
#define XOR   3
#define NOT   4
#define SHF   5
#define LD    6
#define LDI   7
#define ST    8
#define STI   9
#define STK   10
#define LEA   11
#define BR    12
#define PRT   13
#define ASET  14

/* CPU Flags */
#define SIG_FLAG 0x08
#define NEG_FLAG 0x04
#define ZER_FLAG 0x02
#define POS_FLAG 0x01

/* Other settings */
char halted;
char currentState;
int cycleCount, instructionCount;


/* Function pointers */
void (*nextState)();

/* Registers */
unsigned char  stackpt, flags, mdr, ir, ia;
unsigned char  accumulator[ACCUMULATOR_COUNT];
unsigned short mar;  /* Memory Address Register */
unsigned short pc;   /* Program Counter */
unsigned char  amux; /* Accumulator MUX pointer */

/* Memory blocks */
unsigned char memory[65536];
unsigned char breakpoints[65536];
unsigned char* stackmem;

#endif /*CPU_H*/
