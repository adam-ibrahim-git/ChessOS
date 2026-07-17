# ChessOS

A 16-bit x86 operating system I wrote from scratch in C. It boots on bare metal in QEMU and runs a chess game I wrote to run on it.

My operating systems final only needed an OS that booted. I wanted to understand the whole stack, so I wrote a real kernel with its own system calls and a file system, put a command shell on top, and rewrote my Python chess engine as a 710-line C program that runs as a user program on the OS.

The ~330 lines of assembly (bootload.asm, kernel.asm, lib.asm) were provided by the course as the entry stub. Everything else is my C, about 3,100 lines. I do not write assembly and this project does not claim that I do.

Built solo for CSC 380 (Operating Systems), Spring 2026, under Dr. Dave Toth. I presented it and defended it in a live demo.

## What it does

- Boots on bare metal. The BIOS loads the bootloader, which reads the kernel off disk and jumps to it. No underlying OS, no standard library.
- Kernel (`kernel.c`, ~720 lines) with system calls for printing, keyboard input, disk read and write, and loading and running programs.
- A sector-mapped file system. A disk map and directory track which sectors belong to which files, and the kernel reads, writes, lists, copies, and deletes files by walking that map.
- A command shell (`shell.c`, ~1,585 lines) that tokenizes input, dispatches built-in commands (type, dir, copy, execute, chess, and more), and launches programs.
- Chess (`chess.c`, 710 lines): my Python chess engine rewritten in C, with legal moves for all six pieces and check and checkmate detection.

## Files

| File | Lines | What it is |
|---|---|---|
| `kernel.c` | ~720 | Kernel: system calls, disk I/O, loading programs, text display |
| `shell.c` | ~1,585 | Command shell |
| `chess.c` | 710 | Chess, written in C, runs on the OS |
| `loadFile.c` | ~130 | Host-side tool for writing files onto the disk image |
| `bootload.asm`, `kernel.asm`, `lib.asm` | ~330 | Course-provided assembly stub (not mine) |
| `compileOS.sh` | | Build script |
| `dir.img`, `map.img` | | Initial file-system sectors |

## Build and run

It uses the 16-bit bcc/as86/ld86 toolchain, nasm, and QEMU.

```bash
sudo apt-get install bin86 bcc nasm qemu-system-x86
./compileOS.sh
qemu-system-i386 -fda floppya.img
```

Once it boots, type `help` to see commands, `dir` to list files, and `chess` to play.

## License

MIT, see [LICENSE](LICENSE).
