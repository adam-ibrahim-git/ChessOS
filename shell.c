/*
 * CSC 380 final project
 * Adam Ibrahim
 * Dr. Dave Toth
 *
 * shell.c
 */

void main();
void parseTokens(char* line, char* a, char* b, char* c);
int  isEmpty(char* line);
int  strEq(char* a, char* b);
int  fileSectors(char* name);
int  matchEntry(char* dir, int offset, char* name);
void printBuf(char* buf);

void doType(char* arg);
void doExecute(char* arg);
void doDelete(char* arg);
void doCopy(char* src, char* dst);
void doRename(char* o, char* n);
void doDir();
void doLs();
void doCreate(char* arg);
void doHelp();
void doFs(char* sub, char* arg);

void initProcs();
void doPs();
void doScheduler(char* arg);
void doSimulate();
void doCompareSched();
void runFCFS(int* comp);
void runSJF(int* comp);
void runRR(int* comp);
void runPRI(int* comp);
int  avgTurnaround(int* comp);

void initMem();
void doMemUsage();
void doMemMap();
void doMalloc(char* arg);
void doMemfree(char* arg);
void doCompareMem();
int  allocBlock(int kb, int strategy);
void freeBlock(int idx);
void merge();

void doUptime();
void doSleep(char* arg);
void doTime(char* a, char* b, char* c);
void dispatch(char* a, char* b, char* c);

int  myMod(int a, int b);
int  myDiv(int a, int b);
int  parseInt(char* s);
void printNum(int n);
void printSpaces(int n);

/* process table */
char* pName[8];
int   pState[8];     /* 0=RUN 1=READY 2=BLOCKED 3=DONE */
int   pPriority[8];
int   pArrival[8];
int   pBurst[8];
int   pCount;
int   schedAlg;      /* 0=FCFS 1=SJF 2=RR 3=PRI */
int   rrQ;

/* memory blocks */
int memStart[16];
int memSize[16];
int memOwner[16];   
int memCount;
int memStrategy;    


void main()
{
    char line[80];
    char a[20];
    char b[20];
    char c[20];

    initProcs();
    initMem();
    interrupt(0x21, 0, "ChessOS at your service. Type 'help'.\n\r\0", 0, 0);

    while (1) {
        interrupt(0x21, 0, "ChessOS> \0", 0, 0);
        interrupt(0x21, 1, line, 0, 0);

        if (isEmpty(line) == 1) {
            /* skip */
        } else {
            parseTokens(line, a, b, c);
            dispatch(a, b, c);
        }
    }
}

/*
 * Runs a single command.  Pulled out of main() so the time command can
 * call it on an inner command without code duplication.
 * a - first token (the command name)
 * b - second token (first argument)
 * c - third token (second argument)
 */
void dispatch(char* a, char* b, char* c)
{
    if (strEq(a, "type\0") == 1) {
        doType(b);
    } else if (strEq(a, "execute\0") == 1) {
        doExecute(b);
    } else if (strEq(a, "delete\0") == 1) {
        doDelete(b);
    } else if (strEq(a, "copy\0") == 1) {
        doCopy(b, c);
    } else if (strEq(a, "rename\0") == 1) {
        doRename(b, c);
    } else if (strEq(a, "dir\0") == 1) {
        doDir();
    } else if (strEq(a, "ls\0") == 1) {
        doLs();
    } else if (strEq(a, "fs\0") == 1) {
        doFs(b, c);
    } else if (strEq(a, "create\0") == 1) {
        doCreate(b);
    } else if (strEq(a, "cls\0") == 1) {
        interrupt(0x21, 9, 0, 0, 0);
    } else if (strEq(a, "diskinfo\0") == 1) {
        interrupt(0x21, 10, 0, 0, 0);
    } else if (strEq(a, "help\0") == 1) {
        doHelp();
    } else if (strEq(a, "ps\0") == 1) {
        doPs();
    } else if (strEq(a, "scheduler\0") == 1) {
        doScheduler(b);
    } else if (strEq(a, "simulate\0") == 1) {
        doSimulate();
    } else if (strEq(a, "memUsage\0") == 1 || strEq(a, "memusage\0") == 1) {
        doMemUsage();
    } else if (strEq(a, "memmap\0") == 1) {
        doMemMap();
    } else if (strEq(a, "malloc\0") == 1) {
        doMalloc(b);
    } else if (strEq(a, "memfree\0") == 1) {
        doMemfree(b);
    } else if (strEq(a, "uptime\0") == 1) {
        doUptime();
    } else if (strEq(a, "sleep\0") == 1) {
        doSleep(b);
    } else if (strEq(a, "time\0") == 1) {
        doTime(b, c, "\0");
    } else if (strEq(a, "compare\0") == 1) {
        if (strEq(b, "scheduler\0") == 1) {
            doCompareSched();
        } else if (strEq(b, "memory\0") == 1) {
            doCompareMem();
        } else {
            interrupt(0x21, 0,
                "Usage: compare scheduler | compare memory\n\r\0",
                0, 0);
        }
    } else if (strEq(a, "exit\0") == 1) {
        interrupt(0x21, 0, "You cannot escape me. (or quit a bare-metal OS)\n\r\0", 0, 0);
    } else {
        interrupt(0x21, 0, "Sorry, that's probably too advanced for me. Type help.\n\r\0", 0, 0);
    }
}

/*
 * Calculates a mod b.
 * a - the dividend
 * b - the divisor
 * Returns a mod b.
 */
