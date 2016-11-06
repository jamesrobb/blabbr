/* A TCP echo server with timeouts.
 *
 * Note that you will not need to use select and the timeout for a
 * tftp server. However, select is also useful if you want to receive
 * from multiple sockets at the same time. Read the documentation for
 * select on how to do this (Hint: Iterate with FD_ISSET()).
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <math.h>
#include <glib.h>
#include <locale.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <wchar.h>
#include <linux/random.h>

// open ssl headers
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include "client_connection.h"
#include "chatroom.h"
#include "constants.h"
#include "log.h"
#include "util.h"

// GLOBAL VARIABLES
static int exit_fd[2];

#define  SERVER_CERT            "./blabbr.crt"
#define  SERVER_PKEY            "./blabbr.key"

// FUNCTION DECLERATIONS
gint g_tree_wchar_cmp(gconstpointer a,  gconstpointer b, G_GNUC_UNUSED gpointer user_data);

gint g_tree_cmp(gconstpointer a,  gconstpointer b, G_GNUC_UNUSED gpointer user_data);

void send_message_to_chatroom(gpointer data, gpointer user_data);

gboolean print_available_users(G_GNUC_UNUSED gpointer key, gpointer value, gpointer data);

gboolean print_chatroom_array(gpointer key, G_GNUC_UNUSED gpointer value, gpointer data);

gboolean populate_client_fd_array(gpointer key, G_GNUC_UNUSED gpointer value, gpointer data);

void signal_handler(int signum);

static void initialize_exit_fd(void);

int pem_passwd_cb(char *buf, G_GNUC_UNUSED int size, G_GNUC_UNUSED int rwflag, G_GNUC_UNUSED void *userdata);

// MAIN
int main(int argc, char **argv) {

    // for unicode purposes
    setlocale(LC_ALL, "en_US.utf8");

    // we set a custom logging callback
    g_log_set_handler (NULL, 
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                       httpd_log_all_handler_cb, 
                       NULL);

    if (argc != 2) {
        g_critical("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    initialize_exit_fd();

    // seed random
    srand48(time(NULL));

    // begin setting up SSL
    SSL_library_init(); // loads encryption and hash algs for SSL
    SSL_load_error_strings(); // loads error strings for error reporting

    const SSL_METHOD *ssl_method = SSLv23_server_method();
    SSL_CTX *ssl_ctx = SSL_CTX_new(ssl_method);
    SSL_CTX_set_default_passwd_cb(ssl_ctx, pem_passwd_cb);

    if (SSL_CTX_use_certificate_file(ssl_ctx, SERVER_CERT, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if(SSL_CTX_use_RSAPrivateKey_file(ssl_ctx, SERVER_PKEY, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // begin setting up the server
    const int master_listen_port = strtol(argv[1], NULL, 10);
    int master_socket;
    int new_socket;
    int client_addr_len;
    int select_activity;
    int incoming_sd_max; // max socket (file) descriptor for incoming connections
    fd_set incoming_fds;
    wchar_t data_buffer[TCP_BUFFER_LENGTH];
    struct sockaddr_in server_addr, client_addr;
    struct timeval select_timeout = {1, 0}; // 1 second
    //client_connection *clients[SERVER_MAX_CONN_BACKLOG];
    client_connection *working_client_connection; // client connecting we are currently dealing with
    GTree *clients = g_tree_new_full(g_tree_cmp, NULL, NULL, client_connection_gtree_value_destroy);
    GTree *chatrooms = g_tree_new_full(g_tree_wchar_cmp, NULL, chatroom_gtree_key_destroy, chatroom_gtree_value_destroy);
    int current_connected_count = 0;

    // username/password/salt store
    GKeyFile *user_database = g_key_file_new();
    gchar *user_database_path = "./user_database.conf";
    gchar *user_database_group = "users";
    gchar *user_salt_database_group = "user_salt";
    

    master_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(master_socket == -1) {
        g_critical("unable to create tcp socket. Exiting...");
        exit(EXIT_FAILURE);   
    }

    // we set our master socket to allow multiple connections and resuse 
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    // we use hton(s/l) to convert data into network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(master_listen_port);
    bind(master_socket, (struct sockaddr *) &server_addr, (socklen_t) sizeof(server_addr));

    // we allow a backlog of MAX_CONN_BACKLOG
    if(listen(master_socket, SERVER_MAX_CONN_BACKLOG) == 0) {
        g_info("listening...");
    } else {
        // we were unable to listen for as many backlogged connections as was desired
        GString *listen_error = g_string_new("");
        g_string_printf(listen_error, "%s", strerror(errno));

        g_critical("%s", listen_error->str);
        g_critical("unable to listen for SERVER_MAX_CONN_BACKLOG connections. Exiting...");
        g_string_free(listen_error, TRUE);
        exit(1);
    }

    while (TRUE) {
        GArray *client_fds = g_array_new(TRUE, TRUE, sizeof(int));
        g_tree_foreach(clients, populate_client_fd_array, &client_fds);
        // zero out client address info
        memset(&client_addr, 0, sizeof(client_addr));

        // set appropriate client address length (otherwise client_addr isn't populated correctly by accept())
        client_addr_len = sizeof(client_addr);

        FD_ZERO(&incoming_fds);
        FD_SET(master_socket, &incoming_fds);
        FD_SET(exit_fd[0], &incoming_fds);

        if(exit_fd[0] > master_socket) {
            incoming_sd_max = exit_fd[0] + 1;
        } else {
            incoming_sd_max = master_socket + 1;
        }

        for(unsigned int i = 0; i < client_fds->len; i++) {

            int client_fd = g_array_index (client_fds, int, i);

            if(client_fd > incoming_sd_max) {
                incoming_sd_max = client_fd;
            }

            FD_SET(client_fd, &incoming_fds);

        }

        select_timeout.tv_sec = 1;
        select_activity = select(incoming_sd_max + 1, &incoming_fds, NULL, NULL, &select_timeout);
        g_info("tick!");

        // do we catch a signal?
        if (FD_ISSET(exit_fd[0], &incoming_fds)) {
            int signum;

            while(TRUE) {
                if (read(exit_fd[0], &signum, sizeof(signum)) == -1) {
                    if (errno == EAGAIN) {
                        break;
                    } else {
                        perror("read()");
                        exit(EXIT_FAILURE);
                    }
                }
            }

            if (signum == SIGINT) {
                g_warning("we are exiting");
                break;
                exit(EXIT_SUCCESS);
            } else if (signum == SIGTERM) {
                /* Clean-up and exit. */
                g_warning("sigterm exiting now");
                break;
            }
                    
        }

        // this is done to handle the case where select was not interrupted (ctrl+c) but returned an error.
        if(errno != EINTR && select_activity < 0){
            g_warning("select() error");
        }

        // new incoming connection
        if(FD_ISSET(master_socket, &incoming_fds)) {
            g_info("master socket ISSET");
            new_socket = accept(master_socket, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len);

            if(new_socket > -1) {

                g_info("new connection on socket fd %d, ip %s:%d", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                bool found_socket = FALSE;
                if(current_connected_count < SERVER_MAX_CONN_BACKLOG) {

                    //client_connection_reset(clients[i]);
                    working_client_connection = malloc(sizeof(client_connection));
                    client_connection_init(working_client_connection);

                    SSL *new_ssl = SSL_new(ssl_ctx);

                    working_client_connection->ssl = new_ssl;
                    working_client_connection->bio_ssl = BIO_new(BIO_s_socket());
                    BIO_set_fd(working_client_connection->bio_ssl, new_socket, BIO_NOCLOSE);
                    SSL_set_bio(new_ssl, working_client_connection->bio_ssl, working_client_connection->bio_ssl);

                    int ssl_status = SSL_accept(new_ssl);

                    if(ssl_status <= 0) {
                        ssl_print_error(ssl_status);
                    }

                    working_client_connection->ssl_connected = (ssl_status == 1 ? TRUE : FALSE);
                    working_client_connection->fd = new_socket;
                    found_socket = TRUE;


                    if(working_client_connection->ssl_connected) {
                        g_info("sending welcome message to %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        server_log_access(inet_ntoa(client_addr.sin_addr),
                                          ntohs(client_addr.sin_port),
                                          "connected");
                        client_send_welcome(working_client_connection);
                    }

                    g_tree_insert(clients, &working_client_connection->fd, working_client_connection);
                    current_connected_count++;
            }

                if(found_socket == FALSE) {
                    g_critical("no free client connection slots available, unable to track this connection");
                    close(new_socket);
                }

            } else {

                g_critical("error establishing connection to client");

            }
        }

        for(unsigned int i = 0; i < client_fds->len; i++) {
            
            int client_fd = g_array_index (client_fds, int, i);
            working_client_connection = (client_connection*) g_tree_lookup(clients, &client_fd);
            getpeername(working_client_connection->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);
            //g_info("client fd is %d", client_fd);

            if(FD_ISSET(client_fd, &incoming_fds)) {
                working_client_connection->last_activity = time(NULL);
                working_client_connection->timeout_notification = FALSE;
                
                //working_client_connection = (client_connection*) g_tree_lookup(clients, &client_fd);
                gboolean shutdown_ssl = FALSE;
                gboolean connection_closed = FALSE;

                // zero out data_buffer for "safe" reading
                memset(data_buffer, 0, TCP_BUFFER_LENGTH * sizeof(wchar_t));

                int ret = SSL_read(working_client_connection->ssl, data_buffer, TCP_BUFFER_LENGTH * sizeof(wchar_t));

                // if(working_client_connection->authenticated != TRUE) {
                //     wchar_t please_authenticate[] = L"Before proceeding please authenticate\n\n"
                //                                      "You can use '/regi <username> <password>' to create an account\n"
                //                                      "To log into your account use '/user <username>' to be prompted for password";
                //     SSL_write(working_client_connection->ssl, please_authenticate, wcslen(please_authenticate) * sizeof(wchar_t));
                //     continue;
                // }

                // we do ret <= 0 because this indicates error / shutdown (hopefully a clean one)
                // ideally we will detect the error and deal with it but you know
                if(ret <= 0) {
                    connection_closed = TRUE;
                }

                if(SSL_RECEIVED_SHUTDOWN == SSL_get_shutdown(working_client_connection->ssl) || connection_closed) {
                    SSL_shutdown(working_client_connection->ssl);
                    shutdown_ssl = TRUE;
                }

                if(shutdown_ssl || connection_closed) {
                    g_info("closing connection on socket fd %d, ip %s, port %d", 
                            working_client_connection->fd, 
                            inet_ntoa(client_addr.sin_addr), 
                            ntohs(client_addr.sin_port));

                    // log that user disconnected
                    server_log_access(inet_ntoa(client_addr.sin_addr),
                                          ntohs(client_addr.sin_port),
                                          "disconnected");

                    // remove the user from his chatroom if he was in one.
                    if(working_client_connection->current_chatroom != NULL) {
                        GList **previous_chatroom = (GList**) g_tree_lookup(chatrooms, working_client_connection->current_chatroom);
                        *previous_chatroom = g_list_remove(*previous_chatroom, working_client_connection);
                    }

                    shutdown(working_client_connection->fd, SHUT_RDWR);
                    close(working_client_connection->fd);

                    g_tree_remove(clients, &working_client_connection->fd);
                    current_connected_count--;
                    
                    // we continue because nothing else to be done with this client connection now
                    continue;
                }

                if(ret > 0) {
                    // checking wether input was command
                    if(wcsncmp(L"/", data_buffer, 1) == 0) {
                        wchar_t* buffer;
                        wchar_t* token;

                        wchar_t user_command[] = L"/user ";
                        wchar_t join_command[] = L"/join ";
                        wchar_t list_command[] = L"/list";
                        wchar_t who_command[] = L"/who";
                        wchar_t say_command[] = L"/say ";
                        wchar_t help_command[] = L"/help";
                        wchar_t nick_command[] = L"/nick ";

                        gboolean command_entered = FALSE;

                        if(wcsncmp(user_command, data_buffer, wcslen(user_command)) == 0) {
                            command_entered = TRUE;

                            // setup GkeyFile
                            // future: add error handling for file handling
                            g_key_file_load_from_file (user_database, user_database_path, G_KEY_FILE_NONE, NULL);
                            token = wcstok(data_buffer, L" ", &buffer);
                            // do it one more time to skip the '/regi ' part because we already know it's there
                            token = wcstok(NULL, L" ", &buffer); // this should then be the username part of the command
                            wchar_t *username = token;
                            int username_len = wcslen(username);

                            // check wether username is already in user_database
                            gchar *username_mbs = wchars_to_gchars(username);
                            //g_info("gchar username is %s", username_mbs);

                            gchar *password_in_store = (gchar *) g_key_file_get_value(user_database, user_database_group, username_mbs, NULL);

                            wchar_t *password = wcstok(NULL, L" ", &buffer); // this should then be the password part of the command
                            gchar *password_mbs = wchars_to_gchars(password);
                            unsigned char hash[SHA256_DIGEST_LENGTH];
                            gchar hash_string[65];
                            gchar salt_string[31];
                            gboolean registered = FALSE;
                            gboolean authenticated = FALSE;
                            gchar access_message[username_len * sizeof(wchar_t) + 22]; // +21 for null terminator and info (ex. authentication error)
                            memset(access_message, 0, username_len * sizeof(wchar_t) + 22);

                            if(password_in_store == NULL) {
                                for(int i = 0; i < 30; i++) {
                                    sprintf(salt_string + (i * 2), "%02x", (int) floor(drand48() * 255.0));
                                }
                            } else {
                                gchar *salt_in_store = (gchar *) g_key_file_get_value(user_database, user_salt_database_group, username_mbs, NULL);
                                strncpy(salt_string, salt_in_store, 30);
                                g_free(salt_in_store);
                            }
                            salt_string[30] = '\0';


                            SHA256_CTX sha256;
                            SHA256_Init(&sha256);
                            for(int i = 0; i < 10000; i++) {
                                SHA256_Update(&sha256, password_mbs, gchar_array_len(password_mbs)-1);
                                SHA256_Update(&sha256, salt_string, 30);
                            }
                            SHA256_Final(hash, &sha256);
                            
                            for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                                sprintf(hash_string + (i * 2), "%02x", hash[i]);
                            }
                            hash_string[64] = 0;

                            // username doesn't exist
                            if(password_in_store == NULL) {

                                g_info("new registration: username %s - hash_password %s - salt %s", username_mbs, hash_string, salt_string);
                                g_key_file_set_value(user_database, user_database_group, username_mbs, hash_string);
                                g_key_file_set_value(user_database, user_salt_database_group, username_mbs, salt_string);
                                g_key_file_save_to_file(user_database, user_database_path, NULL);
                                registered = TRUE;

                                sprintf(access_message, "%s %s", username_mbs, "authenticated");

                            } else {

                                if(strncmp(hash_string, password_in_store, 64) == 0) {
                                    authenticated = TRUE;

                                    sprintf(access_message, "%s %s", username_mbs, "authenticated");

                                } else {
                                    sprintf(access_message, "%s %s", username_mbs, " authentication error");
                                }

                            }

                            server_log_access(inet_ntoa(client_addr.sin_addr),
                                              ntohs(client_addr.sin_port),
                                              access_message);

                            if(registered || authenticated) {

                                int ip_bytes_needed = 16 * sizeof(wchar_t);
                                wchar_t client_ip_char_wchar[ip_bytes_needed];
                                swprintf(client_ip_char_wchar, ip_bytes_needed, L"%hs", inet_ntoa(client_addr.sin_addr));
                                wchar_t *client_ip_address = g_malloc(ip_bytes_needed); // ipaddr max is 4 * 3 char blocks and 3 dots in between + null term
                                memset(client_ip_address, 0, (ip_bytes_needed));
                                wcscpy(client_ip_address, client_ip_char_wchar);
                                
                                int client_port_number = ntohs(client_addr.sin_port);

                                working_client_connection->ip_address = client_ip_address;
                                working_client_connection->port_number = client_port_number;

                                wchar_t *client_username = g_malloc(sizeof(wchar_t) * (wcslen(username) + 1));
                                wcscpy(client_username, username);
                                wchar_t *client_nickname = g_malloc(sizeof(wchar_t) * (wcslen(username) + 1));
                                wcscpy(client_nickname, username);

                                working_client_connection->username = client_username;
                                working_client_connection->nickname = client_nickname;
                                working_client_connection->authenticated = TRUE;

                                wchar_t auth_good[] = L"You have successfully authenticated. Blabbr Time!";
                                SSL_write(working_client_connection->ssl, auth_good, wcslen(auth_good) * sizeof(wchar_t));

                            } else {

                                wchar_t auth_error[] = L"There was an error authenticating you. Either the username is taken, or your password was wrong.";
                                SSL_write(working_client_connection->ssl, auth_error, wcslen(auth_error) * sizeof(wchar_t));
                            }

                            g_free(password_mbs);
                            g_free(username_mbs);
                            g_free(password_in_store);
                        }

                        if(wcsncmp(join_command, data_buffer, wcslen(join_command)) == 0) {
                            command_entered = TRUE;

                            token = wcstok(data_buffer, L" ", &buffer); // now it's just the /join part
                            token = wcstok(NULL, L" ", &buffer); // now it should be the chatroom name part

                            wchar_t *chatroom_name = g_malloc((wcslen(token)+1) * sizeof(wchar_t));
                            memset(chatroom_name, 0, (wcslen(token)+1) * sizeof(wchar_t));
                            chatroom_name = wcscpy(chatroom_name, token);

                            GList **working_chatroom = (GList**) g_tree_lookup(chatrooms, chatroom_name);

                            // remove the user from his previous chatroom if he was in one.
                            if(working_client_connection->current_chatroom != NULL) {
                                GList **previous_chatroom = (GList**) g_tree_lookup(chatrooms, working_client_connection->current_chatroom);
                                *previous_chatroom = g_list_remove(*previous_chatroom, working_client_connection);
                            }
                            // add him to the one he is trying to join
                            g_free(working_client_connection->current_chatroom);
                            working_client_connection->current_chatroom = chatroom_name;

                            if(working_chatroom != NULL) {
                                *working_chatroom = g_list_prepend(*working_chatroom, working_client_connection);
                            } else {
                                wchar_t *chatroom_key = g_malloc((wcslen(chatroom_name)+1) * sizeof(wchar_t));
                                memset(chatroom_key, 0, (wcslen(chatroom_name)+1) * sizeof(wchar_t));
                                chatroom_key = wcscpy(chatroom_key, chatroom_name);
                                GList *new_chatroom = NULL;
                                GList **new_chatroom_pointer = g_malloc(sizeof(GList **));

                                new_chatroom = g_list_prepend(new_chatroom, working_client_connection);
                                *new_chatroom_pointer = new_chatroom;
                                g_tree_insert(chatrooms, chatroom_key, new_chatroom_pointer);
                            }

                        }

                        if(wcscmp(list_command, data_buffer) == 0) {
                            command_entered = TRUE;

                            wchar_t *chatroom_list;
                            int bytes_needed = ((sizeof(wchar_t) * 200) * g_tree_nnodes(chatrooms))+4;
                            chatroom_list = g_malloc(bytes_needed); 
                            memset(chatroom_list, 0, bytes_needed);
                            g_tree_foreach(chatrooms, print_chatroom_array, chatroom_list);
                            SSL_write(working_client_connection->ssl, chatroom_list, wcslen(chatroom_list) * sizeof(wchar_t));
                            g_free(chatroom_list);
                        }

                        if(wcscmp(who_command, data_buffer) == 0) {
                            command_entered = TRUE;

                            wchar_t *available_user_list;
                            int bytes_needed = ((sizeof(wchar_t) * 200) * current_connected_count)+4;
                            available_user_list = g_malloc(bytes_needed);
                            memset(available_user_list, 0, bytes_needed);
                            g_info("current conn %d and curr g nnodes %d", current_connected_count, g_tree_nnodes(clients));
                            
                            g_tree_foreach(clients, print_available_users, available_user_list);
                            
                            SSL_write(working_client_connection->ssl, available_user_list, wcslen(available_user_list) * sizeof(wchar_t));
                            g_free(available_user_list);
                        }

                        if(wcsncmp(say_command, data_buffer, wcslen(say_command)) == 0) {
                            command_entered = TRUE;

                            token = wcstok(data_buffer, L" ", &buffer); // this is the /msg part
                            token = wcstok(NULL, L" ", &buffer); // this is the username we want to send to
                            
                        }
                        if(wcscmp(help_command, data_buffer) == 0) {
                            command_entered = TRUE;

                            wchar_t *command_help_text = L"Use '/user <username> to be prompted for password to login\n"
                                                          "Use '/regi <username> <password> to register on the blabbr service\n"
                                                          "Use '/list' to see list of public chatrooms\n"
                                                          "Use '/join <chatroom>' to join/create public chatroom\n"
                                                          "Use '/who' to see list of online usernames\n"
                                                          "Use '/say <username> <message>' to send private message";
                            int bytes_needed = wcslen(command_help_text) * sizeof(wchar_t) + 4; // plus 4 for null terminator
                            
                            SSL_write(working_client_connection->ssl, command_help_text, bytes_needed);
                        }
                        if(wcsncmp(nick_command, data_buffer, wcslen(nick_command)) == 0) {
                            command_entered = TRUE;

                            token = wcstok(data_buffer, L" ", &buffer); // this is the /nick part
                            token = wcstok(NULL, L" ", &buffer); // this is the nickname the user wants
                            if(token != NULL) {
                                wchar_t *client_nickname = g_malloc(sizeof(wchar_t) * (wcslen(token) + 1));
                                memset(client_nickname, 0, sizeof(wchar_t) * (wcslen(token) + 1));
                                wcscpy(client_nickname, token);
                                g_free(working_client_connection->nickname);
                                
                                working_client_connection->nickname = client_nickname;
                                wchar_t *nickname_change_text = L"Your nickname has been changed";
                                int bytes_needed = wcslen(nickname_change_text) * sizeof(wchar_t) + 4; // plus 4 for null terminator
                                SSL_write(working_client_connection->ssl, nickname_change_text, bytes_needed);
                            }
                            else {
                                wchar_t *nickname_error_text = L"usage '/nick <nickname>'";
                                int bytes_needed = wcslen(nickname_error_text) * sizeof(wchar_t) + 4; // plus 4 for null terminator
                                SSL_write(working_client_connection->ssl, nickname_error_text, bytes_needed);
                            }
                        }
                        if(!command_entered) {
                            wchar_t *nickname_error_text = L"Invalid command type '/help' for a list of commands";
                            int bytes_needed = wcslen(nickname_error_text) * sizeof(wchar_t) + 4; // plus 4 for null terminator
                            SSL_write(working_client_connection->ssl, nickname_error_text, bytes_needed);
                        }


                    }
                    // if the sent text wasn't a command
                    else {
                        if(working_client_connection->current_chatroom != NULL) {
                            // get the chatroom of the sender
                            GList *chatroom = *((GList**) g_tree_lookup(chatrooms, working_client_connection->current_chatroom));
                            int bytes_needed = ret + ((wcslen(working_client_connection->username) + 2) * sizeof(wchar_t)) + 4; // plus 4 for null terminator

                            wchar_t *message_to_send = g_malloc(bytes_needed); 
                            memset(message_to_send, 0, bytes_needed);

                            wcscpy(message_to_send, working_client_connection->nickname);
                            wcscat(message_to_send, L": ");
                            wcscat(message_to_send, (wchar_t *) data_buffer);

                            g_list_foreach(chatroom, send_message_to_chatroom, message_to_send);

                            g_free(message_to_send);
                        }
                        else {
                            wchar_t *error_no_chatroom = L"You need to be part of a chatroom to chat\n"
                                                          "Use '/list' to see list of public chatrooms\n"
                                                          "Use '/join <chatroom>' to join/create public chatroom\n"
                                                          "Use '/who' to see list of online usernames\n"
                                                          "Use '/say <username> <message>' to send private message";
                            int bytes_needed = wcslen(error_no_chatroom) * sizeof(wchar_t) + 4; // plus 4 for null terminator
                            
                            SSL_write(working_client_connection->ssl, error_no_chatroom, bytes_needed);
                        }
                        
                    }
                }
            } 
            else {

                if(time(NULL) - working_client_connection->last_activity >= CONNECTION_TIMEOUT - 5 && working_client_connection->timeout_notification != TRUE) {
                    wchar_t connection_time_out[] = L"your connection is about to be timed out";
                    SSL_write(working_client_connection->ssl, connection_time_out, wcslen(connection_time_out) * sizeof(wchar_t));
                    working_client_connection->timeout_notification = TRUE;
                }
                // handling timeouts
                if(time(NULL) - working_client_connection->last_activity >= CONNECTION_TIMEOUT) {
                    // wchar_t connection_timed_out[] = L"your connection timed out";
                    // SSL_write(working_client_connection->ssl, connection_timed_out, wcslen(connection_timed_out) * sizeof(wchar_t));
                    // log that user timed out
                    server_log_access(inet_ntoa(client_addr.sin_addr),
                                          ntohs(client_addr.sin_port),
                                          "timed out");

                    // remove the user from his chatroom if he was in one.
                    if(working_client_connection->current_chatroom != NULL) {
                        GList *previous_chatroom = (GList*) g_tree_lookup(chatrooms, working_client_connection->current_chatroom);
                        previous_chatroom = g_list_remove(previous_chatroom, working_client_connection);
                    }

                    shutdown(working_client_connection->fd, SHUT_RDWR);
                    close(working_client_connection->fd);

                    g_tree_remove(clients, &working_client_connection->fd);
                    current_connected_count--;
                }
            }
        }
        // we free up the client fds GArray
        g_array_free(client_fds, TRUE);
    }

    // we free up the gkeyfile database
    g_key_file_free(user_database);

    // We free up the client connections and chatroom GTree
    g_tree_destroy(chatrooms);
    g_tree_destroy(clients);
    //BIO_vfree(master_bio);
    SSL_CTX_free(ssl_ctx);
    // for(int i = 0; i < SERVER_MAX_CONN_BACKLOG; i++) {
    //     client_connection_reset(clients[i]);
    // }

    exit(EXIT_SUCCESS);
}


