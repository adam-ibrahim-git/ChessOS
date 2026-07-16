;kernel.asm
;Michael Black, 2007

;kernel.asm contains assembly functions that you can use in your kernel

	.global _putInMemory
	.global _interrupt
	.global _makeInterrupt21
	.global _makeInterrupt22
	.global _makeInterrupt1C
	.global _sleepTicks
	.global _launchProgram
	.extern _handleInterrupt21
	.extern _handleInterrupt22
	.extern _handleInterrupt1C
	.extern _i_ticks
	.global _printhex

;void putInMemory (int segment, int address, char character)
_putInMemory:
	push bp
	mov bp,sp
	push ds
	mov ax,[bp+4]
	mov si,[bp+6]
	mov cl,[bp+8]
	mov ds,ax
	mov [si],cl
	pop ds
	pop bp
	ret

;int interrupt (int number, int AX, int BX, int CX, int DX)
_interrupt:
	push bp
	mov bp,sp
	mov ax,[bp+4]	;get the interrupt number in AL
	push ds		;use self-modifying code to call the right interrupt
	mov bx,cs
	mov ds,bx
	mov si,#intr
	mov [si+1],al	;change the 00 below to the contents of AL
	pop ds
	mov ax,[bp+6]	;get the other parameters AX, BX, CX, and DX
	mov bx,[bp+8]
	mov cx,[bp+10]
	mov dx,[bp+12]

intr:	int #0x00	;call the interrupt (00 will be changed above)

	mov ah,#0	;we only want AL returned
	pop bp
	ret

;void makeInterrupt21()
;this sets up the interrupt 0x21 vector
;when an interrupt 0x21 is called in the future, 
;_interrupt21ServiceRoutine will run

_makeInterrupt21:
	;get the address of the service routine
	mov dx,#_interrupt21ServiceRoutine
	push ds
	mov ax, #0	;interrupts are in lowest memory
	mov ds,ax
	mov si,#0x84	;interrupt 0x21 vector (21 * 4 = 84)
	mov ax,cs	;have interrupt go to the current segment
	mov [si+2],ax
	mov [si],dx	;set up our vector
	pop ds
	ret

;this is called when interrupt 21 happens
;it will call your function:
;void handleInterrupt21 (int AX, int BX, int CX, int DX)
_interrupt21ServiceRoutine:
	push dx
	push cx
	push bx
	push ax
	call _handleInterrupt21
	pop ax
	pop bx
	pop cx
	pop dx

	iret

;void makeInterrupt22()
;sets up INT 0x22, a second syscall vector whose handler runs with
;DS = ES = CS so it can read kernel globals and string literals
_makeInterrupt22:
	mov dx,#_interrupt22ServiceRoutine
	push ds
	mov ax,#0
	mov ds,ax
	mov si,#0x88	;0x22 * 4
	mov ax,cs
	mov [si+2],ax
	mov [si],dx
	pop ds
	ret

;void handleInterrupt22 (int AX, int BX, int CX, int DX)
;the C handler reads its parameters from [bp+4] onward, so the AX/BX/CX/DX
;pushes have to be the LAST ones before the call.  The segment registers
;and SI/DI go down lower on the stack so they don't shift the params.
_interrupt22ServiceRoutine:
	push es
	push ds
	push di
	push si
	push dx
	push cx
	push bx
	push ax
	mov bx,cs	;use bx as scratch, caller's bx is on the stack now
	mov ds,bx	;point DS at the kernel so globals + literals work
	mov es,bx
	call _handleInterrupt22
	pop ax
	pop bx
	pop cx
	pop dx
	pop si
	pop di
	pop ds
	pop es
	iret

;void makeInterrupt1C()
;hooks the BIOS user-timer-tick (called ~18.2 times/sec from INT 0x08)
_makeInterrupt1C:
	mov dx,#_interrupt1CServiceRoutine
	push ds
	mov ax,#0
	mov ds,ax
	mov si,#0x70	;0x1C * 4
	mov ax,cs
	mov [si+2],ax
	mov [si],dx
	pop ds
	ret

;_interrupt1CServiceRoutine:
;the BIOS user-timer hook.  pushes everything, sets DS=ES=CS so the C
;handler can touch our globals, calls _handleInterrupt1C, restores, irets
_interrupt1CServiceRoutine:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push ds
	push es
	mov ax,cs
	mov ds,ax
	mov es,ax
	call _handleInterrupt1C
	pop es
	pop ds
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	iret

;void sleepTicks(int n)
;busy-waits until i_ticks has advanced by n, with IF set so the timer
;keeps incrementing i_ticks while we wait
_sleepTicks:
	push bp
	mov bp,sp
	push ds
	mov ax,cs
	mov ds,ax
	mov bx,[bp+4]		;n
	add bx,[_i_ticks]	;target = current + n
	sti			;allow timer to fire while we spin
sleeploop:
	cmp [_i_ticks],bx
	jb sleeploop
	pop ds
	pop bp
	ret

;this is called to start a program that is loaded into memory
;void launchProgram(int segment)
_launchProgram:
	mov bp,sp
	mov bx,[bp+2]	;get the segment into bx

	mov ax,cs	;modify the jmp below to jump to our segment
	mov ds,ax	;this is self-modifying code
	mov si,#jump
	mov [si+3],bx	;change the first 0000 to the segment

	mov ds,bx	;set up the segment registers
	mov ss,bx
	mov es,bx

	mov sp,#0xfff0	;set up the stack pointer
	mov bp,#0xfff0

jump:	jmp #0x0000:0x0000	;and start running (the first 0000 is changed above)

;printhex is used for debugging only
;it prints out the contents of ax in hexadecimal
_printhex:
        push bx
        push ax
        push ax
        push ax
        push ax
        mov al,ah
        mov ah,#0xe
        mov bx,#7
        shr al,#4
        and al,#0xf
        cmp al,#0xa
        jb ph1
        add al,#0x7
ph1:    add al,#0x30
        int 0x10

        pop ax
        mov al,ah
        mov ah,#0xe
        and al,#0xf
        cmp al,#0xa
        jb ph2
        add al,#0x7
ph2:    add al,#0x30
        int 0x10

        pop ax
        mov ah,#0xe
        shr al,#4
        and al,#0xf
        cmp al,#0xa
        jb ph3
        add al,#0x7
ph3:    add al,#0x30
        int 0x10

        pop ax
        mov ah,#0xe
        and al,#0xf
        cmp al,#0xa
        jb ph4
        add al,#0x7
ph4:    add al,#0x30
        int 0x10

        pop ax
        pop bx
        ret

