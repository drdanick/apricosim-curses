; asmsyntax=apricos
;
; Jumps to 0x8000, where high.asm is loaded.
;
#segment 0
    LAh 0x80
    LAl 0x00
    JMP
