#!/bin/bash

nasm bootload.asm
bcc -ansi -c -o kernel.o kernel.c
as86 kernel.asm -o kernel_asm.o
ld86 -o kernel -d kernel.o kernel_asm.o

bcc -ansi -c -o shell.o shell.c
as86 lib.asm -o lib.o
ld86 -o shell -d shell.o lib.o

bcc -ansi -c -o chess.o chess.c
ld86 -o chess -d chess.o lib.o

gcc -o loadFile loadFile.c

dd if=/dev/zero of=floppya.img bs=512 count=2880
dd if=bootload of=floppya.img bs=512 count=1 conv=notrunc
dd if=map.img of=floppya.img bs=512 count=1 seek=1 conv=notrunc
dd if=dir.img of=floppya.img bs=512 count=1 seek=2 conv=notrunc
dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3

# create the  text files
cat > readme << 'END'
Welcome to ChessOS!
Type help to see all commands.
END

cat > moves << 'END'
1. e4 e5 2. Nf3 Nc6 3. Bb5  (Ruy Lopez)
1. d4 d5 2. c4              (Queens Gambit)
1. e4 c5                    (Sicilian Defense)
END

cat > help << 'END'
Commands:
  type <f>            print a file
  fs cat <f>          same as type
  dir / ls            list files
  copy <a> <b>        duplicate a file
  delete <f>          remove a file
  rename <a> <b>      rename a file
  create <f>          enter text, blank line ends
  execute <f>         run a program
  cls                 clear screen
  diskinfo            disk usage
  ps                  process table
  scheduler <ALG>     FCFS, SJF, RR, or PRI
  simulate            run scheduler on the workload
  compare scheduler   run all 4 schedulers
  memUsage            memory usage + fragmentation bar
  memmap              show every block
  malloc <kb>         allocate
  memfree <block#>    free a block
  compare memory      compare first/best/worst fit
  uptime              show seconds since boot
  sleep <sec>         wait the given number of seconds
  time <cmd>          run a command and print elapsed time
  help                print this

END

./loadFile shell
./loadFile chess
./loadFile readme
./loadFile moves
./loadFile help
./loadFile tstprg
./loadFile tstpr2
