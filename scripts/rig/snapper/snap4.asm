; SNAP4.COM -- guest-side capture probe, v4 (docs/46).
;
; Host-side capture on this machine is measured dead: `screencapture -x` returns an all-black image even
; with a real Metal window ("DISPLAY: Using 1671x1253 window size in windowed mode on display-0"), System
; Events reports 0 windows for the dosbox-staging process, and 0.83 has no shell-internal capture command
; ("Illegal command: capture", measured by redirecting it to a file).  So the frames have to come from
; inside the guest.
;
; v3 dumped from the INT 21h hook and captured nothing: a DOS4GW guest reaches DOS through the DPMI
; callback, not through the real-mode IVT.  INT 1Ch *does* fire in-game (v1 wrote a 65 KB tick log from
; there), so v4 writes from INT 1Ch only, and reports enough to tell a missing hook from a failed write:
;
;   C:\S4LOG.BIN  16-byte records, one per ~1 s (18 ticks):
;       dw ticks   INT 8 hits
;       dw t1c     INT 1C hits
;       dw flag    = 0x5334 ("S4") marker, plus bit 8 set on install-context records
;       db indos   byte at 0050:0080 as seen in the tick context (0 = DOS idle, so a write is legal)
;       db err     which DOS call failed: 0 none, 1 open, 2 seek, 3 write
;       dw wrote   bytes DOS reports as taken by AH=40h
;       dw handle  handle used
;       dw seq     record counter
;   C:\S4VID.BIN  2048-byte sample of the 0xA0000 window, appended every 4th record, so the byte path for a
;                 framebuffer dump can be validated before paying for 64 KB per frame.
;
; Fixes over the earlier probe (the `mov ax,0x4202 / mov bx,ax` pair in snapprobe.asm handed the *function
; number* to DOS as the file handle): the handle is loaded from our own variable, the log is deleted before
; being created, the INDOS byte is *reported* rather than used to gate the write, and each DOS call's error
; is recorded separately.

BITS 16
ORG 100h

RECORD_EVERY equ 18
VID_EVERY    equ 4
SAMPLE       equ 2048

section .text

start:
    call delfiles
    ; create both files here, so the tick-context open() below always has something to append to
    ; (the tick path deliberately does not create: an open failure there must be visible as err=1).
    mov ah, 0x3C
    xor cx, cx
    mov dx, logname
    int 0x21
    jc .nofile1
    mov bx, ax
    mov ah, 0x3E
    int 0x21
.nofile1:
    mov ah, 0x3C
    xor cx, cx
    mov dx, vidname
    int 0x21
    jc .nofile2
    mov bx, ax
    mov ah, 0x3E
    int 0x21
.nofile2:

    ; INT 8 (timer IRQ) and INT 1Ch (timer tick end).
    xor ax, ax
    mov es, ax
    mov bx, 8 * 4
    mov ax, [es:bx]
    mov [cs:old8_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old8_seg], ax
    mov word [es:bx], int8_hook
    mov word [es:bx + 2], cs

    mov bx, 0x1C * 4
    mov ax, [es:bx]
    mov [cs:old1c_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old1c_seg], ax
    mov word [es:bx], int1c_hook
    mov word [es:bx + 2], cs

    ; Record zero, written from the install (foreground) context: proof the write path works at all.
    call mkrecord
    ; A video sample from the install context too, so the sample path has a known-good first block.
    call copy_sample
    call vidwrite

.resident:
    mov dx, msg
    mov ah, 0x09
    int 0x21
    mov dx, hook_end
    add dx, 15
    shr dx, 4
    int 0x27

int8_hook:
    inc word [cs:ticks]
    db 0xEA
old8_off dw 0
old8_seg dw 0

