#define _XOPEN_SOURCE_EXTENDED 1

#include "snack.h"
#include "config.h"

/* Internal functions */
static void internal_command();                // Command processing
static void internal_edit();                   // Main edit loop
static void internal_exit();                   // Gracefully exit
static void internal_loadfile(Buffer *buffer); // Load file
static void internal_paint();                  // Repaint screen
static void internal_setup();                  // Setup editor
static void internal_term();                   // Initialize terminal

/* Safe memory function wrappers */
static void *safe_calloc(size_t nmemb, size_t size);
static void *safe_malloc(size_t size);
static void *safe_realloc(void *ptr, size_t size);
static char *safe_strdup(const char *s);

/* Go go go */
int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");

  internal_setup();

  // TMP
  current_buffer->filename = (argc > 1 ?
      safe_strdup(argv[1]) : safe_strdup("/tmp/lol.c"));

  internal_loadfile(current_buffer);
  internal_edit();
  internal_exit();

  return EXIT_SUCCESS;
}


/**
 * Internal functions
 */

void internal_command() {
  unsigned int i;
  int c_length = strlen(c);
  Selection s = {
    .start = current_buffer->cursor,
    .end = current_buffer->cursor
  };

  for (i = 0; i < ARRAY_LENGTH(key_maps); ++i) {
    if (key_maps[i].mode == current_mode) {
      if (strncmp(c, key_maps[i].operator, c_length) == 0) {
        key_maps[i].action(current_buffer, &s);
        break;
      }
    }
  }
}

void internal_edit() {
  int ch;
  int c_width;

  while (current_status & Status_running) {
    internal_paint();

    // Grab full utf8 character
    if ((c_width = utf8_wgetch(editor_window, c)) == ERR) {
      continue;
    }

    ch = c[0];

    if (ch >= KEY_MIN)
      continue;

    internal_command();
  }
}

void internal_exit() {
  if (current_buffer != NULL) {
    Line *l;
    Line *n;

    for (l = current_buffer->first_line; l != NULL; l = n) {
      n = l->next;
      free(l->c);
      free(l);
    }

    if (current_buffer->filename != NULL)
      free(current_buffer->filename);
    free(current_buffer);
  }

  endwin();
}

void internal_loadfile(Buffer *buffer) {
  unsigned int  i;
  unsigned int  i_prev;
  int           fd;
  ssize_t       bytes_read;
  char          file_buffer[BUFSIZ];
  char         *line_buffer = NULL;
  size_t        line_buffer_capacity = 0;
  size_t        line_buffer_size = 0;
  Line         *l = NULL;
  Line         *l_prev = NULL;

  if (!buffer->filename)
    return;

  if ((fd = open(buffer->filename, O_RDONLY | O_CREAT)) == -1)
    err(errno, "Unable to open file: %s", buffer->filename);

  while ((bytes_read = read(fd, file_buffer, BUFSIZ))) {
    if (bytes_read == -1)
      err(errno, "Unable to read file: %s", buffer->filename);

    // Split into lines
    for (i_prev = i = 0; i < bytes_read; i++) {
      if (file_buffer[i] == '\n') {
        l = (Line *)safe_calloc(1, sizeof(Line));
        l->length = 0;
        l->visual_length = 0;
        l->c = NULL;
        l->prev = NULL;
        l->next = NULL;

        // Link lines
        if (l_prev) {
          l_prev->next = l;
          l->prev = l_prev;
        }
        else
          buffer->first_line = l;

        // Copy characters to line
        size_t diff = (i - i_prev);
        size_t offset = ((size_t)file_buffer + i_prev);

        if (line_buffer) {
          line_buffer_capacity += (BUCKETS(diff) * LINSIZ) + 1;
          line_buffer = safe_realloc(line_buffer, line_buffer_capacity);
          line_buffer[line_buffer_capacity] = '\0';
          memset((line_buffer + line_buffer_size), 0, (line_buffer_capacity - line_buffer_size));
          memcpy((line_buffer + line_buffer_size), (void *)offset, diff);

          l->c = line_buffer;
          l->length = (line_buffer_size + diff);
          l->visual_length = utf8_characters(l->c);

          line_buffer = NULL;
          line_buffer_capacity = line_buffer_size = 0;
        }
        else {
          size_t capacity = (BUCKETS(diff) * LINSIZ) + 1;
          l->c = safe_calloc(capacity, sizeof(char));
          memcpy(l->c, (void *)offset, diff);
          l->length = diff;
          l->visual_length = utf8_characters(l->c);
        }

        // Retain previous line for linking
        i_prev = i + 1;
        l_prev = l;
      }
    }

    // Append line_buffer if line wasn't consumed
    if (i_prev < (bytes_read - 1)) {
      size_t diff = (i - i_prev);
      size_t offset = ((size_t)file_buffer + i_prev);

      if (!line_buffer) {
        line_buffer_capacity = (BUCKETS(diff) * LINSIZ);
        line_buffer_size = 0;
        line_buffer = safe_calloc(line_buffer_capacity, sizeof(char));
      }
      else if ((line_buffer_size + diff) >= line_buffer_capacity) {
        // Not enough space in line buffer, add more buckets
        line_buffer_capacity += (BUCKETS(diff) * LINSIZ);
        line_buffer = safe_realloc(line_buffer, line_buffer_capacity);
        memset((line_buffer + line_buffer_size), 0, (line_buffer_capacity - line_buffer_size));
      }

      memcpy((line_buffer + line_buffer_size), (void *)offset, diff);
      line_buffer_size += diff;
    }
  }

  // Dangling line
  if (line_buffer) {
    l = (Line *)safe_calloc(1, sizeof(Line));
    l->length = 0;
    l->visual_length = 0;
    l->c = NULL;
    l->prev = NULL;
    l->next = NULL;

    // Link lines
    if (l_prev) {
      l_prev->next = l;
      l->prev = l_prev;
    }
    else
      buffer->first_line = l;

    l->c = line_buffer;
    l->length = line_buffer_size;
    l->visual_length = utf8_characters(l->c);

    line_buffer = NULL;
    line_buffer_capacity = line_buffer_size = 0;
  }

  if (close(fd) == ERR)
    err(errno, "Unable to close file");

  buffer->last_line = l;
  buffer->cursor->line = buffer->first_line;
  buffer->cursor->offset = 0;
}

