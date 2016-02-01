#include "cmdserv_tokenize.h"

#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Start a new token if the previous character was a space.
 */
#define NEW_TOKEN_AFTER_SPACE() do {            \
    if (space) {                                \
      argv[argc++] = str + dst;                 \
      space = false;                            \
    }                                           \
  } while (0)

/**
 * Wrapping isspace() for use on char.
 *
 * While most libc implementations of ctype, and most environments
 * (compiler/OS), do work with a char handed in to isspace(), and
 * other character classification functions (or, often actually
 * macros), it's not according to the specs, and compilation actually
 * failed on some systems (e.g. some BSDs).  From CTYPE(3) on NetBSD
 * 7.0: "Values of type char or signed char must first be cast to
 * unsigned char, to ensure that the values are within the correct
 * range.  The result should then be cast to int to avoid warnings
 * from some compilers."
 */
#define ISSPACE_CHAR(c) isspace((int)(unsigned char)c)

int cmdserv_tokenize(char *str, char **argv, int argc_max) {
  int argc   = 0;
  bool esc   = false;
  char quote = '\0';
  bool space = true;
  size_t dst = 0;
  
  for (size_t src = 0; str[src] != '\0' && argc < argc_max; src++) {
    
    if (esc) {                       /*-- ESCAPED CHARACTER ---------*/
      NEW_TOKEN_AFTER_SPACE();

      str[dst++] = str[src];
      esc = false;

    } else if (quote) {              /*-- INSIDE QUOTES -------------*/
      NEW_TOKEN_AFTER_SPACE();

      if (str[src] == quote) {       /*     End quoted string        */
        quote = false;

      } else if (str[src] == '\\') { /*     Start an escape sequence */
        esc = true;

      } else {                       /*     Normal character         */
        str[dst++] = str[src];
      }

    } else {                         /*-- NORMAL SEQUENCE -----------*/

      if (ISSPACE_CHAR(str[src])) {  /*     Space separating tokens  */
        space = true;
        str[dst++] = '\0';

      } else if (str[src] == '\\') { /*     Start an escape sequence */
        esc = true;

      } else if (str[src] == '"'     /*     Start a quoted string    */
                 || str[src] == '\'') {
        quote = str[src];

      } else {                       /*     Normal character         */
        NEW_TOKEN_AFTER_SPACE();

        str[dst++] = str[src];
      }
    }                                /*------------------------------*/
  }
  
  if (argc >= argc_max)
    return -1;
  
  str[dst]   = '\0';
  argv[argc] = NULL;
  
  return argc;
}