int1c_hook:
    inc word [cs:t1c]
    cmp byte [cs:busy], 0
    jne .chain
    mov ax, [cs:t1c]
    sub ax, [cs:lastrec]
    cmp ax, RECORD_EVERY
    jb .chain
    mov [cs:lastrec], ax
    mov byte [cs:busy], 1

    pusha
    push ds
    push es

    ; record fields, filled in the tick context
    mov ax, [cs:ticks]
    mov [cs:rec + 0], ax
    mov ax, [cs:t1c]
    mov [cs:rec + 2], ax
    mov word [cs:rec + 4], 0x5334
    mov ax, [cs:seq]
    inc ax
    mov [cs:seq], ax
    mov [cs:rec + 14], ax

    ; what does DOS think its own reentrancy state is right here?
    push ds
    mov ax, 0x0050
    mov ds, ax
    mov al, [ds:0x0080]
    pop ds
    mov byte [cs:rec + 6], al
    mov byte [cs:rec + 7], 0

    call mkrecord

    ; every 4th record, also append a window sample
    mov ax, [cs:seq]
    xor dx, dx
    mov bx, VID_EVERY
    div bx
    test dx, dx
    jnz .done
    call copy_sample
    call vidwrite

.done:
    pop es
    pop ds
    popa
    mov byte [cs:busy], 0
.chain:
    db 0xEA
old1c_off dw 0
old1c_seg dw 0

; mkrecord: open/seek-end/write/close of the 16-byte record, errors recorded.  DS must be cs.
mkrecord:
    mov ah, 0x3D
    mov al, 2
    mov dx, logname
    int 0x21
    jc .open_failed
    mov [cs:handle], ax
    mov bx, ax                       ; THE fix: BX = the handle, not the function number
    mov ax, 0x4202                   ; seek, from end
    xor cx, cx
    xor dx, dx
    int 0x21
    jc .seek_failed
    mov ah, 0x40
    mov bx, [cs:handle]
    mov cx, 16
    mov dx, rec
    int 0x21
    jc .write_failed
    mov [cs:rec + 10], ax            ; bytes taken
    jmp .close
.write_failed:
    mov byte [cs:rec + 7], 3
    xor ax, ax
    mov [cs:rec + 10], ax
    jmp .close
.seek_failed:
    mov byte [cs:rec + 7], 2
    jmp .close
.open_failed:
    mov byte [cs:rec + 7], 1
    jmp .end
.close:
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21
.end:
    ret

; copy_sample: 2048 bytes from the 0xA0000 window into our own buffer (INT 21h AH=40 reads DS:DX, so the
; sample has to be copied out of the VGA window before the write).
copy_sample:
    push ds
    push es
    push si
    push di
    mov ax, 0xA000
    mov ds, ax
    mov ax, cs
    mov es, ax
    xor si, si
    mov di, vidbuf
    mov cx, SAMPLE / 2
    cld
    rep movsw
    pop di
    pop si
    pop es
    pop ds
    ret

; vidwrite: append the sample to C:\S4VID.BIN.
vidwrite:
    mov ah, 0x3D
    mov al, 2
    mov dx, vidname
    int 0x21
    jc .v_end
    mov [cs:handle], ax
    mov bx, ax
    mov ax, 0x4202
    xor cx, cx
    xor dx, dx
    int 0x21
    mov ah, 0x40
    mov bx, [cs:handle]
    mov cx, SAMPLE
    mov dx, vidbuf
    int 0x21
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21
.v_end:
    ret

delfiles:
    mov ah, 0x41
    mov dx, logname
    int 0x21
    mov ah, 0x41
    mov dx, vidname
    int 0x21
    ret

msg db 'SNAP4 resident', 13, 10, '$'
logname db 'C:\S4LOG.BIN', 0
vidname db 'C:\S4VID.BIN', 0

ticks   dw 0
t1c     dw 0
lastrec dw 0
seq     dw 0
busy    db 0
handle  dw 0

rec:
    times 16 db 0

vidbuf:
    times SAMPLE db 0

hook_end:
