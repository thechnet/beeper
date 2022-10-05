/*
beeper.c - Beeper
Modified 2022-10-05
*/

/* Interface headers. */
#include "internal.h"

/* Implementation headers. */
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/*
*** Interface.
*/

/*
Create a beeper and initialize it.
*/
bp_beeper bp_beeper_new(char *identifier)
{
  /* Allocate memory for beeper. */
  bp_beeper_struct *beeper = malloc(sizeof(*beeper));
  if (!beeper)
    return NULL;
  
  /* Duplicate identifier string. */
  char *identifier_duplicate = strdup(identifier);
  if (!identifier_duplicate)
    return NULL;
  
  /* Allocate memory for recipients and initialize them. */
  size_t recipients_size = BP_SIZE_RECIPIENTS;
  bp_recipient *recipients = malloc(recipients_size*sizeof(*recipients));
  if (!recipients)
    return NULL;
  for (size_t i=0; i<recipients_size; i++)
    recipients[i] = bp_recipient_reset();
  
  /* Allocate memory for themes and initialize them. */
  size_t themes_size = BP_SIZE_THEMES;
  bp_theme *themes = malloc(themes_size*sizeof(*themes));
  if (!themes)
    return NULL;
  for (size_t i=0; i<themes_size; i++)
    themes[i] = bp_theme_reset();
  
  /* Initialize beeper. */
  *beeper = (bp_beeper_struct){
    .identifier = identifier_duplicate,
    .recipients = recipients,
    .recipients_size = recipients_size,
    .themes = themes,
    .themes_size = themes_size
  };
  
  /* Register new beeper as active beeper. */
  bp_active_beeper = beeper;
  
  return (bp_beeper)beeper;
}

/*
Select a beeper as active beeper.
*/
int bp_beeper_select(bp_beeper beeper)
{
  if (!beeper)
    return BP_INVALID_1ST;
  
  bp_active_beeper = (bp_beeper_struct*)beeper;
  
  return BP_SUCCESS;
}

/*
Free the active beeper and all its members. The active beeper is reset to NULL.
*/
int bp_destroy(void)
{
  if (!bp_active_beeper)
    return BP_INACTIVE;
  bp_beeper_struct *beeper = (bp_beeper_struct*)bp_active_beeper;
  
  /* Free the active beeper. */
  free(beeper->identifier);
  free(beeper->recipients);
  for (size_t i=0; i<beeper->themes_size; i++)
    if (beeper->themes[i].name)
      free(beeper->themes[i].name);
  free(beeper->themes);
  free(beeper);
  
  /* Reset active beeper. */
  bp_active_beeper = NULL;
  
  return BP_SUCCESS;
}

/*
Add a recipient to the active beeper.
*/
int bp_recipient_add(FILE *stream, bool wide, bool formatted)
{
  if (!bp_active_beeper)
    return BP_INACTIVE;
  if (!stream)
    return BP_INVALID_1ST;
  
  /* Get slot for recipient. */
  bp_recipient *recipient = bp_recipients_get_empty_slot();
  if (!recipient)
    return BP_NO_MEMORY;
  
  /* Register recipient. */
  *recipient = (bp_recipient){
    .stream = stream,
    .wide = wide,
    .formatted = formatted
  };
  
  return BP_SUCCESS;
}

/*
Remove a recipient from the active beeper.
*/
int bp_recipient_remove(FILE *stream)
{
  if (!bp_active_beeper)
    return BP_INACTIVE;
  if (!stream)
    return BP_INVALID_1ST;
  
  /* Get pointer to recipient. */
  bp_recipient *recipient = bp_recipient_find_by_stream(stream);
  if (!recipient)
    return BP_INVALID_1ST;
  
  /* Reset slot. */
  *recipient = bp_recipient_reset();
  
  return BP_SUCCESS;
}

/*
Bind a style in the active beeper.
*/
int bp_style_set(char *name, bp_style style)
{
  if (!bp_active_beeper)
    return BP_INACTIVE;
  if (!name)
    return BP_INVALID_1ST;
  if (!BP_COLOR_IS_VALID(style.foreground_color) || !BP_COLOR_IS_VALID(style.background_color))
    return BP_INVALID_COLOR;
  
  bp_theme *slot = NULL;
  
  /* If theme with provided name already exists, overwrite it; ... */
  slot = bp_theme_find_by_name(name, false);
  
  /* ... otherwise, create a new one. */
  if (!slot) {
    if (!(slot = bp_themes_get_empty_slot()))
      return BP_NO_MEMORY;
    if (!(slot->name = strdup(name)))
      return BP_NO_MEMORY;
  }
  
  /* Bind style. */
  slot->style = style;
  
  return BP_SUCCESS;
}

/*
Unbind a style in the active beeper.
*/
int bp_style_unset(char *name)
{
  if (!bp_active_beeper)
    return BP_INACTIVE;
  if (!name)
    return BP_INVALID_1ST;
  
  /* Get pointer to theme. */
  bp_theme *theme = bp_theme_find_by_name(name, false);
  if (!theme)
    return BP_INVALID_1ST;
  
  /* Reset slot. */
  *theme = bp_theme_reset();
  
  return BP_SUCCESS;
}

/*
Print message. This function should not be used directly; use the macros defined
in beeper.h.
*/
int _bp_beep(bp_beeper with, char *file, int32_t line, char *style_name, const void *format, ...)
{
  if (!bp_active_beeper)
    return BP_INACTIVE;
  assert(style_name);
  
  /* Get beeper. */
  bp_beeper_struct *beeper = (bp_beeper_struct*)(with ? with : bp_active_beeper);
  if (!beeper)
    return with ? BP_INVALID_1ST : BP_INACTIVE;
  
  /* Get theme. */
  bp_theme *theme = bp_theme_find_by_name(style_name, true);
  if (!theme)
    return with ? BP_INVALID_2ND : BP_INVALID_1ST;
  
  /* Initialize variable arguments. */
  va_list values;
  
  /* Print message content. */
  for (size_t i=0; i<beeper->recipients_size; i++)
    if (beeper->recipients[i].stream) {
      bp_print_message_prelude(beeper->recipients[i], *theme, file, line);
      va_start(values, format);
      bp_print_message_content(beeper->recipients[i], format, values);
      bp_print_message_postlude(beeper->recipients[i], theme->style);
    }
  
  /* Finalize variable arguments. */
  va_end(values);
  
  /* Invoke callback. */
  if (theme->style.callback)
    (*theme->style.callback)();
  
  return BP_SUCCESS;
}

/*
*** Implementation data.
*/

/*
Holds a pointer to the currently active beeper. All operations are performed
on this beeper.
*/
static bp_beeper_struct *bp_active_beeper = NULL;

/*
The built-in themes. These are available as baseline to all beepers.
The user can bind their own styles, which take precedence over the built-
in ones.
*/
static bp_theme bp_builtin_themes[] = {
  {
    .name = "success",
    .style = {
      .foreground_color = BP_COLOR_GREEN
    }
  },
  {
    .name = "info",
    .style = {
      .foreground_color = BP_COLOR_BLUE
    }
  },
  {
    .name = "warn",
    .style = {
      .foreground_color = BP_COLOR_YELLOW,
      .bold = true
    }
  },
  {
    .name = "fail",
    .style = {
      .foreground_color = BP_COLOR_RED,
      .bold = true,
      .callback = &abort
    }
  },
  {
    .name = "debug",
    .style = {
      .underline = true,
      .negative = true
    }
  },
};

/*
*** Implementation.
*/

/*
Return an empty recipient.
*/
static bp_recipient bp_recipient_reset(void)
{
  return (bp_recipient){
    .stream = NULL,
    .wide = false,
    .formatted = false
  };
}

