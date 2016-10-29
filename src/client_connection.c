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

	if(send(client->fd, welcome, welcome_len, 0) != (int) welcome_len) {

		memset(&client_addr, 0, sizeof(client_addr));
		getpeername(client->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);

		g_critical("failed to send() welcome message on socket fd %d, ip %s, port %d", 
					client->fd, 
					inet_ntoa(client_addr.sin_addr), 
					ntohs(client_addr.sin_port));
	}

}

void reset_client_connection(client_connection *client) {
    client->last_activity = time(NULL);
    client->fd = CONN_FREE;
    client->close = FALSE;
    client->authenticated = FALSE;

    if(client->username != NULL) {
    	g_free(client->username);
    }
}