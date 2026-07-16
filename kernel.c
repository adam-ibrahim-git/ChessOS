/*
 * CSC 380 final project
 * Adam Ibrahim
 * Dr. Dave Toth
 *
 * kernel.c
 * INT 0x21 calls:
 *   ax=0  printString    bx=string
 *   ax=1  readString     bx=buffer
 *   ax=2  readSector     bx=buffer cx=sector
 *   ax=3  readFile       bx=name   cx=buffer
 *   ax=4  executeProgram bx=name   cx=segment
 *   ax=5  terminate
 *   ax=6  writeSector    bx=buffer cx=sector
 *   ax=7  deleteFile     bx=name
 *   ax=8  writeFile      bx=name   cx=buffer dx=numSectors
 *   ax=9  clearScreen
 *   ax=10 diskInfo
 *
 * INT 0x22 calls (run with DS=CS so kernel globals/literals work):
 *   ax=0  printUptime
 *   ax=1  sleep   cx=#ticks
 *   ax=2  timeStart
 *   ax=3  timeEnd
 *
 * INT 0x1C is the BIOS user timer hook.  Our handler increments i_ticks
 * and updates the on-screen clock.
 */

/* tick counters maintained by the timer */
int i_ticks;
int i_ticksThisSec;
int i_ss;
int i_mm;
int i_hh;
int i_savedTicks;

void printString(char*);
void readString(char*);
void readSector(char* buffer, int sector);
void writeSector(char* buffer, int sector);
void readFile(char* name, char* buffer);
void writeFile(char* name, char* buffer, int numSectors);
void deleteFile(char* name);
void executeProgram(char* name, int segment);
void terminate();
void clearScreen();
void diskInfo();
void chessSplash();
void putAt(int row, int col, char* str, int color);
void printNumber(int n);
void updateClockDisplay();
void printUptime();
void printElapsed();
void clockCell(int col, char ch);
void sleepTicks(int n);
int  nameMatches(char* dir, int offset, char* name);
int  mod(int a, int b);
int  div(int a, int b);
void handleInterrupt21(int ax, int bx, int cx, int dx);
void handleInterrupt22(int ax, int bx, int cx, int dx);
void handleInterrupt1C();


void main()
{
    i_ticks        = 0;
    i_ticksThisSec = 0;
    i_ss           = 0;
    i_mm           = 0;
    i_hh           = 0;
    i_savedTicks   = 0;

    makeInterrupt21();
    makeInterrupt22();
    makeInterrupt1C();

    chessSplash();

    interrupt(0x10, 0x0200, 0, 0, 0x1000);

    /* load the shell at segment 0x2000 and run it */
    interrupt(0x21, 4, "shell\0", 0x2000, 0);

    while (1) {
    }
}

/*
 * Prints a string to the screen using BIOS interrupt 0x10.  Walks the array
 * one character at a time until it hits the null.
 * chars - pointer to the string
 */
void printString(char* chars)
{
    int i;
    i = 0;
    while (chars[i] != 0x0) {
        interrupt(0x10, 0xe*256 + chars[i], 0, 0, 0);
        i = i + 1;
    }
}

/*
 * Reads a line typed at the keyboard using BIOS interrupt 0x16.
 * chars - buffer that will receive the typed line
 */
void readString(char* chars)
{
    char in;
    int i;
    in = 0;
    i = 0;
    while (in != 0xd) {
        in = interrupt(0x16, 0, 0, 0, 0);
        if (in == 0xd) {
            /* user typed enter */
        } else if (in == 0x8) {
            /* backspace */
            if (i > 0) {
                i = i - 1;
                interrupt(0x10, 0xe*256 + 0x8, 0, 0, 0);
            }
        } else {
            chars[i] = in;
            /* also print it to screen */
            interrupt(0x10, 0xe*256 + in, 0, 0, 0);
            i = i + 1;
        }
    }
    chars[i] = 0xa;
    chars[i+1] = 0x0;

    interrupt(0x10, 0xe*256 + 0xa, 0, 0, 0);
    interrupt(0x10, 0xe*256 + 0xd, 0, 0, 0);
}

/*
 * Calculates a mod b.
 * a - the dividend
 * b - the divisor
 * Returns a mod b.
 */
int mod(int a, int b)
{
    while (a >= b) {
        a = a - b;
    }
    return a;
}

/*
 * Calculates integer division a / b.
 * a - the dividend
 * b - the divisor
 * Returns the integer quotient of a divided by b.
 */
int div(int a, int b)
{
    int q;
    q = 0;
    while (q * b <= a) {
        q = q + 1;
    }
    return q - 1;
}

/*
 * Reads one 512-byte sector from the floppy using BIOS interrupt 0x13
 * buffer - 512-byte buffer that will receive the sector contents
 * sector - sector number
 */
