# ChessOS

A bare-metal, 16-bit real-mode x86 operating system, written from scratch in C, that boots in QEMU into a fully playable chess game.

My Operating Systems final only required an OS that *booted*. Instead of stopping there, I wrote a real kernel with its own system calls and file system, built a command shell on top of it, and then rewrote my 1,000-line Python chess engine as a 710-line C program that runs as a user process **on the OS I built**.

> **Scope, stated honestly:** the ~330-line assembly bootstrap (`bootload.asm`, `kernel.asm`, `lib.asm`) was provided by the course as the entry stub. **My work is the ~3,100 lines of C** — the kernel, the file system, the shell, and the chess program. I don't write assembly, and this project doesn't claim that I do.

Built solo for CSC 380 (Operating Systems), Spring 2026, under Dr. Dave Toth. Presented and defended in a live demo.

---

## What it does

- **Boots on bare metal.** The BIOS loads the bootloader from sector 0; the bootloader pulls the kernel off disk into memory and jumps to it. No underlying OS, no standard library — just the hardware, the BIOS interrupts, and my code.
- **Custom kernel with system calls.** `kernel.c` (~720 lines) implements the core services — printing, keyboard input, disk read/write, process loading and execution — exposed to user programs through a hand-rolled interrupt-based system-call interface.
- **Sector-mapped file system.** A disk map (sector 1) and directory (sector 2) track which sectors belong to which files; the kernel reads, writes, lists, copies, and deletes files by walking that map. Files live at sector 13 and up.
- **A real command shell.** `shell.c` (~1,585 lines) parses input into tokens, dispatches built-in commands (`type`, `dir`/`ls`, `copy`, `fs` subcommands, `execute`, `chess`, …), and launches other programs as processes.
- **Chess as a user program.** `chess.c` (710 lines) is my Python chess engine rewritten in C to run on ChessOS: full legal-move generation for all six piece types, check/checkmate detection, and a board rendered to the OS's text display.

Full architecture write-up: **[TECHNICAL_DEEP_DIVE.md](TECHNICAL_DEEP_DIVE.md)** — the boot story, memory layout, the syscall mechanism, and the file-system format, in the level of detail I used to defend the project.

---

## Repository map

| File | Lines | What it is |
|---|---|---|
| `kernel.c` | ~720 | Kernel: syscalls, disk I/O, process loading, text display |
| `shell.c` | ~1,585 | Command shell: tokenizer, dispatch, built-in commands |
| `chess.c` | ~710 | Playable chess, written in C, runs as a program on the OS |
| `loadFile.c` | ~130 | Host-side utility for writing files onto the disk image |
| `bootload.asm`, `kernel.asm`, `lib.asm` | ~330 | **Course-provided** assembly bootstrap (not my work) |
| `compileOS.sh` | — | Build script: assembles, compiles, and lays out the floppy image |
| `dir.img`, `map.img` | — | Initial file-system sectors (directory + sector map) |
| `TECHNICAL_DEEP_DIVE.md` | — | Deep technical documentation |
| `PRESENTATION.md` | — | The talk I gave defending the project |

---

## Build & run

ChessOS targets the classic 16-bit `bcc` / `as86` / `ld86` toolchain and runs under QEMU.

```bash
# Toolchain (Linux): dev86 (provides bcc/as86/ld86), nasm, qemu
sudo apt-get install bin86 bcc nasm qemu-system-x86

# Build the bootable floppy image
./compileOS.sh

# Boot it
qemu-system-i386 -fda floppya.img
```

Once booted, type `help` to list commands, `dir` to see files, and `chess` to play.

---

## Why I built it this way

The assignment rewarded a blinking cursor. I wanted to understand the whole stack — how a machine goes from `0x7C00` to running my own program — so I built enough OS to actually *host* software, then gave it something non-trivial to host. Writing the chess engine twice (Python, then C on bare metal with no libraries) forced me to understand what every line actually depends on. That's the project I'm most proud of.

## License

MIT — see [LICENSE](LICENSE).
