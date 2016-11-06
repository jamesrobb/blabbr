#include <ncursesw/curses.h>
#include <curses.h>
#include <locale.h>
#include <glib.h>
#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// open ssl headers
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include "chat_ui.h"
#include "constants.h"
#include "line_buffer.h"
#include "log.h"
#include "util.h"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#define NC_UNUSED(x) (void)(x)

// GLOBAL VARIABLES
static int exit_fd[2];

// FUNCTION DECLERATIONS
void signal_handler(int signum);

static void initialize_exit_fd(void);

void text_area_append(GSList **text_area_lines_ref, wchar_t *message, int *text_area_lines_count, int *draw_text_area);

// MAIN
int main(int argc, char **argv) {

	// for unicode purposes
	setlocale(LC_ALL, "");

	if (argc > 3 || argc < 2) {
        g_critical("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char server_ip[16] = {0};
    int server_port = strtol(argv[1], NULL, 10);

    if(argc == 3) {
    	strncpy(server_ip, argv[2], 15);
    } else {
    	strncpy(server_ip, "127.0.0.1", 9);
    }

	// we define this variable earlier than others for logging purposes
	GSList *text_area_lines = NULL;
	GSList **text_area_lines_ref = &text_area_lines;

	//we set a custom logging callback
    g_log_set_handler (NULL, 
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                       client_log_all_handler_cb, 
                       text_area_lines_ref);

    // gui variables
	WINDOW *header_win;
	WINDOW *sidebar_win;
	WINDOW *text_area_win;
	WINDOW *input_area_header_win;
	WINDOW *input_area_win;

	int exit_main_loop = 0;
	int exit_status = EXIT_SUCCESS;

	short origf, origb;
	int draw_gui = 1;
	int draw_text_area = 0;
	int text_area_lines_count = 0;
	
	wchar_t quit_command[] = L"/quit";
	wchar_t bye_command[] = L"/bye";
	wchar_t user_command[] = L"/user";
	wchar_t space[] = L" ";
	size_t quit_command_len = wcslen(quit_command);
	size_t bye_command_len = wcslen(bye_command);
	size_t user_command_len = wcslen(user_command);
	struct input_buffer user_input_buffer;
	line_buffer_make(&user_input_buffer);
	
	// server variables
	int server_fd;
	int select_activity;
	int select_timeout_usec = 20000; // 20th of a second
	int ssl_error;
	fd_set read_fds;
	struct timeval select_timeout;
	struct sockaddr_in server;

	// start random numbers
	srand(time(NULL));

	// time to establish a connection
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &server.sin_addr);
    server.sin_port = htons(server_port);

    int connect_res = connect(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (connect_res == -1) {
        g_critical("connecting to server failed");
        exit_main_loop = 1;
        exit_status = EXIT_FAILURE;
    }
    g_info("server port for server connection: %d", server_port);

    // begin setting up SSL
    SSL_library_init(); // loads encryption and hash algs for SSL
    SSL_load_error_strings(); // loads error strings for error reporting

    const SSL_METHOD *ssl_method = SSLv23_client_method();
    SSL_CTX *ssl_ctx = SSL_CTX_new(ssl_method);
    SSL *ssl = SSL_new(ssl_ctx);

    // we create the BIO for the server connection
    BIO *server_bio = BIO_new(BIO_s_socket());
    BIO_set_fd(server_bio, server_fd, BIO_NOCLOSE);
    SSL_set_bio(ssl, server_bio, server_bio);

    ssl_error = SSL_connect(ssl);

    if(ssl_error != 1) {
    	ssl_print_error(ssl_error);
    	exit(EXIT_FAILURE);
    }

    // set configuration for UI
	initscr();
	cbreak();
	//curs_set(0);
	start_color();
	use_default_colors();
	pair_content(0, &origf, &origb);
	init_pair(1, COLOR_GREEN, origb);
	init_pair(WARNING_PAIR, COLOR_RED, origb);
	init_pair(HEADER_BG_PAIR, COLOR_GREEN, COLOR_GREEN);
	init_pair(HEADER_FG_PAIR, COLOR_WHITE, COLOR_GREEN);
	init_pair(HEADER_FG_PAIR, COLOR_WHITE, COLOR_GREEN);
	init_pair(SERVER_USER_PAIR, COLOR_GREEN, origb);
	init_pair(OTHER_USER_PAIR, COLOR_WHITE, origb);
	init_pair(CURRENT_USER_PAIR, COLOR_CYAN, origb);
	noecho();
	keypad(stdscr, TRUE);

	// create UI windows
	header_win = newwin(1, COLS, 0, 0);
	sidebar_win = newwin(LINES - 1, SIDEBAR_WIDTH, 1, COLS - SIDEBAR_WIDTH);
	text_area_win = newwin(LINES - 4, COLS - SIDEBAR_WIDTH, 1, 0);
	input_area_header_win = newwin(1, COLS - SIDEBAR_WIDTH, LINES - 3, 0);
	input_area_win = newwin(2, COLS - SIDEBAR_WIDTH, LINES - 2, 0);
	timeout(0);
	intrflush(stdscr, 1);

	// build the UI
	if(draw_gui) {
		draw_gui = 0;
		gui_clear_all();
		gui_create_header(header_win);
		gui_create_siderbar(sidebar_win);
		gui_create_text_area(text_area_win, text_area_lines);
		gui_create_input_area_header(input_area_header_win);
		gui_create_input_area(input_area_win, NULL);
	}

	// move the cursor into the start of the input area
	wmove(input_area_win, 0, 0);
	wrefresh(input_area_win);

	BIO_set_nbio(server_bio, 1);

	initialize_exit_fd();

	while(1 && !exit_main_loop) {

		// select functionality
		int server_bio_fd = BIO_get_fd(server_bio, NULL);
		int highest_fd = exit_fd[0] > server_bio_fd ? exit_fd[0] : server_bio_fd;
		FD_ZERO(&read_fds);
		FD_SET(exit_fd[0], &read_fds);
		FD_SET(server_bio_fd, &read_fds);
		select_timeout.tv_sec = 0;
		select_timeout.tv_usec = select_timeout_usec;
		select_activity = select(highest_fd + 1, &read_fds, NULL, NULL, &select_timeout);

		// do we catch a signal?
		if (FD_ISSET(exit_fd[0], &read_fds)) {
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
                exit_main_loop = 1;
                exit_status = EXIT_SUCCESS;
                break;

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

		if(FD_ISSET(server_bio_fd, &read_fds)) {

			gboolean shutdown_ssl = FALSE;
            gboolean connection_closed = FALSE;

			wchar_t *recv_message = g_malloc(WCHAR_STR_MAX * sizeof(wchar_t));
			memset(recv_message, 0, WCHAR_STR_MAX * sizeof(wchar_t));
			int recv_len = SSL_read(ssl, recv_message, WCHAR_STR_MAX - 1);
			//ssize_t recv_len = recv(server_fd, recv_message, WCHAR_STR_MAX - 1, 0);

			if(recv_len <= 0) {
                connection_closed = TRUE;
            }

            if(SSL_RECEIVED_SHUTDOWN == SSL_get_shutdown(ssl) || connection_closed) {
                SSL_shutdown(ssl);
                shutdown_ssl = TRUE;
            }

            if(shutdown_ssl || connection_closed) {

                shutdown(server_fd, SHUT_RDWR);
                close(server_fd);

                exit_main_loop = 1;
                exit_status = EXIT_SUCCESS;
                break;

            } else {

				recv_message[recv_len+1] = '\0';
				text_area_lines = g_slist_append(text_area_lines, recv_message);
				text_area_lines_count++;
				draw_text_area = 1;

			}

			//g_info("recv() received a message of length %d", (int) recv_len);
		}

		wchar_t user_line[WCHAR_STR_MAX];
		int user_line_len = line_buffer_get_line_non_blocking(&user_input_buffer, user_line, WCHAR_STR_MAX);

		if(user_line_len > 0) {
			// we finally got some input
			if(wcsncmp(quit_command, user_line, quit_command_len) == 0) {
				exit_main_loop = 1;
				SSL_shutdown(ssl);
				break;
			} 
			else if(wcsncmp(bye_command, user_line, bye_command_len) == 0) {
				exit_main_loop = 1;
				break;
			}
			else if (wcsncmp(user_command, user_line, user_command_len) == 0) {

				gui_create_input_area(input_area_win, NULL);
				wrefresh(input_area_win);

				wchar_t *state;
				wchar_t *token = wcstok(user_line, L" ", &state);
				int tokens = 1;
				gboolean user_command_error = FALSE;

				while(token != NULL) {

					if(tokens >= 2) {
						user_command_error = TRUE;
						break;
					}

					token = wcstok(NULL, L" ", &state);
					tokens++;

					if(tokens == 2) {

						wint_t password[61];
						memset(password, 0, 61);
						wget_wstr(input_area_win, password);
						password[60] = '\0';
						int password_input_length = wint_chars_len(password, 60);

						if(password_input_length < 6 || password_input_length > 50) {

							wchar_t *password_error_message = g_malloc(sizeof(wchar_t) * 58);
							mbstowcs(password_error_message, "Passwords must be between 6 and 50 characters inclusively", 58);
							password_error_message[57] = '\0';

							text_area_append(text_area_lines_ref, password_error_message, &text_area_lines_count, &draw_text_area);

						} else {

							unsigned char hash[SHA256_DIGEST_LENGTH];
							gchar hash_string[65];
							wchar_t hash_string_wide[65];
							SHA256_CTX sha256;
	                        SHA256_Init(&sha256);

	                        for(int i = 0; i < 20000; i++) {
	                            SHA256_Update(&sha256, password, password_input_length * sizeof(wint_t));
	                        }
	                        SHA256_Final(hash, &sha256);

	                        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                                sprintf(hash_string + (i * 2), "%02x", hash[i]);
                            }
                            hash_string[64] = 0;
	                        mbstowcs(hash_string_wide, hash_string, 65);

	                        // + 2 for spaces, +65 for hash with null terminator
	                        int user_command_payload_len = sizeof(wchar_t) * (wcslen(user_command) + wcslen(token) + 2 + 65);
	                        wchar_t *user_command_payload = g_malloc(user_command_payload_len);
	                        memset(user_command_payload, 0, user_command_payload_len);
	                        wcscat(user_command_payload, user_command);
	                        wcscat(user_command_payload, space);
	                        wcscat(user_command_payload, token);
	                        wcscat(user_command_payload, space);
	                        wcscat(user_command_payload, hash_string_wide);

	                        //text_area_append(text_area_lines_ref, user_command_payload, &text_area_lines_count, &draw_text_area);
	                        SSL_write(ssl, user_command_payload, user_command_payload_len);
	                        g_free(user_command_payload);

	                        g_free(ui_username);
	                        wchar_t *username = g_malloc((wcslen(token) + 1) * sizeof(wchar_t));
	                        memset(username, 0, (wcslen(token) + 1) * sizeof(wchar_t));
	                        wcscat(username, token);
	                        ui_username = username;
                    	}

                    	break;
					}
				}

				if(user_command_error) {

					wchar_t *register_error_message = g_malloc(sizeof(wchar_t) * 29);
					mbstowcs(register_error_message, "Invalid registration command", 29);
					register_error_message[28] = '\0';

					text_area_append(text_area_lines_ref, register_error_message, &text_area_lines_count, &draw_text_area);

				}

			} else {

				wchar_t *str = g_malloc(user_line_len * sizeof(wchar_t));
				memcpy(str, user_line, user_line_len * sizeof(wchar_t));
				SSL_write(ssl, str, user_line_len * sizeof(wchar_t));
				// text_area_lines = g_slist_append(text_area_lines, str);
				// text_area_lines_count++;
				// draw_text_area = 1;
				g_free(str);
			}

		}

		if(draw_text_area) {
			gui_create_text_area(text_area_win, text_area_lines);
			wrefresh(text_area_win);
			draw_text_area = 0;
		}

		gui_create_input_area(input_area_win, &user_input_buffer);
		wrefresh(input_area_win);
	}

	// time to cleanup
	//BIO_vfree(server_bio);
	SSL_free(ssl);
	SSL_CTX_free(ssl_ctx);
	g_slist_free_full(text_area_lines, g_free);
	endwin();
	exit(exit_status);

	return 0;
}

// FUNCTION IMPLEMENTATIONS
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

void text_area_append(GSList **text_area_lines_ref, wchar_t* message, int *text_area_lines_count, int *draw_text_area) {

	(*text_area_lines_ref) = g_slist_append(*text_area_lines_ref, message);
	(*text_area_lines_count)++;
	(*draw_text_area) = 1;

}