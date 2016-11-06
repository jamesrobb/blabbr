#ifndef CHATROOM_H
#define CHATROOM_H

#include <glib.h>
#include <wchar.h>

// open ssl headers
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

typedef struct _chatroom {
    GList *chatroom_clients;
} chatroom;

void chatroom_gtree_key_destroy(gpointer data);

void chatroom_gtree_value_destroy(gpointer data);

#endif