int myMod(int a, int b)
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
int myDiv(int a, int b)
{
    int q;
    q = 0;
    while (q * b <= a) {
        q = q + 1;
    }
    return q - 1;
}

/*
 * Parses a non-negative decimal integer out of a string.  Stops at the
 * first non-digit character.
 * s - null-terminated string starting with digits
 * Returns the parsed integer (0 if no digits at the start).
 */
int parseInt(char* s)
{
    int n;
    int i;
    n = 0;
    i = 0;
    while (s[i] >= '0' && s[i] <= '9') {
        n = n * 10 + (s[i] - '0');
        i = i + 1;
    }
    return n;
}

/*
 * Prints a non-negative integer to the screen as decimal using INT 0x21
 * ax=0.  Builds the digit string in a small buffer.
 * n - the integer to print
 */
void printNum(int n)
{
    char buf[7];
    int i;

    if (n < 0) {
        interrupt(0x21, 0, "-\0", 0, 0);
        n = -n;
    }
    if (n == 0) {
        interrupt(0x21, 0, "0\0", 0, 0);
        return;
    }
    buf[6] = 0;
    i = 6;
    while (n > 0) {
        i = i - 1;
        buf[i] = '0' + myMod(n, 10);
        n = myDiv(n, 10);
    }
    interrupt(0x21, 0, &buf[i], 0, 0);
}

/*
 * Prints n spaces to the screen.
 * n - the number of spaces to print
 */
void printSpaces(int n)
{
    int i;
    for (i = 0; i < n; i = i + 1) {
        interrupt(0x21, 0, " \0", 0, 0);
    }
}

/*
 * Checks whether a line of input contains nothing but spaces before its
 * terminator.  Used so that hitting Enter on an empty prompt does not
 * trigger the bad-command path.
 * line - the line as returned by readString (ends with 0xa or 0x00)
 * Returns 1 if the line is empty, 0 otherwise.
 */
int isEmpty(char* line)
{
    int i;
    i = 0;
    while (line[i] != 0 && line[i] != 0xa) {
        if (line[i] != ' ') {
            return 0;
        }
        i = i + 1;
    }
    return 1;
}

/*
 * Compares two strings character by character.
 * a - the first string
 * b - the second string
 * Returns 1 if the strings are equal, 0 otherwise.
 */
int strEq(char* a, char* b)
{
    int i;
    i = 0;
    while (a[i] != 0 && b[i] != 0) {
        if (a[i] != b[i]) {
            return 0;
        }
        i = i + 1;
    }
    if (a[i] == 0 && b[i] == 0) {
        return 1;
    }
    return 0;
}

/*
 * Splits a line of input into up to three space-separated tokens.  Stops
 * at the first 0xa or 0x00.  Tokens past the third are dropped.
 * line - the input line
 * a - buffer that will receive the first token
 * b - buffer that will receive the second token
 * c - buffer that will receive the third token (rest of the line)
 */
void parseTokens(char* line, char* a, char* b, char* c)
{
    int i;
    int j;
    i = 0;

    j = 0;
    while (line[i] != ' ' && line[i] != 0xa && line[i] != 0) {
        a[j] = line[i];
        i = i + 1;
        j = j + 1;
    }
    a[j] = 0;
    while (line[i] == ' ') {
        i = i + 1;
    }

    j = 0;
    while (line[i] != ' ' && line[i] != 0xa && line[i] != 0) {
        b[j] = line[i];
        i = i + 1;
        j = j + 1;
    }
    b[j] = 0;
    while (line[i] == ' ') {
        i = i + 1;
    }

    j = 0;
    while (line[i] != 0xa && line[i] != 0) {
        c[j] = line[i];
        i = i + 1;
        j = j + 1;
    }
    c[j] = 0;
}

/* file system commands */

/*
 * Compares a 6-byte name field in dir at the given offset against a search
 * name.  The directory pads short names with 0x00.
 * dir - pointer to a 512-byte directory sector
 * offset - byte offset into dir where the entry's name starts
 * name - null-terminated string to look for
 * Returns 1 if the names match, 0 otherwise.
 */
int matchEntry(char* dir, int offset, char* name)
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
 * Reads the directory and returns how many sectors a file occupies.  Used
 * by copy and rename so they know how many sectors to write to the
 * destination.
 * name - null-terminated filename to look up
 * Returns the sector count, or 0 if the file is not found.
 */
int fileSectors(char* name)
{
    char dir[512];
    int i;
    int j;
    int count;

    interrupt(0x21, 2, dir, 2, 0);
    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] != 0 && matchEntry(dir, i, name) == 1) {
            count = 0;
            for (j = 6; j < 32; j = j + 1) {
                if (dir[i + j] != 0) {
                    count = count + 1;
                }
            }
            return count;
        }
    }
    return 0;
}

/*
 * Prints a buffer to the screen.
 * buf - buffer to print
 */
void printBuf(char* buf)
{
    int i;
    int start;
    char saved;

    start = 0;
    i = 0;
    while (buf[i] != 0 && i < 13312) {
        if (buf[i] == 0xa || buf[i] == 0xd) {
            saved = buf[i];
            buf[i] = 0;
            interrupt(0x21, 0, buf + start, 0, 0);
            buf[i] = saved;
            if (saved == 0xa) {
                interrupt(0x21, 0, "\n\r\0", 0, 0);
            }
            start = i + 1;
        }
        i = i + 1;
    }
    if (start < i) {
        interrupt(0x21, 0, buf + start, 0, 0);
    }
}

/*
 * Reads a file into a local buffer and prints its contents to the screen.
 * arg - filename to print
 */
