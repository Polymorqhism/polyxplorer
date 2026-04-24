#define _GNU_SOURCE
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
int file_count = 0;
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
void cleanup(Line *lines, Cursor *cur)
{
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

int store_contents(char *abs_path, Line *lines, Cursor *cur)
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

        if(get_type(buf) == 0 && file_count < DIR_FILES_MAX/2) { // get only dirs
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
            }
        }

        if(get_type(buf) == 1 && file_count < DIR_FILES_MAX/2) { // get only files
            if(line_count >= cur->max_lines) {
                break;
            }
            Line new_line;
            new_line.file_name = strdup(en->d_name);
            new_line.abs_path = strdup(buf);
            new_line.is_directory = 0;

            lines[line_count] = new_line;

            line_count++;
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
            fflush(stdout);
            exit(0);
        }
    }
}

void get_contents(char *path, Cursor *cur, Line *lines)
{
    char *abs_path;
    abs_path = realpath(path, NULL);
    if(store_contents(abs_path, lines, cur) == -1) {
        perror("failed to store contents of the directory");
        return;
    }

    free(abs_path);

    render_ui(lines, cur);

    handle_input(cur, lines);
}
