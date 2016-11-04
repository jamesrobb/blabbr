/* A TCP echo server with timeouts.
 *
 * Note that you will not need to use select and the timeout for a
 * tftp server. However, select is also useful if you want to receive
 * from multiple sockets at the same time. Read the documentation for
 * select on how to do this (Hint: Iterate with FD_ISSET()).
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <glib.h>
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

// open ssl headers
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include "client_connection.h"
#include "constants.h"
#include "log.h"
#include "util.h"

// GLOBAL VARIABLES
static int exit_fd[2];

#define  SERVER_CERT            "./blabbr.crt"
#define  SERVER_PKEY            "./blabbr.key"

// FUNCTION DECLERATIONS
int sockaddr_in_cmp(const void *addr1, const void *addr2);

gint g_tree_cmp(gconstpointer a,  gconstpointer b, G_GNUC_UNUSED gpointer user_data);

gboolean populate_client_fd_array(gpointer key, G_GNUC_UNUSED gpointer value, gpointer data);

void signal_handler(int signum);

static void initialize_exit_fd(void);

int pem_passwd_cb(char *buf, G_GNUC_UNUSED int size, G_GNUC_UNUSED int rwflag, G_GNUC_UNUSED void *userdata);

// MAIN
int main(int argc, char **argv) {


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
    int current_connected_count = 0;

    //BIO *master_bio;
    

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
            g_info("CLIENT_FD_ARRAY %d", client_fd);

        }

        for(unsigned int i = 0; i < client_fds->len; i++) {

            int client_fd = g_array_index (client_fds, int, i);

            if(client_fd > incoming_sd_max) {
                incoming_sd_max = client_fd;
            }

            g_info("FD_SET %d", client_fd);
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

            g_info("client fd is %d", client_fd);

            if(FD_ISSET(client_fd, &incoming_fds)) {
                
                working_client_connection = (client_connection*) g_tree_lookup(clients, &client_fd);
                gboolean shutdown_ssl = FALSE;
                gboolean connection_closed = FALSE;

                // zero out data_buffer for "safe" reading
                memset(data_buffer, 0, TCP_BUFFER_LENGTH * sizeof(wchar_t));

                int ret = SSL_read(working_client_connection->ssl, data_buffer, TCP_BUFFER_LENGTH * sizeof(wchar_t));
                getpeername(working_client_connection->fd, (struct sockaddr*)&client_addr , (socklen_t*)&client_addr_len);
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

                    shutdown(working_client_connection->fd, SHUT_RDWR);
                    close(working_client_connection->fd);

                    g_tree_remove(clients, &working_client_connection->fd);
                    current_connected_count--;

                    // we continue because nothing else to be done with this client connection now
                    continue;
                }
            } 
        }

        // we free up the client fds GArray
        g_array_free(client_fds, TRUE);
    }

    // We free up the client connections GTree
    g_tree_destroy(clients);
    //BIO_vfree(master_bio);
    SSL_CTX_free(ssl_ctx);
    // for(int i = 0; i < SERVER_MAX_CONN_BACKLOG; i++) {
    //     client_connection_reset(clients[i]);
    // }

    exit(EXIT_SUCCESS);
}


// FUNCTION IMPLEMENTATIONS

/* This can be used to build instances of GTree that index on
   the address of a connection. */
int sockaddr_in_cmp(const void *addr1, const void *addr2) {
     const struct sockaddr_in *_addr1 = addr1;
     const struct sockaddr_in *_addr2 = addr2;

     /* If either of the pointers is NULL or the addresses
        belong to different families, we abort. */
     g_assert((_addr1 == NULL) || (_addr2 == NULL) ||
              (_addr1->sin_family != _addr2->sin_family));

    if (_addr1->sin_addr.s_addr < _addr2->sin_addr.s_addr) {
        return -1;
    } else if (_addr1->sin_addr.s_addr > _addr2->sin_addr.s_addr) {
        return 1;
    } else if (_addr1->sin_port < _addr2->sin_port) {
        return -1;
    } else if (_addr1->sin_port > _addr2->sin_port) {
        return 1;
    }

    return 0;
}


/* This can be used to build instances of GTree that index on
   the file descriptor of a connection. */
// gint fd_cmp(gconstpointer fd1,  gconstpointer fd2, gpointer G_GNUC_UNUSED data) {
//     return GPOINTER_TO_INT(fd1) - GPOINTER_TO_INT(fd2);
// }

gint g_tree_cmp(gconstpointer a,  gconstpointer b, G_GNUC_UNUSED gpointer user_data) {
    return *((int*) a) - *((int*) b);
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