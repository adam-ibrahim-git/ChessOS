# ChessOS Technical Deep Dive

This is the bible.  Read it slowly.  Every section answers questions a
professor could realistically ask in defense of this project.  Memorize the
ones that scare you.

---

## 1. The boot story (in real detail)

When you "boot" a PC, the CPU starts executing instructions from a fixed
ROM address.  That ROM is the **BIOS** (Basic Input/Output System), maybe
~100 KB of firmware burned into a chip on the motherboard.  In QEMU,
SeaBIOS plays the same role.

The BIOS does three things in order:

1. **POST** (power-on self-test): checks RAM, keyboard controller, video
   adapter, etc.
2. **Init peripherals**: programs the timer chip (PIT), the interrupt
   controller (PIC), the disk controller.
3. **Find a bootable device**: tries each candidate (floppy, hard disk,
   CD, network) in order.  For each one, it reads sector 0 — the first
   512 bytes — into RAM at physical address 0x7C00.  It then checks the
   last two bytes of that sector: if they're 0x55 0xAA in that order,
   the BIOS treats this as a valid bootloader and far-jumps to 0:0x7C00.

For us, sector 0 of `floppya.img` is the compiled output of `bootload.asm`.
That file ends with:

```asm
times 510-($-$$) db 0   ; pad to byte 510 with zeros
dw 0xAA55               ; magic number (low byte first = 55, then AA)
```

Without that magic, the BIOS would skip our floppy.

When the BIOS jumps to `0:0x7C00`, our bootloader is now running.  CS=0,
IP=0x7C00, but the bootloader uses `org 0h` and works with relative
offsets, so this works out.

The bootloader does:

```asm
mov ax, KSEG       ; KSEG = 0x1000
mov ds, ax         ; set up DS, SS, ES at 0x1000
mov ss, ax
mov es, ax
mov sp, 0xfff0     ; stack at top of that segment

mov cl, KSTART+1   ; CL = 4 (CHS sectors are 1-indexed; sector 3 = relative 4)
mov dh, 0          ; head 0
mov ch, 0          ; cylinder/track 0
mov ah, 2          ; INT 0x13 AH=2 = read sectors
mov al, KSIZE      ; AL = 10 sectors
mov dl, 0          ; drive 0 (floppy A)
mov bx, 0          ; ES:BX = 0x1000:0x0000 = read into segment 0x1000
int 0x13           ; BIOS does the disk read

jmp KSEG:0         ; far jump to 0x1000:0x0000 = our kernel
```

That `int 0x13` reads 10 sectors (sectors 3 through 12) into memory at
0x1000:0x0000.  Then the far jump transfers control to my kernel's first
instruction.

**Why sectors 3 through 12, not 1 through 10?**  Because sectors 1 and 2
are reserved for the file system metadata (sector 1 = disk map, sector 2
= directory).  My kernel lives at sectors 3-12, my files at sectors 13+.

**Why 10 sectors?**  Because `KSIZE = 10`.  That's a hardcoded constant.
If my kernel grew bigger than 5120 bytes I'd have to bump it (and update
the disk map to mark more sectors as used by the kernel).

**Why segment 0x1000?**  Two reasons.  Segment 0 is reserved for the
interrupt vector table (the bottom 1 KB) and the BIOS data area.  Segment
0x0700 is where the BIOS originally loaded the bootloader (via 0x7C00 =
0x0000:0x7C00 = 0x07C0:0x0000 viewed differently).  Segment 0x1000
(physical 0x10000) is far enough up to be safe and aligned to 64 KB.

---

## 2. CPU state in 16-bit real mode

This is what your professor will hammer.  Memorize it.

The 8086 was a 16-bit CPU released in 1978.  Modern x86 CPUs retain
backwards-compatibility with that mode and call it **real mode**.  When a
PC powers on, it starts in real mode.  Modern OSes (Linux, Windows)
switch to 32-bit or 64-bit protected mode within milliseconds, but my
ChessOS stays in real mode forever.

### Registers

- General-purpose 16-bit: **AX, BX, CX, DX**
  - Each splits into two 8-bit halves: AH/AL, BH/BL, CH/CL, DH/DL
  - AH = "high byte" of AX, AL = "low byte" of AX
  - `AX = AH * 256 + AL`
- Pointer/index 16-bit: **SI, DI, BP, SP**
  - SI = source index (string operations)
  - DI = destination index
  - BP = base pointer (stack frame)
  - SP = stack pointer (top of stack)
- Segment 16-bit: **CS, DS, SS, ES**
  - CS = code segment
  - DS = data segment
  - SS = stack segment
  - ES = extra segment (used by string operations and by us for disk reads)
- Flags register: **FLAGS** (zero flag, carry flag, sign flag, **interrupt flag (IF)**, etc.)
- Instruction pointer: **IP** (offset of next instruction within CS)

### Memory addressing: segment:offset

A 16-bit register can hold 0..65535, so a single register can address 64 KB.
The IBM PC needed access to 1 MB (which is 20 bits = 0..1048575).

The 8086 solution: **two 16-bit numbers**, smushed together:

```
physical_address = (segment << 4) + offset
                 = segment * 16 + offset
```

So `segment 0xB000, offset 0x8000`:
```
physical = 0xB000 * 16 + 0x8000
         = 0xB0000 + 0x8000
         = 0xB8000
```

That's where text-mode video memory lives.

You can reach the same physical address with multiple segment:offset
pairs.  `0xB7F0:0x0100` also gives you `0xB7F00 + 0x0100 = 0xB8000`.