// FUNCTION IMPLEMENTATIONS
void send_message_to_chatroom(gpointer data, gpointer user_data) {
    SSL_write(((client_connection *) data)->ssl, (wchar_t *) user_data, wcslen((wchar_t *) user_data) * sizeof(wchar_t));
}

gint g_tree_wchar_cmp(gconstpointer a,  gconstpointer b, G_GNUC_UNUSED gpointer user_data) {
    // unsigned int minLength = wcslen((wchar_t *) b);
    // if(wcslen((wchar_t *) a) < minLength) {
    //     minLength = wcslen((wchar_t *) a);
    // }
    return wcscmp((wchar_t *) a, (wchar_t *) b);
}

gint g_tree_cmp(gconstpointer a,  gconstpointer b, G_GNUC_UNUSED gpointer user_data) {
    return *((int*) a) - *((int*) b);
}

gboolean print_available_users(G_GNUC_UNUSED gpointer key, gpointer value, gpointer data) {
    
    client_connection *client = (client_connection *) value;
    if(client->authenticated == TRUE) {
        if(client->username != NULL) {
            wcscat(data, client->username);
        }
        else {
            wcscat(data, L"[NO USERNAME]");
        }
        wcscat(data, L" : ");
        if(client->ip_address != NULL) {
            wcscat(data, client->ip_address);
        }
        else{
            wcscat(data, L"[NO IPADDRESS]");
        }
        wcscat(data, L" : ");
        wchar_t port_number[20]; 
        swprintf(port_number, 20, L"%d", client->port_number);
        wcscat(data, port_number);
        
        wcscat(data, L" : ");
        if(client->current_chatroom != NULL) {
            wcscat(data, client->current_chatroom);
        }
        else {
            wcscat(data, L"[NO CURRENT CHATROOM]");
        }
        wcscat(data, L"\n");
    }
    return FALSE;
}

