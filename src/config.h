#include "snack.h"

/* Actions */
void action_quit(Buffer *b, Selection *s) {
  current_status &= ~Status_running;
}

void action_mode_insert() {
  current_mode = Mode_insert;
}

void action_mode_normal() {
  current_mode = Mode_normal;
}

/* Key mappings */
static const KeyMapping key_maps[] = {
  { .mode = Mode_normal, .operator = "q",    .action = action_quit },
  { .mode = Mode_normal, .operator = "i",    .action = action_mode_insert },
  { .mode = Mode_insert, .operator = "\033", .action = action_mode_normal }
};