void readSector(char* buffer, int sector)
{
    int relSec;
    int head;
    int track;
    relSec = mod(sector, 18) + 1;
    head   = mod(div(sector, 18), 2);
    track  = div(sector, 36);
    interrupt(0x13, 2*256 + 1, buffer, track*256 + relSec, head*256);
}

/*
 * Writes one 512-byte sector to the floppy.
 * buffer - 512-byte buffer holding the data to write
 * sector - sector number
 */
void writeSector(char* buffer, int sector)
{
    int relSec;
    int head;
    int track;
    relSec = mod(sector, 18) + 1;
    head   = mod(div(sector, 18), 2);
    track  = div(sector, 36);
    interrupt(0x13, 3*256 + 1, buffer, track*256 + relSec, head*256);
}

/*
 * Compares a 6-byte name field in dir against a search name.
 * dir - pointer to a 512-byte directory sector
 * offset - byte offset into dir where the entry's name starts
 * name - string to look for
 * Returns 1 if the names match, 0 if they do not.
 */
int nameMatches(char* dir, int offset, char* name)
{
    int j;
    int k;
    for (j = 0; j < 6; j = j + 1) {
        if (name[j] == 0) {
            for (k = j; k < 6; k = k + 1) {
                if (dir[offset + k] != 0) {
                    return 0;
                }
            }
            return 1;
        }
        if (dir[offset + j] != name[j]) {
            return 0;
        }
    }
    return 1;
}

/*
 * Loads a file from the disk into a memory buffer.  Reads the directory
 * (sector 2), finds the entry whose name matches, then reads each of that
 * entry's sectors into successive 512-byte windows of the buffer.  Each
 * directory entry has bytes 0-5 = name and bytes 6-31 = sector list with
 * 0 marking unused slots.
 * name - null-terminated filename to load
 * buffer - destination buffer (must be big enough for the whole file)
 */
void readFile(char* name, char* buffer)
{
    char dir[512];
    int i;
    int j;
    int sectorNum;

    readSector(dir, 2);

    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] == 0) {
            /* empty entry */
        } else if (nameMatches(dir, i, name)) {
            for (j = 6; j < 32; j = j + 1) {
                sectorNum = dir[i + j];
                if (sectorNum < 0) {
                    sectorNum = sectorNum + 256;
                }
                if (sectorNum == 0) {
                    return;
                }
                readSector(buffer, sectorNum);
                buffer = buffer + 512;
            }
            return;
        }
    }
}

/*
 * Loads a program file into a target memory segment and jumps to it.  The
 * launched program runs in place of whatever was previously at that
 * segment.  This call never returns.
 * name - null-terminated filename of the program
 * segment - segment to load and run the program at (e.g. 0x2000 for shell)
 */
void executeProgram(char* name, int segment)
{
    char buffer[13312];
    int i;
    readFile(name, buffer);
    for (i = 0; i < 13312; i = i + 1) {
        putInMemory(segment, i, buffer[i]);
    }
    launchProgram(segment);
}

/*
 * Reloads the shell.  User programs call this through INT 0x21 ax=5 when
 * they finish running, which brings the user back to the shell prompt.
 */
void terminate()
{
    interrupt(0x21, 4, "shell\0", 0x2000, 0);
}

/*
 * Removes a file from the disk.
 * name - filename to delete
 */
void deleteFile(char* name)
{
    char map[512];
    char dir[512];
    int i;
    int j;
    int sectorNum;

    readSector(map, 1);
    readSector(dir, 2);

    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] == 0) {
            /* empty entry */
        } else if (nameMatches(dir, i, name)) {
            for (j = 6; j < 32; j = j + 1) {
                sectorNum = dir[i + j];
                if (sectorNum < 0) {
                    sectorNum = sectorNum + 256;
                }
                if (sectorNum != 0) {
                    map[sectorNum] = 0x00;
                }
            }
            dir[i] = 0x00;
            writeSector(map, 1);
            writeSector(dir, 2);
            return;
        }
    }
}

/*
 * Writes a new file to disk.
 * name - filename for the new file
 * buffer - data to write
 * numSectors - how many sectors the file should occupy
 */
