#include <glib.h>
#include <stdlib.h>
#include <locale.h>

#include "log.h"

const gchar * log_level_to_string (GLogLevelFlags level) {
  
	switch (level) {
		case G_LOG_LEVEL_ERROR: return "ERROR";
		case G_LOG_LEVEL_CRITICAL: return "CRITICAL";
		case G_LOG_LEVEL_WARNING: return "WARNING";
		case G_LOG_LEVEL_MESSAGE: return "MESSAGE";
		case G_LOG_LEVEL_INFO: return "INFO";
		case G_LOG_LEVEL_DEBUG: return "DEBUG";
		default: return "UNKNOWN";

	}

	return "UNKNOWN";
}

void server_log_all_handler_cb (G_GNUC_UNUSED const gchar *log_domain, 
						       GLogLevelFlags log_level,
						       const gchar *message,
						       G_GNUC_UNUSED gpointer user_data) {

	// for unicode purposes
    setlocale(LC_ALL, "en_US.utf8");

	// we get the current time
	GDateTime *now = g_date_time_new_now_utc();

	// format a log message given the current time and log messaged passed to this function
	GString *error_string = g_string_new(NULL);
	g_string_printf(error_string,
					"%02d-%02d-%02dT%02d:%02d:%02dZ %s: %s \n", 
					g_date_time_get_year(now),
					g_date_time_get_month(now),
					g_date_time_get_day_of_month(now),
					g_date_time_get_hour(now),
					g_date_time_get_minute(now),
					g_date_time_get_second(now),
					log_level_to_string(log_level), 
					message);
	
	// print log message out to screen
	g_print("%s", error_string->str);

	write_to_log_file(SERVER_DEBUG_LOG_FILE_LOCATION, error_string);

	g_date_time_unref(now);
	g_string_free(error_string, TRUE);

	return;
}

void server_log_access(gchar *client_ip,
					  int client_port, 
					  gchar *message) {

	// for unicode purposes
    setlocale(LC_ALL, "en_US.utf8");

	// we get the current time
	GDateTime *now = g_date_time_new_now_utc();

	// format a log message given the current time and log messaged passed to this function
	GString *access_string = g_string_new(NULL);
	g_string_printf(access_string,
					"%02d-%02d-%02dT%02d:%02d:%02dZ %s:%d %s \n", 
					g_date_time_get_year(now),
					g_date_time_get_month(now),
					g_date_time_get_day_of_month(now),
					g_date_time_get_hour(now),
					g_date_time_get_minute(now),
					g_date_time_get_second(now),
					client_ip,
					client_port,
					message);
	
	// print log message out to screen
	g_print("%s", access_string->str);

	g_string_append(access_string, "\n");
	write_to_log_file(SERVER_ACCESS_LOG_FILE_LOCATION, access_string);

	g_date_time_unref(now);
	g_string_free(access_string, TRUE);

	return;

}

void client_log_all_handler_cb (G_GNUC_UNUSED const gchar *log_domain, 
						       GLogLevelFlags log_level,
						       const gchar *message,
						       gpointer user_data) {

	// for unicode purposes
    setlocale(LC_ALL, "en_US.utf8");

	// we get the current time
	GDateTime *now = g_date_time_new_now_utc();

	// format a log message given the current time and log messaged passed to this function
	GString *error_string = g_string_new(NULL);
	g_string_printf(error_string,
					"%02d-%02d-%02dT%02d:%02d:%02dZ %s: %s", 
					g_date_time_get_year(now),
					g_date_time_get_month(now),
					g_date_time_get_day_of_month(now),
					g_date_time_get_hour(now),
					g_date_time_get_minute(now),
					g_date_time_get_second(now),
					log_level_to_string(log_level), 
					message);


	// we add log message to text area
	if(user_data != NULL) {
		wchar_t *w_error = malloc(sizeof(wchar_t) * (error_string->len + 1));
		mbstowcs(w_error, error_string->str, error_string->len + 1);

		GSList **text_area_lines = (GSList **)user_data;
		*text_area_lines = g_slist_append(*text_area_lines, w_error);
	}

	g_string_append(error_string, "\n");
	write_to_log_file(CLIENT_DEBUG_LOG_FILE_LOCATION, error_string);

	g_date_time_unref(now);
	g_string_free(error_string, TRUE);

	return;
}

void write_to_log_file(gchar *file_location, GString *error_string) {

	// write log message to file
	FILE *log_fp;
	log_fp = fopen(file_location, "a");
	fwrite(error_string->str, (size_t) sizeof(gchar), (size_t) error_string->len, log_fp);
	fclose(log_fp);

	return;
}