void doType(char* arg)
{
    char buf[13312];
    int i;

    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: type <file>\n\r\0", 0, 0);
        return;
    }
    for (i = 0; i < 13312; i = i + 1) {
        buf[i] = 0;
    }
    interrupt(0x21, 3, arg, buf, 0);
    printBuf(buf);
}

/*
 * Loads a program file at segment 0x2000 and runs it.  The shell is
 * overwritten in memory; when the program calls terminate the kernel
 * reloads the shell.
 * arg - filename of the program to run
 */
void doExecute(char* arg)
{
    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: execute <file>\n\r\0", 0, 0);
        return;
    }
    interrupt(0x21, 4, arg, 0x2000, 0);
}

/*
 * Removes a file from the disk by sending its name to the kernel via
 * INT 0x21 ax=7.
 * arg - filename to delete
 */
void doDelete(char* arg)
{
    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: delete <file>\n\r\0", 0, 0);
        return;
    }
    interrupt(0x21, 7, arg, 0, 0);
    interrupt(0x21, 0, "Deleted.\n\r\0", 0, 0);
}

/*
 * Duplicates a file under a new name.  Looks up the source file's sector
 * count, reads the source into a buffer, then writes the buffer to the
 * destination.
 * src - source filename
 * dst - destination filename
 */
void doCopy(char* src, char* dst)
{
    char buf[13312];
    int n;

    if (src[0] == 0 || dst[0] == 0) {
        interrupt(0x21, 0, "Usage: copy <src> <dst>\n\r\0", 0, 0);
        return;
    }
    n = fileSectors(src);
    if (n == 0) {
        interrupt(0x21, 0, "Source not found.\n\r\0", 0, 0);
        return;
    }
    interrupt(0x21, 3, src, buf, 0);
    interrupt(0x21, 8, dst, buf, n);
    interrupt(0x21, 0, "Copied.\n\r\0", 0, 0);
}

/*
 * Renames a file by copying it to the new name and deleting the original.
 * The kernel does not have a native rename system call.
 * o - the old (current) filename
 * n - the new filename
 */
void doRename(char* o, char* n)
{
    char buf[13312];
    int s;

    if (o[0] == 0 || n[0] == 0) {
        interrupt(0x21, 0, "Usage: rename <old> <new>\n\r\0", 0, 0);
        return;
    }
    s = fileSectors(o);
    if (s == 0) {
        interrupt(0x21, 0, "File not found.\n\r\0", 0, 0);
        return;
    }
    interrupt(0x21, 3, o, buf, 0);
    interrupt(0x21, 8, n, buf, s);
    interrupt(0x21, 7, o, 0, 0);
    interrupt(0x21, 0, "Renamed.\n\r\0", 0, 0);
}

/*
 * Lists every non-empty file in the directory under a "Files:" header.
 * Each name is printed on its own line indented two spaces.
 */
void doDir()
{
    char dir[512];
    char line[16];
    int i;
    int j;
    int k;

    interrupt(0x21, 2, dir, 2, 0);
    interrupt(0x21, 0, "Files:\n\r\0", 0, 0);
    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] != 0) {
            line[0] = ' ';
            line[1] = ' ';
            k = 2;
            for (j = 0; j < 6; j = j + 1) {
                if (dir[i + j] == 0) {
                    j = 6;
                } else {
                    line[k] = dir[i + j];
                    k = k + 1;
                }
            }
            line[k] = '\n';
            line[k + 1] = '\r';
            line[k + 2] = 0;
            interrupt(0x21, 0, line, 0, 0);
        }
    }
}

/* Lists every non-empty file in the directory. */
void doLs()
{
    char dir[512];
    char line[16];
    int i;
    int j;
    int k;

    interrupt(0x21, 2, dir, 2, 0);
    for (i = 0; i < 512; i = i + 32) {
        if (dir[i] != 0) {
            k = 0;
            for (j = 0; j < 6; j = j + 1) {
                if (dir[i + j] == 0) {
                    j = 6;
                } else {
                    line[k] = dir[i + j];
                    k = k + 1;
                }
            }
            line[k] = '\n';
            line[k + 1] = '\r';
            line[k + 2] = 0;
            interrupt(0x21, 0, line, 0, 0);
        }
    }
}

/*
 * Routes the fs command to one of its sub-commands: cat, ls, or rm.
 * sub - the sub-command name (cat, ls, or rm)
 * arg - filename argument for cat and rm (ignored for ls)
 */
void doFs(char* sub, char* arg)
{
    if (strEq(sub, "cat\0") == 1) {
        doType(arg);
    } else if (strEq(sub, "ls\0") == 1) {
        doLs();
    } else if (strEq(sub, "rm\0") == 1) {
        doDelete(arg);
    } else {
        interrupt(0x21, 0, "Usage: fs cat <file> | fs ls | fs rm <f>\n\r\0", 0, 0);
    }
}

/*
 * Prompts the user for lines of text until they enter an empty line, then
 * writes the accumulated buffer to disk under the given name.  The number
 * of sectors is rounded up so the entire content survives.
 * arg - filename to save to
 */
