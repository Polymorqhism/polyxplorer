#define _GNU_SOURCE
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include "util.h"

void get_abs_path(Contents *contents, char *path)
{
    char *abs_path;
    abs_path = realpath(path, NULL);
    contents->abs_path = abs_path;
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
void cleanup_contents(Contents *contents)
{
    if(contents->abs_path) {
        free(contents->abs_path);
    }

    // since freeing the struct will not always free the children within it, we have to go from the inside out of the array.
    // we have no real choice except freeing the inner elements in the char ** (char ptr ptr, or simpler - an array of strings eg. {"inner"} <- outer)
    // then freeing the array itself which would be {} by the end of it, thus the for loop

    if(contents->files && contents->file_count) {
        for(size_t i = 0; i < contents->file_count; i++) {
            if(contents->files[i]) {
                free(contents->files[i]); // sorry never nesters
            }
        }
        free(contents->files); // no more touching contents->files
    }

    if(contents->dirs && contents->dir_count) {
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

    // no need to free those ints (not ptrs)
    // using 'contents' ptr again WILL lead to use-after-free (though this should be implicitly known)
}

int get_files(Contents *contents, char *abs_path) // absolute path for clarity
{
    DIR *dr;
    struct dirent *en;
    dr = opendir(abs_path);
    if(!dr) {
        perror("cannot continue");
        return -1;
    }

    while((en = readdir(dr)) != NULL) {
        if(en->d_name[0] != '.' && get_type(en->d_name) == 1) { // we skip the directories and only get files; 1 = file & 0 = dir
            contents->files[contents->file_count] = (char *) malloc(FILE_MAX);
            contents->files[contents->file_count] = en->d_name; // these should be relative, not absolute
            contents->file_count++;
        }
    }
    return 0;
    // returns 0 for success, -1 for failure. make sure you fail check this function, if the directory being checked doesn't exist, it will throw an error but you may still process it in Contents *
}

int get_dirs(Contents *contents, char *abs_path) // absolute path for clarity
{
    DIR *dr;
    struct dirent *en;
    dr = opendir(abs_path);
    if(!dr) {
        perror("cannot continue");
        return -1;
    }

    while((en = readdir(dr)) != NULL) {
        if(en->d_name[0] != '.' && get_type(en->d_name) == 0) { // we skip the files and only get dirs; 1 = file & 0 = dir
            contents->dirs[contents->dir_count] = (char *) malloc(FILE_MAX);
            contents->dirs[contents->dir_count] = en->d_name;
            contents->dir_count++;
        }
    }
    return 0;
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
    contents->files = malloc(DIR_FILES_MAX/2); // divide by 2 because the limit is inclusive of directories and files (because i said so)
    contents->dirs = malloc(DIR_FILES_MAX/2);
    get_files(contents, contents->abs_path);
    get_dirs(contents, contents->abs_path);
}
