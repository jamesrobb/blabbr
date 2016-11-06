#include <wchar.h>

#include "chat_ui.h"
#include "constants.h"

wchar_t *ui_username = NULL;

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
			wchar_t *entry = (wchar_t *) iterator->data;
			int entry_len = wcslen(entry);
			short user_color_pair =  0;

			if(wcsncmp(entry, L"SERVER", 6) == 0) {
				user_color_pair = SERVER_USER_PAIR;
			} else if(ui_username != NULL) {

				user_color_pair = CURRENT_USER_PAIR;
				int ui_username_len = wcslen(ui_username);

				for(int i = 0; i < ui_username_len; i++) {

					if(wcsncmp(ui_username + i, entry + i, 1) != 0) {
						user_color_pair = OTHER_USER_PAIR;
						break;
					}

				}

			} else {
				user_color_pair = OTHER_USER_PAIR;
			}

			wattron(window, COLOR_PAIR(user_color_pair));
			wattron(window, A_BOLD);
			for(int i = 0; i < entry_len; i++) {

				wprintw(window, "%lc", entry[i]);

				if(wcsncmp(entry + i + 1, L" ", 1) == 0) {

					wattroff(window, A_BOLD);
					wattroff(window, COLOR_PAIR(user_color_pair));

					if(i + 1 <= entry_len) {
						wprintw(window, ": %ls\n", entry + i + 1);
					} else {
						wprintw(window, "\n");
					}

					break;
				}

			}
			
			wattroff(window, A_BOLD);
			wattroff(window, COLOR_PAIR(user_color_pair));

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