#ifndef UTF8_H
#define UTF8_H 1

#include <ncurses.h>

/**
 * Determine the byte length of a UTF8 character based on the first char.
 *
 * @param ch [char] Character to chek
 *
 * @return [unsigned int] Length in bytes of the UTF8 character
 */
unsigned int utf8_width(char ch);

/**
 * Get the UTF8 character count for a string.
 *
 * @param s [char *] NULL-terminated string to count
 *
 * @return [int] Character count or `ERR` if malformed
 */
int utf8_characters(char *s);

/**
 * Get a UTF8 character from the given window.
 *
 * @param window [WINDOW] Window to get the character from
 * @param c [char *] Pointer to copy (potentially) multi-byte character to (e.g. 7-byte string)
 *
 * @return [int] The length in bytes of the character copited, or `ERR` if no input present
 */
int utf8_wgetch(WINDOW *window, char *c);

#endif
