#include "snack.h"

/* Compound actions */
bool _mv_nextchar_md_ins(Buffer *b, Selection *s) {
  action_move_nextchar(b, s);
  action_mode_insert(b, s);
  return false;
}

bool _mv_eol_md_ins(Buffer *b, Selection *s) {
  action_move_eol(b, s);
  action_mode_insert(b, s);
  return true;
}

bool _mv_bol_md_ins(Buffer *b, Selection *s) {
  action_move_bol(b, s);
  action_mode_insert(b, s);
  return true;
}

bool _in_line_md_ins(Buffer *b, Selection *s) {
  action_insert_line(b, s);
  action_mode_insert(b, s);
  return false;
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

  // Movement + action
  { .mode = Mode_normal, .operator = "a",    .action = _mv_nextchar_md_ins },
  { .mode = Mode_normal, .operator = "A",    .action = _mv_eol_md_ins },
  { .mode = Mode_normal, .operator = "I",    .action = _mv_bol_md_ins },

  // Insertion + action
  { .mode = Mode_normal, .operator = "o",    .action = _in_line_md_ins },
};
