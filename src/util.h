#ifndef UTIL_H
#define UTIL_H

#include <glib.h>
#include <wchar.h>

#include "log.h"

#define LONG_STRING_WARNING	100000

int gchar_array_len(gchar *arr);

int gchar_array_array_len(gchar **arr);

void ghash_table_strstr_iterator(gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data);

void ghash_table_gchar_destroy(gpointer value);

void gchar_char_strip(gchar *destination, gchar* source, gchar strip_char);

void gdateweekday_to_gstring(GDateWeekday day, GString *name);

void gdatemonth_to_gstring(GDateMonth mon, GString *name);

void ssl_print_error(int ssl_error);

gchar* wchars_to_gchars(wchar_t *source);

int wint_chars_len(wint_t *arr, int max);

#endif