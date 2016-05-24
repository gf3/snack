#include "snack.h"

/* Actions */
void action_quit() {
  current_status &= ~Status_running;
}

void action_mode_insert() {
  current_mode = Mode_insert;
}

void action_mode_normal() {
  current_mode = Mode_normal;
}

void action_move_nextline(Buffer *b, Selection *s) {
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
}

void action_move_prevline(Buffer *b, Selection *s) {
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
}

void action_move_nextchar(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = c->line;

  if (c->offset < l->visual_length)
    b->offset_prev = ++(c->offset);
}

void action_move_prevchar(Buffer *b, Selection *s) {
  Position *c = b->cursor;

  if (c->offset > 0)
    b->offset_prev = --(c->offset);
}

void action_move_bof(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = b->first_line;

  c->line = l;

  if ((l->visual_length < c->offset) || (l->visual_length < b->offset_prev))
    c->offset = l->visual_length;
  else if (b->offset_prev != c->offset)
    c->offset = b->offset_prev;
}

void action_move_eof(Buffer *b, Selection *s) {
  Position *c = b->cursor;
  Line *l = b->last_line;

  c->line = l;

  if ((l->visual_length < c->offset) || (l->visual_length < b->offset_prev))
    c->offset = l->visual_length;
  else if (b->offset_prev != c->offset)
    c->offset = b->offset_prev;
}

void action_move_bol(Buffer *b, Selection *s) {
  b->offset_prev = b->cursor->offset = 0;
}

void action_move_eol(Buffer *b, Selection *s) {
  b->offset_prev = b->cursor->offset = b->cursor->line->visual_length;
}

/* Key mappings */
static const KeyMapping key_maps[] = {
  // Misc.
  { .mode = Mode_normal, .operator = "q",    .action = action_quit },
  { .mode = Mode_normal, .operator = "i",    .action = action_mode_insert },
  { .mode = Mode_insert, .operator = "\033", .action = action_mode_normal },

  // Movement
  { .mode = Mode_normal, .operator = "h",    .action = action_move_prevchar },
  { .mode = Mode_normal, .operator = "j",    .action = action_move_nextline },
  { .mode = Mode_normal, .operator = "k",    .action = action_move_prevline },
  { .mode = Mode_normal, .operator = "l",    .action = action_move_nextchar },
  { .mode = Mode_normal, .operator = "H",    .action = action_move_bof },
  { .mode = Mode_normal, .operator = "L",    .action = action_move_eof },
  { .mode = Mode_normal, .operator = "G",    .action = action_move_eof },
  { .mode = Mode_normal, .operator = "0",    .action = action_move_bol },
  { .mode = Mode_normal, .operator = "$",    .action = action_move_eol },
};
