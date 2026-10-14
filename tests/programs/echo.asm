; asmsyntax=apricos
;
; Reads keys with a blocking PRTin 2 and writes each one to port 7, forever.
;
#segment 0
ASET 1
LOOP:
PRTin 2           ; also sets flags, so the JMP is taken
PRTout 7
JMP LOOP
