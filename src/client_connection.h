#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <glib.h>
#include <time.h>
#include <wchar.h>

typedef struct _client_connection {
    time_t last_activity;
    int fd;
    gboolean close; // if this flag is set, we know we can issue close() on the connection
    gboolean authenticated;
    wchar_t *username;
} client_connection;

void client_send_welcome(client_connection *client);

void reset_client_connection(client_connection *client);

#endif