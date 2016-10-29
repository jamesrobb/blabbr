#include "util.h"

void gdateweekday_to_gstring(GDateWeekday day, GString *name) {
	switch(day) {
		case G_DATE_BAD_WEEKDAY:
			g_string_append(name, "Non");
			return;
		case G_DATE_MONDAY: 
			g_string_append(name, "Mon");
			return;
		case G_DATE_TUESDAY:
			g_string_append(name, "Tue");
			return;
		case G_DATE_WEDNESDAY:
			g_string_append(name, "Wed");
			return;
		case G_DATE_THURSDAY:
			g_string_append(name, "Thu");
			return;
		case G_DATE_FRIDAY:
			g_string_append(name, "Fri");
			return;
		case G_DATE_SATURDAY:
			g_string_append(name, "Sat");
			return;
		case G_DATE_SUNDAY:
			g_string_append(name, "Sun");
			return;
		default:
			g_string_append(name, "Non");
			return;
	}
}

void gdatemonth_to_gstring(GDateMonth mon, GString *name) {
	switch(mon) {
		case G_DATE_BAD_MONTH:
			g_string_append(name, "Non");
			return;
		case G_DATE_JANUARY:
			g_string_append(name, "Jan");
			return;
		case G_DATE_FEBRUARY:
			g_string_append(name, "Feb");
			return;
		case G_DATE_MARCH:
			g_string_append(name, "Mar");
			return;
		case G_DATE_APRIL:
			g_string_append(name, "Apr");
			return;
		case G_DATE_MAY:
			g_string_append(name, "May");
			return;
		case G_DATE_JUNE:
			g_string_append(name, "Jun");
			return;
		case G_DATE_JULY:
			g_string_append(name, "Jul");
			return;
		case G_DATE_AUGUST:
			g_string_append(name, "Aug");
			return;
		case G_DATE_SEPTEMBER:
			g_string_append(name, "Sep");
			return;
		case G_DATE_OCTOBER:
			g_string_append(name, "Oct");
			return;
		case G_DATE_NOVEMBER:
			g_string_append(name, "Nov");
			return;
		case G_DATE_DECEMBER:
			g_string_append(name, "Dec");
			return;
	}
}

int gchar_array_len(gchar *arr) {
	// this function assumes arr is null terminated
	int len = 0;

	while(arr[len] != '\0') {
		len++;

		if(len >= LONG_STRING_WARNING) {
			g_warning("string passed to gchar_array_len is very long");
		}
	}

	return len+1;
}

int gchar_array_array_len(gchar **arr) {
	// returns the length of an array of gchar arrays
	int len = 0;

	while(arr[len]) {
		len++;
	}

	return len;
}

void ghash_table_strstr_iterator(gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data) {
	g_print("field: %s, value: %s\n", (gchar*) key, (gchar*) value);
	return;
}

void ghash_table_gchar_destroy(gpointer value) {
	g_free((gchar *) value);
}

// strips strip_char from the source and saves the stripped array to the destination.
void gchar_char_strip(gchar *destination, gchar* source, gchar strip_char) {
	int length = gchar_array_len(source);
	int destpos = 0;
	for(int i = 0; i < length; i++) {
		if(source[i] != strip_char) {
			destination[destpos] = source[i];
			destpos++;
		}
	}
}