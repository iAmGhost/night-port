; SNAP.COM v3 -- guest-side VGA + DAC snapper TSR for the original NIGHT (docs/46).
;
; Why: the playtest harness captures the port's frames headlessly, but the original cannot be
; screenshotted from this host. Measured on this machine:
;   - System Events reports the dosbox-staging process but no usable window (frontmost=false);
;   - `screencapture -x` returns a 6016x3384 image whose every pixel is black (mean 0.0, std 0.0),
;     i.e. screen-recording yields nothing for this session;
;   - synthetic key events cannot be delivered to a background SDL window at all.
; So the capture has to happen inside the guest.
;
; v3 is the fix for what v2 could not do. v2 dumped from the INT 1Ch (end-of-tick) handler, and there
; DOS refuses to write: INT 21h AH=3Ch succeeds but AH=40h takes 0 bytes, because DOS is already in a
; hardware-interrupt context it treats as non-reentrant (the file was left at 796 bytes of header+DAC
; and an empty framebuffer record). v3 therefore does NO DOS work in the tick hook at all: INT 1Ch only
; raises a "frame wanted" flag, and the dump runs from the **INT 21h hook**, at the instant the game
; itself is entering DOS -- the classic TSR-safe point (DOS is not busy yet, INDOS is clear). That also
; means frames are captured at the same rate the game calls DOS, which for NIGHT is often (it loads
; .pcx/.fli/.voc throughout the attract mode).
;
; Mode: NIGHT sets exactly one VESA mode (the single INT 10h AX=4F02h site in the EXE, at 0x3f36e, with
; BX=0x101 = 640x480x8), so a mode-0x13-only snapper bailed on every tick. 640x480x8 is 307200 bytes but
; the real-mode A0000 window is 64 KB, so the dump walks it with INT 10h AX=4F05h, one window per 64 KB
; chunk -- the same trick the game's own still-image loader uses (fn_03628e + fn_03f3be). The window the
; game had selected is saved (AX=4F05 BX=1) and restored afterwards, so the snapper cannot corrupt the
; game's own window-relative reads.
;
; Rules: never do DOS work from an IRQ handler; never re-enter ourselves (in_dump); chain the original
; INT handlers with a far jump; the INT 10 hook only inspects registers; the INT 21h hook saves every
; register and flag of the game's call and restores them before chaining.

BITS 16
ORG 100h

SHOT_EVERY equ 9                     ; 9 ticks / 18.2 Hz ~= 2 shots per second
CHUNK      equ 0x10000               ; real-mode window size at A0000

section .text

start:
    call mkA
    cli
    xor ax, ax
    mov es, ax

    mov bx, 8 * 4                    ; INT 8 -- timer tick, counts
    mov ax, [es:bx]
    mov [cs:old8_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old8_seg], ax
    mov word [es:bx], int8_hook
    mov word [es:bx + 2], cs

    mov bx, 0x1C * 4                 ; INT 1C -- end of tick, only raises the flag
    mov ax, [es:bx]
    mov [cs:old1c_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old1c_seg], ax
    mov word [es:bx], int1c_hook
    mov word [es:bx + 2], cs

    mov bx, 0x10 * 4                 ; INT 10h -- mode tracker
    mov ax, [es:bx]
    mov [cs:old10_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old10_seg], ax
    mov word [es:bx], int10_hook
    mov word [es:bx + 2], cs

    mov bx, 0x21 * 4                 ; INT 21h -- the dump runs here (DOS-reentrant-safe)
    mov ax, [es:bx]
    mov [cs:old21_off], ax
    mov ax, [es:bx + 2]
    mov [cs:old21_seg], ax
    mov word [es:bx], int21_hook
    mov word [es:bx + 2], cs
    sti
    call mkB

    ; proof of life: wait ~1.1 s of BIOS ticks (the chained INT 8 advances the counter), then record how
    ; many of our own INT 8 hits were seen. If this file appears with a non-zero count, the hooks are in.
    mov ax, 0x0040
    mov es, ax
    mov ax, [es:0x006C]
    add ax, 20
    mov bx, ax
.tickwait:
    cmp word [es:0x006C], bx
    jb .tickwait
    mov ax, [cs:ticks]
    mov [cs:probeval + 2], ax
    call mkC            ; C:\SNAPTICK.TXT <- probeval
    call ensure_cap_dir

    ; 64 KB staging buffer: DOSBox's DOS refuses a transfer buffer inside the A0000 window itself, so
    ; each window chunk is copied here first. If the allocation fails we say so and dump nothing -- the
    ; earlier revision copied with ES=0 and scribbled over low memory.
    mov ah, 0x48
    mov bx, CHUNK / 16               ; 4096 paragraphs = 64 KB
    int 0x21
    jc .nofree
    mov [cs:bufseg], ax
