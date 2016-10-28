#include "chat_ui.h"
#include "constants.h"

#include <wchar.h>

void gui_clear_all() {
	clear();
	refresh();
}

void gui_create_header(WINDOW *window) {	
	int rows, cols;
	getmaxyx(window, rows, cols);
	NC_UNUSED(rows);

	wattron(window, COLOR_PAIR(HEADER_BG_PAIR));
	for(int i = 0; i < cols; i++) {
		mvwaddch(window, 0, i, ACS_BLOCK);
	}
	wattroff(window, COLOR_PAIR(HEADER_BG_PAIR));

	wattron(window, COLOR_PAIR(HEADER_FG_PAIR));
	mvwprintw(window, 0, 0, "%s v%s", "  Blabbr", BLABBR_VERSION);
	wattroff(window, COLOR_PAIR(HEADER_FG_PAIR));

	wrefresh(window);
}

void gui_create_siderbar(WINDOW *window) {
	int rows, cols;
	getmaxyx(window, rows, cols);
	NC_UNUSED(cols);

	for(int i = 0; i < rows; i++) {
		mvwaddch(window, i, 0, ACS_VLINE);
	}

	wrefresh(window);
}

void gui_create_text_area(WINDOW *window, GSList *lines) {
	int rows, cols;
	getmaxyx(window, rows, cols);

	scrollok(window, TRUE);
	wclear(window);

	// wattron(window, A_BOLD);
	// wattron(window, COLOR_PAIR(WARNING_PAIR));

	// mvwprintw(window, 0, 0, "Press F1 to exit");
	// mvwprintw(window, 1, 0, "þÞ ðÐ öÖ áÁ éÉ αΑ βΒ γΓ ωΩ - Er unicode að virka?");

	// wattroff(window, A_BOLD);
	// wattroff(window, COLOR_PAIR(WARNING_PAIR));

	wmove(window, 0, 0);

	if(lines != NULL) {
		GSList *iterator = NULL;

		for(iterator = lines; iterator; iterator = iterator->next) {
			wprintw(window, "%ls\n", (wint_t *) iterator->data);
		}

	}

	wrefresh(window);
}

void gui_create_input_area(WINDOW *window) {
	int rows, cols;
	getmaxyx(window, rows, cols);

	scrollok(window, TRUE);

	wattron(window, COLOR_PAIR(HEADER_BG_PAIR));
	for(int i = 0; i < cols; i++) {
		mvwaddch(window, 0, i, ACS_BLOCK);
	}
	wattroff(window, COLOR_PAIR(HEADER_BG_PAIR));

	wattron(window, COLOR_PAIR(0));
	for(int i = 0; i < cols; i++) {
		mvwaddch(window, 1, i, ' ');
	}
	wattroff(window, COLOR_PAIR(0));

	wrefresh(window);
}