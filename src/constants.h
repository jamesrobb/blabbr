#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BLABBR_VERSION							((gchar *) "0.1")
#define SERVER_DEBUG_LOG_FILE_LOCATION			((gchar *) "./server_debug.log")
#define SERVER_ACCESS_LOG_FILE_LOCATION			((gchar *) "./server_access.log")
#define CLIENT_DEBUG_LOG_FILE_LOCATION			((gchar *) "./client_debug.log")
#define BLABBR_LINES_MAX						10
#define WCHAR_STR_MAX							4000
#define TCP_BUFFER_LENGTH						15000 // assumes 15000 wchar_t characters
#define SERVER_MAX_CONN_BACKLOG					50
#define CONN_FREE								-1
#define CONNECTION_TIMEOUT						60

#endif