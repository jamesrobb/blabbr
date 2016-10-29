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

#include "client_connection.h"
#include "constants.h"
#include "log.h"

// GLOBAL VARIABLES
static int exit_fd[2];

// FUNCTION DECLERATIONS
int sockaddr_in_cmp(const void *addr1, const void *addr2);

gint fd_cmp(gconstpointer fd1,  gconstpointer fd2, gpointer G_GNUC_UNUSED data);

void signal_handler(int signum);

static void initialize_exit_fd(void);

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

    // begin setting up the server
    const int master_listen_port = strtol(argv[1], NULL, 10);
    int master_socket;
    int new_socket;
    int client_addr_len;
    int select_activity;
    int incoming_sd_max; // max socket (file) descriptor for incoming connections
    fd_set incoming_fds;
    wchar_t data_buffer[TCP_BUFFER_LENGTH];
    struct sockaddr_in server, client;
    struct sockaddr_in server_addr, client_addr;
    struct timeval select_timeout = {1, 0}; // 1 second
    client_connection *clients[SERVER_MAX_CONN_BACKLOG];
    client_connection *working_client_connection; // client connecting we are currently dealing with

    // we initialize clients sockect fds
    for(int i = 0; i < SERVER_MAX_CONN_BACKLOG; i++) {
        clients[i] = malloc(sizeof(client_connection));
        reset_client_connection(clients[i]);
    }

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

        // zero out client address info and data buffer
        memset(data_buffer, 0, TCP_BUFFER_LENGTH);
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

        for(int i = 0; i < SERVER_MAX_CONN_BACKLOG; i++) {

            working_client_connection = clients[i];

            if(working_client_connection->fd > CONN_FREE) {
                FD_SET(working_client_connection->fd, &incoming_fds);
            }

            if(working_client_connection->fd > incoming_sd_max) {
                incoming_sd_max = working_client_connection->fd;
            }

        }

        select_timeout.tv_sec = 1;
        select_activity = select(incoming_sd_max + 1, &incoming_fds, NULL, NULL, &select_timeout);

        // do we catch a signal?
        if (FD_ISSET(exit_fd[0], &incoming_fds)) {
            int signum;

            for (;;) {
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
            g_info("master socket set");
            new_socket = accept(master_socket, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len);

            if(new_socket > -1) {
                g_info("new connection on socket fd %d, ip %s, port %d", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                bool found_socket = FALSE;
                for(int i = 0; i < SERVER_MAX_CONN_BACKLOG; i++) {

                    if(clients[i]->fd == CONN_FREE) {
                        reset_client_connection(clients[i]);
                        clients[i]->fd = new_socket;
                        found_socket = TRUE;
                        client_send_welcome(clients[i]);
                        break;
                    }

                }

                if(found_socket == FALSE) {
                    g_critical("no free client connection slots available, unable to track this connection");
                    close(new_socket);
                }

            } else {

                g_critical("error establishing connection to client");

            }
        }

    }

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
gint fd_cmp(gconstpointer fd1,  gconstpointer fd2, gpointer G_GNUC_UNUSED data) {
    return GPOINTER_TO_INT(fd1) - GPOINTER_TO_INT(fd2);
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