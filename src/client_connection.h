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
    gboolean timeout_notification;
    int fd;
    int port_number;
    gboolean close; // if this flag is set, we know we can issue close() on the connection
    gboolean authenticated;
    gboolean ssl_connected;
    BIO *bio_ssl;
    SSL *ssl;
    gboolean in_game;
    int game_score;
    short auth_attempts;
    wchar_t *current_opponent;
    wchar_t *ip_address;
    wchar_t *username;
    //wchar_t *nickname; // future: allow users to use nicknames
    wchar_t *current_chatroom;
} client_connection;

void client_send_welcome(client_connection *client);

void client_connection_init(client_connection *client);

void client_connection_reset(client_connection *client);

// this is the GDestroyFunc for the keys of the GTree of client connections
void client_connection_gtree_key_destroy(gpointer data);

// this is the GDestroyFunc for the values of the GTree of client connections
void client_connection_gtree_value_destroy(gpointer data);

#endif