/*
 * CSC 380 final project
 * Adam Ibrahim
 * Dr. Dave Toth
 *
 * chess.c, adapted from my CSC 170 final project.
 *
 * Loaded as a regular file on the floppy.  Type "execute chess" at the
 * shell prompt to start the game.  Calls INT 0x21 ax=5 to return to the
 * shell when finished.
 *
 * Controls:
 *   W A S D    move the cursor (no mouse on bare metal)
 *   Space      select your piece, then move it to the highlighted square
 *   Q          quit back to the shell
 *
 * Pieces (uppercase = white = player 0, lowercase = black = player 1):
 *   P R N B Q K       pawn, rook, knight, bishop, queen, king
 *   .                 empty square
 *
 * Board indexing:  l_board[row*8 + col]
 *   row 0 = top of screen = black home rank
 *   row 7 = bottom of screen = white home rank
 *   col 0 = leftmost (a-file)
 */

void main();
void initBoard(char* l_board);
void drawBoard(char* l_board, int i_curRow, int i_curCol, int i_selRow, int i_selCol, int i_player, char* s_status);
void putCharAt(int i_row, int i_col, char ch, int i_attr);
void putStrAt(int i_row, int i_col, char* s, int i_attr);

int  isOwnPiece(char ch, int i_player);
int  isOpponent(char ch, int i_player);
int  myAbs(int x);

int  isLegalMove(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC, int i_player);
int  pawnLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC, int i_player);
int  rookLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC);
int  bishopLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC);
int  knightLegal(int i_fromR, int i_fromC, int i_toR, int i_toC);
int  queenLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC);
int  kingLegal(int i_fromR, int i_fromC, int i_toR, int i_toC);
int  pathClear(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC);
int  isInCheck(char* l_board, int i_player);
int  tryMove(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC, int i_player);

/*
 * Entry point of the chess game.  Sets up the board in starting position,
 * draws it, and loops reading keys.  W/A/S/D move the cursor.  Space (or
 * Enter) selects a piece, then moves it to the highlighted square.  Q
 * quits back to the shell.
 */
void main()
{
    char l_board[64];
    int  i_curRow;
    int  i_curCol;
    int  i_selRow;
    int  i_selCol;
    int  i_player;
    int  i_key;
    int  b_gameOver;
    int  b_moved;
    int  b_check;

    initBoard(l_board);
    /* start cursor on white's e-pawn (e2) */
    i_curRow = 6;
    i_curCol = 4;
    /* -1 = nothing selected yet */
    i_selRow = -1;
    i_selCol = -1;
    /* 0 = white moves first */
    i_player = 0;
    b_gameOver = 0;

    drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");

    while (b_gameOver == 0) {
        /* read a key from the keyboard via INT 0x16 */
        i_key = interrupt(0x16, 0, 0, 0, 0);

        if (i_key == 'w' || i_key == 'W') {
            if (i_curRow > 0) {
                i_curRow = i_curRow - 1;
            }
            drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");
        } else if (i_key == 's' || i_key == 'S') {
            if (i_curRow < 7) {
                i_curRow = i_curRow + 1;
            }
            drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");
        } else if (i_key == 'a' || i_key == 'A') {
            if (i_curCol > 0) {
                i_curCol = i_curCol - 1;
            }
            drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");
        } else if (i_key == 'd' || i_key == 'D') {
            if (i_curCol < 7) {
                i_curCol = i_curCol + 1;
            }
            drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");
        } else if (i_key == 'q' || i_key == 'Q') {
            b_gameOver = 1;
        } else if (i_key == ' ' || i_key == 0x0d) {
            /* space or enter: select / move / deselect */
            if (i_selRow == -1) {
                /* nothing selected yet, try to pick up the piece under the cursor */
                if (isOwnPiece(l_board[i_curRow*8 + i_curCol], i_player) == 1) {
                    i_selRow = i_curRow;
                    i_selCol = i_curCol;
                    drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, "Piece selected.\0");
                } else {
                    drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, "Not your piece.\0");
                }
            } else if (i_curRow == i_selRow && i_curCol == i_selCol) {
                /* clicked the same square, deselect */
                i_selRow = -1;
                i_selCol = -1;
                drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");
            } else {
                /* try to move from selected square to current cursor */
                b_moved = tryMove(l_board, i_selRow, i_selCol, i_curRow, i_curCol, i_player);
                i_selRow = -1;
                i_selCol = -1;
                if (b_moved == 1) {
                    /* successful move, switch player */
                    i_player = 1 - i_player;
                    /* warn if the new player is now in check */
                    b_check = isInCheck(l_board, i_player);
                    if (b_check == 1) {
                        drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, "CHECK!\0");
                    } else {
                        drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, " \0");
                    }
                } else {
                    drawBoard(l_board, i_curRow, i_curCol, i_selRow, i_selCol, i_player, "Illegal move!\0");
                }
            }
        }
    }

    /* clear screen and return to the shell.  We call ax=4 with our own
     * "shell" literal instead of ax=5 (terminate) because terminate uses
     * a kernel-segment string and DS is still chess's segment here, so
     * the kernel would read garbage and crash. */
    interrupt(0x21, 9, 0, 0, 0);
    interrupt(0x21, 4, "shell\0", 0x2000, 0);
}

