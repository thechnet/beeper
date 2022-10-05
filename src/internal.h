/*
internal.h - Beeper
Modified 2022-10-05
*/

#ifndef BP_INTERNAL_H
#define BP_INTERNAL_H

/* Definitions used both in the implementation and by the user. */
#include "shared.h"

/*
*** Constants.
*/

#define BP_SIZE_RECIPIENTS ((size_t)4)
#define BP_SIZE_THEMES ((size_t)8)
#define BP_FORMAT_YEAR "%04d-"
#define BP_FORMAT_YEAR_WIDE L"" BP_FORMAT_YEAR
#define BP_FORMAT_MONTH_AND_DAY "%02d-%02d "
#define BP_FORMAT_MONTH_AND_DAY_WIDE L"" BP_FORMAT_MONTH_AND_DAY
#define BP_FORMAT_HOUR_AND_MINUTE "%02d:%02d"
#define BP_FORMAT_HOUR_AND_MINUTE_WIDE L"" BP_FORMAT_HOUR_AND_MINUTE
#define BP_FORMAT_SECOND ":%02d"
#define BP_FORMAT_SECOND_WIDE L"" BP_FORMAT_SECOND
#define BP_FORMAT_ID "(%s) "
#define BP_FORMAT_ID_WIDE L"(%hs) "
#define BP_FORMAT_STYLE_NAME "[%s] "
#define BP_FORMAT_STYLE_NAME_WIDE L"[%hs] "
#define BP_FORMAT_ORIGIN "%s:%" PRId32 ": "
#define BP_FORMAT_ORIGIN_WIDE L"%hs:%" PRId32 ": "

/*
*** Macros.
*/

#define BP_COLOR_IS_VALID(color_) \
  (color_ == BP_COLOR_default || (color_ >= BP_COLOR_BLACK && color_ <= BP_COLOR_SILVER) || (color_ >= BP_COLOR_GRAY && color_ <= BP_COLOR_WHITE))

/*
*** Internal data types.
*/

typedef struct bp_recipient_ {
  FILE *stream;
  bool wide;
  bool formatted;
} bp_recipient;

typedef struct bp_theme_ {
  char *name;
  bp_style style;
} bp_theme;

typedef struct bp_beeper_ {
  char *identifier;
  bp_recipient *recipients;
  size_t recipients_size;
  bp_theme *themes;
  size_t themes_size;
} bp_beeper_struct;

/*
*** Implementation data declarations.
*/

static bp_beeper_struct *bp_active_beeper;
static bp_theme bp_builtin_themes[];

/*
*** Implementation declarations.
*/

static bp_recipient bp_recipient_reset(void);
static bp_recipient *bp_recipients_get_empty_slot(void);
static bp_recipient *bp_recipient_find_by_stream(FILE *pointer);
static bp_theme bp_theme_reset(void);
static bp_theme *bp_themes_get_empty_slot(void);
static bp_theme *bp_theme_find_by_name(char *name, bool search_builtin);
static void bp_print_message_prelude(bp_recipient recipient, bp_theme theme, char *file, int32_t line);
static void bp_print_message_content(bp_recipient recipient, const void *format, va_list values);
static void bp_print_message_postlude(bp_recipient recipient, bp_style style);

#endif /* !BP_INTERNAL_H */
