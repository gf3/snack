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
#define BUCKETS(length)     (length / LINSIZ + (length % LINSIZ != 0))

/* Enums */
typedef enum {
  Mode_normal,                                 // Normal mode
  Mode_insert,                                 // Insert mode
  Mode_replace                                 // Replace mode
} Mode ;

typedef enum {
  Status_running = 1                           // Run main edit loop
} Status ;

/* Types */
typedef struct Line Line;
struct Line {
  char *c;                                     // Line content
  int buckets;                                 // Buckets of LINSIZ used to allocate c
  int length;                                  // Line length in bytes
  int visual_length;                           // Line "character" count
  Line *prev;                                  // Previous line
  Line *next;                                  // Next line
};

typedef struct Position {
  int offset;                                  // Offset in line
  Line *line;                                  // Line of position
} Position;

typedef struct Selection {
  Position *start;
  Position *end;
} Selection;

typedef struct Buffer {
  Position *cursor;                            // Position in buffer
  int       offset_prev;                       // Previous cursor offset, used for maintaining column on vertical movement
  char     *filename;                          // Filename
  Line     *first_line;                        // First line of file
  Line     *last_line;                         // Last line of file
} Buffer;

typedef struct KeyMapping {
  Mode mode;
  char *operator;
  void (*action)(Buffer *b, Selection *s);
} KeyMapping;


/* State variables */
static char    c[7];                           // Input
static int     cols;                           // Columns
static int     rows;                           // Rows
static char    title[BUFSIZ];                  // Editor title
static char   *title_temp = NULL;              // Temporary editor title
static WINDOW *editor_window;                  // Main editor window
static WINDOW *status_window;                  // Status bar window
static Buffer *current_buffer;                 // Current buffer
static Mode    current_mode;                   // Current mode of the editor
static long    current_status;                 // Current status of the editor

/* Actions */
void action_quit();
void action_mode_insert();
void action_mode_normal();
void action_move_nextline(Buffer *b, Selection *s);
void action_move_prevline(Buffer *b, Selection *s);
void action_move_nextchar(Buffer *b, Selection *s);
void action_move_prevchar(Buffer *b, Selection *s);
void action_move_bof(Buffer *b, Selection *s);
void action_move_eof(Buffer *b, Selection *s);
void action_move_bol(Buffer *b, Selection *s);
void action_move_eol(Buffer *b, Selection *s);

#endif /* ifndef SNACK_H */
