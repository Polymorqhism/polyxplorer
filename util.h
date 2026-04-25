#define DIR_FILES_MAX 512 // if you have more than 512 files in your directory, there's something wrong with you
#define PATH_MAX 4096
#define FILE_MAX 255 // this exists for both file lengths and directories
#include <stddef.h>
#include <sys/types.h>

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

typedef struct {
    int fd;
    pid_t pid;
} Clipboard;

/*
  use Cursor struct initialised once in main() to track the cursor location and the Line it is on
  the current line is determined by lines[Cursor's selected_index]
  check bounds by checking if it exceeds the global variable which consists of the file count
  no use of column because we arent doing anything which requires it

  combined, we can use Cursor to track the cursor location, which also stores the current line depending on the current row

  -> why a struct instead of a global variable?
  because we might need more elements to keep track of in the future, or for custom builds of polyxplorer
 */
typedef struct {
    int selected_index;
    int max_lines;
    int line_count;
} Cursor;

int get_type(char *abs_path);
void get_contents(char *path, Cursor *cur, Line *lines, Clipboard *cb);
int get_terminal_height();

// cleaning lines requires cursor struct
void cleanup(Line *lines, Cursor *cur, Clipboard *cb);
void copy_file(Cursor *cur, Line *lines, Clipboard *cb);
Clipboard init_clipboard();