void doCreate(char* arg)
{
    char buf[13312];
    char line[80];
    int idx;
    int i;
    int sectors;

    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: create <file>\n\r\0", 0, 0);
        return;
    }
    for (i = 0; i < 13312; i = i + 1) {
        buf[i] = 0;
    }
    interrupt(0x21, 0, "Enter text. Empty line ends.\n\r\0", 0, 0);
    idx = 0;
    sectors = -1;
    while (sectors < 0) {
        interrupt(0x21, 1, line, 0, 0);
        if (line[0] == 0xa || line[0] == 0) {
            sectors = 0;
        } else {
            i = 0;
            while (line[i] != 0 && idx < 13311) {
                buf[idx] = line[i];
                idx = idx + 1;
                i = i + 1;
            }
        }
    }
    /* round up to whole sectors */
    i = idx;
    sectors = 0;
    while (i >= 512) {
        sectors = sectors + 1;
        i = i - 512;
    }
    if (i > 0) {
        sectors = sectors + 1;
    }
    if (sectors == 0) {
        sectors = 1;
    }
    interrupt(0x21, 8, arg, buf, sectors);
    interrupt(0x21, 0, "Saved.\n\r\0", 0, 0);
}

/*
 * Loads the help file from disk and prints it to the screen.
 */
void doHelp()
{
    char buf[13312];
    int i;

    for (i = 0; i < 13312; i = i + 1) {
        buf[i] = 0;
    }
    interrupt(0x21, 3, "help\0", buf, 0);
    printBuf(buf);
}

/* scheduler simulation */


void initProcs()
{
    pName[0]     = "shell\0";
    pState[0]    = 0;
    pPriority[0] = 5;
    pArrival[0]  = 0;
    pBurst[0]    = 99;

    pName[1]     = "knight\0";
    pState[1]    = 1;
    pPriority[1] = 3;
    pArrival[1]  = 0;
    pBurst[1]    = 8;

    pName[2]     = "bishop\0";
    pState[2]    = 1;
    pPriority[2] = 3;
    pArrival[2]  = 1;
    pBurst[2]    = 4;

    pName[3]     = "rook\0";
    pState[3]    = 1;
    pPriority[3] = 2;
    pArrival[3]  = 2;
    pBurst[3]    = 9;

    pName[4]     = "queen\0";
    pState[4]    = 1;
    pPriority[4] = 9;
    pArrival[4]  = 3;
    pBurst[4]    = 5;

    pName[5]     = "pawn\0";
    pState[5]    = 2;
    pPriority[5] = 1;
    pArrival[5]  = 4;
    pBurst[5]    = 2;

    pCount   = 6;
    schedAlg = 0;
    rrQ      = 2;
}

/*
 * Prints the process table (PID, state, priority, name) and the average
 * turnaround under the active scheduler.
 */
void doPs()
{
    int i;
    int comp[8];
    int len;
    char* st;

    interrupt(0x21, 0, "PID  STATE    PRIORITY  NAME\n\r\0", 0, 0);
    for (i = 0; i < pCount; i = i + 1) {
        printNum(i + 1);
        if (i + 1 < 10) {
            printSpaces(4);
        } else {
            printSpaces(3);
        }

        if (pState[i] == 0) {
            st = "RUNNING\0";
            len = 7;
        } else if (pState[i] == 1) {
            st = "READY\0";
            len = 5;
        } else if (pState[i] == 2) {
            st = "BLOCKED\0";
            len = 7;
        } else {
            st = "DONE\0";
            len = 4;
        }
        interrupt(0x21, 0, st, 0, 0);
        printSpaces(9 - len);

        printNum(pPriority[i]);
        printSpaces(9);

        interrupt(0x21, 0, pName[i], 0, 0);
        interrupt(0x21, 0, "\n\r\0", 0, 0);
    }

    /* run the active scheduler so we have a number to report */
    for (i = 0; i < 8; i = i + 1) {
        comp[i] = 0;
    }
    if (schedAlg == 0) {
        runFCFS(comp);
    } else if (schedAlg == 1) {
        runSJF(comp);
    } else if (schedAlg == 2) {
        runRR(comp);
    } else {
        runPRI(comp);
    }

    interrupt(0x21, 0, "avg turnaround: \0", 0, 0);
    printNum(avgTurnaround(comp));
    interrupt(0x21, 0, "\n\r\0", 0, 0);
}

/*
 * Switches the active scheduling algorithm.  Accepts FCFS, SJF, RR, or
 * PRI (case insensitive on the first letter).
 * arg - the algorithm name as a string
 */
void doScheduler(char* arg)
{
    if (strEq(arg, "FCFS\0") == 1 || strEq(arg, "fcfs\0") == 1) {
        schedAlg = 0;
        interrupt(0x21, 0, "Switched to first come first served\n\r\0", 0, 0);
    } else if (strEq(arg, "SJF\0") == 1 || strEq(arg, "sjf\0") == 1) {
        schedAlg = 1;
        interrupt(0x21, 0, "Switched to shortest job first\n\r\0", 0, 0);
    } else if (strEq(arg, "RR\0") == 1 || strEq(arg, "rr\0") == 1) {
        schedAlg = 2;
        interrupt(0x21, 0, "Switched to round robin\n\r\0", 0, 0);
    } else if (strEq(arg, "PRI\0") == 1 || strEq(arg, "pri\0") == 1) {
        schedAlg = 3;
        interrupt(0x21, 0, "Switched to priority\n\r\0", 0, 0);
    } else {
        interrupt(0x21, 0, "Usage: scheduler <FCFS|SJF|RR|PRI>\n\r\0", 0, 0);
    }
}

/*
 * Runs the workload through the active scheduler and prints a per-process
 * table (arrival, burst, completion, turnaround) and the average turnaround.
 */
