; asmsyntax=apricos
;
; Loaded at 0x8000: writes 0x48 to port 7 and idles. jumphigh.asm jumps here.
;
#segment 128
    ASET 1
    LAR 0x48
    PRTout 7
SPIN:
    ASET 2
    LAR 10
    PRTout 0             ; sleep 100 ms
    JMP SPIN
