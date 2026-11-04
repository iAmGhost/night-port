; SNAP.COM -- real-mode INT 10h snapshot TSR for capturing the ORIGINAL game's frames.
;
; Why: dosbox-staging 0.83 has no image capture command (docs/46 measured five host-side routes dead), and the
; game takes the timer IRQ into its protected-mode IDT, so a tick-driven TSR never runs (also docs/46). What the
; game cannot avoid is the real-mode video interrupt: its protected-mode code reaches VESA through DPMI's
; "call real-mode interrupt" service, which dispatches the REAL-MODE INT 10h vector. Hooking that vector gives a
; trigger at every mode set, and a hook of the window-position call gives one at every framebuffer-window slide.
;
; Record layout appended to SNAP.BIN (little endian):
;   0  'N','G','H','1'
;   4  u8  AH of the intercepted call
;   5  u8  AL of the intercepted call
;   6  u32 ECX at entry (VESA 4F05: window position)
;   10 u32 EDX at entry (VESA 4F05: window type in DL)
;   14 u32 sequence number
;   18 u32 VGA window bytes written (65536)
;   22 u8[65536] A000:0 .. A000:FFFF
;
; Build: nasm -f bin -o SNAP.COM snapr5.asm

BITS 16
ORG 0x100

start:
    ; ---- create/truncate SNAP.BIN --------------------------------------------
    mov  ah, 0x3C
    xor  cx, cx
    mov  dx, fname - start + 0x100
    int  0x21
    jc   short quit
    mov  [handle], ax

    ; ---- chain vector for INT 10h -------------------------------------------
    cli
    xor  ax, ax
    mov  es, ax
    mov  bx, [es:0x40]
    mov  [old_off], bx
    mov  bx, [es:0x42]
    mov  [old_seg], bx
    mov  word [es:0x40], my10 - start + 0x100
    mov  word [es:0x42], cs
    sti

    ; ---- stay resident -------------------------------------------------------
    mov  dx, (end - start + 0xF + 0x100) >> 4
    int  0x27

quit:
    mov  ah, 0x4C
    int  0x21
    ret

; ---- the hook ----------------------------------------------------------------
my10:
    pusha
    push es
    push ds
    mov  ax, [cs:save_ax]
    mov  [cs:save_cx], ecx
    mov  [cs:save_dx], edx
    mov  ax, 0x4F02
    cmp  ah, 0x4F
    jne  short .d_plain
    cmp  al, 0x02
    je   short .d_grab
    cmp  al, 0x05
    je   short .d_grab
    jmp  short .d_done
.d_plain:
    cmp  ah, 0x00
    jne  short .d_done
.d_grab:
    call dump_window
.d_done:
    pop  ds
    pop  es
    popa
    jmp  far word [cs:old_off]

; ---- append the 64 KiB window plus its header --------------------------------
dump_window:
    pusha
    push ds
    push es
    mov  ax, cs
    mov  ds, ax
    mov  ax, [seq]
    inc  ax
    mov  [seq], ax

    mov  si, hdr - start + 0x100
    mov  byte [si+0], 'N'
    mov  byte [si+1], 'G'
    mov  byte [si+2], 'H'
    mov  byte [si+3], '1'
    mov  ax, [save_ax]
    mov  byte [si+4], ah
    mov  byte [si+5], al
    mov  eax, [save_cx]
    mov  [si+6], eax
    mov  eax, [save_dx]
    mov  [si+10], eax
    mov  eax, [seq]
    mov  [si+14], eax
    mov  dword [si+18], 65536

    mov  ah, 0x40
    mov  bx, [handle]                ; INT 21h/40h: BX = handle, DS:DX = buffer, CX = count
    mov  dx, hdr - start + 0x100
    mov  cx, 22
    int  0x21                        ; header

    ; the payload comes straight out of the VGA window: DS:DX must point at A000:0
    mov  ah, 0x40
    mov  bx, [handle]
    push ds
    mov  ax, 0xA000
    mov  ds, ax
    xor  dx, dx
    mov  cx, 65536
    int  0x21                        ; 64 KiB of the framebuffer window
    pop  ds                          ; inner push (DS = 0xA000 -> CS)
    pop  es                          ; outer pushes were: pusha, ds, es, then the inner ds
    pop  ds
    popa
    ret

; ---- data -------------------------------------------------------------------
handle  dw 0
seq     dd 0
old_off dw 0
old_seg dw 0
save_ax dw 0
save_cx dd 0
save_dx dd 0
hdr     times 22 db 0
fname   db 'SNAP.BIN', 0
end:
