/*
shared.h - Beeper
Modified 2022-10-03
*/

#ifndef BP_SHARED_H
#define BP_SHARED_H

/* Required for declarations. */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/*
*** Errors.
*/

#define BP_SUCCESS ((int)0) /* No problems were encountered. */
#define BP_INACTIVE ((int)1) /* No active beeper is set. */
#define BP_INVALID_1ST ((int)-2) /* The first argument was invalid. */
#define BP_INVALID_2ND ((int)-3) /* The second argument was invalid. */
#define BP_INVALID_COLOR ((int)-4) /* A color value was invalid. */
#define BP_NO_MEMORY ((int)-5) /* Out of memory. */

/*
*** Interface data types.
*/

typedef enum bp_color_ {
  BP_COLOR_default = 0,
  BP_COLOR_BLACK = 30,
  BP_COLOR_DARK_RED = 31,
  BP_COLOR_DARK_GREEN = 32,
  BP_COLOR_DARK_YELLOW = 33,
  BP_COLOR_DARK_BLUE = 34,
  BP_COLOR_DARK_PURPLE = 35,
  BP_COLOR_DARK_AQUA = 36,
  BP_COLOR_SILVER = 37,
  BP_COLOR_GRAY = 90,
  BP_COLOR_RED = 91,
  BP_COLOR_GREEN = 92,
  BP_COLOR_YELLOW = 93,
  BP_COLOR_BLUE = 94,
  BP_COLOR_PURPLE = 95,
  BP_COLOR_AQUA = 96,
  BP_COLOR_WHITE = 97
} bp_color;

typedef struct bp_style_ {
  bp_color background_color;
  bp_color foreground_color;
  bool bold;
  bool dim;
  bool italic;
  bool underline;
  bool underline_thick;
  bool overline;
  bool blinking;
  bool negative;
  bool strikethrough;
  bool hide_identifier;
  bool show_style;
  bool hide_origin;
  bool no_newline;
  void (*callback)(void);
} bp_style;

/* A handle to a beeper. */
typedef void *bp_beeper;

/*
*** Interface.
*/

bp_beeper bp_beeper_new(char *identifier);
int bp_beeper_select(bp_beeper beeper);
int bp_destroy(void);
int bp_recipient_add(FILE *stream, bool wide, bool formatted);
int bp_recipient_remove(FILE *stream);
int bp_style_set(char *name, bp_style style);
int bp_style_unset(char *name);

/* Do not use this function directly; use the macros provided for it. */
int _bp_beep(bp_beeper with, char *file, int32_t line, char *style_name, const void *format, ...);

#endif /* !BP_SHARED_H */
