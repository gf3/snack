#include "utf8.h"

unsigned int utf8_width(char ch) {
  if (~ch & 0x80)
    return 1;
  if (~ch & 0x20)
    return 2;
  if (~ch & 0x10)
    return 3;
  if (~ch & 0x08)
    return 4;
  if (~ch & 0x04)
    return 5;
  if (~ch & 0x02)
    return 6;
  return 1;
}

int utf8_characters(char *s) {
  unsigned int i = 0;
  unsigned int length = 0;
  unsigned int offset;

  while (s[i++] != '\0') {
    length++;

    if ((offset = utf8_width(s[i - 1])) > 1) {
      while (--offset) {
        if ((s[i++] & 0xC0) != 0x80)
          return ERR;
      }
    }
  }

  return length;
}

int utf8_wgetch(WINDOW *window, char *c) {
  unsigned int i;
  unsigned int c_width;
  char ch = wgetch(window);

  if (ch == ERR)
    return ERR;

  c[0] = ch;

  // Grab full utf8 character
  if ((c_width = utf8_width(ch)) > 1) {
    for (i = 1; i < c_width; i++) {
      if ((c[i] = wgetch(window)) == ERR)
        break;
    }
    for (; i < 7; i++)
      c[i] = '\0';
  }
  else
    c[1] = c[2] = c[3] = c[4] = c[5] = c[6] = '\0';

  return c_width;
}
