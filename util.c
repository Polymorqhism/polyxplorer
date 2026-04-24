#define _GNU_SOURCE
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

int line_count = 0;
static char *esc = "\x1B";

#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

int get_terminal_height()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    return w.ws_row;
}


int get_terminal_width()
{

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    return w.ws_col;
}

void get_abs_path(Contents *contents, char *path)
{
    char *abs_path;
    abs_path = realpath(path, NULL);
    if(abs_path) {
        contents->abs_path = abs_path;
    }
}

// get_type returns 0 for directory, 1 for file, and -1 for failure
int get_type(char *abs_path)
{
    int type = -1; // set default to fail
    DIR *directory = opendir(abs_path);

    if(directory != NULL) {
        closedir(directory); // it is a dir
        type = 0;
    }

    if(errno == ENOTDIR) {
        type = 1;
        errno = 0;
    }


    return type;
}

// frees necessary pointers
void cleanup(Contents *contents, Line *lines, Cursor *cur)
{
    if(contents) {
        if(contents->abs_path) {
            free(contents->abs_path);
        }


        if(contents->files) {
            for(size_t i = 0; i < contents->file_count; i++) {
                if(contents->files[i]) {
                    free(contents->files[i]); // clear element mallocation
                }
            }
            free(contents->files); // no more touching contents->files
        }

        if(contents->dirs) {
            for(size_t i = 0; i < contents->dir_count; i++) {
                if(contents->dirs[i]) {
                    free(contents->dirs[i]);
                }
            }
            free(contents->dirs); // no more touching contents->dirs
        }
        if(contents) {
            free(contents); // then finally clear contents
        }

        // no need to free non-malloced pointers, they'll get cleaned up with the freeing of the struct naturally
        // using 'contents' ptr again WILL lead to use-after-free
    }

    if(lines) {
        if(!cur) {
            perror("cannot free lines w/out cursor struct");
        }
        for(int i = 0; i < line_count; i++) {
            free(lines[i].abs_path);
            free(lines[i].file_name);
        }

        free(lines);
    }
}

int store_contents(Contents *contents, char *abs_path, Line *lines, Cursor *cur)
{
    DIR *dr;
    struct dirent *en;
    dr = opendir(abs_path);
    if(!dr) {
        perror("cannot continue");
        return -1;
    }

    while((en = readdir(dr)) != NULL) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%s/%s", abs_path, en->d_name);

        if(get_type(buf) == 0 && contents->dir_count < DIR_FILES_MAX/2) { // get only dirs
            if(strcmp(en->d_name, ".") && strcmp(en->d_name, "..")) {
                if(line_count >= cur->max_lines) {
                    break;
                }
                Line new_line;
                char *file_name = strdup(en->d_name);
                new_line.abs_path = strdup(buf);
                new_line.file_name = file_name;
                new_line.is_directory = 1;

                lines[line_count] = new_line;

                line_count++;
                // contents->dirs[contents->dir_count] = strdup(file_name);
                // contents->dir_count++;
                // free(file_name);
            }
        }

        if(get_type(buf) == 1 && contents->file_count < DIR_FILES_MAX/2) { // get only files
            if(line_count >= cur->max_lines) {
                break;
            }
            Line new_line;
            new_line.file_name = strdup(en->d_name);
            new_line.abs_path = strdup(buf);
            new_line.is_directory = 0;

            lines[line_count] = new_line;

            line_count++;
            // char *file_name = strdup(en->d_name);
            // contents->files[contents->file_count] = strdup(en->d_name);
            // contents->file_count++;
            // free(file_name);
        }

    }

    closedir(dr);
    return 0;
}


// source: https://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c
off_t fsize(const char *filename)
{
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return -1;
}


// display lines based on the Line struct, not file names
void render_ui(Line *lines, Cursor *cur)
{
    int current_row = 1;
    for(int i = 0; i<line_count; i++) {
        printf("%s[%d;1f", esc, current_row); // set it to current row
        printf("%s[2K", esc); // clears the whole line to prevent format override
        char *real_file = realpath(lines[i].file_name, NULL);
        off_t file_size = fsize(real_file);
        if(file_size >= 0) {
            if(i == cur->selected_index) {
                printf("%s[7m", esc); // inverts color for selected one
                printf("%s[2K", esc); // clears the whole line
            } else if(lines[i].is_directory) {
                printf("%s[34m", esc);
            }

            printf("[%jd\tB]\t\t%s", file_size, lines[i].file_name); // print it out
            current_row++;
        }

        printf("%s[0m", esc); // resets all modes
        free(real_file);
    }
    printf("\n");
}


// we handle input based on this loop and call functions as necessary to branch out
void handle_input(Cursor *cur, Line *lines)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if(c == 'q') {
            break;
        }

        if(c == 'j') {
            if(cur->selected_index+1 < line_count) {
                cur->selected_index++;
                render_ui(lines, cur);
                fflush(stdout);
            }
        } else if(c == 'k') {
            if(cur->selected_index-1 >= 0) {
                cur->selected_index--;
                render_ui(lines, cur);
                fflush(stdout);
            }
        } else if(c == '\n') {
            printf("%s\n", lines[cur->selected_index].abs_path);
            fflush(stdout);
        }
    }
}

void get_contents(Contents *contents, char *path, Cursor *cur, Line *lines)
{
    /* to fill:

       char *abs_path;
       char **files;
       char **dirs;
       size_t file_count;
       size_t dir_count;

    */

    get_abs_path(contents, path);
    contents->file_count = 0;
    contents->dir_count = 0;
    contents->files = malloc(sizeof(char *) * DIR_FILES_MAX/2); // divide by 2 because the limit is inclusive of directories and files (because i said so)
    contents->dirs = malloc(sizeof(char *) * DIR_FILES_MAX/2);

    if(store_contents(contents, contents->abs_path, lines, cur) == -1) {
        perror("failed to store contents of the directory");
        return;
    }

    render_ui(lines, cur);

    handle_input(cur, lines);
}
