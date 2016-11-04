#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <wchar.h>

#include "client_connection.h"
#include "util.h"

void client_send_welcome(client_connection *client) {

	wchar_t *welcome = L"Welcome to Blabbr! Please authenticate.";
	int welcome_len = 40 * sizeof(wchar_t);
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
    client->fd = CONN_FREE;
    client->close = FALSE;
    client->authenticated = FALSE;
    client->ssl_connected = FALSE;
    client->bio_ssl = NULL;
    client->ssl = NULL;
    client->username = NULL;
}

void client_connection_reset(client_connection *client) {
    client->last_activity = time(NULL);
    client->fd = CONN_FREE;
    client->close = FALSE;
    client->authenticated = FALSE;
    client->ssl_connected = FALSE;

    // if(client->bio != NULL) {
    // 	BIO_vfree(client->bio);
    // }

    if(client->ssl != NULL) {
    	SSL_free(client->ssl);
    }

    if(client->username != NULL) {
    	g_free(client->username);
    }
}

void client_connection_gtree_key_destroy(gpointer data) {
    g_free((int *) data);
}

void client_connection_gtree_value_destroy(gpointer data) {
    client_connection_reset((client_connection*) data);
    g_free((client_connection*) data);
}