/*
 * Sets up the chess board in starting position.  Empties every square,
 * then places the black home rank, black pawns, white pawns, and white
 * home rank.
 * l_board - 64-byte buffer that will receive the starting board
 */
void initBoard(char* l_board)
{
    int i;

    for (i = 0; i < 64; i = i + 1) {
        l_board[i] = '.';
    }

    /* black home rank (row 0) */
    l_board[0] = 'r';
    l_board[1] = 'n';
    l_board[2] = 'b';
    l_board[3] = 'q';
    l_board[4] = 'k';
    l_board[5] = 'b';
    l_board[6] = 'n';
    l_board[7] = 'r';

    /* black pawns (row 1) */
    for (i = 8; i < 16; i = i + 1) {
        l_board[i] = 'p';
    }

    /* white pawns (row 6) */
    for (i = 48; i < 56; i = i + 1) {
        l_board[i] = 'P';
    }

    /* white home rank (row 7) */
    l_board[56] = 'R';
    l_board[57] = 'N';
    l_board[58] = 'B';
    l_board[59] = 'Q';
    l_board[60] = 'K';
    l_board[61] = 'B';
    l_board[62] = 'N';
    l_board[63] = 'R';
}

/*
 * Writes one character at (row, col) with the given color attribute using
 * two BIOS calls: one to set the cursor and one to write the character.
 * i_row - screen row (0 to 24)
 * i_col - screen column (0 to 79)
 * ch - character to write
 * i_attr - color attribute byte (high nibble bg, low nibble fg)
 */
void putCharAt(int i_row, int i_col, char ch, int i_attr)
{
    /* INT 0x10 AH=02h: set cursor.  BH=page=0, DH=row, DL=col */
    interrupt(0x10, 0x0200, 0, 0, i_row*256 + i_col);
    /* INT 0x10 AH=09h: write char with attribute.  AL=char, BL=attr, CX=count */
    interrupt(0x10, 0x0900 + ch, i_attr, 1, 0);
}

/*
 * Writes a null-terminated string starting at (row, col) by calling
 * putCharAt for each character.
 * i_row - screen row to start at
 * i_col - screen column to start at
 * s - null-terminated string to write
 * i_attr - color attribute byte applied to every character
 */
void putStrAt(int i_row, int i_col, char* s, int i_attr)
{
    int i;
    int i_writeCol;

    i = 0;
    i_writeCol = i_col;
    while (s[i] != 0) {
        putCharAt(i_row, i_writeCol, s[i], i_attr);
        i_writeCol = i_writeCol + 1;
        i = i + 1;
    }
}

/*
 * Clears the screen and redraws the whole game: the title, the rank/file
 * labels, every square (with cursor and selection highlighted), and the
 * status line.  Called after every keystroke.
 * l_board - the current 64-square board
 * i_curRow - cursor row (0 to 7)
 * i_curCol - cursor column (0 to 7)
 * i_selRow - selected row (-1 if nothing is selected)
 * i_selCol - selected column (-1 if nothing is selected)
 * i_player - whose turn it is (0 = white, 1 = black)
 * s_status - one-line status message printed under the board
 */