void doSimulate()
{
    int comp[8];
    int i;
    char* alg;

    for (i = 0; i < 8; i = i + 1) {
        comp[i] = 0;
    }
    if (schedAlg == 0) {
        alg = "FCFS\0";
        runFCFS(comp);
    } else if (schedAlg == 1) {
        alg = "SJF\0";
        runSJF(comp);
    } else if (schedAlg == 2) {
        alg = "RR (q=2)\0";
        runRR(comp);
    } else {
        alg = "Priority\0";
        runPRI(comp);
    }

    interrupt(0x21, 0, "Workload through ", 0, 0);
    interrupt(0x21, 0, alg, 0, 0);
    interrupt(0x21, 0, ":\n\rNAME      ARRIVAL  BURST  COMPLETE  TURNAROUND\n\r\0", 0, 0);

    for (i = 1; i < pCount; i = i + 1) {
        interrupt(0x21, 0, pName[i], 0, 0);
        printSpaces(10 - 6);
        printNum(pArrival[i]);
        printSpaces(7);
        printNum(pBurst[i]);
        printSpaces(6);
        printNum(comp[i]);
        printSpaces(8);
        printNum(comp[i] - pArrival[i]);
        interrupt(0x21, 0, "\n\r\0", 0, 0);
    }
    interrupt(0x21, 0, "Avg turnaround: \0", 0, 0);
    printNum(avgTurnaround(comp));
    interrupt(0x21, 0, "\n\r\0", 0, 0);
}

/*
 * Runs every scheduling algorithm against the same workload and prints
 * the average turnaround for each so they can be compared side by side.
 */
void doCompareSched()
{
    int comp[8];
    int i;

    interrupt(0x21, 0,
        "Same workload through every scheduler:\n\r\0", 0, 0);
    interrupt(0x21, 0,
        "Algorithm    Avg Turnaround\n\r\0", 0, 0);

    for (i = 0; i < 8; i = i + 1) {
        comp[i] = 0;
    }
    runFCFS(comp);
    interrupt(0x21, 0, "FCFS         \0", 0, 0);
    printNum(avgTurnaround(comp));
    interrupt(0x21, 0, "\n\r\0", 0, 0);

    for (i = 0; i < 8; i = i + 1) {
        comp[i] = 0;
    }
    runSJF(comp);
    interrupt(0x21, 0, "SJF          \0", 0, 0);
    printNum(avgTurnaround(comp));
    interrupt(0x21, 0, "\n\r\0", 0, 0);

    for (i = 0; i < 8; i = i + 1) {
        comp[i] = 0;
    }
    runRR(comp);
    interrupt(0x21, 0, "RR (q=2)     \0", 0, 0);
    printNum(avgTurnaround(comp));
    interrupt(0x21, 0, "\n\r\0", 0, 0);

    for (i = 0; i < 8; i = i + 1) {
        comp[i] = 0;
    }
    runPRI(comp);
    interrupt(0x21, 0, "Priority     \0", 0, 0);
    printNum(avgTurnaround(comp));
    interrupt(0x21, 0, "\n\r\0", 0, 0);

    interrupt(0x21, 0,
        "\n\rLinux uses CFS, which is closer to weighted RR with priority.\n\r\0",
        0, 0);
    interrupt(0x21, 0,
        "SJF gives the best avg turnaround on this workload.\n\r\0",
        0, 0);
}

/*
 * Calculates the average turnaround time over the batch processes (every
 * process except the shell).  Turnaround = completion - arrival.
 * comp - completion-time array filled in by one of the run functions
 * Returns the average turnaround, or 0 if no process completed.
 */
int avgTurnaround(int* comp)
{
    int sum;
    int i;
    int n;

    sum = 0;
    n = 0;
    for (i = 1; i < pCount; i = i + 1) {
        if (comp[i] > 0) {
            sum = sum + (comp[i] - pArrival[i]);
            n = n + 1;
        }
    }
    if (n == 0) {
        return 0;
    }
    return myDiv(sum, n);
}

/*
 * Simulates First Come First Served scheduling.  Picks the unfinished
 * process with the earliest arrival time, runs it to completion, and
 * repeats until everyone is done.
 * l_comp - output array; l_comp[i] gets the completion time of process i
 */
void runFCFS(int* l_comp)
{
    int b_done[8];
    int i;
    int t;
    int i_chosen;
    int i_earliest;
    int i_finished;
    int i_total;

    for (i = 0; i < 8; i = i + 1) {
        b_done[i] = 0;
    }
    t = 0;
    i_finished = 0;
    i_total = pCount - 1;

    while (i_finished < i_total) {
        i_chosen = -1;
        i_earliest = 9999;
        for (i = 1; i < pCount; i = i + 1) {
            if (b_done[i] == 0 && pArrival[i] < i_earliest) {
                i_earliest = pArrival[i];
                i_chosen = i;
            }
        }
        if (t < pArrival[i_chosen]) {
            t = pArrival[i_chosen];
        }
        t = t + pBurst[i_chosen];
        l_comp[i_chosen] = t;
        b_done[i_chosen] = 1;
        i_finished = i_finished + 1;
    }
}

/*
 * Simulates Shortest Job First scheduling (non-preemptive).  At each
 * step, of the processes that have already arrived, picks the one with
 * the smallest burst and runs it to completion.
 * l_comp - output array of completion times
 */