void writeFile(char* name, char* buffer, int numSectors)
{
    char map[512];
    char dir[512];
    int dirIndex;
    int i;
    int j;
    int s;
    int freeSector;

    readSector(map, 1);
    readSector(dir, 2);

    /* find a free directory entry (first byte 0x00) */
    dirIndex = -1;
    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] == 0) {
            dirIndex = i;
            i = 512;
        }
    }
    if (dirIndex < 0) {
        return;
    }

    /* zero the whole entry first.  a deleted entry only has its first byte
     * cleared, so old name and sector pointers can still be there. */
    for (j = 0; j < 32; j = j + 1) {
        dir[dirIndex + j] = 0;
    }

    /* copy the name into bytes 0-5 */
    for (j = 0; j < 6; j = j + 1) {
        if (name[j] == 0) {
            j = 6;
        } else {
            dir[dirIndex + j] = name[j];
        }
    }

    /* allocate a free sector and write each block */
    for (s = 0; s < numSectors; s = s + 1) {
        freeSector = -1;
        for (i = 0; i < 256; i = i + 1) {
            if (map[i] == 0) {
                freeSector = i;
                i = 256;
            }
        }
        if (freeSector < 0) {
            /* disk full, stop */
            s = numSectors;
        } else {
            map[freeSector] = 0xFF;
            dir[dirIndex + 6 + s] = freeSector;
            writeSector(buffer, freeSector);
            buffer = buffer + 512;
        }
    }

    writeSector(map, 1);
    writeSector(dir, 2);
}

/*
 * Writes a string directly to video memory at (row, col).
 * row - screen row (0 to 24)
 * col - screen column (0 to 79)
 * str -  string to draw
 * color - attribute byte to use for every character of the string
 */
void putAt(int row, int col, char* str, int color)
{
    int offset;
    int i;
    offset = (row * 80 + col) * 2;
    i = 0;
    while (str[i] != 0) {
        putInMemory(0xB000, 0x8000 + offset, str[i]);
        putInMemory(0xB000, 0x8000 + offset + 1, color);
        offset = offset + 2;
        i = i + 1;
    }
}

/*
 * Paints a chess board at launch
 */
void chessSplash()
{
    int row;
    int col;
    int i;
    int i_boardTop;
    int i_boardLeft;
    int i_videoOff;
    int i_sqColor;
    char piece;
    char* l_whitePieces;
    char* l_blackPieces;

    i_boardTop  = 6;
    i_boardLeft = 32;
    l_whitePieces = "RNBQKBNR\0";
    l_blackPieces = "rnbqkbnr\0";

    for (i = 0; i < 80*25; i = i + 1) {
        putInMemory(0xB000, 0x8000 + i*2,     ' ');
        putInMemory(0xB000, 0x8000 + i*2 + 1, 0x07);
    }

    putAt(2, 30, "    ChessOS  v1.0    \0", 0x4F);
    putAt(4, 22, "By Adam Ibrahim\0", 0x0E);

    for (row = 0; row < 8; row = row + 1) {
        for (col = 0; col < 8; col = col + 1) {
            if (mod(row + col, 2) == 0) {
                i_sqColor = 0x70;
            } else {
                i_sqColor = 0x1F;
            }

            piece = ' ';
            if (row == 0) {
                piece = l_blackPieces[col];
            } else if (row == 1) {
                piece = 'p';
            } else if (row == 6) {
                piece = 'P';
            } else if (row == 7) {
                piece = l_whitePieces[col];
            }

            i_videoOff = ((i_boardTop + row) * 80 + i_boardLeft + col*2) * 2;
            putInMemory(0xB000, 0x8000 + i_videoOff,     ' ');
            putInMemory(0xB000, 0x8000 + i_videoOff + 1, i_sqColor);
            putInMemory(0xB000, 0x8000 + i_videoOff + 2, piece);
            putInMemory(0xB000, 0x8000 + i_videoOff + 3, i_sqColor);
        }
    }
}

/* Wipes the screen by overwriting every cell of video memory with a space */
void clearScreen()
{
    int i;
    for (i = 0; i < 80*25; i = i + 1) {
        putInMemory(0xB000, 0x8000 + i*2,     ' ');
        putInMemory(0xB000, 0x8000 + i*2 + 1, 0x07);
    }
    interrupt(0x10, 0x0200, 0, 0, 0);
}

/*
 * Prints a non-negative integer as decimal using printString.  Builds the
 * digit string in a small buffer using mod and div, then prints from the
 * leftmost filled position.
 * n - the number to print
 */
void printNumber(int n)
{
    char buf[7];
    int i;
    buf[6] = 0;
    if (n == 0) {
        buf[5] = '0';
        printString(&buf[5]);
        return;
    }
    i = 6;
    while (n > 0) {
        i = i - 1;
        buf[i] = '0' + mod(n, 10);
        n = div(n, 10);
    }
    printString(&buf[i]);
}

/*
 * Reports disk usage to the screen.  Walks the disk map to count used and
 * free sectors, then walks the directory to count files.  Used by the
 * shell's diskinfo command.
 */
