#define PATH_MAX 4096
#define DIR_FILES_MAX 512 // if you have more than 512 files in your directory, there's something wrong with you
#define FILE_MAX 255 // this exists for both file lengths and directories
#include <stddef.h>

typedef struct {
    char *abs_path;
    char **files;
    char **dirs;
    size_t file_count;
    size_t dir_count;
} Contents;

void get_abs_path(Contents *contents, char *path);
int get_type(char *abs_path);
void cleanup_contents(Contents *contents);
void get_contents(Contents *contents, char *path);
