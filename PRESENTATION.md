# ChessOS Presentation Playbook

10-15 minute presentation. Slides + live demo. Whole class + professor, Q&A after.
Lab machine with graphical display.

---

## Time budget (target: 12 min)

| Time | What |
|------|------|
| 0:00-0:30 | Opening hook |
| 0:30-3:30 | Slides (~3 min, 5 slides) |
| 3:30-11:30 | Live demo (~8 min) |
| 11:30-12:00 | Closing |

Then Q&A fills the rest. If you start running long, the part to cut is the
memory simulator (mid-demo). Always keep the chess game.

---

## Pre-demo checklist (do 10 min before)

- [ ] Sit at the lab machine, NOT SSH'd in
- [ ] Open a graphical terminal
- [ ] `cd ~/Desktop/ChessOS`
- [ ] `./compileOS.sh` — make sure it ends with no errors and prints
      "kernel size: NNNN bytes"
- [ ] Open a second terminal and queue this command (do not press Enter yet):
      `qemu-system-x86_64 -drive format=raw,file=floppya.img,index=0,if=floppy`
- [ ] Slides open, full-screen, on slide 1
- [ ] If using two monitors: slides on the projector, terminal on your laptop
- [ ] Have your phone with this file open as a backup reference
- [ ] One more dry-run of the demo on your own
- [ ] Water nearby

---

## Opening hook (0:00-0:30)

Read this almost word for word, it's the first 25 seconds:

> "When you open your laptop, you click an icon and an app starts. There's a
> whole operating system underneath that nobody sees. For my final project,
> I built one of those operating systems from scratch.  No Linux, no Windows.
> When my computer boots up, it reads my code from the very first byte of
> the disk and runs it.  And because I had to make it stand out, I made it
> chess-themed.  Let's get into it."

Then click to slide 2.

---

## Slides (3 min total)

5 slides at ~35 seconds each. Build them in Google Slides or PowerPoint
(your choice). The notes below are what each slide should contain and what
to say.

### Slide 1: Title

```
        ChessOS
     A Bare-Metal Operating System

     Adam Ibrahim
     CSC 380, Spring 2026
     Dr. Dave Toth
```

Just sit on this for ~5 seconds while the audience reads.  Then deliver the
opening hook above.

### Slide 2: What is it?

Bullet list:
- Boots from a floppy disk image in QEMU
- No host OS underneath; my code runs on the bare CPU
- Has its own bootloader, kernel, shell, file system
- Comes with a two-player chess game and a chess-themed UI
- Built on top of the Project A-D framework from class

What to say:
> "ChessOS is a tiny operating system that runs in a virtual machine called
> QEMU. The CPU starts up, reads the first 512 bytes of my floppy image, and
> jumps into my code. From that point on, every byte of code that runs is
> mine. There's no operating system underneath.  On top of the Project A-D
> framework that we built in class, I added a chess theme, a real two-player
> chess game, simulators for process scheduling and memory allocation, and a
> live timer interrupt handler that draws a clock in the corner of the
> screen."

### Slide 3: Boot chain (architecture diagram)

A simple flow diagram, top to bottom:

```
   [ BIOS ]        reads sector 0 of the floppy
       ↓
   [ bootloader ]  loads kernel from sectors 3-12
       ↓
   [ kernel ]      installs INT 0x21, draws splash, loads shell
       ↓
   [ shell ]       waits for commands
       ↓
   [ chess.c ]     loaded by the shell on demand
```

What to say:
> "Here's the boot chain.  When the computer powers on, the BIOS reads the
> very first sector off the disk - that's my bootloader, exactly 512 bytes.
> The bootloader loads my kernel from sectors 3 through 12 and jumps to
> it.  My kernel installs its system call handler, paints the chess splash,
> and loads the shell.  The shell is a separate program on the disk - it
> waits for commands. When I type 'execute chess', the shell loads my chess
> game off the disk and runs it as another program.  All of this is real
> hardware behavior - real BIOS calls, real interrupts, a real file system
> on a real disk image."

### Slide 4: Features

Two columns, no wall of text:

```
File system            Process scheduling
  type, dir, ls          ps, scheduler, simulate
  copy, rename           4 algorithms: FCFS, SJF, RR, Priority
  delete, create         compare scheduler (side by side)

Memory allocation      Timer + extras
  memUsage, memmap       Live HH:MM:SS clock
  malloc, memfree        uptime, sleep, time <cmd>
  3 fit strategies       Two-player chess game
```

What to say:
> "I built four pillars on top of the file system. A process scheduler with
> four algorithms you can switch between at runtime. A memory allocator
> with three different fit strategies and a fragmentation visualization.
> A timer interrupt handler that drives a live clock and a 'time this
> command' feature.  And a full two-player chess game I ported from my
> Intro Python final project - my own code, adapted for bare metal."