void runSJF(int* l_comp)
{
    int b_done[8];
    int i;
    int t;
    int i_chosen;
    int i_best;
    int i_finished;
    int i_total;

    for (i = 0; i < 8; i = i + 1) {
        b_done[i] = 0;
    }
    t = 0;
    i_finished = 0;
    i_total = pCount - 1;

    while (i_finished < i_total) {
        i_chosen = -1;
        i_best = 9999;
        for (i = 1; i < pCount; i = i + 1) {
            if (b_done[i] == 0 && pArrival[i] <= t && pBurst[i] < i_best) {
                i_best = pBurst[i];
                i_chosen = i;
            }
        }
        if (i_chosen < 0) {
            /* nobody is available, idle one tick */
            t = t + 1;
        } else {
            t = t + pBurst[i_chosen];
            l_comp[i_chosen] = t;
            b_done[i_chosen] = 1;
            i_finished = i_finished + 1;
        }
    }
}

/*
 * Simulates Round Robin scheduling with a fixed quantum.  Maintains a
 * ready queue.  Each turn, dequeues a process, runs it for rrQ ticks
 * (or its remaining time if smaller), then either appends it to the
 * back if it has work left or records its completion.
 * l_comp - output array of completion times
 */
void runRR(int* l_comp)
{
    int l_rem[8];
    int l_queue[64];
    int i_head;
    int i_tail;
    int b_added[8];
    int i;
    int t;
    int p;
    int i_slice;
    int i_finished;
    int i_total;

    i_total = pCount - 1;
    for (i = 0; i < 8; i = i + 1) {
        l_rem[i]   = pBurst[i];
        b_added[i] = 0;
    }
    i_head = 0;
    i_tail = 0;
    t = 0;
    i_finished = 0;

    while (i_finished < i_total) {
        /* admit any newly arrived processes */
        for (i = 1; i < pCount; i = i + 1) {
            if (b_added[i] == 0 && pArrival[i] <= t) {
                l_queue[i_tail] = i;
                i_tail = i_tail + 1;
                b_added[i] = 1;
            }
        }

        if (i_head == i_tail) {
            t = t + 1;
        } else {
            p = l_queue[i_head];
            i_head = i_head + 1;
            if (l_rem[p] > rrQ) {
                i_slice = rrQ;
            } else {
                i_slice = l_rem[p];
            }
            t = t + i_slice;
            l_rem[p] = l_rem[p] - i_slice;

            for (i = 1; i < pCount; i = i + 1) {
                if (b_added[i] == 0 && pArrival[i] <= t) {
                    l_queue[i_tail] = i;
                    i_tail = i_tail + 1;
                    b_added[i] = 1;
                }
            }

            if (l_rem[p] > 0) {
                l_queue[i_tail] = p;
                i_tail = i_tail + 1;
            } else {
                l_comp[p] = t;
                i_finished = i_finished + 1;
            }
        }
    }
}

/*
 * Simulates Priority scheduling. Of the processes that
 * have arrived, picks the one with the highest priority value and runs
 * it to completion.
 * l_comp - output array of completion times
 */
void runPRI(int* l_comp)
{
    int b_done[8];
    int i;
    int t;
    int i_chosen;
    int i_best;
    int i_finished;
    int i_total;

    for (i = 0; i < 8; i = i + 1) {
        b_done[i] = 0;
    }
    t = 0;
    i_finished = 0;
    i_total = pCount - 1;

    while (i_finished < i_total) {
        i_chosen = -1;
        i_best = -1;
        for (i = 1; i < pCount; i = i + 1) {
            if (b_done[i] == 0 && pArrival[i] <= t && pPriority[i] > i_best) {
                i_best = pPriority[i];
                i_chosen = i;
            }
        }
        if (i_chosen < 0) {
            t = t + 1;
        } else {
            t = t + pBurst[i_chosen];
            l_comp[i_chosen] = t;
            b_done[i_chosen] = 1;
            i_finished = i_finished + 1;
        }
    }
}

/* memory simulation
 *
 * 4096 KB heap kept as a sorted list of contiguous blocks.  Each block has
 * a start, size, and owner pid (-1 = free).  malloc finds a fit and splits
 * if needed.  free marks the block free and merges adjacent free blocks.
 */

/*
 * Sets up the simulated heap with eight pre-allocated blocks.
 */
void initMem()
{
    memStart[0] = 0;     memSize[0] = 256;  memOwner[0] = 1;   /* shell  */
    memStart[1] = 256;   memSize[1] = 64;   memOwner[1] = 2;   /* knight */
    memStart[2] = 320;   memSize[2] = 64;   memOwner[2] = -1;  /* gap    */
    memStart[3] = 384;   memSize[3] = 128;  memOwner[3] = 3;   /* bishop */
    memStart[4] = 512;   memSize[4] = 256;  memOwner[4] = 4;   /* rook   */
    memStart[5] = 768;   memSize[5] = 128;  memOwner[5] = -1;  /* gap    */
    memStart[6] = 896;   memSize[6] = 200;  memOwner[6] = 5;   /* queen  */
    memStart[7] = 1096;  memSize[7] = 3000; memOwner[7] = -1;  /* tail   */
    memCount = 8;
    memStrategy = 0;
}

/*
 * Prints a one-line summary of memory usage (total, used, free,
 * fragmentation percent)
 * Used blocks render as '=', big free runs as '_', small free as '-'.
 */
