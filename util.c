#define _GNU_SOURCE
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
/*

  for reference:
  typedef struct {
  char *abs_path;
  char **files;
  char **dirs;
  size_t file_count;
  size_t dir_count;
  } Contents;

*/
void cleanup(Contents *contents)
{

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

int store_contents(Contents *contents, char *abs_path)
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
                char *file_name = strdup(en->d_name);
                contents->dirs[contents->dir_count] = strdup(file_name);
                contents->dir_count++;
                free(file_name);
            }
        }

        if(get_type(buf) == 1 && contents->file_count < DIR_FILES_MAX/2) { // get only files
            char *file_name = strdup(en->d_name);
            contents->files[contents->file_count] = strdup(en->d_name);
            contents->file_count++;
            free(file_name);
        }

    }

    closedir(dr);
    return 0;
}

void display_dir(Contents *contents)
{
    for(size_t i = 0; i < contents->dir_count; i++) {
        printf("%s/\n", contents->dirs[i]);
    }

    for(size_t i = 0; i < contents->file_count; i++) {
        printf("%s\n", contents->files[i]);
    }
}

void get_contents(Contents *contents, char *path)
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

    if(store_contents(contents, contents->abs_path) == -1) {
        perror("failed to store contents of the directory");
        return;
    }

    display_dir(contents);
}