void diskInfo()
{
    char map[512];
    char dir[512];
    int i;
    int used;
    int free;
    int files;

    readSector(map, 1);
    readSector(dir, 2);

    used = 0;
    free = 0;
    for (i = 0; i < 256; i = i + 1) {
        if (map[i] == 0) {
            free = free + 1;
        } else {
            used = used + 1;
        }
    }

    files = 0;
    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] != 0) {
            files = files + 1;
        }
    }

    printString("Disk: \0");
    printNumber(used);
    printString(" used, \0");
    printNumber(free);
    printString(" free (of 256 sectors)\n\r\0");
    printString("Files: \0");
    printNumber(files);
    printString("\n\r\0");
}

/*
 * Our interrupt handler for INT 0x21 system calls.
 * ax - which system call to make
 * bx - first argument
 * cx - second argument
 * dx - third argument
 */
void handleInterrupt21(int ax, int bx, int cx, int dx)
{
    if (ax == 0) {
        printString(bx);
    } else if (ax == 1) {
        readString(bx);
    } else if (ax == 2) {
        readSector(bx, cx);
    } else if (ax == 3) {
        readFile(bx, cx);
    } else if (ax == 4) {
        executeProgram(bx, cx);
    } else if (ax == 5) {
        terminate();
    } else if (ax == 6) {
        writeSector(bx, cx);
    } else if (ax == 7) {
        deleteFile(bx);
    } else if (ax == 8) {
        writeFile(bx, cx, dx);
    } else if (ax == 9) {
        clearScreen();
    } else if (ax == 10) {
        diskInfo();
    } else {
        printString("Bad int 0x21 call\n\r\0");
    }
}

/* timer interrupts */

/*
 * Helper function for the on-screen clock.
 * col - screen column to write to
 * ch - character to write
 */
void clockCell(int col, char ch)
{
    int off;
    off = col * 2;
    putInMemory(0xB000, 0x8000 + off,     ch);
    putInMemory(0xB000, 0x8000 + off + 1, 0x4F);
}

/*
 * Redraws the HH:MM:SS clock in the top-right of the screen using the
 * current values of i_hh, i_mm, and i_ss.  Uses div and mod to split each
 * value into two digits.
 */
void updateClockDisplay()
{
    int tens;
    int ones;

    tens = div(i_hh, 10);
    ones = mod(i_hh, 10);
    clockCell(70, '0' + tens);
    clockCell(71, '0' + ones);
    clockCell(72, ':');

    tens = div(i_mm, 10);
    ones = mod(i_mm, 10);
    clockCell(73, '0' + tens);
    clockCell(74, '0' + ones);
    clockCell(75, ':');

    tens = div(i_ss, 10);
    ones = mod(i_ss, 10);
    clockCell(76, '0' + tens);
    clockCell(77, '0' + ones);
}

/*
 * Handler for INT 0x1C, the BIOS user-timer hook.  Called by the asm
 * wrapper in kernel.asm.  Fires about 18.2 times per second.  Bumps the
 * tick counters and, once per second, redraws the clock display.
 */
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

/*
 * Prints "Up N seconds (M ticks)" to the screen using the current value
 * of i_ticks.  Called by the shell's uptime command via INT 0x22 ax=0.
 */
void printUptime()
{
    int secs;
    secs = div(i_ticks, 18);
    printString("Up \0");
    printNumber(secs);
    printString(" seconds (\0");
    printNumber(i_ticks);
    printString(" ticks)\n\r\0");
}

/*
 * Prints "Took N ticks (M seconds)" where N is the difference between the
 * current i_ticks and the saved i_savedTicks.  Called by the shell's
 * time <cmd> command via INT 0x22 ax=3 after the inner command runs.
 */
void printElapsed()
{
    int diff;
    int secs;
    diff = i_ticks - i_savedTicks;
    if (diff < 0) {
        diff = 0;
    }
    secs = div(diff, 18);
    printString("Took \0");
    printNumber(diff);
    printString(" ticks (\0");
    printNumber(secs);
    printString(" seconds)\n\r\0");
}

/*
 * Dispatcher for INT 0x22 system calls.  Called by the asm wrapper in
 * kernel.asm with DS = ES = CS so it can read kernel globals and string
 * literals safely.
 * ax - which call to make (0 = printUptime, 1 = sleep, 2 = timeStart,
 *      3 = timeEnd)
 * bx - currently unused
 * cx - tick count to sleep when ax = 1
 * dx - currently unused
 */
void handleInterrupt22(int ax, int bx, int cx, int dx)
{
    if (ax == 0) {
        printUptime();
    } else if (ax == 1) {
        sleepTicks(cx);
    } else if (ax == 2) {
        i_savedTicks = i_ticks;
    } else if (ax == 3) {
        printElapsed();
    } else {
        printString("Bad int 0x22 call\n\r\0");
    }
}