void doMemUsage()
{
    int i;
    int j;
    int used;
    int free;
    int largest;
    int frag;
    int barLen;
    int cells;
    int produced;
    char ch[2];

    used = 0;
    free = 0;
    largest = 0;
    for (i = 0; i < memCount; i = i + 1) {
        if (memOwner[i] == -1) {
            free = free + memSize[i];
            if (memSize[i] > largest) {
                largest = memSize[i];
            }
        } else {
            used = used + memSize[i];
        }
    }
    if (free == 0) {
        frag = 0;
    } else {
        frag = 100 - myDiv(largest * 100, free);
    }

    interrupt(0x21, 0, "Total: \0", 0, 0);
    printNum(4096);
    interrupt(0x21, 0, " KB | Used: \0", 0, 0);
    printNum(used);
    interrupt(0x21, 0, " KB | Free: \0", 0, 0);
    printNum(free);
    interrupt(0x21, 0, " KB\n\r\0", 0, 0);
    interrupt(0x21, 0, "Fragmentation: \0", 0, 0);
    printNum(frag);
    interrupt(0x21, 0, "%\n\r\0", 0, 0);

    /* visual bar: '=' for used, '_' for big free run, '-' for small free */
    barLen = 36;
    interrupt(0x21, 0, "[\0", 0, 0);
    produced = 0;
    ch[1] = 0;
    for (i = 0; i < memCount; i = i + 1) {
        cells = myDiv(memSize[i] * barLen, 4096);
        if (cells == 0) {
            cells = 1;
        }
        for (j = 0; j < cells && produced < barLen; j = j + 1) {
            if (memOwner[i] != -1) {
                ch[0] = '=';
            } else if (memSize[i] >= 1024) {
                ch[0] = '_';
            } else {
                ch[0] = '-';
            }
            interrupt(0x21, 0, ch, 0, 0);
            produced = produced + 1;
        }
    }
    while (produced < barLen) {
        interrupt(0x21, 0, "_\0", 0, 0);
        produced = produced + 1;
    }
    interrupt(0x21, 0, "]\n\r\0", 0, 0);
}

/*
 * Prints every memory block: index, start address, size, and owner.
 * Owner is either a process name from the process table or "free".
 */
void doMemMap()
{
    int i;
    int p;
    char* who;

    interrupt(0x21, 0, "BLOCK  START   SIZE   OWNER\n\r\0", 0, 0);
    for (i = 0; i < memCount; i = i + 1) {
        printNum(i);
        printSpaces(7);
        printNum(memStart[i]);
        printSpaces(8);
        printNum(memSize[i]);
        printSpaces(7);
        if (memOwner[i] == -1) {
            interrupt(0x21, 0, "free\0", 0, 0);
        } else {
            p = memOwner[i];
            if (p >= 1 && p <= pCount) {
                who = pName[p - 1];
            } else {
                who = "user\0";
            }
            interrupt(0x21, 0, who, 0, 0);
        }
        interrupt(0x21, 0, "\n\r\0", 0, 0);
    }
}

/*
 * Allocates a new block from the simulated heap using the given strategy.
 * Splits a free block if it is bigger than the requested size.
 * kb - size to allocate in KB
 * strategy - 0 = first-fit, 1 = best-fit, 2 = worst-fit
 * Returns the index of the new block, or -1 if allocation failed.
 */
int allocBlock(int kb, int strategy)
{
    int i;
    int chosen;
    int chosenSize;
    int j;

    chosen = -1;
    if (strategy == 1) {
        chosenSize = 9999;
    } else {
        chosenSize = -1;
    }

    for (i = 0; i < memCount; i = i + 1) {
        if (memOwner[i] == -1 && memSize[i] >= kb) {
            if (strategy == 0) {
                chosen = i;
                i = memCount;
            } else if (strategy == 1) {
                if (memSize[i] < chosenSize) {
                    chosen = i;
                    chosenSize = memSize[i];
                }
            } else {
                if (memSize[i] > chosenSize) {
                    chosen = i;
                    chosenSize = memSize[i];
                }
            }
        }
    }
    if (chosen < 0) {
        return -1;
    }

    if (memSize[chosen] == kb) {
        memOwner[chosen] = 99;
        return chosen;
    }
    if (memCount >= 16) {
        return -1;
    }
    /* shift later blocks right one slot */
    for (j = memCount; j > chosen; j = j - 1) {
        memStart[j] = memStart[j - 1];
        memSize[j]  = memSize[j - 1];
        memOwner[j] = memOwner[j - 1];
    }
    memCount = memCount + 1;
    memOwner[chosen] = 99;
    memSize[chosen]  = kb;
    memStart[chosen + 1] = memStart[chosen] + kb;
    memSize[chosen + 1]  = memSize[chosen + 1] - kb;
    return chosen;
}

/*
 * Marks a block free and merges it with adjacent free blocks.
 * idx - index of the block to free
 */
void freeBlock(int idx)
{
    if (idx < 0 || idx >= memCount) {
        return;
    }
    if (memOwner[idx] == -1) {
        return;
    }
    memOwner[idx] = -1;
    merge();
}

/*
 * Walks the block list and merges any two adjacent free blocks into one.
 * Without this, alloc/free cycles would accumulate fragmentation forever.
 */
void merge()
{
    int i;
    int j;

    i = 0;
    while (i < memCount - 1) {
        if (memOwner[i] == -1 && memOwner[i + 1] == -1) {
            memSize[i] = memSize[i] + memSize[i + 1];
            for (j = i + 1; j < memCount - 1; j = j + 1) {
                memStart[j] = memStart[j + 1];
                memSize[j]  = memSize[j + 1];
                memOwner[j] = memOwner[j + 1];
            }
            memCount = memCount - 1;
        } else {
            i = i + 1;
        }
    }
}

/*
 * Parses a size out of arg and allocates that many KB using the active
 * strategy.  Prints the resulting block index and offset on success.
 * arg - size to allocate as a string of digits
 */
