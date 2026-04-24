/*
  refer to https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 for ansi codes used here, but there will be comments too

  util.c / util.h               -  will consist of helper functions
  polyxplorer.c / polyxplorer.h -  will consist of the main functions and executions of functions

  TODO:
    - undone
  . - pending
  x - finished

  open & print user-specified directory contents                   [x]
  ansi codes to move choice around (with J & K)                    [x]
  allow directory traversal                                        [x]
  modify files (r/d) operations                                    [x]
  add page support for many files (by getting term width)          [ ]
  open non-binary files in default text editor (default polyedit)  [ ]
*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include <unistd.h>
#include <signal.h>
#include "polyxplorer.h"
#include <termios.h>

#define VERSION "v1.0.2"
struct termios orig_termios;

void disable_raw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\x1b[?1049l"); // goes back to normal screen
}

void setup_terminal()
{

    // https://stackoverflow.com/questions/54352563/block-sigint-from-terminating-program
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);
    sigprocmask(SIG_BLOCK, &block_set, NULL);

    // use ansi codes instead of system() to clear because i said so
    printf("\x1b[?1049h"); // switches to alternate screen
    printf("\x1B[H"); // sets cur to (0, 0)
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char *argv[])
{
    Cursor *cur = malloc(sizeof(Cursor));
    cur->max_lines = get_terminal_height() - 1;
    cur->selected_index = 0;
    cur->line_count = 0;
    if(argc == 1) {
        setup_terminal();
        Line *lines = malloc(sizeof(Line) * (get_terminal_height() - 2));
        if(!lines) {
            perror("no memory :rofl: ???");
            return 1;
        }
        get_contents(".", cur, lines); // set rel path to '.' because i dont want to force users to enter something like `px .` when using this
        cleanup(lines, cur);
    }


    char *bin_name = argv[0];
    if(argc == 2) {
        if(strcmp(argv[1], "--help")) {
            setup_terminal();
            Line *lines = malloc(sizeof(Line) * (get_terminal_height() - 1));
            if(!lines) {
                perror("no memory :rofl: ???");
                return 1;
            }
            get_contents(argv[1], cur, lines);
            cleanup(lines, cur);
        } else {
            printf("polyxplorer %s\n\nofficial page: https://github.com/Polymorqhism/polyxplorer\n\n%s --help\t- to show this help page\n%s [path]\t- open directory, leave blank for cwd\n", VERSION, bin_name, bin_name);
            free(cur);
            return 1;
        }
    }

    disable_raw();
    free(cur);
}
