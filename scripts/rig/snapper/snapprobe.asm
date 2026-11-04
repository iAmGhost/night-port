; SNAPPROBE.COM -- minimal in-guest instrumentation for the DOSBox capture path (docs/46).
;
; Question this answers, in one run: while the DOS4GW game is running in protected mode, do the
; real-mode INT 8 / INT 1Ch / INT 10h / INT 21h hooks installed by a real-mode TSR still get called?
; v3 put its dump in the INT 21h hook (the DOS-reentrant-safe point) and captured nothing at all, which
; is what a DOS4GW guest that reaches DOS through the DPMI "real mode call" path -- or one that bypasses
; the real-mode IVT entirely -- would look like. Rather than guess, count.
;
; The probe appends one 16-byte record every ~1 s to C:\SNAPPROBE.BIN:
;   dw ticks        our INT 8 hits              (does the timer chain reach us in-game?)
;   dw t1c          our INT 1Ch hits            (does the tick-end chain reach us in-game?)
;   dw i21          our INT 21h hits            (does the guest's DOS traffic reach the RM IVT?)
;   dw i10          our INT 10h hits            (does the guest's video traffic reach the RM IVT?)
;   dd mode         last mode we saw set        (0x13 vs 0x101: is the VESA set-mode visible?)
;   dw err          DOS error from the last write (0 = write worked from this context)
;   dw wro          bytes DOS says it wrote
;
; All DOS work happens in the INT 1Ch handler. That context is not officially reentrant-safe; v1 wrote a
; 65,492-byte tick log from there, so it evidently does reach DOS in DOSBox -- the record below also
; records whether the write actually landed, so a 0-byte result is distinguishable from a missing hook.

BITS 16
ORG 100h

RECORD_EVERY equ 18                  ; ~1 record per second at 18.2 Hz

section .text

start:
    call mkA
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

    mov bx, 0x10 * 4
    mov ax, [es:bx]
    mov [cs:old10_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old10_seg], ax
    mov word [es:bx], int10_hook
    mov word [es:bx + 2], cs

    mov bx, 0x21 * 4
    mov ax, [es:bx]
    mov [cs:old21_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old21_seg], ax
    mov word [es:bx], int21_hook
    mov word [es:bx + 2], cs
    call mkB

    ; open the log once, keep the handle; from here on every record is a single INT 21h AH=40h
    ; no delete-first: DOSBox's DOS reports an error from AH=41h here and the create then fails; the
    ; host deletes the log before each run instead.
    mov ah, 0x3C
    xor cx, cx
    mov dx, logname
    int 0x21
    jc .cre_failed
    mov [cs:handle], ax
    mov ah, 0x40
    mov bx, ax
    mov cx, rec_end - rec
    mov dx, rec
    int 0x21                         ; record zero: written from the install (foreground) context
.cre_failed:
    call mkC                       ; reached by fall-through as well: mkC only marks that install got here
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

int21_hook:
    inc word [cs:i21]
    db 0xEA
old21_off dw 0
old21_seg dw 0

int10_hook:
    inc word [cs:i10]
    cmp ax, 0x4F02                   ; VESA set mode
    jne .not_vesa
    and bx, 0x01FF
    mov [cs:mode], bx
    jmp .chain
.not_vesa:
    cmp ah, 0x00
    jne .chain
    cmp al, 0x0F
    jae .chain
    movzx ax, al
    mov [cs:mode], ax
.chain:
    db 0xEA
old10_off dw 0
old10_seg dw 0

int1c_hook:
    inc word [cs:t1c]
    ; DOS reentrancy guard: doing DOS work while another DOS call is in flight is what silently costs the
    ; write its bytes (INDOS at 0050:0080 is non-zero exactly then). Check it, plus our own busy flag.
    cmp byte [cs:busy], 0
    jne .chain
    push ds
    mov ax, 0x0050
    mov ds, ax
    cmp byte [ds:0x0080], 0
    pop ds
    jne .chain
    mov ax, [cs:t1c]
    sub ax, [cs:lastrec]
    cmp ax, RECORD_EVERY
    jb .chain
    mov [cs:lastrec], ax
    mov byte [cs:busy], 1

    mov ax, [cs:ticks]
    mov [cs:rec + 0], ax
    mov ax, [cs:t1c]
    mov [cs:rec + 2], ax
    mov ax, [cs:i21]
    mov [cs:rec + 4], ax
    mov ax, [cs:i10]
    mov [cs:rec + 6], ax
    mov ax, [cs:mode]
    mov [cs:rec + 8], ax
    mov word [cs:rec + 10], 0
    mov word [cs:rec + 12], 0

    pusha                            ; the game's registers must survive us
    push ds                          ; DS belongs to the interrupted (protected-mode guest) context,
    mov ax, cs                       ; so the transfer buffer must be addressed through our own segment
    mov ds, ax
    ; open / append / write / close for every record. Keeping the handle open across the whole run does
    ; not work for this measurement: DOSBox buffers the writes and a killed session loses the lot -- which
    ; is very likely what made the earlier snapper revisions look like "the write took 0 bytes".
    mov ah, 0x3D
    mov al, 2                        ; read/write
    mov dx, logname
    int 0x21
    jc .wro_close
    mov [cs:handle], ax
    mov ax, 0x4202                   ; seek to end
    mov bx, ax
    xor cx, cx
    xor dx, dx
    int 0x21
    mov ah, 0x40
    mov bx, [cs:handle]
    mov cx, rec_end - rec
    mov dx, rec
    int 0x21
    mov [cs:rec + 14], ax            ; bytes DOS reports as taken
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21
.wro_close:
    mov byte [cs:busy], 0
    pop ds
    popa
.chain:
    db 0xEA
old1c_off dw 0
old1c_seg dw 0

mkA:
    mov ah, 0x3C
    xor cx, cx
    mov dx, mkAp
    int 0x21
    ret
mkB:
    mov ah, 0x3C
    xor cx, cx
    mov dx, mkBp
    int 0x21
    ret
mkC:
    mov ah, 0x3C
    xor cx, cx
    mov dx, mkCp
    int 0x21
    ret

msg db 'SNAPPROBE resident', 13, 10, '$'
mkAp db 'C:\SPA.TXT', 0
mkBp db 'C:\SPB.TXT', 0
mkCp db 'C:\SPC.TXT', 0

ticks   dw 0
t1c     dw 0
i21     dw 0
i10     dw 0
lastrec dw 0
busy    db 0
handle  dw 0
mode    dd 0x13

rec:
    times 16 db 0
rec_end:

logname db 'C:\SNAPPRB.BIN', 0

hook_end:
