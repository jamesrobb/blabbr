#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <wchar.h>

#include "chatroom.h"
#include "util.h"


void chatroom_gtree_value_destroy(gpointer data) {
	g_list_free(data);
}