void drawBoard(char* l_board, int i_curRow, int i_curCol, int i_selRow, int i_selCol, int i_player, char* s_status)
{
    int  row;
    int  col;
    int  i_attr;
    int  i_screenRow;
    int  i_screenCol;
    char piece;
    char l_label[2];
    char* s_turn;

    /* clear screen via the kernel.  We don't use INT 0x10 ax=0x0003 (set
     * video mode) because curses-mode QEMU has trouble with mid-run mode
     * changes and ends up showing a black screen. */
    interrupt(0x21, 9, 0, 0, 0);

    if (i_player == 0) {
        s_turn = "ChessOS Chess  -  Whites turn\0";
    } else {
        s_turn = "ChessOS Chess  -  Blacks turn\0";
    }
    putStrAt(1, 24, s_turn, 0x0F);

    /* file labels (a-h) above the board */
    putStrAt(3, 28, "a  b  c  d  e  f  g  h\0", 0x07);

    l_label[1] = 0;
    for (row = 0; row < 8; row = row + 1) {
        /* rank 8 at top, rank 1 at bottom */
        l_label[0] = '0' + (8 - row);
        i_screenRow = 4 + row;

        /* rank labels on either side */
        putStrAt(i_screenRow, 25, l_label, 0x07);
        putStrAt(i_screenRow, 53, l_label, 0x07);

        for (col = 0; col < 8; col = col + 1) {
            piece = l_board[row*8 + col];

            /* pick the square's color attribute */
            if (row == i_curRow && col == i_curCol) {
                /* cursor: green bg, black fg */
                i_attr = 0x20;
            } else if (row == i_selRow && col == i_selCol) {
                /* selected: red bg, black fg */
                i_attr = 0x40;
            } else if (((row + col) & 1) == 0) {
                /* light square: gray bg */
                i_attr = 0x70;
            } else {
                /* dark square: brown bg */
                i_attr = 0x60;
            }

            i_screenCol = 28 + col*3;
            putCharAt(i_screenRow, i_screenCol,     ' ',   i_attr);
            putCharAt(i_screenRow, i_screenCol + 1, piece, i_attr);
            putCharAt(i_screenRow, i_screenCol + 2, ' ',   i_attr);
        }
    }

    putStrAt(12, 28, "a  b  c  d  e  f  g  h\0", 0x07);

    putStrAt(14, 24, s_status, 0x0E);
    putStrAt(16, 19, "WASD: move    Space: select/move    Q: quit\0", 0x07);
}

/*
 * Returns the absolute value of x.
 * x - any integer
 * Returns x if x is non-negative, -x otherwise.
 */
int myAbs(int x)
{
    if (x < 0) {
        return -x;
    }
    return x;
}

/*
 * Checks whether a piece belongs to the given player.  White pieces are
 * uppercase letters, black pieces are lowercase letters, '.' is empty.
 * ch - the piece character to check
 * i_player - which player to check (0 = white, 1 = black)
 * Returns 1 if ch is i_player's piece, 0 otherwise.
 */
int isOwnPiece(char ch, int i_player)
{
    if (ch == '.') {
        return 0;
    }
    if (i_player == 0) {
        /* white = uppercase letters */
        if (ch >= 'A' && ch <= 'Z') {
            return 1;
        }
        return 0;
    } else {
        /* black = lowercase letters */
        if (ch >= 'a' && ch <= 'z') {
            return 1;
        }
        return 0;
    }
}

/*
 * Checks whether a piece belongs to the opponent of the given player.
 * ch - the piece character to check
 * i_player - which player is "us" (the opponent is the other one)
 * Returns 1 if ch is the opponent's piece, 0 otherwise.
 */
int isOpponent(char ch, int i_player)
{
    if (ch == '.') {
        return 0;
    }
    if (isOwnPiece(ch, i_player) == 1) {
        return 0;
    }
    return 1;
}

/*
 * Checks that no pieces stand between (fromR, fromC) and (toR, toC),
 * exclusive of the endpoints.  Used by sliding pieces (rook, bishop,
 * queen) to make sure they don't jump over other pieces.
 * l_board - the current board
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * Returns 1 if the path is clear, 0 if a piece blocks it.
 */
int pathClear(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC)
{
    int dr;
    int dc;
    int r;
    int c;

    if (i_toR > i_fromR) {
        dr = 1;
    } else if (i_toR < i_fromR) {
        dr = -1;
    } else {
        dr = 0;
    }

    if (i_toC > i_fromC) {
        dc = 1;
    } else if (i_toC < i_fromC) {
        dc = -1;
    } else {
        dc = 0;
    }

    r = i_fromR + dr;
    c = i_fromC + dc;
    while (r != i_toR || c != i_toC) {
        if (l_board[r*8 + c] != '.') {
            return 0;
        }
        r = r + dr;
        c = c + dc;
    }
    return 1;
}