void doMalloc(char* arg)
{
    int sz;
    int idx;

    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: malloc <kb>\n\r\0", 0, 0);
        return;
    }
    sz = parseInt(arg);
    if (sz <= 0) {
        interrupt(0x21, 0, "Size must be positive.\n\r\0", 0, 0);
        return;
    }
    idx = allocBlock(sz, memStrategy);
    if (idx < 0) {
        interrupt(0x21, 0, "Allocation failed.\n\r\0", 0, 0);
    } else {
        interrupt(0x21, 0, "Allocated block #\0", 0, 0);
        printNum(idx);
        interrupt(0x21, 0, " (\0", 0, 0);
        printNum(sz);
        interrupt(0x21, 0, " KB at offset \0", 0, 0);
        printNum(memStart[idx]);
        interrupt(0x21, 0, ")\n\r\0", 0, 0);
    }
}

/*
 * Parses a block index out of arg and frees that block.
 * arg - block index as a string of digits
 */
void doMemfree(char* arg)
{
    int idx;

    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: memfree <block#>\n\r\0", 0, 0);
        return;
    }
    idx = parseInt(arg);
    freeBlock(idx);
    interrupt(0x21, 0, "Freed.\n\r\0", 0, 0);
}

/*
 * Saves the current memory state, runs the same alloc/free pattern under
 * each of the three strategies, prints the resulting fragmentation, and
 * restores the saved state.
 */
void doCompareMem()
{
    int saveStart[16];
    int saveSize[16];
    int saveOwner[16];
    int saveCount;
    int strategy;
    int i;
    int j;
    int used;
    int free;
    int largest;
    int frag;
    char* names[3];

    names[0] = "First-fit\0";
    names[1] = "Best-fit\0";
    names[2] = "Worst-fit\0";

    saveCount = memCount;
    for (i = 0; i < memCount; i = i + 1) {
        saveStart[i] = memStart[i];
        saveSize[i]  = memSize[i];
        saveOwner[i] = memOwner[i];
    }

    interrupt(0x21, 0, "Same alloc/free pattern, three strategies:\n\r\0", 0, 0);
    interrupt(0x21, 0, "Strategy     Fragmentation\n\r\0", 0, 0);

    for (strategy = 0; strategy < 3; strategy = strategy + 1) {
        memCount = saveCount;
        for (i = 0; i < saveCount; i = i + 1) {
            memStart[i] = saveStart[i];
            memSize[i]  = saveSize[i];
            memOwner[i] = saveOwner[i];
        }

        allocBlock(100, strategy);
        allocBlock(40,  strategy);
        allocBlock(300, strategy);
        for (j = 0; j < memCount; j = j + 1) {
            if (memOwner[j] == 99 && memSize[j] == 40) {
                freeBlock(j);
                j = memCount;
            }
        }
        allocBlock(60, strategy);

        used = 0;
        free = 0;
        largest = 0;
        for (i = 0; i < memCount; i = i + 1) {
            if (memOwner[i] == -1) {
                free = free + memSize[i];
                if (memSize[i] > largest) {
                    largest = memSize[i];
                }
            } else {
                used = used + memSize[i];
            }
        }
        if (free == 0) {
            frag = 0;
        } else {
            frag = 100 - myDiv(largest * 100, free);
        }

        interrupt(0x21, 0, names[strategy], 0, 0);
        printSpaces(13 - 9);
        printNum(frag);
        interrupt(0x21, 0, "%\n\r\0", 0, 0);
    }

    /* restore real state */
    memCount = saveCount;
    for (i = 0; i < saveCount; i = i + 1) {
        memStart[i] = saveStart[i];
        memSize[i]  = saveSize[i];
        memOwner[i] = saveOwner[i];
    }

    interrupt(0x21, 0,
        "\n\rLinux malloc (glibc) uses size-binned free lists, not a flat\n\r\0", 0, 0);
    interrupt(0x21, 0,
        "list like this; best-fit ideas without the slow scan.\n\r\0", 0, 0);
}

/* timer commands */

/*
 * Asks the kernel to print the system uptime via INT 0x22 ax=0.
 */
void doUptime()
{
    interrupt(0x22, 0, 0, 0, 0);
}

/*
 * Sleeps for the given number of seconds by sending the kernel a tick
 * count via INT 0x22 ax=1.
 * arg - seconds to sleep, as a string of digits
 */
void doSleep(char* arg)
{
    int n;

    if (arg[0] == 0) {
        interrupt(0x21, 0, "Usage: sleep <seconds>\n\r\0", 0, 0);
        return;
    }
    n = parseInt(arg);
    if (n <= 0) {
        interrupt(0x21, 0, "Seconds must be positive.\n\r\0", 0, 0);
        return;
    }
    /* convert seconds to ticks (~18 ticks/sec) */
    interrupt(0x22, 1, 0, n * 18, 0);
}

/*
 * Runs an inner shell command and prints how long it took.  Snapshots the
 * tick count before, dispatches the inner command, then asks the kernel
 * to compute and print the elapsed time.
 * a - first token of the inner command
 * b - second token of the inner command
 * c - third token of the inner command
 */
void doTime(char* a, char* b, char* c)
{
    if (a[0] == 0) {
        interrupt(0x21, 0, "Usage: time <command>\n\r\0", 0, 0);
        return;
    }
    /* timeStart */
    interrupt(0x22, 2, 0, 0, 0);
    /* run the inner command */
    dispatch(a, b, c);
    /* timeEnd: kernel prints elapsed */
    interrupt(0x22, 3, 0, 0, 0);
}