/*
Find or create an empty slot in the active beepers recipients and return it.
*/
static bp_recipient *bp_recipients_get_empty_slot(void)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  bp_beeper_struct *beeper = (bp_beeper_struct*)bp_active_beeper;
  
  bp_recipient *recipient = NULL;
  
  /* Find empty slot in existing array. */
  for (size_t i=0; i<beeper->recipients_size; i++)
    if (!beeper->recipients[i].stream) {
      recipient = beeper->recipients+i;
      break;
    }
  
  /* No empty slot exists, grow array. */
  if (!recipient) {
    size_t recipients_size_new = 2*beeper->recipients_size;
    bp_recipient *recipients_new = realloc(beeper->recipients, recipients_size_new*sizeof(*recipients_new));
    if (!recipients_new)
      return NULL;
    for (size_t i=beeper->recipients_size; i<recipients_size_new; i++)
      recipients_new[i] = bp_recipient_reset();
    recipient = beeper->recipients+beeper->recipients_size;
    beeper->recipients_size = recipients_size_new;
    beeper->recipients = recipients_new;
  }
  
  assert(recipient);
  return recipient;
}

/*
Find a recipient by stream. The function prefers custom themes over built-in ones.
*/
static bp_recipient *bp_recipient_find_by_stream(FILE *stream)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  assert(stream);
  
  /* Get the active beeper. */
  bp_beeper_struct *beeper = (bp_beeper_struct*)bp_active_beeper;
  
  bp_recipient *recipient = NULL;
  
  /* Search recipients. */
  for (size_t i=0; i<beeper->recipients_size; i++)
    if (beeper->recipients[i].stream == stream) {
      recipient = beeper->recipients+i;
      break;
    }
  
  return recipient;
}

/*
Return an empty theme.
*/
static bp_theme bp_theme_reset(void)
{
  return (bp_theme){
    .name = NULL,
    .style = (bp_style){ 0 }
  };
}

/*
Find or create an empty slot in the active beeper's themes and return it.
*/
static bp_theme *bp_themes_get_empty_slot(void)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  bp_beeper_struct *beeper = (bp_beeper_struct*)bp_active_beeper;
  
  bp_theme *theme = NULL;
  
  /* Find empty slot in existing array. */
  for (size_t i=0; i<beeper->themes_size; i++)
    if (!beeper->themes[i].name) {
      theme = beeper->themes+i;
      break;
    }
  
  /* No empty slot exists, grow array. */
  if (!theme) {
    size_t themes_size_new = 2*beeper->themes_size;
    bp_theme *themes_new = realloc(beeper->themes, themes_size_new*sizeof(*themes_new));
    if (!themes_new)
      return NULL;
    for (size_t i=beeper->themes_size; i<themes_size_new; i++)
      themes_new[i] = bp_theme_reset();
    theme = beeper->themes+beeper->themes_size;
    beeper->themes_size = themes_size_new;
    beeper->themes = themes_new;
  }
  
  assert(theme);
  return theme;
}

/*
Find a theme by name. The function prefers custom themes over built-in ones.
*/
static bp_theme *bp_theme_find_by_name(char *name, bool search_builtin)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  assert(name);
  
  /* Get the active beeper. */
  bp_beeper_struct *beeper = (bp_beeper_struct*)bp_active_beeper;
  
  bp_theme *theme = NULL;
  
  /* Search custom themes. */
  for (size_t i=0; i<beeper->themes_size; i++)
    if (beeper->themes[i].name && !strcmp(beeper->themes[i].name, name)) {
      theme = beeper->themes+i;
      break;
    }
  
  /* Search built-in themes. */
  if (!theme && search_builtin)
    for (size_t i=0; i<sizeof(bp_builtin_themes)/sizeof(*bp_builtin_themes); i++)
      if (bp_builtin_themes[i].name && !strcmp(bp_builtin_themes[i].name, name)) {
        theme = bp_builtin_themes+i;
        break;
      }
  
  return theme;
}