**Maximum addressable**: with both segment and offset being 16-bit, the
biggest physical address is `0xFFFF * 16 + 0xFFFF = 0xFFFEF`, just under
1 MB.  (The 8086 had only 20 address pins, so it really did wrap at 1 MB.
Later CPUs added the A20 line and could go beyond, but we don't.)

### Memory map of our system

```
0x00000 - 0x003FF   Interrupt Vector Table (256 × 4 bytes)
0x00400 - 0x004FF   BIOS Data Area
0x00500 - 0x07BFF   free
0x07C00 - 0x07DFF   bootloader (briefly, before it loads the kernel)
0x07E00 - 0x0FFFF   free
0x10000 - 0x1FFFF   our kernel (segment 0x1000)
0x20000 - 0x2FFFF   user program: shell or chess (segment 0x2000)
0x30000 - 0x9FFFF   free
0xA0000 - 0xAFFFF   VGA graphics buffer (we don't use this)
0xB0000 - 0xB7FFF   monochrome text buffer (we don't use this)
0xB8000 - 0xBFFFF   color text buffer (THIS is where we draw)
0xC0000 - 0xFFFFF   ROM extensions, BIOS itself
```

Each segment is up to 64 KB.  Within a segment, offsets go 0x0000 to
0xFFFF.

### Stack

The stack lives in segment SS, growing downward from SP.  PUSH decrements
SP by 2, then writes to SS:SP.  POP reads from SS:SP, then increments SP
by 2.  CALL pushes the return address (IP).  RET pops it.  INT pushes
FLAGS, CS, IP.  IRET pops IP, CS, FLAGS.

For our kernel, SS=0x1000, SP starts at 0xFFF0.  That gives us up to ~64
KB of stack space (growing down from 0xFFF0 to 0).  Plenty.

For the shell or chess, SS=0x2000, SP=0xFFF0.  Same deal.

---

## 3. Interrupts: how INT and IRET actually work

The interrupt vector table (IVT) lives at physical 0 to 0x3FF.  It's 256
entries of 4 bytes each.  Each entry is a `segment:offset` pair (offset
first, then segment, in little-endian) pointing to that interrupt's
handler function.

| IVT slot | Address  | Handles                                  |
|----------|----------|------------------------------------------|
| 0x00     | 0x0000   | divide by zero                           |
| 0x08     | 0x0020   | hardware timer interrupt (PIT)           |
| 0x10     | 0x0040   | BIOS video services                      |
| 0x13     | 0x004C   | BIOS disk services                       |
| 0x16     | 0x0058   | BIOS keyboard services                   |
| 0x1A     | 0x0068   | BIOS time-of-day services                |
| 0x1C     | 0x0070   | BIOS user timer hook (we install here)   |
| 0x21     | 0x0084   | (no default; we install our syscalls here)  |
| 0x22     | 0x0088   | (no default; we install our second vector) |

When the CPU encounters `INT N`:

1. Push FLAGS onto the stack (SS:SP)
2. Push CS onto the stack
3. Push IP (the address right after the INT instruction) onto the stack
4. Clear IF (interrupts now disabled)
5. Read IVT[N] which is at physical address `N * 4`:
   - Bytes 0-1 = handler offset
   - Bytes 2-3 = handler segment
6. Set CS = handler segment, IP = handler offset
7. Continue execution at new CS:IP

The handler runs.  When it's done, IRET:

1. Pop IP
2. Pop CS
3. Pop FLAGS (this restores IF)
4. Continue execution at restored CS:IP

That's the whole mechanism.  Hardware interrupts (timer, keyboard) and
software interrupts (`INT N` instruction) work identically.  Hardware
interrupts only fire when IF=1; that's why ISRs run with IF=0 (they
shouldn't be interrupted by other hardware).

### Installing a handler

To install a handler at vector N, you just write to IVT[N].  My
`_makeInterrupt21` does:

```asm
mov dx, #_interrupt21ServiceRoutine   ; offset within kernel CS
push ds
mov ax, #0
mov ds, ax                            ; DS = 0 so we can write to IVT
mov si, #0x84                         ; 0x21 * 4
mov ax, cs                            ; AX = kernel segment
mov [si+2], ax                        ; IVT[0x21].segment = kernel CS
mov [si], dx                          ; IVT[0x21].offset = handler addr
pop ds
ret
```

Now any `INT 0x21` jumps into my handler.

`_makeInterrupt22` does the same at IVT[0x22] (offset 0x88).
`_makeInterrupt1C` does the same at IVT[0x1C] (offset 0x70).

---

## 4. The BIOS interrupts we use

### INT 0x10 — Video

| AH | What |
|----|------|
| 0x00 | Set video mode (AL = mode; 0x03 = 80x25 color text) |
| 0x02 | Set cursor position (BH = page, DH = row, DL = col) |
| 0x09 | Write character with attribute at cursor (AL = char, BH = page, BL = attr, CX = count) |
| 0x0E | Teletype output (AL = char) — this prints AL and advances the cursor |

We mostly use 0x0E (teletype) for normal text output, and 0x09 (write
char with attribute) for chess piece colors.

### INT 0x13 — Disk

| AH | What |
|----|------|
| 0x02 | Read sectors |
| 0x03 | Write sectors |

For both: AL = sector count, ES:BX = buffer, CH = cylinder, CL = sector
(1-indexed!), DH = head, DL = drive (0 = floppy A).

### INT 0x16 — Keyboard

| AH | What |
|----|------|
| 0x00 | Wait for keypress; returns AL = ASCII, AH = scan code |
| 0x01 | Check if a keypress is available (non-blocking) |

We use AH=0 in `readString`.  The reason `readString` only sees ASCII and
not arrow keys is that arrow keys return AL=0 with the scan code in AH,
and our `interrupt()` helper zeros AH on return.

### INT 0x1A — System time

| AH | What |
|----|------|
| 0x00 | Get system time (returns CX:DX = ticks since midnight) |

We don't use this directly because BIOS-managed ticks live at 0x40:0x6C
and we maintain our own counter via INT 0x1C.

### INT 0x1C — User timer hook

This one is special.  The PC's PIT (Programmable Interval Timer chip)
fires hardware interrupt 0x08 about 18.2 times per second.  The BIOS's
built-in INT 0x08 handler updates the system time at 0x40:0x6C and then
calls **INT 0x1C** as a hook for user code.  By default INT 0x1C just
does `IRET` (no-op).  We install our own handler there to get a callback
every tick.

Why is the rate exactly 18.2 Hz?  The PIT runs at 1.193182 MHz (a
specific crystal frequency).  By default the BIOS programs it to count
down from 65536 (the maximum), so it fires every 65536 / 1193182 ≈ 0.0549
seconds, which is ≈ 18.2065 Hz.

That's why my code uses 18 ticks per second as the round number.  It
drifts about 1% from real time but that's fine for our purposes.

---

## 5. Our INT 0x21 service-call mechanism

We chose 0x21 to evoke the historical DOS syscall vector (DOS used INT
0x21 for everything from "print a character" to "open a file").  Our
service routine in `kernel.asm`:

```asm
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
```

Push order matters because the C function reads its parameters off the
stack.  After `push bp; mov bp, sp` inside the C function:

```
[bp+0]  = saved BP
[bp+2]  = return address (from CALL)
[bp+4]  = AX  ← first parameter
[bp+6]  = BX  ← second parameter
[bp+8]  = CX  ← third parameter
[bp+10] = DX  ← fourth parameter
```

C function signature is `void handleInterrupt21(int ax, int bx, int cx, int dx)`.

Inside the handler, a big if/else chain dispatches based on AX:

| AX | Function |
|----|----------|
| 0  | printString(BX) |
| 1  | readString(BX) |
| 2  | readSector(BX, CX) |
| 3  | readFile(BX, CX) |
| 4  | executeProgram(BX, CX) |
| 5  | terminate() |
| 6  | writeSector(BX, CX) |
| 7  | deleteFile(BX) |
| 8  | writeFile(BX, CX, DX) |
| 9  | clearScreen() |
| 10 | diskInfo() |

Critical detail: **the handler runs with DS = caller's segment**.  INT
doesn't change DS automatically.  So when the shell calls `INT 0x21`,
DS is still 0x2000 (shell's segment) inside the handler.  This is fine
for handlers that dereference user pointers — `printString(bx)` reads
`bx[i]` with DS = shell's segment, which is where the string lives.

But if the handler tried to read a kernel global or a kernel string
literal, it would fail because those are at offsets within the kernel's
segment (0x1000), and DS = 0x2000.  That's why I added a second vector.

---

## 6. Our INT 0x22 (the kernel-DS syscalls)

INT 0x22 has a wrapper that explicitly switches DS = ES = CS before
calling the C handler:

```asm
_interrupt22ServiceRoutine:
    push es
    push ds
    push di
    push si
    push dx
    push cx
    push bx
    push ax
    mov bx, cs        ; use bx as scratch (caller's bx is on the stack)
    mov ds, bx        ; DS = kernel
    mov es, bx        ; ES = kernel
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
```

Now the C handler can safely access:
- Kernel globals like `i_ticks`, `i_ss`, `i_mm`, `i_hh`
- Kernel string literals like `"Up "`, `"seconds"`, `"Took "`

Push order is critical: AX/BX/CX/DX go LAST so they end up nearest the
top of the stack, where the C calling convention expects them.  The
segment registers and SI/DI go LOWER on the stack so they don't shift the
parameters' offsets.  This is one of the bugs we hit during development.

| AX | Function |
|----|----------|
| 0  | printUptime() |
| 1  | sleepTicks(CX) |
| 2  | timeStart (i_savedTicks = i_ticks) |
| 3  | timeEnd (printElapsed) |

---

## 7. The timer interrupt handler

`_interrupt1CServiceRoutine` is similar to INT 0x22's wrapper.  It
preserves all registers, sets DS = ES = CS, calls
`handleInterrupt1C()`, and IRETs.

The C handler:

```c
void handleInterrupt1C()
{
    i_ticks        = i_ticks + 1;
    i_ticksThisSec = i_ticksThisSec + 1;
    if (i_ticksThisSec >= 18) {
        i_ticksThisSec = 0;
        i_ss = i_ss + 1;
        if (i_ss >= 60) {
            i_ss = 0;
            i_mm = i_mm + 1;
            if (i_mm >= 60) {
                i_mm = 0;
                i_hh = i_hh + 1;
            }
        }
        updateClockDisplay();
    }
}
```

Every tick, bump `i_ticks`.  Once 18 ticks accumulate (≈ 1 second), bump
`i_ss` and redraw the clock at the top right of the screen.

`updateClockDisplay` paints "HH:MM:SS" using `putInMemory` to write
directly to video memory at row 0, columns 70-77.  Color attribute 0x4F
= red background, white foreground.

### Why does sleep need STI?

When INT 0x21 (or any interrupt) entry happens, the CPU automatically
clears IF.  So while my INT 0x21 handler is running, the timer can't
fire.  If `sleepTicks` were just a busy-wait reading `i_ticks`, the
counter would never advance and the loop would run forever.

The fix in `_sleepTicks`:

```asm
_sleepTicks:
    push bp
    mov bp, sp
    push ds
    mov ax, cs
    mov ds, ax
    mov bx, [bp+4]      ; n
    add bx, [_i_ticks]  ; target = current + n
    sti                 ; ENABLE interrupts so timer keeps firing
sleeploop:
    cmp [_i_ticks], bx
    jb sleeploop
    pop ds
    pop bp
    ret
```

`sti` sets IF=1.  Now the timer fires during the wait, increments
`i_ticks`, and the loop eventually exits.  When IRET returns from the
INT 0x22 handler, FLAGS is restored from the stack (IF returns to its
pre-INT value), so the STI doesn't leak out.

---

## 8. Video memory and how the screen works

The text-mode framebuffer is at physical 0xB8000 (segment 0xB000, offset
0x8000).  The screen is 80 columns × 25 rows = 2000 cells.

Each cell is **2 bytes**:
- Even byte: the ASCII character code
- Odd byte: the color attribute

The attribute byte is `bbbbffff`:
- Bits 7-4: background color
- Bits 3-0: foreground color

Color values:

| Value | Color       | Value | Color (bright)   |
|-------|-------------|-------|------------------|
| 0     | black       | 8     | dark gray        |
| 1     | blue        | 9     | light blue       |
| 2     | green       | 10    | light green      |
| 3     | cyan        | 11    | light cyan       |
| 4     | red         | 12    | light red        |
| 5     | magenta     | 13    | light magenta    |
| 6     | brown       | 14    | yellow           |
| 7     | light gray  | 15    | white            |

So `0x70` = bg light gray, fg black (I use this for light chess squares).
`0x4F` = bg red, fg white (the title banner and the clock).
`0x1F` = bg blue, fg white (dark chess squares).

**Writing a cell**:

To put 'X' in red at row 5, column 10:
```c
int offset = (5 * 80 + 10) * 2;
putInMemory(0xB000, 0x8000 + offset,     'X');
putInMemory(0xB000, 0x8000 + offset + 1, 0x4F);
```

`putInMemory` is in `kernel.asm` and does the segment switch internally:

```asm
_putInMemory:
    push bp
    mov bp, sp
    push ds
    mov ax, [bp+4]    ; segment
    mov si, [bp+6]    ; offset
    mov cl, [bp+8]    ; byte to write
    mov ds, ax
    mov [si], cl      ; write [DS:SI] = CL
    pop ds
    pop bp
    ret
```

It saves DS, sets DS to the requested segment, writes one byte at the
requested offset, then restores DS.

### Why use BIOS teletype for the shell?

The shell's print path goes shell → INT 0x21 ax=0 → kernel's
`printString` → BIOS INT 0x10 ah=0x0E (teletype).  We use teletype
because the BIOS handles cursor advancement, scrolling, and tab/newline
expansion.  Doing all that ourselves would be more code.

For the chess splash and the live clock, I use `putInMemory` directly
because I need positional control with specific colors, which BIOS
teletype doesn't give me.

---

## 9. The file system

Layout on the floppy:

```
sector 0       bootloader (512 bytes, ends with 0x55 0xAA)
sector 1       disk map  (512 bytes; one byte per sector, 0xFF = used, 0x00 = free)
sector 2       directory (512 bytes; 16 entries × 32 bytes each)
sector 3+      file data
```

The disk has 2880 sectors total (1.44 MB) but the map only describes the
first 256 (because each sector number is one byte, 0-255).  So the
addressable file system is 256 × 512 = 128 KB.  Beyond that, sectors
exist on the floppy but our file system can't reach them.

### Directory entry (32 bytes)

```
bytes 0-5:    filename (padded with 0x00 if shorter than 6)
bytes 6-31:   list of up to 26 sector numbers; 0 = unused slot
```

If `dir[i] == 0x00`, that whole 32-byte entry is empty.

So a file is at most 26 sectors × 512 bytes = **13312 bytes** = 13 KB.
This is why my `executeProgram` has a `char buffer[13312]`: that's the
maximum possible file size.

### Reading a file

`readFile(name, buffer)`:

1. `readSector(dir, 2)` — load the directory into a 512-byte local array.
2. Walk dir 32 bytes at a time. For each entry:
   - If `dir[i] == 0` → skip (empty entry)
   - Else compare bytes 0-5 against `name` byte-by-byte (with 0x00
     padding logic for short names)
   - If they match: walk bytes 6-31, treating each non-zero byte as a
     sector number, calling `readSector(buffer, that_sector)` and then
     `buffer = buffer + 512` to advance to the next 512-byte window.
3. Stop at the first 0-sector slot.

### Writing a file

`writeFile(name, buffer, numSectors)`:

1. `readSector(map, 1)` and `readSector(dir, 2)` — load both metadata
   sectors.
2. Find a free directory entry (first byte == 0).
3. Zero the whole 32-byte entry first (because deleted entries only have
   their first byte cleared, leaving stale name suffix and sector
   pointers).  This is actually a real bug we fixed during development.
4. Copy the name into bytes 0-5.
5. For each of `numSectors` sectors:
   - Find a free sector by scanning the map for a 0x00.
   - Mark it 0xFF.
   - Write the sector number into the next slot of the directory entry.
   - Write 512 bytes from buffer to that sector.
   - Advance buffer by 512.
6. Write the modified map and directory back.

### Deleting a file

`deleteFile(name)`:

1. Read map and directory.
2. Find the file's entry by name.
3. For each sector in its sector list, write 0x00 to the corresponding
   map byte (free that sector).
4. Write 0x00 to byte 0 of the directory entry (mark entry empty).
5. Write map and dir back.

The file's data is **not** erased.  The sectors will simply be reused
the next time someone calls `writeFile`.  This is normal; real file
systems work the same way.

---

## 10. CHS conversion (the `mod 18`, `mod 2`, `div 36` math)

A 1.44 MB 3.5" floppy has:
- 80 cylinders (tracks)
- 2 heads (one per side of the disk)
- 18 sectors per track
- Total: 80 × 2 × 18 = 2880 sectors of 512 bytes = 1.44 MB

BIOS INT 0x13 expects **physical** addressing as Cylinder/Head/Sector,
not as a flat sector number.  We use flat sector numbers everywhere in
our code (sector 0, sector 1, ...) so we have to convert.

The disk is laid out so that consecutive flat sectors visit cylinders
in this order:
- Sectors 0-17 = cylinder 0, head 0, sectors 1-18
- Sectors 18-35 = cylinder 0, head 1, sectors 1-18
- Sectors 36-53 = cylinder 1, head 0, sectors 1-18
- Sectors 54-71 = cylinder 1, head 1, sectors 1-18
- ...

So:
- `cylinder = sector / 36` (each cylinder has 36 flat sectors)
- `head = (sector / 18) % 2` (which side of the cylinder)
- `relative_sector = (sector % 18) + 1` (BIOS sectors are 1-indexed!)

That `+1` is the easy thing to forget.  CHS sectors start at 1, not 0.
If you forget it, you'll consistently read the wrong sector and corrupt
the file system.

Our `readSector`:

```c
relSec = mod(sector, 18) + 1;
head   = mod(div(sector, 18), 2);
track  = div(sector, 36);
interrupt(0x13, 2*256 + 1, buffer, track*256 + relSec, head*256);
```

The `2*256 + 1` packs AH=2 (read), AL=1 (count).
The `track*256 + relSec` packs CH = track, CL = sector.
The `head*256` packs DH = head, DL = 0 (drive A).

`writeSector` is identical except `2*256` becomes `3*256` (AH=3 = write).

---

## 11. Why bcc, and what that costs us

`bcc` is Bruce's C Compiler — a 16-bit C compiler from the late 80s,
written by Bruce Evans, originally for the Minix-like Linux fork.  It's
the only practical free 16-bit C compiler that can produce code for
8086/8088 real mode.

Why not gcc?  gcc generates 32-bit or 64-bit machine code by default.
You can technically tell it to generate 16-bit code with `-m16` but the
output assumes a flat memory model and doesn't fit our segment-based
real-mode setup.

bcc is **K&R-flavored**, meaning it follows the original Kernighan & Ritchie
1978 C, not the later ANSI C89 standard:

| Modern C | bcc |
|----------|-----|
| `// comments` | NOT supported.  Must use `/* */` |
| `int main(int argc, char* argv[])` | Use `void main()` |
| Mixed declarations/code | Variables must be declared at top of function |
| `#include <stdio.h>` | NOT supported.  No standard library |
| `printf("%d", n)` | NOT available.  Write your own |
| `a / b`, `a % b` | NOT supported (this is bcc-specific).  Must write your own |
| `a += 1` | Sometimes works.  We use `a = a + 1` to be safe |

Why no native `/` or `%`?  bcc targets old 8086/8088 CPUs that didn't have
fast integer divide.  Some bcc configurations omit the divide entirely to
avoid the cost.  Either way, our `mod()` and `div()` are repeated
subtraction loops:

```c
int mod(int a, int b) { while (a >= b) { a = a - b; } return a; }
int div(int a, int b) { int q = 0; while (q * b <= a) { q = q + 1; } return q - 1; }
```

`mod(100, 7)` runs the subtraction loop 14 times.  It's O(a/b).  For our
small inputs (sector numbers, screen positions, counters) this is
fast enough; for a real OS doing arithmetic in tight loops it would be
unacceptable.

### `int` is 16 bits

bcc's `int` is 16 bits, range −32768 to +32767 (signed) or 0 to 65535
(unsigned).  This bit me with `i_earliest = 99999;` in my scheduler,
which silently wrapped to a negative number.  Now I use 9999.

### `char` is signed by default

In bcc, `char` is signed, range −128 to +127.  When we read a sector
number from disk that's > 127 (like 0xFF = 255), reading it into an int
gives -1, not 255.  My `readFile` and `deleteFile` have:
```c
sectorNum = dir[i + j];
if (sectorNum < 0) sectorNum = sectorNum + 256;
```
to recover the unsigned value.

---

## 12. The shell

The shell runs at segment 0x2000.  Its `main()`:

1. Initialize the simulated process table (`initProcs`)
2. Initialize the simulated memory layout (`initMem`)
3. Print "ChessOS at your service. Type 'help'."
4. Forever:
   - Print "ChessOS> "
   - `readString(line)` — reads keyboard input until Enter
   - If line is empty, skip
   - Otherwise, `parseTokens(line, a, b, c)` splits into three space-separated tokens
   - `dispatch(a, b, c)` routes the command

`dispatch` is the big if/else.  Pulled out of `main()` so that the
`time <cmd>` command can call it on an inner command.

### Command flow example: `type readme`

1. Shell's `readString` gets "type readme\n\0"
2. `parseTokens` produces a="type", b="readme", c=""
3. `dispatch` matches `"type\0"`, calls `doType(b)`
4. `doType` zeros a 13312-byte buffer, calls `INT 0x21 ax=3` with bx=arg, cx=buffer
5. CPU jumps to kernel's INT 0x21 handler.  ax=3 → `readFile(bx, cx)`
6. `readFile` does `readSector(localDir, 2)` to load directory
7. `readFile` walks dir, finds entry with name="readme"
8. For each sector pointer in that entry, `readSector(buffer, sectorN)`
9. Returns to shell's `doType`
10. `doType` calls `printBuf(buf)` which prints with LF→LF+CR expansion
11. Shell loops back to print prompt

### Why does the shell use INT 0x21 instead of calling kernel functions?

Modularity.  The shell could be replaced with a different program that
implements the same interface.  Or the kernel could be replaced (say,
with a different file system) without changing the shell.  All the
shell knows about is the system call ABI.

This is the same reason real operating systems have a syscall interface.

---

## 13. The chess game

Chess runs at segment 0x2000 (replacing the shell while it's loaded).

### Board representation

```c
char l_board[64];
```

64 bytes.  `l_board[row * 8 + col]` is the piece at (row, col).

Pieces:
- White (uppercase): P R N B Q K
- Black (lowercase): p r n b q k
- Empty: '.'

Row 0 = top of screen = black home rank (rank 8).
Row 7 = bottom = white home rank (rank 1).

### Drawing the board

Each square is 3 cells wide.  The board occupies columns 28-51, rows 4-11.
For each square, set the cursor with INT 0x10 ah=0x02, then write 3
characters (`" P "` for white pawn, etc.) with INT 0x10 ah=0x09 setting
the color attribute based on light/dark/cursor/selected.

### Move legality

`isLegalMove(board, fromR, fromC, toR, toC, player)`:

1. Bounds check: destination must be on the board, source ≠ destination.
2. Source must be the moving player's piece (uppercase if player=0, lowercase if player=1).
3. Destination must NOT be the moving player's own piece.
4. Dispatch by piece type to a per-piece checker:
   - Pawn: forward 1 (or 2 from starting rank) into empty, or diagonal 1 to capture
   - Rook: same row OR same column, with `pathClear`
   - Bishop: same row-distance as column-distance, with `pathClear`
   - Knight: |Δrow|=2,|Δcol|=1 or |Δrow|=1,|Δcol|=2 (no path check; knights jump)
   - Queen: rook or bishop logic
   - King: |Δrow|≤1 and |Δcol|≤1 (one step in any direction)

`pathClear(board, fromR, fromC, toR, toC)` walks one step at a time from
source toward destination (exclusive) and checks every intermediate
square is empty.

### Self-check prevention

`tryMove(board, fromR, fromC, toR, toC, player)`:

1. Check `isLegalMove(...)`.  If false, return 0.
2. Tentatively apply the move (save the destination's old contents first).
3. `isInCheck(board, player)` — find your own king, scan all opponent
   pieces, ask "can any of them legally move to my king's square?".  If
   yes, you're in check.
4. If we're now in check, roll back the move and return 0.
5. Otherwise return 1.

The recursion is the tricky part.  `isInCheck` calls `isLegalMove` for
each opponent piece against the king's square.  `isLegalMove` does NOT
itself call `isInCheck`, which would cause infinite recursion.  Only
`tryMove` calls `isInCheck`, and only after the legality check.

### The Q-key quit path

When you press Q:
```c
b_gameOver = 1;   // exit the loop
...
interrupt(0x21, 9, 0, 0, 0);              // clearScreen
interrupt(0x21, 4, "shell\0", 0x2000, 0); // executeProgram(shell)
```

The shell is loaded back over chess in segment 0x2000 and execution
jumps to it.  Your local state in chess is lost (it was on the stack in
segment 0x2000, which is being overwritten), but you're back at the
prompt with the kernel's tick counter intact.

---

## 14. Why two syscall vectors (the DS problem in depth)

This question is the most technical thing in the project.  Be ready for it.

### The setup

When the shell calls INT 0x21:
- CS goes from shell's segment (0x2000) to kernel's segment (0x1000) (because IVT[0x21].segment was set to kernel CS by `_makeInterrupt21`)
- DS does NOT change.  DS is still 0x2000 (shell's data segment)

This is fine for handlers that only dereference user-supplied pointers:
- `printString(bx)`: `bx` is a pointer into shell's segment.  Reading
  `bx[i]` uses DS-relative addressing.  DS = 0x2000.  Read happens at
  shell's segment, which is where the string lives.  Works.
- `readSector(bx, cx)`: `bx` is a pointer to shell's buffer.  BIOS uses
  ES:BX, and ES is also still shell's segment.  Read goes into shell's
  buffer.  Works.

### When it breaks

Imagine I want a syscall like `printUptime()` that prints "Up N seconds"
where "Up " and "seconds" are kernel-side string literals:

```c
void printUptime() {
    printString("Up ");        // "Up " is in kernel's data segment
    printNumber(secs);
    printString(" seconds");   // ditto
}
```

If this runs while DS = 0x2000 (shell's segment), the dereference of the
string literal pointer reads from shell's segment instead of kernel's.
You get garbage characters.

Same problem with kernel globals: `i_ticks`, `i_savedTicks`, etc.  Their
addresses are within kernel's segment, but DS = 0x2000.

### Possible solutions and why we chose this one

**Option A**: Modify `_interrupt21ServiceRoutine` to set DS = CS at entry
and restore at exit.  But this would break every existing handler that
relies on DS being the caller's segment for user-pointer access.

**Option B**: In each handler that needs kernel data, manually save and
switch DS.  Verbose, fiddly, easy to get wrong.

**Option C** (what we did): Add a second interrupt vector.  INT 0x22
sets DS = ES = CS at entry, perfect for kernel-data handlers.  INT 0x21
keeps the existing convention.  Two completely separate interrupt
service routines, two completely separate handler dispatchers.

It costs a few extra lines of asm and a duplicate dispatcher in C, but
it's the cleanest way.

### The push order bug we hit

When I first wrote `_interrupt22ServiceRoutine`, I had this push order:

```asm
push dx
push cx
push bx
push ax
push si           ; oops: pushes happen AFTER ax/bx/cx/dx
push di
push ds
push es
mov ax, cs
mov ds, ax
mov es, ax
call _handleInterrupt22
```

After all the pushes, the stack from top to bottom was:
`[es, ds, di, si, ax, bx, cx, dx]`

When the C function does `push bp; mov bp, sp`, [bp+4] = es, not ax.
The dispatch then read `ax = es value = some segment address ≈ 0x2000`,
which didn't match any of 0/1/2/3, so it always hit "Bad int 0x22 call".

Fix: push the segment registers FIRST, then the parameter registers
LAST, so AX/BX/CX/DX end up nearest the top of the stack:

```asm
push es
push ds
push di
push si
push dx
push cx
push bx
push ax
mov bx, cs        ; use bx as scratch (the saved bx is on the stack)
mov ds, bx
mov es, bx
call _handleInterrupt22
```

Now [bp+4] = ax, [bp+6] = bx, etc.  Correct.

---

## 15. Process scheduler simulator (the algorithms in detail)

Six processes are pre-loaded:

| pid | name   | state    | priority | arrival | burst |
|-----|--------|----------|----------|---------|-------|
| 1   | shell  | RUNNING  | 5        | 0       | 99    |
| 2   | knight | READY    | 3        | 0       | 8     |
| 3   | bishop | READY    | 3        | 1       | 4     |
| 4   | rook   | READY    | 2        | 2       | 9     |
| 5   | queen  | READY    | 9        | 3       | 5     |
| 6   | pawn   | BLOCKED  | 1        | 4       | 2     |

The shell is treated specially: skipped during simulations (it represents
the interactive process).  The other five are batch jobs.

### FCFS (First Come First Served)

Pick the unfinished job with the earliest arrival time, run it to
completion, repeat.

```
t=0:  knight arrives, run knight (8 ticks).  comp[knight]=8.
t=8:  bishop, rook, queen, pawn have all arrived.  Pick earliest = bishop
      (arrival=1).  Run bishop (4 ticks).  comp[bishop]=12.
t=12: pick rook (next earliest, arrival=2).  Run rook (9 ticks).
      comp[rook]=21.
t=21: pick queen (arrival=3).  Run queen (5 ticks).  comp[queen]=26.
t=26: pick pawn.  Run pawn (2 ticks).  comp[pawn]=28.

turnaround = comp - arrival
knight: 8 - 0  = 8
bishop: 12 - 1 = 11
rook:   21 - 2 = 19
queen:  26 - 3 = 23
pawn:   28 - 4 = 24

avg = (8 + 11 + 19 + 23 + 24) / 5 = 85 / 5 = 17
```

### SJF (Shortest Job First, non-preemptive)

Of the jobs that have already arrived, pick the smallest burst.  Run to
completion.

```
t=0:  only knight has arrived. Run knight (8 ticks). comp=8.
t=8:  bishop, rook, queen, pawn all arrived.
      Bursts: bishop=4, rook=9, queen=5, pawn=2.  Pick pawn.  Run 2 ticks.
      comp[pawn]=10.
t=10: pick bishop (burst=4).  Run.  comp[bishop]=14.
t=14: pick queen (burst=5).  Run.  comp[queen]=19.
t=19: pick rook (burst=9).  Run.  comp[rook]=28.

turnaround:
knight: 8 - 0  = 8
pawn:   10 - 4 = 6
bishop: 14 - 1 = 13
queen:  19 - 3 = 16
rook:   28 - 2 = 26

avg = (8 + 6 + 13 + 16 + 26) / 5 = 69 / 5 = 13
```

SJF wins by 4 ticks of average turnaround on this workload.

### Round Robin (RR), quantum=2

Maintain a FIFO ready queue.  Each turn, dequeue the front, run it for
`min(remaining, quantum)` ticks, advance the clock.  Re-admit any
newly-arrived processes.  If the dequeued process still has work, append
it to the back; otherwise record completion.

This is more bookkeeping but the textbook algorithm.

### Priority (non-preemptive)

Of the jobs that have arrived, pick the one with the highest priority
value.  Run to completion.

```
t=0:  knight (pri=3) only.  Run knight 8 ticks.  comp[knight]=8.
t=8:  all arrived. Priorities: bishop=3, rook=2, queen=9, pawn=1.
      Pick queen.  Run 5 ticks.  comp[queen]=13.
t=13: pick bishop (pri=3).  Run 4 ticks.  comp[bishop]=17.
t=17: pick rook (pri=2).  Run 9 ticks.  comp[rook]=26.
t=26: pick pawn (pri=1).  Run 2 ticks.  comp[pawn]=28.
```

### Average turnaround

```c
int avgTurnaround(int* comp) {
    int sum = 0, n = 0;
    for (int i = 1; i < pCount; i++) {
        if (comp[i] > 0) {
            sum += (comp[i] - pArrival[i]);
            n++;
        }
    }
    return n == 0 ? 0 : myDiv(sum, n);
}
```

We average over batch jobs only (skip i=0, the shell).

---

## 16. Memory allocator simulator (in detail)

Heap is 4096 KB.  Block list is sorted by start address.  Each block:

```c
int memStart[16];   // block's start offset within the heap (in KB)
int memSize[16];    // block's size in KB
int memOwner[16];   // owner pid (-1 if free)
int memCount;       // number of blocks currently in use
```

Initial layout (chess-themed):

```
Start (KB)  Size (KB)  Owner
0           256        shell
256         64         knight
320         64         FREE
384         128        bishop
512         256        rook
768         128        FREE
896         200        queen
1096        3000       FREE (the big tail)
```

Total used: 256+64+128+256+200 = 904 KB.
Total free: 64+128+3000 = 3192 KB.
Total: 904 + 3192 = 4096.  ✓

### allocBlock(kb, strategy)

1. Scan the block list looking for a free block that fits.
2. By strategy:
   - 0 (first-fit): take the first one that fits
   - 1 (best-fit): take the smallest one that fits
   - 2 (worst-fit): take the largest one that fits
3. If chosen block is exactly the right size, just flip its owner to "user" (99).
4. Otherwise SPLIT: shift later blocks right one slot, the chosen block
   becomes "user-owned at requested size", the new block right after
   becomes free with the leftover size.

### freeBlock(idx)

1. Mark the block free.
2. Call `coalesce()`.

### coalesce()

Walk the block list.  Whenever two adjacent blocks are both free, merge them:
- `memSize[i] += memSize[i+1]`
- Shift later blocks left one slot to fill the gap
- Decrement memCount

This prevents fragmentation from accumulating across alloc/free cycles.

### Fragmentation metric

```
fragmentation = 100 - (largestFreeBlock * 100 / totalFree)
```

If all free space is one big contiguous block, fragmentation = 0%.
If free space is split across many small blocks, the largest-to-total
ratio shrinks and fragmentation approaches 100%.

This is one common metric.  Real allocators have other measures like
"external fragmentation" (free space that can't be allocated because no
single block is big enough).

---

## 17. The build process

### compileOS.sh, line by line

```sh
nasm bootload.asm
```
NASM assembles `bootload.asm` and produces a flat binary file `bootload`
of exactly 512 bytes.

```sh
bcc -ansi -c -o kernel.o kernel.c
```
bcc compiles `kernel.c` to `kernel.o` (16-bit object file).  `-ansi`
enables ANSI C89 mode (function prototypes work, etc).  `-c` means
"compile only, don't link".

```sh
as86 kernel.asm -o kernel_asm.o
```
`as86` is the 16-bit assembler that pairs with bcc.  Assembles
`kernel.asm` to a 16-bit object.

```sh
ld86 -o kernel -d kernel.o kernel_asm.o
```
`ld86` is the 16-bit linker.  `-o kernel` = output binary.  `-d` =
strip / "raw" output (no headers, no relocations, just code+data ready
to load at a fixed segment).  Combines kernel.o (C) and kernel_asm.o
(asm).

```sh
bcc -ansi -c -o shell.o shell.c
as86 lib.asm -o lib.o
ld86 -o shell -d shell.o lib.o
```
Same idea for the shell, but linked against `lib.asm` (the user-space
assembly helpers — just `interrupt()`) instead of `kernel.asm`.

```sh
bcc -ansi -c -o chess.o chess.c
ld86 -o chess -d chess.o lib.o
```
Same for chess.  Notice it reuses `lib.o` (no separate assemble of
lib.asm).

```sh
gcc -o loadFile loadFile.c
```
`loadFile.c` is a host-side utility, compiled with regular gcc to run
on Linux.  It reads files from the host filesystem and writes them
into the floppy image's file system.

```sh
dd if=/dev/zero of=floppya.img bs=512 count=2880
```
Create a fresh empty floppy image.  2880 sectors × 512 bytes = 1.44 MB.

```sh
dd if=bootload of=floppya.img bs=512 count=1 conv=notrunc
```
Write the bootloader binary to sector 0 of the floppy image.
`conv=notrunc` prevents dd from truncating the output file (so the rest
of the floppy stays as zeros).

```sh
dd if=map.img of=floppya.img bs=512 count=1 seek=1 conv=notrunc
dd if=dir.img of=floppya.img bs=512 count=1 seek=2 conv=notrunc
```
Write the initial disk map and directory at sectors 1 and 2.  `seek=N`
means "skip N output blocks before writing".

```sh
dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3
```
Write the kernel binary starting at sector 3.

```sh
./loadFile shell
./loadFile chess
./loadFile readme
./loadFile moves
./loadFile help
./loadFile tstprg
./loadFile tstpr2
```
Each of these invocations adds one file to the floppy image's file
system: it finds a free directory entry, allocates free sectors,
writes the file's bytes into them, updates map and dir.

After all this, `floppya.img` is ready to boot.

### Why each tool

- `nasm` = the bootloader is in NASM syntax (more modern).
- `bcc` + `as86` + `ld86` = paired tools for 16-bit C and asm code.  The
  bcc C frontend doesn't include its own assembler/linker; it uses
  these as86/ld86 partners.
- `gcc` = standard 64-bit C, only used for the host-side `loadFile`
  utility.

---

## 18. Last-minute facts to know cold

- **Why does my kernel start at sector 3?** Because sectors 0, 1, 2 are
  reserved for bootloader, disk map, directory.
- **How big is each sector?** 512 bytes.
- **How many sectors on the floppy?** 2880 (1.44 MB).
- **Why does my file system only address 256 of those?** Because each
  sector pointer is one byte (0-255).
- **What's the maximum file size?** 26 sectors × 512 bytes = 13312 bytes.
- **What's the maximum number of files?** 16 (the directory has 16 32-byte entries).
- **What segment is video memory at?** 0xB000, offset 0x8000 (physical 0xB8000).
- **What size is the screen in text mode?** 80 columns × 25 rows.
- **How big is each cell?** 2 bytes (char + attribute).
- **What is `0x55 0xAA`?** The magic number at the end of every valid
  bootloader, telling the BIOS this is bootable.
- **What is `INT 0x13 AH=2`?** The BIOS call to read sectors from disk.
- **What is `INT 0x10 AH=0x0E`?** BIOS teletype output (print one
  character at the cursor and advance).
- **What's the timer frequency?** ~18.2 Hz.
- **Why 18.2 Hz exactly?** PIT runs at 1.193182 MHz divided by 65536.
- **What's the max value of a 16-bit signed int?** 32767.
- **Why did 99999 break my scheduler?** Because it wraps to a negative
  number in 16 bits (99999 mod 65536 = 34463 unsigned, but interpreted
  as signed = −31073).
- **Why two syscall vectors (INT 0x21 and INT 0x22)?** Different DS
  conventions.  INT 0x21 keeps DS = caller's segment for user-pointer
  access.  INT 0x22 sets DS = CS for kernel-data access.

You're ready.
