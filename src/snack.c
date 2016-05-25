#define _XOPEN_SOURCE_EXTENDED 1

#include "snack.h"
#include "config.h"

/* Internal functions */
static bool internal_command();                    // Command processing
static void internal_edit();                       // Main edit loop
static void internal_exit();                       // Gracefully exit
static void internal_loadfile(Buffer *buffer);     // Load file
static void internal_paint();                      // Repaint screen
static void internal_setup();                      // Setup editor
static void internal_term();                       // Initialize terminal
static Position *internal_insert(Position *p, char *c, size_t size); // Parse and insert data at position

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
  if (argc > 1)
    current_buffer->filename = safe_strdup(argv[1]);

  internal_loadfile(current_buffer);
  internal_edit();
  internal_exit();

  return EXIT_SUCCESS;
}


/**
 * Internal functions
 */

bool internal_command() {
  unsigned int i;
  int c_length = strlen(c);
  Selection s = {
    .start = current_buffer->cursor,
    .end = current_buffer->cursor
  };

  for (i = 0; i < ARRAY_LENGTH(key_maps); ++i) {
    if (key_maps[i].mode == current_mode) {
      if (strncmp(c, key_maps[i].operator, c_length) == 0) {
        return key_maps[i].action(current_buffer, &s);
      }
    }
  }

  return true;
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

    if (!internal_command())
      continue;

    if (current_mode == Mode_insert) {
      // TODO: Move to action
      current_buffer->cursor = internal_insert(current_buffer->cursor, c, strlen(c));
      current_status |= Status_dirty;
    }
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

Position *internal_insert(Position *p, char *c, size_t size) {
  unsigned int  i;
  unsigned int  i_prev;
  Position     *end_p;
  char         *append_buffer = NULL;
  unsigned int  append_length = 0;
  Line         *l             = p->line;
  Line         *l_next        = NULL;
  unsigned int  p_offset      = p->offset;

  end_p = (Position *)safe_calloc(1, sizeof(Position));
  end_p->line = p->line;
  end_p->offset = p->offset;

  // XXX: Should I require a line or should I insert a line and set it to first
  // if NULL?
  // TODO: Possibly check to see if we're inserting between a multibyte char

  // Split into lines
  for (i_prev = i = 0; i <= size; i++) {
    if ((c[i] == '\n') || (c[i] == '\0')) {
      size_t diff = (i - i_prev);
      size_t buckets = BUCKETS(l->length + diff);
      size_t offset = ((size_t)c + i_prev);

      // Resize
      if (l->buckets < buckets) {
        size_t capacity = (buckets * LINSIZ) + 1;
        l->buckets = buckets;
        l->c = safe_realloc(l->c, capacity);
      }

      // Insert in middle of line?
      if ((int)p_offset != l->visual_length) {
        append_length = (l->length - p_offset);
        append_buffer = safe_calloc(append_length, sizeof(char));
        memcpy(append_buffer, (l->c + p_offset), append_length);
        memset((l->c + p_offset), 0, append_length); // XXX: append_length may need a +1
        l->length -= append_length;
      }

      memcpy((l->c + p_offset), (void *)offset, diff);
      l->length += diff;
      l->visual_length = utf8_characters(l->c);
      l->c[l->length] = '\0';
      l->dirty = true;

      if (c[i] == '\n') {
        l_next = (Line *)safe_calloc(1, sizeof(Line));
        l->dirty = true;
        l_next->length = 0;
        l_next->visual_length = 0;
        l_next->buckets = 1;
        l_next->c = (char *)safe_calloc(LINSIZ + 1, sizeof(char));
        l_next->prev = l;
        l_next->next = l->next;
        l_next->prev->next = l_next;
        if (l_next->next)
          l_next->next->prev = l_next;

        l = l_next;
        i_prev = i + 1; // Skip new line character
      }

      // Insertion point for next line
      p_offset = l->visual_length;
    }
  }

  // Final append
  if (append_buffer) {
    size_t buckets = BUCKETS(l->length + append_length);

    if (buckets < l->buckets) {
      l->buckets = buckets;
      l->c = safe_realloc(l->c, (l->length + append_length + 1));
      memset((l->c + p_offset), 0, (append_length + 1));
    }

    memcpy((l->c + p_offset), append_buffer, append_length);
    l->length += append_length;
    l->visual_length = utf8_characters(l->c);
    l->c[l->length] = '\0';
    l->dirty = true;
    free(append_buffer);
  }

  // Update selection
  end_p->line = l;
  end_p->offset = p_offset;

  return end_p;
}

void internal_loadfile(Buffer *buffer) {
  int       fd;
  ssize_t   bytes_read;
  char      file_buffer[BUFSIZ + 1];
  Position *last_position = buffer->cursor;

  if (!buffer->filename)
    return;

  // TODO: Remove lines in buffer and reset cursor?

  if ((fd = open(buffer->filename, O_RDONLY | O_CREAT)) == -1)
    err(errno, "Unable to open file: %s", buffer->filename);

  while ((bytes_read = read(fd, file_buffer, BUFSIZ))) {
    if (bytes_read == -1)
      err(errno, "Unable to read file: %s", buffer->filename);
    file_buffer[BUFSIZ] = '\0';
    last_position = internal_insert(last_position, file_buffer, bytes_read);
  }

  if (close(fd) == ERR)
    err(errno, "Unable to close file");

  buffer->last_line = last_position->line;;
  buffer->cursor->line = buffer->first_line;
  buffer->cursor->offset = 0;
}

void internal_paint() {
  int i;
  Line *l;
  int cursor_row = 0;
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
      /* if (l->dirty) { */
        wchar_t row[cols];
        memset(row, '\0', cols);
        if ((int)mbstowcs(row, l->c, cols) == ERR)
          err(errno, "Unable to convert multi-byte string to widechar string");
        wmove(editor_window, (rows_visible - rows_left), 0);
        wclrtoeol(editor_window);
        waddwstr(editor_window, row);
        /* mvwaddwstr(editor_window, (rows_visible - rows_left), 0, row); */
        /* l->dirty = false; */
      /* } */

      if (l == current_buffer->cursor->line)
        cursor_row = (rows_visible - rows_left);

      rows_left--;

      l = l->next;
    }
  }

  // Cursor
  if (current_buffer->cursor->line)
    wmove(editor_window, cursor_row, current_buffer->cursor->offset);

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

  // Count lines
  // TODO: Cache
  if ((l = current_buffer->first_line)) {
    total_lines = 1;
    while ((l = l->next))
      total_lines++;
  }

  if (title_temp) {
    snprintf(title, BUFSIZ, "%s", title_temp);
  }
  else {
    snprintf(title, BUFSIZ, "Snack %s%s (%s) ␤%d,%d:%d",
        (current_buffer->filename ? current_buffer->filename : "<No Name>"),
        (current_status & Status_dirty ? "[+]" : ""),
        (current_mode == Mode_insert ? "Insert" : "Normal"),
        (cursor_row + 1),
        total_lines,
        (current_buffer->cursor->offset + 1));
  }

  title_temp = NULL;

  mvwaddnstr(status_window, 0, 0, title, cols);

  // Go
  wnoutrefresh(status_window);
  wnoutrefresh(editor_window);
  doupdate();
}