/*
 * Checks whether a pawn move is legal.  Pawns move forward one square
 * into an empty square, or two squares from their starting rank, or
 * diagonally one square to capture an opponent.
 * l_board - the current board
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * i_player - the pawn's owner (0 = white moves up, 1 = black moves down)
 * Returns 1 if the pawn move is legal, 0 otherwise.
 */
int pawnLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC, int i_player)
{
    int  i_dir;
    int  i_startRow;
    char target;

    if (i_player == 0) {
        /* white moves up the screen = decreasing row */
        i_dir = -1;
        i_startRow = 6;
    } else {
        i_dir = 1;
        i_startRow = 1;
    }

    target = l_board[i_toR*8 + i_toC];

    /* forward one square */
    if (i_toC == i_fromC && i_toR == i_fromR + i_dir && target == '.') {
        return 1;
    }

    /* forward two squares from starting rank, both squares must be empty */
    if (i_toC == i_fromC && i_toR == i_fromR + 2*i_dir && i_fromR == i_startRow) {
        if (target == '.' && l_board[(i_fromR + i_dir)*8 + i_fromC] == '.') {
            return 1;
        }
    }

    /* diagonal capture */
    if (myAbs(i_toC - i_fromC) == 1 && i_toR == i_fromR + i_dir) {
        if (isOpponent(target, i_player) == 1) {
            return 1;
        }
    }

    return 0;
}

/*
 * Checks whether a rook move is legal.  Rooks move in a straight line
 * along a row or column with no pieces in the way.
 * l_board - the current board
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * Returns 1 if the rook move is legal, 0 otherwise.
 */
int rookLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC)
{
    if (i_fromR != i_toR && i_fromC != i_toC) {
        return 0;
    }
    return pathClear(l_board, i_fromR, i_fromC, i_toR, i_toC);
}

/*
 * Checks whether a bishop move is legal.  Bishops move diagonally with
 * no pieces in the way (same number of rows and columns of distance).
 * l_board - the current board
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * Returns 1 if the bishop move is legal, 0 otherwise.
 */
int bishopLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC)
{
    if (myAbs(i_toR - i_fromR) != myAbs(i_toC - i_fromC)) {
        return 0;
    }
    return pathClear(l_board, i_fromR, i_fromC, i_toR, i_toC);
}

/*
 * Checks whether a knight move is legal.  Knights move in an L-shape:
 * either 2 rows and 1 column, or 1 row and 2 columns.  Knights can jump
 * over other pieces, so no path-clear check is needed.
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * Returns 1 if the knight move is legal, 0 otherwise.
 */
int knightLegal(int i_fromR, int i_fromC, int i_toR, int i_toC)
{
    int dr;
    int dc;

    dr = myAbs(i_toR - i_fromR);
    dc = myAbs(i_toC - i_fromC);
    if (dr == 2 && dc == 1) {
        return 1;
    }
    if (dr == 1 && dc == 2) {
        return 1;
    }
    return 0;
}

/*
 * Checks whether a queen move is legal.  Queens move like a rook or like
 * a bishop, so we delegate to those functions.
 * l_board - the current board
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * Returns 1 if the queen move is legal, 0 otherwise.
 */
int queenLegal(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC)
{
    if (i_fromR == i_toR || i_fromC == i_toC) {
        return rookLegal(l_board, i_fromR, i_fromC, i_toR, i_toC);
    }
    if (myAbs(i_toR - i_fromR) == myAbs(i_toC - i_fromC)) {
        return bishopLegal(l_board, i_fromR, i_fromC, i_toR, i_toC);
    }
    return 0;
}

/*
 * Checks whether a king move is legal.  Kings move one square in any
 * direction (no castling implemented).
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * Returns 1 if the king move is legal, 0 otherwise.
 */
int kingLegal(int i_fromR, int i_fromC, int i_toR, int i_toC)
{
    if (myAbs(i_toR - i_fromR) <= 1 && myAbs(i_toC - i_fromC) <= 1) {
        return 1;
    }
    return 0;
}