gboolean print_chatroom_array(gpointer key, G_GNUC_UNUSED gpointer value, gpointer data) {
    wcscat(data, (wchar_t *) key);
    wcscat(data, L"\n");
    return FALSE;
}

gboolean populate_client_fd_array(gpointer key, G_GNUC_UNUSED gpointer value, gpointer data) {
    GArray **fd_array = (GArray **) data;
    *fd_array = g_array_append_val(*fd_array, *((int *)key));
    return FALSE;
}

void signal_handler(int signum) {
    int _errno = errno;

    if (write(exit_fd[1], &signum, sizeof(signum)) == -1 && errno != EAGAIN) {
        abort();
    }

    fsync(exit_fd[1]);
    errno = _errno;
}

void initialize_exit_fd(void) {

    // establish the self pipe for signal handling
    if (pipe(exit_fd) == -1) {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    // make read and write ends of pipe nonblocking
    int flags;        
    flags = fcntl(exit_fd[0], F_GETFL);
    if (flags == -1) {
        perror("fcntl-F_GETFL");
        exit(EXIT_FAILURE);
    }
    // Make read end nonblocking
    flags |= O_NONBLOCK;
    if (fcntl(exit_fd[0], F_SETFL, flags) == -1) {
        perror("fcntl-F_SETFL");
        exit(EXIT_FAILURE);
    }
    flags = fcntl(exit_fd[1], F_GETFL);
    if (flags == -1) {
        perror("fcntl-F_SETFL");
        exit(EXIT_FAILURE);
    }
    // make write end nonblocking
    flags |= O_NONBLOCK;
    if (fcntl(exit_fd[1], F_SETFL, flags) == -1) {
        perror("fcntl-F_SETFL");
        exit(EXIT_FAILURE);
    }

    // set the signal handler
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // restart interrupted reads
    sa.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

}

int pem_passwd_cb(char *buf, G_GNUC_UNUSED int size, G_GNUC_UNUSED int rwflag, G_GNUC_UNUSED void *userdata) {
    memcpy(buf, "blabbr", 6);
    return 6;
}