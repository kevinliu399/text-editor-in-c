#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

/* defines */
#define CTRL_KEY(k) ((k) & 0x1f)

/* data */

struct editorConfig
{
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

struct editorConfig E;

/* terminal */

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");
    atexit(disableRawMode); // register the function to be called automatically when exiting

    struct termios raw = E.orig_termios;

    // c_lflag -> local_flags
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    /*
    ECHO : makes it so each typed key is printed
    ICANON : canonycal flag
    ISIG: CTRL+Z || CTRL+C
    IEXTEN: CTRL-V

    ICRNL: carriage returns
    IXON: CTRL+S && CTRL+Q: software flow control, ans `s` stops data from being transmitted and `q` resumesit

    OPOST: output processing such as `n` -> `\r\n`
    */

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

// wait for keypress and returns it
char editorReadKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }
    return c;
}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    // on sucess, ioctl will place the number of columns and rows in winsize struct, else gives -1
    // also make sure that the value returned is not 0!

    // however, not guaranteed to work on all systems > so we must approach it differently in case
    // 1. Move cursor bottom-right and we use escape sequence to query the position of the cursor
    // this is done by using C and B commands (Cursor forward and down) and we use big value to make sure
    // it is at the edge
    if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        editorReadKey();
        return -1;
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/* output */
void editorDrawRows()
{
    int y;
    for (y = 0; y < E.screenrows; y++)
    {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    /* 4: 4 bytes to terminal
    1st byte : \x1b which is escape char (value of 27)
    2,3,4 byte: [2j
    This is an escape sequence, and it always starts with \x1b followed by [, then we use 2 to indicate the whole screen and j for the key to press
    */
    write(STDERR_FILENO, "\x1b[H", 3);
    /*
    H command : cursor position, default is position 1;1 (first row and column)
    */

    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3); // reposition the cursor after drawing
}

/* input */

// wait for keypress and handles it
void editorProcessKeypress()
{
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }
}

/* init */

void initEditor()
{
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
}

int main()
{
    enableRawMode();
    initEditor();

    while (1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}