### Slide 5: Live demo transition

Just a black slide with one line:

```
        Let's run it.
```

Click to it, then alt-tab to your terminal.

---

## Live demo script (8 min)

Run these in order. The exact commands are in code blocks.  The
narration after each is what to say while it runs (or right after).
Don't read command output to the audience, let them see it.

### Demo 1 — Boot (0:30)

```sh
qemu-system-x86_64 -drive format=raw,file=floppya.img,index=0,if=floppy
```

Press Enter, then immediately step back so the audience sees the screen.

> "Watch the boot. (point) That's my bootloader running.  In a second...
> there's the chess splash, drawn straight to video memory by my kernel.
> Top right corner — that little red clock — that's a live timer interrupt
> running underneath everything I do for the rest of the demo. The
> 'ChessOS shell ready' line is the shell taking over.  We're live."

### Demo 2 — File system (1:00)

```
ChessOS> dir
ChessOS> type readme
ChessOS> type moves
```

Narrate:

> "The file system is on the disk.  'dir' walks the directory at sector 2
> and prints every entry. 'type' loads a file by walking those sector
> pointers and prints its contents. (after type readme) That's the
> README I wrote. (after type moves) And these are some chess openings I
> loaded onto the disk at build time."

```
ChessOS> create demo
The professor said this would be hard.
And it kind of was.
                                                ← empty Enter to finish
ChessOS> type demo
```

> "I can write files at runtime too. 'create' takes input until I hit an
> empty line, then writes it to disk. (after type demo) And there it is,
> coming back from the disk."

### Demo 3 — Process scheduler (1:30)

```
ChessOS> ps
```

> "The OS itself runs one program at a time, but I built a simulator for
> what would happen if it ran multiple. There's a fake process table
> with chess-themed names - knight, bishop, rook, queen, pawn. Each has
> a priority and a burst time."

```
ChessOS> scheduler SJF
ChessOS> simulate
```

> "I switched the scheduler to Shortest Job First. 'simulate' runs the
> workload through that algorithm and reports completion times and
> turnaround for each process."

```
ChessOS> compare scheduler
```

> "And this is the killer feature. Same workload, all four scheduling
> algorithms - First Come First Served, Shortest Job First, Round Robin,
> and Priority - and the average turnaround for each. SJF wins, which is
> what the textbook would predict."

### Demo 4 — Memory allocator (1:30)

```
ChessOS> memUsage
```

> "Memory.  Total is 4 megabytes, broken into eight blocks.  See that
> bar at the bottom? Equals signs are used blocks, dashes are small free
> blocks, underscores are big free runs.  This shows fragmentation
> visually.  Right now it's 5 percent fragmented."

```
ChessOS> memmap
ChessOS> malloc 200
ChessOS> memUsage
```

> "'memmap' shows every block: who owns it, where it starts, how big it
> is.  I just allocated 200 KB using first-fit. (after second memUsage)
> Notice the fragmentation went up because we split a free block."

```
ChessOS> compare memory
```

> "Same allocation pattern, three strategies — first-fit, best-fit, and
> worst-fit — and the resulting fragmentation. Best-fit produces the
> least, which again, textbook prediction."

### Demo 5 — Timer interrupts (1:30)

```
ChessOS> uptime
```

> "Look at the clock in the corner — it's been counting since boot.
> 'uptime' reads my tick counter."

```
ChessOS> sleep 3
```

> "When I run 'sleep 3', my OS hangs for three seconds. Watch the clock —
> it KEEPS counting, because my sleep handler enables interrupts so the
> timer keeps firing during the wait. Then control comes back. (waits 3s,
> control returns)"

```
ChessOS> time dir
```

> "And 'time' wraps any command and tells me how long it took. There's
> 'dir', and now it tells me 'took N ticks, M seconds.' This is built on
> the same timer interrupt that drives the clock."

### Demo 6 — Chess (2:00, the finale)

```
ChessOS> execute chess
```

> "And the finale. (chess board appears, full color)  This is a full
> two-player chess game running INSIDE my operating system. I ported it
> from my Intro Python final project. White moves first.  W and S move
> the cursor up and down, A and D left and right.  Space picks up a
> piece, space again drops it on a destination."

Play out a quick opening:
1. Move e-pawn to e4 (W to row 4, then space, then S to row 4 col 4, then space)
2. Black pawn e7 to e5 (W to row 1 col 4, space, S to row 3 col 4, space)
3. White knight g1 to f3 (cursor to white knight, space, to f3, space)
4. Black knight g8 to f6 (mirror)

> "Pawn captures pawn. Knight defends a square. The game enforces every
> piece's movement rules - pawns can't move backwards, knights jump in
> L-shapes, bishops are diagonal only - and it prevents you from making a
> move that would put your own king in check.  When a player IS in check,
> it warns 'CHECK!'"

