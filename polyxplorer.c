/*
  hopefully this code is well documented enough (with comments and code clarity) that you reading this (yes YOU) can read and understand it
  if i get tired of documentation all this itll be apparent

  util.c / util.h               -  will consist of helper functions
  polyxplorer.c / polyxplorer.h -  will consist of the main functions and executions of functions

  please dont steal my code, you can find better
  you should alias polyxplorer to be px or similar for ease of access

  TODO:

  easy:

  open & print user-specified directory contents                   [ ]
  ansi codes to move choice around (with J & K)                    [ ]
  allow directory traversal                                        [ ]
  open non-binary files in default text editor (default polyedit)  [ ]

  hard:

  implementation with polycomm  (maybe)                            [ ]


  the plan:
  - Contents struct ptr will consist of the current directory; it will change with the actual current directory being viewed w/ the help of get_contents()
  - use raw ansi codes to navigate between options
  - r = rename
  - d = delete (with y/n confirmation)
  - o = open the file in default text editor (if file != binary)

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include "util.h"
#include "polyxplorer.h"

#define VERSION "v0.0.0"

int main(int argc, char *argv[])
{
    Contents *contents = malloc(sizeof(Contents));

    if(argc < 2) {
        get_contents(contents, "."); // set rel path to '.' because i dont want to force users to enter something like `px .` when using this
    }

    if(argc == 2) {
        char *bin_name = argv[0];
        if(!strcmp(argv[1], "--help")) {
            printf("polyxplorer %s\n\nofficial page: https://github.com/Polymorqhism/polyxplorer\n\n%s --help\t- to show this help page\n%s [path]\t- open directory. will open cwd\n", VERSION, bin_name, bin_name);
        }
    }

    cleanup_contents(contents); // clean up after yourself
}