.nofree:
    mov dx, msg_install
    mov ah, 0x09
    int 0x21

    mov dx, hook_end
    add dx, 15
    shr dx, 4
    int 0x27

; ------------------------------------------------------------------ INT 8 (count)

int8_hook:
    inc word [cs:ticks]
    db 0xEA
old8_off dw 0
old8_seg dw 0

; ------------------------------------------------------------------ INT 1C (want a frame)

int1c_hook:
    mov ax, [cs:ticks]
    sub ax, [cs:last_shot]
    cmp ax, SHOT_EVERY
    jb .chain
    cmp byte [cs:bufseg + 0], 0      ; no staging buffer -> nothing to capture into
    je .chain
    cmp byte [cs:pending], 0
    jne .chain
    mov byte [cs:pending], 1
.chain:
    db 0xEA
old1c_off dw 0
old1c_seg dw 0

; ------------------------------------------------------------------ INT 10 (mode tracker)

int10_hook:
    cmp ax, 0x4F02                   ; VESA set mode: mode number in BX
    jne .not_vesa
    and bx, 0x01FF                   ; bit 15 = preserve VGA, not part of the mode
    mov [cs:cur_mode], bx
    jmp .chain
.not_vesa:
    cmp ah, 0x00
    jne .chain
    cmp al, 0x0F                     ; modes 0x00..0x0E set with AH=0
    jae .chain
    mov [cs:cur_mode], ax
.chain:
    db 0xEA
old10_off dw 0
old10_seg dw 0

; ------------------------------------------------------------------ INT 21h (the dump)

int21_hook:
    cmp byte [cs:in_dump], 0
    jne .chain                       ; our own DOS calls inside the dump
    cmp byte [cs:pending], 0
    je .chain
    pusha
    pushf
    push ds
    push es
    mov byte [cs:pending], 0
    mov byte [cs:in_dump], 1
    call do_dump
    mov byte [cs:in_dump], 0
    pop es
    pop ds
    popf
    popa
.chain:
    db 0xEA
old21_off dw 0
old21_seg dw 0

; ------------------------------------------------------------------ the dump

do_dump:
    mov ax, cs
    mov ds, ax

    movzx eax, word [cs:cur_mode]
    cmp eax, 0x13
    je .geo_small
    cmp eax, 0x101
    je .geo_big
    jmp .done                        ; unknown mode: skip rather than emit junk

.geo_small:
    mov word [cs:hdr_w], 320
    mov word [cs:hdr_h], 200
    jmp .have_geo
.geo_big:
    mov word [cs:hdr_w], 640
    mov word [cs:hdr_h], 480

.have_geo:
    mov ax, [cs:ticks]
    mov [cs:hdr_tick], eax
    mov ax, [cs:cur_mode]
    mov [cs:hdr_mode], eax
    mov word [cs:done_err], 0
    mov word [cs:done_written], 0
    mov word [cs:done_chunk], 0
    mov word [cs:done_total], 0

    mov ah, 0x3C                     ; C:\CAP\SNAPnnnn.DMP (delete first: AH=3Ch refuses an existing name)
    xor cx, cx
    mov dx, fname
    int 0x21
    jc .done
    mov [cs:handle], ax

    mov ah, 0x40                     ; header
    mov bx, [cs:handle]
    mov cx, hdr_end - hdr
    mov dx, hdr
    int 0x21
    jc .close

    mov ax, cs                       ; DAC: 256 x R,G,B via INT 10h AX=1511h
    mov es, ax
    mov dx, dac
    xor bx, bx
    mov cx, 256
    mov ax, 0x1511
    int 0x10

    mov ah, 0x40
    mov bx, [cs:handle]
    mov cx, 768
    mov dx, dac
    int 0x21
    jc .close

    ; save the window the game had selected, so the dump cannot corrupt its reads
    mov ax, 0x4F05
    mov bx, 1                        ; get read window
    mov dx, 0
    int 0x10
    mov [cs:save_win], dx

    mov ax, 0xA000
    mov ds, ax
    mov eax, [cs:hdr_w]
    mov ecx, [cs:hdr_h]
    imul eax, ecx
    mov [cs:total], eax              ; bytes to write in total
    mov [cs:done_total], eax
    mov word [cs:chunkno], 0

.chunkloop:
    movzx eax, word [cs:chunkno]
    mov ecx, eax
    shl ecx, 16                      ; chunk -> byte offset
    mov eax, [cs:total]
    sub eax, ecx
    jle .chunkdone
    mov ecx, CHUNK
    cmp eax, ecx
    jbe .clen
    mov ecx, eax
.clen:
    mov [cs:curlen], ecx

    mov ax, [cs:cur_mode]            ; window switch only in the VESA mode
    cmp ax, 0x101
    jne .copied
    movzx eax, word [cs:chunkno]
    mov dx, ax
    mov ax, 0x4F05
    xor bx, bx                       ; set read window
    int 0x10