void internal_paint() {
  int i;
  Line *l;
  unsigned int total_lines = 0;

  // Paint editor window
  {
    // Naive-implementation (unoptimized, no-scrolling, no-wrapping, paints from 0,0)
    int cols;
    int rows_visible;
    int rows_left;
    Line *l = current_buffer->first_line;

    getmaxyx(editor_window, rows_visible, cols);
    rows_left = rows_visible;

    while (rows_left && l) {
      wchar_t row[cols];
      memset(row, '\0', cols);
      if ((int)mbstowcs(row, l->c, cols) == ERR)
        err(errno, "Unable to convert multi-byte string to widechar string");
      mvwaddwstr(editor_window, (rows_visible - rows_left), 0, row);
      rows_left--;

      l = l->next;
    }
  }

  // Cursor
  if (current_buffer->cursor->line)
    wmove(editor_window, 0, current_buffer->cursor->offset);

  switch (current_mode) {
    case Mode_insert:
      printf(CURSOR_BAR_BLINK);
      break;

    case Mode_replace:
      printf(CURSOR_UNDERLINE_BLINK);
      break;

    case Mode_normal:
    default:
      printf(CURSOR_BLOCK);

  }
  // Paint status window
  wmove(status_window, 0, 0);
  for (i = 0; i < cols; i++)
    waddch(status_window, ' ');
  wclrtoeol(status_window);

  // Count lines
  if ((l = current_buffer->first_line)) {
    total_lines = 1;
    while ((l = l->next))
      total_lines++;
  }

  if (title_temp) {
    snprintf(title, BUFSIZ, "%s", title_temp);
  }
  else {
    snprintf(title, BUFSIZ, "Snack %s (%s) ␤%d,%d:%d",
        (current_buffer->filename ? current_buffer->filename : "<No Name>"),
        (current_mode == Mode_insert ? "Insert" : "Normal"),
        0,
        total_lines,
        (current_buffer->cursor->offset));
  }

  title_temp = NULL;

  mvwaddnstr(status_window, 0, 0, title, cols);

  // Go
  wnoutrefresh(status_window);
  wnoutrefresh(editor_window);
  doupdate();
}

void internal_setup() {
  current_buffer = (Buffer *)safe_calloc(1, sizeof(Buffer));
  current_buffer->cursor = (Position *)safe_calloc(1, sizeof(Position));
  current_mode = Mode_normal;
  current_status = Status_running;

  initscr();
  internal_term();
}

void internal_term() {
  // Initialize terminal
  raw();
  curs_set(1);
  use_default_colors();
  noecho();
  nl();
  set_escdelay(25);
  getmaxyx(stdscr, rows, cols);

  // Editor windows
  if (editor_window)
    delwin(editor_window);

  editor_window = newwin((rows - 1), cols, 1, 0);

  idlok(editor_window, TRUE);
  keypad(editor_window, TRUE);
  meta(editor_window, TRUE);
  nodelay(editor_window, FALSE);
  scrollok(editor_window, FALSE);
  /* wtimeout(editor_window, 0); */

  // Status window
  if (status_window)
    delwin(status_window);

  status_window = newwin(1, cols, 0, 0);

  /* keypad(status_window, TRUE); */
  /* meta(status_window, TRUE); */
  /* nodelay(status_window, FALSE); */
  wattron(status_window, A_REVERSE);
  /* wtimeout(status_window, 0); */
}


/**
 * Safe memory function wrappers
 */

void *safe_calloc(size_t nmemb, size_t size) {
  void *ptr;

  if ((ptr = calloc(nmemb, size)) == NULL)
    err(errno, "Unable to allocate memory (calloc)");

  return ptr;
}

void *safe_malloc(size_t size) {
  void *ptr;

  if ((ptr = malloc(size)) == NULL)
    err(errno, "Unable to allocate memory (malloc)");

  return ptr;
}

void *safe_realloc(void *ptr, size_t size) {
  void *new_ptr;

  if ((new_ptr = realloc(ptr, size)) == NULL)
    err(errno, "Unable to allocate memory (realloc)");

  return new_ptr;
}

static char *safe_strdup(const char *s) {
  char *ptr;

  if ((ptr = strdup(s)) == NULL)
    err(errno, "Unable to duplicate string");

  return ptr;
}
