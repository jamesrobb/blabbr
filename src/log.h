#ifndef LOG_H
#define LOG_H

#include <glib.h>
#include <stdio.h>
#include <stdbool.h>

#include "constants.h"

const gchar * log_level_to_string (GLogLevelFlags level);

void server_log_all_handler_cb (const gchar *log_domain, 
							   GLogLevelFlags log_level, 
							   const gchar *message,
							   gpointer user_data);

void server_log_access(G_GNUC_UNUSED gchar *client_ip, 
					  int client_port,
					  gchar *message);

void client_log_all_handler_cb (const gchar *log_domain, 
							   GLogLevelFlags log_level, 
							   const gchar *message,
							   gpointer user_data);

void write_to_log_file(gchar *file_location, GString *error_string);

#endif