(Optional: if you have time, force a check)

```
Q (to quit)
```

> "Press Q and we're back at the shell. The kernel reloads the shell
> program from disk in place of where chess was running, and we're back
> at the prompt with the clock still ticking."

---

## Closing (11:30-12:00)

Click to a final slide that just says:

```
        Thank you. Questions?
```

What to say:

> "That's ChessOS. About 2,500 lines of C across the kernel, the shell,
> and the chess game, plus the assembly the professor gave us. Built on
> top of Project A through D. Boots in a fraction of a second. Happy to
> answer any questions."

---

## Q&A prep (likely questions + answers)

### Architecture questions

**Q: Why bcc instead of gcc?**
> The CPU starts in 16-bit real mode. gcc generates 64-bit machine code,
> which won't run there. bcc is Bruce's C Compiler, a 16-bit cousin from
> the 80s. It's K&R-flavored, no stdlib, no `#include`, no `/` or `%`
> operators (I wrote my own mod and div). Every weird thing about my C
> code is because of bcc.

**Q: How does the bootloader find the kernel?**
> The bootloader is exactly 512 bytes that the BIOS automatically reads
> from sector 0. It uses BIOS interrupt 0x13 to read 10 more sectors —
> sectors 3 through 12 — into memory at segment 0x1000.  Then it does a
> far jump to 0x1000:0000 which is the kernel's entry point.  Sectors 1
> and 2 are reserved for the disk map and the directory.

**Q: Walk me through what happens when I type 'dir'.**
> The shell calls readString which reads keystrokes via INT 0x16 until
> Enter.  parseTokens splits the line.  dispatch sees 'dir' and calls
> doDir.  doDir does INT 0x21 with ax=2, which is my readSector system
> call, asking for sector 2 — the directory. The kernel handler converts
> sector 2 to CHS coordinates, calls BIOS INT 0x13 to read it.  Back in
> the shell, doDir walks the 16 directory entries, 32 bytes each, and
> prints any entry whose first byte is non-zero.

**Q: How does your file system work?**
> Sector 0 is the bootloader, sector 1 is the disk map, sector 2 is the
> directory, sectors 3+ are file data. The disk map is 512 bytes — one
> byte per sector, 0xFF for used and 0x00 for free. The directory is 16
> entries of 32 bytes each: 6 bytes for the filename, then up to 26
> sector pointers. To read a file, find its directory entry by name, then
> walk those sector numbers reading each one. To write, find a free entry
> in the directory and free sectors in the map, fill them in, write back.

**Q: Why is there a 5 KB limit on your kernel?**
> Because my bootloader reads exactly 10 sectors of 512 bytes each. If
> the kernel grew past that, the rest wouldn't get loaded. I could
> increase it, but I'd have to rewrite the bootloader's hardcoded count
> and update the disk metadata to reserve more sectors for the kernel.

### Scheduling questions

**Q: How is your scheduler different from Linux's?**
> Linux uses CFS - the Completely Fair Scheduler. CFS is closer to a
> weighted round-robin where every process gets a virtual runtime and the
> scheduler always picks whoever's behind. My simulator runs the four
> classical textbook algorithms - FCFS, SJF, Round Robin, and Priority -
> against a fixed workload and reports the average turnaround so you can
> compare. Real Linux blends ideas from these.

**Q: What's the difference between SJF and FCFS on your workload?**
> Same set of processes, same arrival times. FCFS runs them in arrival
> order, so a long job blocks all the short jobs behind it. SJF picks
> the shortest available job at each decision point, so short jobs
> finish first.  In both cases the same total work happens but the
> average turnaround is much lower with SJF — which is the classic
> textbook result.

**Q: Why does Round Robin not always win?**
> Round Robin gives every process a fair time slice but adds context
> switch overhead. My simulation doesn't model that overhead, but if it
> did, RR would lose more on workloads with mostly short jobs because the
> overhead per switch matters.

### Memory questions

**Q: How does your free-list coalescing work?**
> When you free a block, I mark it free and then walk the block list
> looking for adjacent free blocks. If two are next to each other, I
> merge them by extending the first to cover both and shifting later
> blocks down one slot. Without that, alloc/free cycles would accumulate
> fragmentation forever.

**Q: Why not just use Linux's malloc?**
> The OS itself only runs one program at a time and has no kernel
> concept of per-process address spaces. Real malloc relies on the
> kernel giving you arbitrary memory pages. I don't have that. The
> simulator shows the algorithms running on a virtual heap.

### Timer interrupt questions

**Q: How does the timer fire?**
> The PC has a Programmable Interval Timer chip that fires INT 0x08
> about 18.2 times per second. The BIOS handles INT 0x08 internally and
> as part of that calls INT 0x1C as a "user hook." By default INT 0x1C
> is empty.  I install my own handler there so I get a callback every
> tick without breaking BIOS.

