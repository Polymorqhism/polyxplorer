/*
  hopefully this code is well documented enough (with comments and code clarity) that you reading this (yes YOU) can read and understand it
  if i get tired of documentation all this itll be apparent

  refer to https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797 for ansi codes used here, but there will be comments too

  util.c / util.h               -  will consist of helper functions
  polyxplorer.c / polyxplorer.h -  will consist of the main functions and executions of functions

  please dont steal my code, you can find better
  you should alias polyxplorer to be px or similar for ease of access

  TODO:
    - undone
  . - pending
  x - finished

  open & print user-specified directory contents                   [x]
  ansi codes to move choice around (with J & K)                    [x]
  allow directory traversal                                        [.]
  add page support for many files (by getting term width)          [ ]
  modify files (r/d/o) operations                                  [ ]
  open non-binary files in default text editor (default polyedit)  [ ]

  the plan:
  - use raw ansi codes to navigate between options w/ the help of Line and Cursor structs
  - more raw ansi codes for coloring
  - save directory contents internally but display them paginally externally
  - use Line struct array for navigation with Cursor to track where you're at
  - we removed Contents struct because it's no longer used, and a singular global variable suffices
  - r = rename
  - d = delete (with y/n confirmation)
  - o = open the file in default text editor

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include <unistd.h>
#include "polyxplorer.h"
#include <termios.h>

#define VERSION "v0.0.0"
struct termios orig_termios;

void disable_raw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\x1b[?1049l"); // goes back to normal screen
}

void setup_terminal()
{
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

    // use ansi codes instead of system() to clear because i said so
    printf("\x1b[?1049h"); // switches to alternate screen
    printf("\x1B[H"); // sets cur to (0, 0)

    Cursor *cur = malloc(sizeof(Cursor));
    cur->max_lines = get_terminal_height() - 1;
    cur->selected_index = 0;
    if(argc == 1) {
        setup_terminal();
        Line *lines = malloc(sizeof(Line) * (get_terminal_height() - 1));
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
            printf("polyxplorer %s\n\nofficial page: https://github.com/Polymorqhism/polyxplorer\n\n%s --help\t- to show this help page\n%s [path]\t- open directory. will open cwd\n", VERSION, bin_name, bin_name);
            free(cur);
            return 1;
        }
    }

    disable_raw();
    free(cur);
}
