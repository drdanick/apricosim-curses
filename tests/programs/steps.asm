; asmsyntax=apricos
;
; Eight NOPs, then writes 0x42 to port 7 and idles. For stepping and
; breakpoint tests: the NOPs are at 0x0000-0x0007, MARK at 0x0008.
;
#segment 0
NOP
NOP
NOP
NOP
NOP
NOP
NOP
NOP
MARK:
ASET 1
LAR 0x42
PRTout 7
SPIN:
ASET 2
LAR 10
PRTout 0          ; sleep 100 ms
JMP SPIN
