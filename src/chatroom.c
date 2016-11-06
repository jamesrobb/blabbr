#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <wchar.h>

#include "chatroom.h"
#include "util.h"

void chatroom_gtree_key_destroy(gpointer data) {
	g_free((wchar_t *) data);
}

void chatroom_gtree_value_destroy(gpointer data) {
	GList **list_pointer = (GList**) data;
	GList *list = *list_pointer;
	g_list_free(list);
	g_free(list_pointer);
}