/*
Print the message prelude.
*/
static void bp_print_message_prelude(bp_recipient recipient, bp_theme theme, char *file, int32_t line)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  assert(recipient.stream);
  
  /* Get active beeper. */
  bp_beeper_struct *beeper = (bp_beeper_struct*)bp_active_beeper;
  
  bp_style style = theme.style;
  
  /* Get time. */
  time_t timestamp = time(NULL);
  struct tm *t = localtime(&timestamp);
  
  /* Print message prelude. */
  if (recipient.wide) {
    if (recipient.formatted)
      fwprintf(
        recipient.stream,
        L"\033[0m\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm",
        style.background_color != BP_COLOR_default ? style.background_color + 10 : 49,
        style.foreground_color != BP_COLOR_default ? style.foreground_color : 39,
        style.bold ? 1 : (style.dim ? 2 : 22),
        style.italic ? 3 : 23,
        style.underline_thick ? 21 : (style.underline ? 4 : 24),
        style.overline ? 53 : 55,
        style.blinking ? 5 : 25,
        style.negative ? 7 : 27,
        style.strikethrough ? 9 : 29
      );
    if (style.show_date) {
      if (!style.hide_year)
        fwprintf(recipient.stream, BP_FORMAT_YEAR_WIDE, 1900+t->tm_year);
      fwprintf(recipient.stream, BP_FORMAT_MONTH_AND_DAY_WIDE, 1+t->tm_mon, t->tm_mday);
    }
    if (style.show_time) {
      fwprintf(recipient.stream, BP_FORMAT_HOUR_AND_MINUTE_WIDE, t->tm_hour, t->tm_min);
      if (!style.hide_second)
        fwprintf(recipient.stream, BP_FORMAT_SECOND_WIDE, t->tm_sec);
      fputwc(L' ', recipient.stream);
    }
    if (!style.hide_identifier)
      fwprintf(recipient.stream, BP_FORMAT_ID_WIDE, beeper->identifier);
    if (style.show_style)
      fwprintf(recipient.stream, BP_FORMAT_STYLE_NAME_WIDE, theme.name);
    if (!style.hide_origin)
      fwprintf(recipient.stream, BP_FORMAT_ORIGIN_WIDE, file, line);
  } else {
    if (recipient.formatted)
      fprintf(
        recipient.stream,
        "\033[0m\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm\033[%dm",
        style.background_color != BP_COLOR_default ? style.background_color + 10 : 49,
        style.foreground_color != BP_COLOR_default ? style.foreground_color : 39,
        style.bold ? 1 : (style.dim ? 2 : 22),
        style.italic ? 3 : 23,
        style.underline_thick ? 21 : (style.underline ? 4 : 24),
        style.overline ? 53 : 55,
        style.blinking ? 5 : 25,
        style.negative ? 7 : 27,
        style.strikethrough ? 9 : 29
      );
    if (style.show_date) {
      if (!style.hide_year)
        fprintf(recipient.stream, BP_FORMAT_YEAR, 1900+t->tm_year);
      fprintf(recipient.stream, BP_FORMAT_MONTH_AND_DAY, 1+t->tm_mon, t->tm_mday);
    }
    if (style.show_time) {
      fprintf(recipient.stream, BP_FORMAT_HOUR_AND_MINUTE, t->tm_hour, t->tm_min);
      if (!style.hide_second)
        fprintf(recipient.stream, BP_FORMAT_SECOND, t->tm_sec);
      fputc(' ', recipient.stream);
    }
    if (!style.hide_identifier)
      fprintf(recipient.stream, BP_FORMAT_ID, beeper->identifier);
    if (style.show_style)
      fprintf(recipient.stream, BP_FORMAT_STYLE_NAME, theme.name);
    if (!style.hide_origin)
      fprintf(recipient.stream, BP_FORMAT_ORIGIN, file, line);
  }
}

/*
Print the message content.
*/
static void bp_print_message_content(bp_recipient recipient, const void *format, va_list values)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  assert(format);
  
  /* Print message content. */
  if (recipient.wide)
    vfwprintf(recipient.stream, (const wchar_t*)format, values);
  else
    vfprintf(recipient.stream, (const char*)format, values);
}

/*
Print the message postlude.
*/
static void bp_print_message_postlude(bp_recipient recipient, bp_style style)
{
  /* Verify that this function runs under the right circumstances. */
  assert(bp_active_beeper);
  
  /* Print message postlude. */
  if (recipient.wide) {
    if (recipient.formatted)
      fputws(L"\033[0m", recipient.stream);
    if (!style.no_newline)
      fputwc(L'\n', recipient.stream);
  } else {
    if (recipient.formatted)
      fputs("\033[0m", recipient.stream);
    if (!style.no_newline)
      fputc('\n', recipient.stream);
  }
}
