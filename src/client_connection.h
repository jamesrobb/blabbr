#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <glib.h>
#include <time.h>
#include <wchar.h>

// open ssl headers
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>


typedef struct _client_connection {
    time_t last_activity;
    int fd;
    gboolean close; // if this flag is set, we know we can issue close() on the connection
    gboolean authenticated;
    gboolean ssl_connected;
    BIO *bio_ssl;
    SSL *ssl;
    wchar_t *username;
} client_connection;

void client_send_welcome(client_connection *client);

void client_connection_init(client_connection *client);

void client_connection_reset(client_connection *client);

#endif