void internal_setup() {
  Line *l = (Line *)safe_calloc(1, sizeof(Line));
  l->dirty = true;
  l->c = (char *)safe_calloc(LINSIZ + 1, sizeof(char));
  l->buckets = 1;
  l->length = 0;
  l->visual_length = 0;
  l->prev = NULL;
  l->next = NULL;

  current_buffer = (Buffer *)safe_calloc(1, sizeof(Buffer));
  current_buffer->cursor = (Position *)safe_calloc(1, sizeof(Position));
  current_buffer->cursor->offset = 0;
  current_buffer->cursor->line = l;
  current_buffer->offset_prev = 0;
  current_buffer->first_line = l;
  current_buffer->last_line = l;
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


/**
 * Actions
 */

bool action_quit() {
  current_status &= ~Status_running;
  return true;
}

bool action_mode_insert() {
  current_mode = Mode_insert;
  return false;
}

bool action_mode_normal() {
  current_mode = Mode_normal;
  return false;
}

bool action_move_nextline(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = c->line;

  if (l->next) {
    c->line = l->next;

    if ((l->next->visual_length < c->offset) || (l->next->visual_length < b->offset_prev))
      c->offset = l->next->visual_length;
    else if (b->offset_prev != c->offset)
      c->offset = b->offset_prev;
  }
  else
    b->offset_prev = c->offset = l->visual_length;

  return true;
}

bool action_move_prevline(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = c->line;

  if (l->prev) {
    c->line = l->prev;

    if ((l->prev->visual_length < c->offset) || (l->prev->visual_length < b->offset_prev))
      c->offset = l->prev->visual_length;
    else if (b->offset_prev != c->offset)
      c->offset = b->offset_prev;
  }
  else
    b->offset_prev = c->offset = 0;

  return true;
}

bool action_move_nextchar(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = c->line;

  if (c->offset < l->visual_length)
    b->offset_prev = ++(c->offset);

  return true;
}

bool action_move_prevchar(Buffer *b, Selection *s) {
  Position *c = b->cursor;

  if (c->offset > 0)
    b->offset_prev = --(c->offset);

  return true;
}

bool action_move_bof(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = b->first_line;

  c->line = l;

  if ((l->visual_length < c->offset) || (l->visual_length < b->offset_prev))
    c->offset = l->visual_length;
  else if (b->offset_prev != c->offset)
    c->offset = b->offset_prev;

  return true;
}

bool action_move_eof(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = b->last_line;

  c->line = l;

  if ((l->visual_length < c->offset) || (l->visual_length < b->offset_prev))
    c->offset = l->visual_length;
  else if (b->offset_prev != c->offset)
    c->offset = b->offset_prev;

  return true;
}

bool action_move_bol(Buffer *b, Selection *s) {
  b->offset_prev = b->cursor->offset = 0;
  return true;
}

bool action_move_eol(Buffer *b, Selection *s) {
  b->offset_prev = b->cursor->offset = b->cursor->line->visual_length;
  return true;
}

bool action_insert_line(Buffer *b, Selection *s) {
  action_move_eol(b, s);

  b->cursor = internal_insert(b->cursor, "\n", 1);

  current_status |= Status_dirty;

  action_move_bol(b, s);

  // XXX: This might not work if the cursor is moved, maybe repaint whole screen?
  wclrtobot(editor_window);

  return true;
}
