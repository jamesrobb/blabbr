#include <wchar.h>

#include "chat_ui.h"
#include "constants.h"

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
	scrollok(window, TRUE);
	wclear(window);

	wmove(window, 0, 0);

	if(lines != NULL) {
		GSList *iterator = NULL;

		for(iterator = lines; iterator != NULL; iterator = iterator->next) {
			wprintw(window, "%ls\n", (wchar_t *) iterator->data);	
		}

	}

	wrefresh(window);
}

void gui_create_input_area_header(WINDOW *window) {
	int cols = getmaxx(window);

	wattron(window, COLOR_PAIR(HEADER_BG_PAIR));
	for(int i = 0; i < cols; i++) {
		mvwaddch(window, 0, i, ACS_BLOCK);
	}
	wattroff(window, COLOR_PAIR(HEADER_BG_PAIR));

	wrefresh(window);
}

void gui_create_input_area(WINDOW *window, struct input_buffer *buffer) {
	int rows, cols;
	getmaxyx(window, rows, cols);

	scrollok(window, TRUE);

	wattron(window, COLOR_PAIR(0));
	for(int i = 0; i < cols; i++) {

		for(int j = 0; j < rows; j++) {
			mvwaddch(window, j, i, ' ');
		}

	}
	wattroff(window, COLOR_PAIR(0));

	wmove(window, 0, 0);

	if(buffer != NULL) {
		if(buffer->length > 0) {

			for(int i = 0; i < buffer->length; i++ ) {
				wprintw(window, "%lc", buffer->ln[i]);
			}

			wmove(window, buffer->cursor_x / cols, buffer->cursor_x % cols);
		}

		//buffer->cursor_y = buffer->length % cols;
	}


	wrefresh(window);
}