**Q: Why two syscall vectors, INT 0x21 and INT 0x22?**
> They run with different DS conventions. INT 0x21 keeps DS as the
> caller's segment, which is correct for handlers that read user
> pointers — printString, readFile, etc. INT 0x22 sets DS to the kernel
> segment, which is correct for handlers that read kernel globals or
> string literals — uptime, sleep, time. They have to be separate
> because flipping DS in INT 0x21 would break every existing handler.

**Q: Why does sleep need to enable interrupts?**
> Because INT entry automatically clears the interrupt flag. If sleep
> didn't run STI before its busy-wait loop, the timer wouldn't fire
> during the wait, the tick counter would never advance, and the loop
> would never exit. Deadlock. STI fixes it.

### Chess questions

**Q: How does check detection work?**
> After every legal move I tentatively apply it. Then I scan the board
> to find my own king. Then I scan all opponent pieces and ask: can any
> of them legally move to the king's square? If yes, I'm in check, so I
> roll back the move. The trick is using the same isLegalMove function
> recursively — but a "rules-only" version that doesn't itself call
> back into check detection, otherwise infinite recursion.

**Q: What's missing from your chess?**
> No castling, no en passant, no pawn promotion, no checkmate detection
> (only check warning).  Also no AI — it's two-player only. Each of
> those would push past the 13 KB file size limit, but they're each
> doable as future work.

**Q: What happens when you press Q in chess?**
> Chess clears the screen via INT 0x21 ax=9, then calls INT 0x21 ax=4
> with the string "shell" — that's my executeProgram syscall. The
> kernel reads the shell file off the disk into segment 0x2000, where
> chess was running, and jumps to it. Chess's memory gets overwritten
> with the shell. New shell starts fresh.

### Generic / opener questions

**Q: How long did this take?**
> Project A through D was the bulk of the semester.  The chess theme,
> chess game, simulators, and timer interrupt I wrote in (give honest
> answer based on your actual time).  Most of the time was debugging
> tiny things specific to bcc and 16-bit real mode.

**Q: What's the most surprising thing you learned?**
> That every weird-looking thing in this code traces back to an actual
> hardware constraint - segments because we're 16-bit, BIOS interrupts
> because there's nothing else, no division because bcc doesn't support
> it. You don't appreciate stdlib until you don't have it.

**Q: Why chess?**
> The professor said the project should be impressive. I thought a chess
> theme was a fun way to differentiate it.  And then once I had the
> theme, I realized I had a fully-working chess game from my Python class
> that I could port. So now my OS plays chess.

---

## Failure recovery

**If `compileOS.sh` fails right before the demo:**
> "Looks like the build had an issue, give me one second.  (re-run it,
> apologize once if it fails again)"
> 
> If it really won't build, fall back to your slides plus a pre-recorded
> screen capture (see backup plan below).

**If QEMU won't start:**
> Check whether you're SSH'd in by accident.  Use the actual lab
> machine's local terminal.  If still broken, try `-display sdl`
> explicitly.

**If the splash doesn't show but everything else works:**
> Don't address it. Move on. The audience won't notice unless you point
> at it.

**If the clock doesn't show:**
> Don't address it.  Move on.  Same reason.

**If chess crashes or hangs on quit:**
> "And we're going to come back to that one when we have more time. Let
> me show you ..." Move on.  Reboot QEMU and don't repeat chess.

**If a command produces unexpected output:**
> "That's a great question to come back to in Q&A — let me show you the
> next thing." Move on.

**If you completely freeze:**
> "Sorry, lost my place for a sec." Look at this file, find the next
> step, continue.

### Backup plan: pre-recorded video

The week before the demo, do this:
1. Run through the entire demo on the lab machine
2. Use a screen recorder (asciinema, OBS, or just QuickTime if you have access)
3. Save the recording somewhere accessible — Google Drive, USB

If anything catastrophic happens during the live demo, switch to the video
and narrate over it. Better to have it and not need it.

---

## Things to NOT say

- Don't say "this is just a simulation" about the OS itself. The OS is real,
  the scheduler and memory allocator inside it are simulated demonstrations
  of textbook algorithms.
- Don't say "I just modified the professor's code." You wrote the kernel
  extensions, the shell, the chess game, all the simulator code, and the
  timer handler. Be confident.
- Don't apologize for missing features (no castling, no checkmate detection)
  unless directly asked.
- Don't say "I don't know" without a follow-up. If you genuinely don't know,
  say "I'd have to look that up but my best guess is..."

---

## After the demo

When the professor asks the first Q&A question, take a breath.  Don't rush.
Look at this file's Q&A section in your head if you're stuck. If you don't
know an answer, that's fine - "I'd have to check, but I think it works
because..." is a perfectly good answer in an OS class.

Good luck.
