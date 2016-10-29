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
#include <readline/readline.h>

#include "chat_ui.h"
#include "constants.h"
#include "line_buffer.h"
#include "log.h"

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

// MAIN
int main(int argc, char **argv) {

	// for unicode purposes
	setlocale(LC_ALL, "");

	if (argc != 2) {
        g_critical("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    initialize_exit_fd();

	// we define this variable earlier than others for logging purposes
	GSList *text_area_lines = NULL;
	GSList **text_area_lines_ref = &text_area_lines;

	// we set a custom logging callback
    g_log_set_handler (NULL, 
                       G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION, 
                       client_log_all_handler_cb, 
                       text_area_lines_ref);

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
	size_t quit_command_len = wcslen(quit_command);
	struct input_buffer user_input_buffer;
	line_buffer_make(&user_input_buffer);
	
	int server_fd;
	int select_activity;
	int select_timeout_usec = 20000; // 20th of a second
	const int server_port = strtol(argv[1], NULL, 10);
	fd_set read_fds;
	struct timeval select_timeout;
	struct sockaddr_in server;

	// start random numbers
	srand(time(NULL));

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
	noecho();
	keypad(stdscr, TRUE);

	// time to establish a connection
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    server.sin_port = htons(server_port);

    int connect_res = connect(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (connect_res == -1) {
        g_critical("connecting to server failed");
        exit_main_loop = 1;
        exit_status = EXIT_FAILURE;
    }

    // wchar_t harro_msg[] = L"harro server";
    // int send_res = send(server_fd, harro_msg, wcslen(harro_msg), 0);
    // //g_info("%d", send_res);

    g_info("server port for server connection: %d", server_port);

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


	while(1 && !exit_main_loop) {

		// select functionality
		FD_ZERO(&read_fds);
		FD_SET(exit_fd[0], &read_fds);
		FD_SET(server_fd, &read_fds);
		int highest_fd = exit_fd[0] > server_fd ? exit_fd[0] : server_fd;
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

		if(FD_ISSET(server_fd, &read_fds)) {

			wchar_t *recv_message = g_malloc(WCHAR_STR_MAX * sizeof(wchar_t));
			ssize_t recv_len = recv(server_fd, recv_message, WCHAR_STR_MAX - 1, 0);

			recv_message[recv_len+1] = '\0';
			text_area_lines = g_slist_append(text_area_lines, recv_message);
			text_area_lines_count++;
			draw_text_area = 1;

			//g_info("recv() received a message of length %d", (int) recv_len);
		}

		wchar_t user_line[WCHAR_STR_MAX];
		int user_line_len = line_buffer_get_line_non_blocking(&user_input_buffer, user_line, WCHAR_STR_MAX);

		if(user_line_len > 0) {
			// we finally got some input
			if(wcsncmp(quit_command, user_line, quit_command_len) == 0) {
				exit_main_loop = 1;
				break;
			}

			wchar_t *str = g_malloc(user_line_len * sizeof(wchar_t));
			memcpy(str, user_line, user_line_len * sizeof(wchar_t));

			text_area_lines = g_slist_append(text_area_lines, str);
			text_area_lines_count++;
			draw_text_area = 1;
		}

		if(draw_text_area) {
			gui_create_text_area(text_area_win, text_area_lines);
			wrefresh(text_area_win);
			draw_text_area = 0;
		}

		gui_create_input_area(input_area_win, &user_input_buffer);
		wrefresh(input_area_win);
	}

	sleep(1);

	// time to cleanup
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