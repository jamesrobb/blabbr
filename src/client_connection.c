#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <wchar.h>

#include "client_connection.h"
#include "util.h"

void client_send_welcome(client_connection *client) {
    wchar_t *welcome = L"SERVER Welcome to Blabbr! Type /help for helpful information. Please use '/user <username>' authenticate.";
    int welcome_len = (wcslen(welcome) + 1) * sizeof(wchar_t);
    int client_addr_len;
    struct sockaddr_in client_addr;

    int bio_sent = SSL_write(client->ssl, welcome, welcome_len);

    g_info("bio sent is %d", bio_sent);

    if(bio_sent != (int) welcome_len) {
    // if(send(client->fd, welcome, welcome_len, 0) != (int) welcome_len) {

        memset(&client_addr, 0, sizeof(client_addr));
        getpeername(client->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);

        g_critical("failed to send() welcome message on socket fd %d, ip %s, port %d", 
                    client->fd, 
                    inet_ntoa(client_addr.sin_addr), 
                    ntohs(client_addr.sin_port));
    }
}

void client_connection_init(client_connection *client) {
    client->last_activity = time(NULL);
    client->timeout_notification = FALSE;
    client->fd = CONN_FREE;
    client->ip_address = NULL;
    client->port_number = -1;
    client->close = FALSE;
    client->authenticated = FALSE;
    client->ssl_connected = FALSE;
    client->bio_ssl = NULL;
    client->ssl = NULL;
    client->in_game = FALSE;
    client->game_score = 0;
    client->current_opponent = NULL;
    client->username = NULL;
    client->nickname = NULL;
    client->current_chatroom = NULL;
}

void client_connection_reset(client_connection *client) {
    client->last_activity = time(NULL);
    client->timeout_notification = FALSE;
    client->fd = CONN_FREE;
    client->port_number = -1;
    client->close = FALSE;
    client->authenticated = FALSE;
    client->ssl_connected = FALSE;
    if(client->ssl != NULL) {
        SSL_free(client->ssl);
    }
    client->in_game = FALSE;
    client->game_score = 0;
    
    g_free(client->current_opponent);
    client->current_opponent = NULL;

    g_free(client->ip_address);
    client->ip_address = NULL;

    g_free(client->username);
    client->username = NULL;

    g_free(client->nickname);
    client->nickname = NULL;

    g_free(client->current_chatroom);
    client->current_chatroom = NULL;
}

void client_connection_gtree_key_destroy(gpointer data) {
    g_free((int *) data);
}

void client_connection_gtree_value_destroy(gpointer data) {
    client_connection_reset((client_connection*) data);
    g_free((client_connection*) data);
}