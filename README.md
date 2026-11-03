# ChessOS

A 16-bit x86 operating system in C. It boots from a floppy image in QEMU and runs a two-player chess game I wrote for it.

I built it for CSC 380 (Operating Systems) at Centre College in Spring 2026, under Dr. Dave Toth, and presented it in a live demo. The course's Projects A to D supplied the framework (Michael Black's teaching OS: a bootloader, assembly helpers and a host-side tool that writes files onto the disk image) and specified the core: system calls through INT 0x21, a file system on the floppy and a command shell. I wrote those in C, then kept going.

## What I added beyond the course projects

- A two-player chess game (`chess.c`) that runs as a program on the OS, ported from the Python chess game I wrote in my intro programming course.
- A live HH:MM:SS clock in the corner of the screen, driven by the INT 0x1C timer interrupt, plus `uptime`, `sleep` and `time <command>`.
- A second system-call vector, INT 0x22.
- A simulated process scheduler (FCFS, SJF, Round Robin, Priority) with a side-by-side comparison of all four.
- A simulated memory allocator (first fit, best fit, worst fit) with a fragmentation display.
- A chess-themed boot splash painted straight into video memory, and a `diskinfo` command.

## The chess game

White moves first. Pieces move by the normal rules, except that castling, en passant and promotion are not implemented. The game rejects any move that leaves your own king in check and shows "CHECK!" when the side to move is attacked. It does not detect checkmate or stalemate.

Controls: W/A/S/D move the cursor, Space picks up a piece and Space again drops it, Q quits back to the shell.

## Real and simulated

Real hardware paths: the BIOS loads the bootloader from sector 0; disk I/O goes through INT 0x13, the keyboard through INT 0x16, and the screen through INT 0x10 and direct writes to 0xB8000; files persist on the floppy image between boots; INT 0x21 and INT 0x22 are installed in the interrupt vector table; the clock runs off INT 0x1C.

Simulated: the scheduler and the memory allocator. The OS runs one program at a time and has no per-process address spaces.

## Commands

Once it boots, type `help`.

```
type <file>, fs cat <file>   print a file
dir, ls, fs ls               list files
copy <a> <b>                 duplicate a file
rename <a> <b>               rename a file
delete <file>, fs rm <file>  remove a file
create <file>                type lines, a blank line saves
execute <file>               run a program (execute chess starts the game)
diskinfo                     used and free sectors, file count
cls                          clear the screen

ps, scheduler <ALG>, simulate, compare scheduler           scheduler simulator (ALG = FCFS, SJF, RR, PRI)
memUsage, memmap, malloc <kb>, memfree <n>, compare memory   memory simulator
uptime, sleep <sec>, time <cmd>                             timer commands
```

## Files

| File | Lines | What it is |
|---|---|---|
| `kernel.c` | 718 | Kernel: the Project B to D system calls, disk I/O and program loading, extended with the timer, splash and `diskinfo` |
| `shell.c` | 1,585 | The Project C and D shell, extended with the simulators and timer commands |
| `chess.c` | 710 | The chess game |
| `compileOS.sh` | | Build script |
| `kernel.asm` | | Course framework assembly helpers, plus the INT 0x1C, INT 0x22 and sleep routines |
| `bootload.asm`, `lib.asm`, `loadFile.c`, `map.img`, `dir.img`, `tstprg`, `tstpr2`, `message.txt` | | Course framework |

## Build and run

It uses the 16-bit bcc/as86/ld86 toolchain, nasm, gcc and QEMU.

```bash
sudo apt-get install bin86 bcc nasm gcc qemu-system-x86
./compileOS.sh
qemu-system-i386 -fda floppya.img
```

The kernel has to fit in the 10 sectors the bootloader reads (5 KB); it compiles to about 3.5 KB. The shell is about 6 KB and the chess game about 5 KB.

## Credits and license

The framework files (`bootload.asm`, the course parts of `kernel.asm`, `lib.asm`, `loadFile.c`, `map.img`, `dir.img`, `tstprg`, `tstpr2`, `message.txt`) are by Dr. Michael Black (American University), modified over the years by Dr. Sara Sprenkle (Washington and Lee), Dr. Emily Hill (Drew University) and Dr. Dave Toth for CSC 380. My files (`kernel.c`, `shell.c`, `chess.c`, `compileOS.sh`) are MIT licensed; see [LICENSE](LICENSE) and [NOTICE](NOTICE).
