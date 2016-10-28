#include "chat_ui.h"
#include "constants.h"

#include <ncursesw/curses.h>
#include <curses.h>
#include <locale.h>
#include <glib.h>
#include <wchar.h>
#include <stdlib.h>

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

#define NC_UNUSED(x) (void)(x)

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
	// for unicode purposes
	setlocale(LC_ALL, "");

	WINDOW *header_win;
	WINDOW *sidebar_win;
	WINDOW *text_area_win;
	WINDOW *input_area_win;

	short origf, origb;
	int draw_gui = 1;
	int text_area_lines_count = 0;

	GSList *text_area_lines = NULL;

	wchar_t quit_command[] = L"/quit";
	size_t quit_command_len = wcslen(quit_command);

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
	//noecho();
	keypad(stdscr, TRUE);	

	// create UI windows
	header_win = newwin(1, COLS, 0, 0);
	sidebar_win = newwin(LINES - 1, SIDEBAR_WIDTH, 1, COLS - SIDEBAR_WIDTH);
	text_area_win = newwin(LINES - 4, COLS - SIDEBAR_WIDTH, 1, 0);
	input_area_win = newwin(3, COLS - SIDEBAR_WIDTH, LINES - 3, 0);

	// build the UI
	if(draw_gui) {
		draw_gui = 0;
		gui_clear_all();
		gui_create_header(header_win);
		gui_create_siderbar(sidebar_win);
		gui_create_text_area(text_area_win, text_area_lines);
		gui_create_input_area(input_area_win);
	}

	// move the cursor into the start of the input area
	wmove(input_area_win, 1, 0);
	wrefresh(input_area_win);

	while(1) {

		wchar_t *str = g_malloc(sizeof(wint_t) * WCHAR_STR_MAX);
		wgetn_wstr(input_area_win, (wint_t *) str, WCHAR_STR_MAX);

		if(wcsncmp(quit_command, str, quit_command_len) == 0) {
			g_free(str);
			break;
		}

		text_area_lines = g_slist_append(text_area_lines, str);
		text_area_lines_count++;

		if(text_area_lines_count > BLABBR_LINES_MAX) {
			g_free(text_area_lines->data);
			text_area_lines = g_slist_delete_link(text_area_lines, text_area_lines);
		}

		gui_create_text_area(text_area_win, text_area_lines);

		gui_create_input_area(input_area_win);
		wmove(input_area_win, 1, 0);
		wrefresh(text_area_win);
	}


	// time to cleanup
	g_slist_free_full(text_area_lines, g_free);
	endwin();

	return 0;
}