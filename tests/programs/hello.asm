; asmsyntax=apricos
;
; Puts the terminal in print mode and prints "HI\n" (port 7: 7F 00 48 49 0A),
; then idles. The program from APRICOS_DEVELOPMENT_GUIDE.md section 6.3.
;
#segment 0
ASET 1            ; also sets flags, so the JMP below is taken
LAR 0x7F
PRTout 7
AND 0
PRTout 7          ; TTY now in repeat-PUTC mode
LAR 0x48
PRTout 7          ; 'H'
LAR 0x49
PRTout 7          ; 'I'
LAR 0x0A
PRTout 7          ; newline
SPIN:
ASET 2
LAR 10
PRTout 0          ; sleep 100 ms
JMP SPIN