.copied:
    mov ax, 0xA000                   ; copy the window into the staging buffer
    mov ds, ax
    xor si, si
    mov ax, [cs:bufseg]
    mov es, ax
    xor di, di
    mov ecx, [cs:curlen]
    rep movsb

    mov ax, [cs:bufseg]              ; INT 21h writes from DS:DX
    mov ds, ax
    xor dx, dx
    mov ecx, [cs:curlen]
    mov ah, 0x40
    mov bx, [cs:handle]
    int 0x21
    jc .writefail
    add word [cs:done_written], ax   ; bytes DOS reports as taken
    inc word [cs:chunkno]
    jmp .chunkloop

.writefail:
    mov [cs:done_err], ax
    jmp .close

.chunkdone:
    mov ax, 0x4F05                   ; restore the game's window
    mov bx, 0
    mov dx, [cs:save_win]
    int 0x10

.close:
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21

    mov ax, cs                       ; diagnostic record (DS must be back on our segment: the path
    mov ds, ax                       ; lives there, and DS is A000 or the buffer after the copy)
    call diag_write
    call bump_name
.done:
    mov ax, [cs:ticks]
    mov [cs:last_shot], ax
    ret

; ------------------------------------------------------------------ helpers

; write the 8-byte dump diagnostic to C:\SNPDONE.TXT
diag_write:
    mov ah, 0x41
    mov dx, probe_done
    int 0x21
    mov ah, 0x3C
    xor cx, cx
    mov dx, probe_done
    int 0x21
    jc .nocreate
    mov [cs:handle], ax
    mov ah, 0x40
    mov bx, [cs:handle]
    mov cx, diag_end - diagbuf
    mov dx, diagbuf
    int 0x21
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21
    ret

.nocreate:
    ret

; write probeval (6 bytes) to C:\SNAPTICK.TXT
create_and_write:
    mov ax, cs
    mov ds, ax
    mov ah, 0x41
    mov dx, probe_tick
    int 0x21
    mov ah, 0x3C
    xor cx, cx
    mov dx, probe_tick
    int 0x21
    jc .nocreate2
    mov [cs:handle], ax
    mov ah, 0x40
    mov bx, [cs:handle]
    mov cx, 6
    mov dx, probeval
    int 0x21
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21
.nocreate2:
    ret

ensure_cap_dir:
    mov ah, 0x39
    mov dx, capdir
    int 0x21
    ret

bump_name:
    mov di, fname + 12               ; last of the four digits in "SNAP0000"
    mov cx, 5
.digit:
    mov al, [di]
    inc al
    mov [di], al
    cmp al, '9'
    jbe .done
    mov byte [di], '0'
    dec di
    loop .digit
.done:
    ret

; ---- install-stage markers (debug build only)
mkA:
    mov ah, 0x3C
    xor cx, cx
    mov dx, mA
    int 0x21
    ret
mkB:
    mov ah, 0x3C
    xor cx, cx
    mov dx, mB
    int 0x21
    ret
mkC:
    mov ah, 0x3C
    xor cx, cx
    mov dx, mC
    int 0x21
    mov [cs:handle], ax
    mov ah, 0x40
    mov bx, ax
    mov cx, 6
    mov dx, probeval
    int 0x21
    mov ah, 0x3E
    mov bx, [cs:handle]
    int 0x21
    ret

; ------------------------------------------------------------------ resident data

mA db 'C:\SMKA.TXT', 0
mB db 'C:\SMKB.TXT', 0
mC db 'C:\SMKC.TXT', 0
msg_install db 'SNAP v3: dump from the INT 21h hook (2 shots/s -> C:\CAP)', 13, 10, '$'

ticks     dw 0
last_shot dw 0
pending   db 0
in_dump   db 0
handle    dw 0
total     dd 0
bufseg    dw 0
chunkno   dw 0
curlen    dd 0
save_win  dw 0
cur_mode  dw 0x13
probe_tick dw 0
probeval  db 'TK=', 0, 0, 0

diagbuf:                              ; written by diag_write (8 bytes)
done_err  dw 0
done_mode dw 0                        ; cur_mode, so a bail reason is visible
done_written dw 0
done_total dw 0
done_chunk dw 0
diag_end:

hdr:
    db 'NGSH'
    dd 2                             ; version 2 header: mode, w, h explicit
hdr_mode  dd 0
hdr_w     dd 320
hdr_h     dd 200
hdr_tick  dd 0
hdr_rsv   dd 0
hdr_end:

capdir     db 'C:\CAP', 0
probe_done db 'C:\SNPDONE.TXT', 0
fname      db 'C:\CAP\SNAP0000.DMP', 0
dac times 768 db 0

hook_end:
