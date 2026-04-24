#define _GNU_SOURCE
#include <libgen.h>
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

int file_count = 0;
char *current_path = "";
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
int get_type(char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return -1;

    if (S_ISDIR(st.st_mode))
        return 0;
    else
        return 1;
}

// frees necessary pointers
void cleanup(Line *lines, Cursor *cur)
{
    if(lines) {
        if(!cur) {
            perror("cannot free lines w/out cursor struct");
        }
        for(int i = 0; i < cur->line_count; i++) {
            if (lines[i].abs_path)
                free(lines[i].abs_path);

            if (lines[i].file_name)
                free(lines[i].file_name);
        }
        free(cur);
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

    Line dot_dot;
    char *tmp = strdup(abs_path);
    dot_dot.abs_path = strdup(dirname(tmp));
    free(tmp);
    dot_dot.file_name = strdup("..");
    dot_dot.is_directory = 3;
    lines[cur->line_count] = dot_dot;
    cur->line_count++;

    while((en = readdir(dr)) != NULL) {
        char buf[PATH_MAX];
        snprintf(buf, PATH_MAX, "%s/%s", abs_path, en->d_name);

        if(get_type(buf) == 0 && file_count < DIR_FILES_MAX/2) { // get only dirs
            if(strcmp(en->d_name, ".") && strcmp(en->d_name, "..")) {
                if(cur->line_count >= cur->max_lines) {
                    break;
                }
                Line new_line;
                char *file_name = strdup(en->d_name);
                new_line.abs_path = strdup(buf);
                new_line.file_name = file_name;
                new_line.is_directory = 1;

                lines[cur->line_count] = new_line;

                cur->line_count++;
                file_count++;
            }
        }

        if(get_type(buf) == 1 && file_count < DIR_FILES_MAX/2) { // get only files
            if(cur->line_count >= cur->max_lines) {
                break;
            }
            Line new_line;
            new_line.file_name = strdup(en->d_name);
            new_line.abs_path = strdup(buf);
            new_line.is_directory = 0;

            lines[cur->line_count] = new_line;

            cur->line_count++;
            file_count++;
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
    int current_row = 3;
    printf("%s[2J", esc);
    printf("%s[1;1f", esc); // set it to current row
    printf("%s[1m%s[38;2;255;221;51m", esc, esc); // set formatting for the current dir (bgold)
    printf("%s%s[0m", current_path, esc); // print current dir and clear all modes
    printf("%s[2;1f", esc); // go to row 2
    printf("%4s\tsize\t\tname", "num"); // print the header
    for(int i = 0; i<cur->line_count; i++) {
        printf("%s[%d;1f", esc, current_row); // set it to current row
        printf("%s[2K", esc); // clears the whole line to prevent format override
        char *real_file = realpath(lines[i].file_name, NULL);
        off_t file_size = fsize(lines[i].abs_path);
        if(file_size >= 0) {
            if(i == cur->selected_index) {
                printf("%s[7m", esc); // inverts color for selected one
                printf("%s[2K", esc); // clears the whole line
            } else if(lines[i].is_directory) {
                printf("%s[34m", esc);
            }
            if (lines[i].is_directory == 1)
                printf("%4d\t[DIR]\t\t%s", i + 1, lines[i].file_name);
            else if(lines[i].is_directory == 0)
                printf("%4d\t[%ld]\t\t%s", i + 1, file_size, lines[i].file_name);
            else if(lines[i].is_directory == 3)
                printf("%4d\t[BACK]\t\t%s\n", i + 1, lines[i].file_name);
            current_row++;
        }

        printf("%s[0m", esc); // resets all modes
        free(real_file);
    }
    printf("%s[%d;1f", esc, current_row); // move down
    printf("d = delete; enter = open; r = rename; q = quit");
    fflush(stdout);
}


// we handle input based on this loop and call functions as necessary to branch out
void handle_input(Cursor *cur, Line *lines)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if(c == 'q') {
            cleanup(lines, cur);
            exit(0);
        }

        if(c == 'j') {
            if(cur->selected_index+1 < cur->line_count) {
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
        } else if(c == 'r') {
            int height = get_terminal_height();
            int index = cur->selected_index;
            char r;

            printf("%s[%d;1f", esc, height-1); // set it to last row
            printf("Rename file %d: ", index + 1);
            fflush(stdout);

            int width = get_terminal_width();
            char new[width] = {};

            int col = 0;
            while (read(STDIN_FILENO, &r, 1) == 1) {
                if(r == 127 || r == 8) {
                    if(col > 0) {
                        col--;
                        new[col] = '\0';
                        printf("\b \b"); // go back
                    }

                    fflush(stdout);
                    continue;
                }

                if(r == '\n') {
                    printf("%s[2K", esc); // erase whole line
                    fflush(stdout);
                    break;
                }

                if(col < width - 1) {
                    new[col] = r;
                    col++;
                    printf("%c", r);
                }
                fflush(stdout);
            }

            new[col] = '\0';

            printf("%s[2K\r", esc); // erase whole line
            printf("Renaming...");
            char buf[PATH_MAX];
            snprintf(buf, PATH_MAX, "%s/%s", current_path, new);

            if(rename(lines[index].abs_path, buf) == 0 && col > 0) {
                free(lines[index].file_name); // free both because override
                free(lines[index].abs_path);

                lines[index].file_name = strdup(new);
                lines[index].abs_path = strdup(buf);

                printf("%s[2K\r", esc); // erase whole line
                printf("Successfully renamed.");
                render_ui(lines, cur);
            } else {
                printf("%s[2K\r", esc); // erase whole line
                printf("Failed to rename. Check permissions?");
            }

            fflush(stdout);
        } else if(c == 'd') {
            int height = get_terminal_height();
            char y;

            printf("%s[%d;1f", esc, height-1); // set it to last row
            printf("Delete %s? Click y to confirm.", lines[cur->selected_index].file_name);
            fflush(stdout);

            while (read(STDIN_FILENO, &y, 1) == 1) {
                if(y == 'y') {
                    int index = cur->selected_index;
                    remove(lines[index].abs_path);
                    free(lines[index].abs_path);
                    free(lines[index].file_name);
                    memmove(&lines[index], &lines[index + 1], (cur->line_count - index - 1) * sizeof(lines[0]));
                    cur->line_count--;
                    cur->selected_index = 0;
                    render_ui(lines, cur);
                    break;
                }
            }

            printf("%s[2K", esc); // erase whole line

        } else if(c == '\n') {
            if(lines[cur->selected_index].is_directory < 1) {
                continue;
            }

            char *next_path = strdup(lines[cur->selected_index].abs_path);
            if (!next_path) {
                perror("strdup failed");
                exit(1);
            }

            free(current_path);
            current_path = strdup(next_path);

            int height = get_terminal_height() - 2;

            Line *new_lines = calloc(height, sizeof(Line));
            Cursor *new_cur = malloc(sizeof(Cursor));

            if(!new_lines || !new_cur) {
                perror("no memory ur pc SUCKS...");
                exit(1);
            }

            new_cur->max_lines = height;
            new_cur->selected_index = 0;
            new_cur->line_count = 0;

            file_count = 0;

            if(store_contents(next_path, new_lines, new_cur) == -1) {
                perror("failed to store contents of dir");
                exit(1);
            }

            cleanup(lines, cur);
            free(next_path);

            lines = new_lines;
            cur = new_cur;

            render_ui(lines, cur);
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
    current_path = strdup(abs_path);

    free(abs_path);

    render_ui(lines, cur);

    handle_input(cur, lines);
}
