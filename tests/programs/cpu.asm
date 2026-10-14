; asmsyntax=apricos
;
; Checks instruction semantics. Each result is written to port 7, in order;
; test_cpu.py lists what each byte should be.
;
; Segment 0: the checks
; Segment 1: FAR, the target of the far jump, which writes the last byte
;
#segment 0
    ; Flags are all clear at reset, so this branch is not taken
    JMP RESET_TAKEN
    ASET 1
    LAR 0x01
    PRTout 7             ; 0x01: fell through
    JMP ALU
RESET_TAKEN:
    ASET 1
    LAR 0xEE
    PRTout 7             ; 0xEE: the branch was taken
ALU:
    ; 0xFF + 1 carries out and leaves 0: flags O and Z
    LAR 0xFF
    ADD 1
    PRTin 4              ; $a1 = flags
    PRTout 7             ; 0x0A
    ; AND zero-extends its immediate
    LAR 0xFF
    AND 5
    PRTout 7             ; 0x05
    ; OR immediates only reach bits 3..0
    LAR 0xF0
    OR 3
    PRTout 7             ; 0xF3
    LAR 0x0F
    NOT
    PRTout 7             ; 0xF0
    LAR 0x03
    SHFl 4
    PRTout 7             ; 0x30
    LAR 0x80
    SHFr 4
    PRTout 7             ; 0x08: logical shift, zero fill
    ; Combine two registers through the stack: 7 + 5
    ASET 2
    LAR 5
    SPUSH
    ASET 1
    LAR 7
    SPOP ADD
    PRTout 7             ; 0x0C
    ; The stack pointer is back to its reset value after the pop
    PRTin 5
    PRTout 7             ; 0xFF
    ; Store and load through $mr
    LAh 0x70
    LAl 0x00
    LAR 0x5A
    ST
    LAR 0
    LD
    PRTout 7             ; 0x5A
    ; LDI reads data after the code
    LDI DATA
    PRTout 7             ; 0x99
    ; A bare JMP goes to $mr, here in another segment
    LAh FAR
    LAl FAR
    JMP
DATA:
    .fill 0x99
    .padseg 0

FAR:
    ASET 1
    LAR 0x42
    PRTout 7             ; 0x42
SPIN:
    ASET 2
    LAR 10
    PRTout 0             ; sleep 100 ms
    JMP SPIN
