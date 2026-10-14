; asmsyntax=apricos
;
; Exercises the disk ports. Writes to port 7, in order:
;   the status after selecting disk 0 (0, or 0xFF if 0.dsk is missing);
;   the status after then selecting disk 5, which has no file;
;   the first byte of track 1, sector 2;
;   0x77 once it has written the buffer page, with 0xA5 at offset 0x10,
;   to track 2, sector 3.
;
#segment 0
    ASET 1
    AND 0
    PRTout 3             ; select disk 0
    PRTin 3
    PRTout 7
    LAR 5
    PRTout 3             ; select disk 5 (no file)
    PRTin 3
    PRTout 7
    ; Read track 1, sector 2 into 0xFE00
    LAR 1
    PRTout 4
    LAR 2
    PRTout 5
    LAh 0xFE
    LAl 0x00
    LD
    PRTout 7
    ; Write the buffer, with 0xA5 at 0xFE10, to track 2, sector 3
    LAl 0x10
    LAR 0xA5
    ST
    LAR 2
    PRTout 4
    LAR 3
    PRTout 6
    LAR 0x77
    PRTout 7             ; done
SPIN:
    ASET 2
    LAR 10
    PRTout 0             ; sleep 100 ms
    JMP SPIN
