#include <unistd.h>
#include <termios.h>

void enableRawMode() {
    struct termios raw;

    tcgetattr(STDIN_FILENO, &raw);

    // c_lflag -> local_flags
    raw.c_lflag &= ~(ECHO); // echo makes it so each typed key is printed

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


int main() {
    enableRawMode();

    char c;

    // read 1 byte from stdin until EOF
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}