/*
  hopefully this code is well documented enough (with comments and code clarity) that you reading this (yes YOU) can read and understand it
  if i get tired of documentation all this itll be apparent

  util.c / util.h               -  will consist of helper functions
  polyxplorer.c / polyxplorer.h -  will consist of the main functions and executions of functions

  please dont steal my code, you can find better
  you should alias polyxplorer to be px or similar for ease of access

  TODO:
  . - pending
  x - finished
    - undone

  easy:

  open & print user-specified directory contents                   [x]
  ansi codes to move choice around (with J & K)                    [.]
  allow directory traversal                                        [ ]
  modify files (r/d/o) operations                                  [ ]
  open non-binary files in default text editor (default polyedit)  [ ]

  the plan:
  - Contents struct ptr will consist of the current directory; it will change with the actual current directory being viewed w/ the help of get_contents()
  - use raw ansi codes to navigate between options w/ the help of Line and Cursor structs
  - use Line struct array for navigation with Cursor to track where you're at
  - r = rename
  - d = delete (with y/n confirmation)
  - o = open the file in default text editor (if file != binary)

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
}

void setup_terminal()
{
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char *argv[])
{
    Contents *contents = malloc(sizeof(Contents));

    if(argc == 1) {
        get_contents(contents, "."); // set rel path to '.' because i dont want to force users to enter something like `px .` when using this
    }


    char *bin_name = argv[0];
    if(argc == 2) {
        if(strcmp(argv[1], "--help")) {
            get_contents(contents, argv[1]);
        } else {
            printf("polyxplorer %s\n\nofficial page: https://github.com/Polymorqhism/polyxplorer\n\n%s --help\t- to show this help page\n%s [path]\t- open directory. will open cwd\n", VERSION, bin_name, bin_name);
            free(contents);
            return 1;
        }
    }

    cleanup(contents); // clean up after yourself
}
