#ifndef SNACK_H
#define SNACK_H 1

#include "wacs.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libc.h>
#include <locale.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "utf8.h"

/* Constants */
#define CURSOR_BAR             "\x1b[\x36 q"
#define CURSOR_BAR_BLINK       "\x1b[\x35 q"
#define CURSOR_BLOCK           "\x1b[\x32 q"
#define CURSOR_BLOCK_BLINK     "\x1b[\x30 q"
#define CURSOR_UNDERLINE       "\x1b[\x34 q"
#define CURSOR_UNDERLINE_BLINK "\x1b[\x33 q"
#define LINSIZ                 64

/* Macros */
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))
#define BUCKETS(length)     ((length) / LINSIZ + ((length) % LINSIZ != 0))

/* Enums */
typedef enum {
  Mode_normal,                    // Normal mode
  Mode_insert,                    // Insert mode
  Mode_replace                    // Replace mode
} Mode ;

typedef enum {
  Status_running = 1,             // Run main edit loop
  Status_dirty   = 1 << 1         // Current buffer is dirty (TODO: Move to buffer)
} Status ;

/* Types */
typedef struct Line Line;
struct Line {
  char   *c;                      // Line content
  size_t buckets;                 // Buckets of LINSIZ used to allocate c
  int    length;                  // Line length in bytes
  int    visual_length;           // Line "character" count
  bool   dirty;                   // Needs a repaint?
  Line  *prev;                    // Previous line
  Line  *next;                    // Next line
};

typedef struct Position {
  int   offset;                   // Offset in line
  Line *line;                     // Line of position
} Position;

typedef struct Selection {
  Position *start;
  Position *end;
} Selection;

typedef struct Buffer {
  Position *cursor;               // Position in buffer
  int       offset_prev;          // Previous cursor offset, used for maintaining column on vertical movement
  char     *filename;             // Filename
  Line     *first_line;           // First line of file
  Line     *last_line;            // Last line of file
} Buffer;

typedef struct KeyMapping {
  Mode  mode;                     // Mode the mapping applies to (e.g. Mode_normal)
  char *operator;                 // String to match
  bool  (*action)(Buffer *b, Selection *s);
} KeyMapping;


/* State variables */
static char    c[7];              // Input
static int     cols;              // Columns
static int     rows;              // Rows
static char    title[BUFSIZ];     // Editor title
static char   *title_temp = NULL; // Temporary editor title
static WINDOW *editor_window;     // Main editor window
static WINDOW *status_window;     // Status bar window
static Buffer *current_buffer;    // Current buffer
static Mode    current_mode;      // Current mode of the editor
static long    current_status;    // Current status of the editor

/* Actions */
static bool action_quit();

static bool action_mode_insert();
static bool action_mode_normal();

static bool action_move_nextline(Buffer *b, Selection *s);
static bool action_move_prevline(Buffer *b, Selection *s);
static bool action_move_nextchar(Buffer *b, Selection *s);
static bool action_move_prevchar(Buffer *b, Selection *s);
static bool action_move_bof(Buffer *b, Selection *s);
static bool action_move_eof(Buffer *b, Selection *s);
static bool action_move_bol(Buffer *b, Selection *s);
static bool action_move_eol(Buffer *b, Selection *s);

static bool action_insert_line(Buffer *b, Selection *s);

#endif /* ifndef SNACK_H */
