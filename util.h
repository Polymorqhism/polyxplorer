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


/*
  use Line structs for when we use ansi codes to print it
  using file_name for printing it
  utilise with Cursor struct for easy tracking
 */
typedef struct {
    char *file_name;
    char *abs_path;
    int is_directory;
} Line;

/*
  use Cursor struct initialised once in main() to track the cursor location and the Line it is on
  the current line is determined by lines[Cursor's selected_index]
  no use of column because we arent doing anything which requires it

  combined, we can use Cursor to track the cursor location, which also stores the current line depending on the current row
 */
typedef struct {
    int selected_index;
} Cursor;

void get_abs_path(Contents *contents, char *path);
int get_type(char *abs_path);
void cleanup(Contents *contents);
void get_contents(Contents *contents, char *path);