/*
 * Top-level move legality check.  Verifies the destination is on the
 * board, the source has the player's piece, and the destination doesn't
 * already hold the player's own piece.  Then dispatches to the per-piece
 * function.  Does NOT check for self-check, that is done by tryMove.
 * l_board - the current board
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * i_player - the moving player (0 = white, 1 = black)
 * Returns 1 if the move follows the piece's rules, 0 otherwise.
 */
int isLegalMove(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC, int i_player)
{
    char piece;
    char target;

    if (i_toR < 0 || i_toR > 7 || i_toC < 0 || i_toC > 7) {
        return 0;
    }
    if (i_fromR == i_toR && i_fromC == i_toC) {
        return 0;
    }

    piece  = l_board[i_fromR*8 + i_fromC];
    target = l_board[i_toR*8 + i_toC];

    if (isOwnPiece(piece, i_player) == 0) {
        return 0;
    }
    if (isOwnPiece(target, i_player) == 1) {
        return 0;
    }

    if (piece == 'P' || piece == 'p') {
        return pawnLegal(l_board, i_fromR, i_fromC, i_toR, i_toC, i_player);
    }
    if (piece == 'R' || piece == 'r') {
        return rookLegal(l_board, i_fromR, i_fromC, i_toR, i_toC);
    }
    if (piece == 'N' || piece == 'n') {
        return knightLegal(i_fromR, i_fromC, i_toR, i_toC);
    }
    if (piece == 'B' || piece == 'b') {
        return bishopLegal(l_board, i_fromR, i_fromC, i_toR, i_toC);
    }
    if (piece == 'Q' || piece == 'q') {
        return queenLegal(l_board, i_fromR, i_fromC, i_toR, i_toC);
    }
    if (piece == 'K' || piece == 'k') {
        return kingLegal(i_fromR, i_fromC, i_toR, i_toC);
    }

    return 0;
}

/*
 * Checks whether the given player's king is currently under attack.
 * Finds the king on the board, then asks whether any opponent piece can
 * legally move to the king's square.
 * l_board - the current board
 * i_player - the player whose king we are checking (0 = white, 1 = black)
 * Returns 1 if the player's king is in check, 0 otherwise.
 */
int isInCheck(char* l_board, int i_player)
{
    int  i_kingRow;
    int  i_kingCol;
    int  row;
    int  col;
    char myKing;

    if (i_player == 0) {
        myKing = 'K';
    } else {
        myKing = 'k';
    }

    i_kingRow = -1;
    i_kingCol = -1;
    for (row = 0; row < 8; row = row + 1) {
        for (col = 0; col < 8; col = col + 1) {
            if (l_board[row*8 + col] == myKing) {
                i_kingRow = row;
                i_kingCol = col;
            }
        }
    }
    /* no king on board (shouldn't happen) */
    if (i_kingRow == -1) {
        return 0;
    }

    for (row = 0; row < 8; row = row + 1) {
        for (col = 0; col < 8; col = col + 1) {
            if (isOpponent(l_board[row*8 + col], i_player) == 1) {
                if (isLegalMove(l_board, row, col, i_kingRow, i_kingCol, 1 - i_player) == 1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*
 * Tries to make a move.  If the move follows piece rules and would not
 * leave the moving player's king in check, the move is applied.
 * Otherwise nothing is changed.
 * l_board - the current board (modified in place if the move is made)
 * i_fromR - source row
 * i_fromC - source column
 * i_toR - destination row
 * i_toC - destination column
 * i_player - the moving player (0 = white, 1 = black)
 * Returns 1 if the move was made, 0 if it was rejected.
 */
int tryMove(char* l_board, int i_fromR, int i_fromC, int i_toR, int i_toC, int i_player)
{
    char saved;
    char fromPiece;
    int  b_check;

    if (isLegalMove(l_board, i_fromR, i_fromC, i_toR, i_toC, i_player) == 0) {
        return 0;
    }

    /* tentatively apply */
    saved     = l_board[i_toR*8 + i_toC];
    fromPiece = l_board[i_fromR*8 + i_fromC];
    l_board[i_toR*8 + i_toC]    = fromPiece;
    l_board[i_fromR*8 + i_fromC] = '.';

    /* if this puts our own king in check, roll back */
    b_check = isInCheck(l_board, i_player);
    if (b_check == 1) {
        l_board[i_fromR*8 + i_fromC] = fromPiece;
        l_board[i_toR*8 + i_toC]    = saved;
        return 0;
    }
    return 1;
}
