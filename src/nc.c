#include <ncursesw/curses.h>
#include <curses.h>
#include <locale.h>
#include <glib.h>
#include <wchar.h>
#include <stdlib.h>

#define HEADER_BG_PAIR	2
#define HEADER_FG_PAIR 	3
#define WARNING_PAIR	7

#define SIDEBAR_WIDTH	15

#define BLABR_VERSION	((gchar *) "0.1")

WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);

WINDOW *header_win;
WINDOW *sidebar_win;
WINDOW *text_area_win;
WINDOW *input_area_win;

void gui_clear_all() {
	clear();
	refresh();
}

void gui_create_header() {	
	int rows, cols;
	getmaxyx(header_win, rows, cols);

	wattron(header_win, COLOR_PAIR(HEADER_BG_PAIR));
	for(int i = 0; i < cols; i++) {
		mvwaddch(header_win, 0, i, ACS_BLOCK);
	}
	wattroff(header_win, COLOR_PAIR(HEADER_BG_PAIR));

	wattron(header_win, COLOR_PAIR(HEADER_FG_PAIR));
	mvwprintw(header_win, 0, 0, "%s v%s", "  Blabbr", BLABR_VERSION);
	wattroff(header_win, COLOR_PAIR(HEADER_FG_PAIR));

	wrefresh(header_win);
}

void gui_create_siderbar() {
	int rows, cols;
	getmaxyx(sidebar_win, rows, cols);

	for(int i = 0; i < rows; i++) {
		mvwaddch(sidebar_win, i, 0, ACS_VLINE);
	}

	wrefresh(sidebar_win);
}

void gui_create_text_area() {
	int rows, cols;
	getmaxyx(text_area_win, rows, cols);

	wattron(text_area_win, A_BOLD);
	wattron(text_area_win, COLOR_PAIR(WARNING_PAIR));

	mvwprintw(text_area_win, 0, 0, "Press F1 to exit");
	mvwprintw(text_area_win, 1, 0, "þÞ ðÐ öÖ áÁ éÉ αΑ βΒ γΓ ωΩ - Er unicode að virka?");

	wattroff(text_area_win, A_BOLD);
	wattroff(text_area_win, COLOR_PAIR(WARNING_PAIR));

	wrefresh(text_area_win);
}

void gui_create_input_area() {
	int rows, cols;
	getmaxyx(input_area_win, rows, cols);

	wattron(input_area_win, COLOR_PAIR(HEADER_BG_PAIR));
	for(int i = 0; i < cols; i++) {
		mvwaddch(input_area_win, 0, i, ACS_BLOCK);
	}
	wattroff(input_area_win, COLOR_PAIR(HEADER_BG_PAIR));

	wattron(input_area_win, COLOR_PAIR(0));
	for(int i = 0; i < cols; i++) {
		mvwaddch(input_area_win, 1, i, ' ');
	}
	wattroff(input_area_win, COLOR_PAIR(0));

	wrefresh(input_area_win);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
	setlocale(LC_ALL, "");

	int ch, row, col;
	short origf, origb;
	char str[150] = "ER EITTHVAÐ AÐ? þú átt að segja ef tíminn er kominn fyrir örpása. Τι ειναι; ΠΟΥ ΕΙΝΑΙ; ΑΥΤΟΧΙΝΙΤΩ!";
	
	WINDOW *my_win;
	int starty, startx, width, height;
	int draw_gui = 1;
	int cur_pos = 0;

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

	height = 3;
	width = 10;
	starty = (LINES - height) / 2;	/* Calculating for a center placement */
	startx = (COLS - width) / 2;	/* of the window		*/
	printw("Press F1 to exit");
	refresh();
	

	header_win = newwin(1, COLS, 0, 0);
	sidebar_win = newwin(LINES - 1, SIDEBAR_WIDTH, 1, COLS - SIDEBAR_WIDTH);
	text_area_win = newwin(LINES - 3, COLS - SIDEBAR_WIDTH, 1, 0);
	input_area_win = newwin(2, COLS - SIDEBAR_WIDTH, LINES - 2, 0);

	if(draw_gui) {
		draw_gui = 0;
		gui_clear_all();
		gui_create_header();
		gui_create_siderbar();
		gui_create_text_area();
		gui_create_input_area();
	}

	wmove(input_area_win, 1, 0);
	wrefresh(input_area_win);
	//sidebar_win = newwin(5, 5, 5, 5);
	//my_win = create_newwin(height, width, starty, startx);

	// attron(A_BOLD);
	// attron(COLOR_PAIR(1));
	// mvprintw(10, 10, "%s", str);
	// attroff(A_BOLD);

	// mvprintw(5, 5, "%s", str);



	while(1) {

		wint_t cht;
		int res = get_wch(&cht);

		mvwprintw(text_area_win, 2, 0, "%d %d", cht, KEY_ENTER);
		wrefresh(text_area_win);

		if(res == KEY_CODE_YES) {

			if(cht == KEY_F(1)) {
				break;
			}

		} else if (res == OK) {
			// stuff
		} else {
			// stuff
		}

		if(cht == 0xA) {
			cur_pos = 0;
			gui_create_input_area();
			wmove(input_area_win, 1, 0);
			wrefresh(input_area_win);
			continue;
		}

		// switch(ch) {

		// 	case KEY_LEFT:
		// 		destroy_win(my_win);
		// 		my_win = create_newwin(height, width, starty,--startx);
		// 		break;
		// 	case KEY_RIGHT:
		// 		destroy_win(my_win);
		// 		my_win = create_newwin(height, width, starty,++startx);
		// 		break;
		// 	case KEY_UP:
		// 		destroy_win(my_win);
		// 		my_win = create_newwin(height, width, --starty,startx);
		// 		break;
		// 	case KEY_DOWN:
		// 		destroy_win(my_win);
		// 		my_win = create_newwin(height, width, ++starty,startx);
		// 		gui_clear_all();
		// 		gui_create_header();
		// 		gui_create_siderbar();
		// 		break;

		// }

		//mvwaddch(input_area_win, 1, cur_pos, ch);
		mvwprintw(input_area_win, 1, cur_pos, "%lc", cht);
		cur_pos++;
		wmove(input_area_win, 1, cur_pos);

		wrefresh(input_area_win);

	}

	endwin();
	return 0;

	// initscr();
	// raw();
	// start_color();
	// use_default_colors();
	// pair_content(0, &origf, &origb);
	// init_pair(1, COLOR_GREEN, origb);
	// keypad(stdscr, TRUE);
	// noecho();

	// printw("type any string to see it in bold!");
	// getstr(str);

	// if(str[0] == KEY_F(1)) {
	// 	printw("f1 has been pressed");
	// } else {
	// 	getmaxyx(stdscr, row, col);
	// 	attron(A_BOLD);
	// 	attron(COLOR_PAIR(1));
	// 	mvprintw(row/2, (col-strlen(str))/2, "%s", str);
	// 	attroff(A_BOLD);
	// }

	// refresh();
	// getch();
	// endwin();

	// return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx) {
	WINDOW * local_win;

	local_win = newwin(height, width, starty, startx);
	box(local_win, 0, 0);
	wrefresh(local_win);

	return local_win;
}

void destroy_win(WINDOW *local_win) {

	/* box(local_win, ' ', ' '); : This won't produce the desired
	 * result of erasing the window. It will leave it's four corners 
	 * and so an ugly remnant of window. 
	 */
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	/* The parameters taken are 
	 * 1. win: the window on which to operate
	 * 2. ls: character to be used for the left side of the window 
	 * 3. rs: character to be used for the right side of the window 
	 * 4. ts: character to be used for the top side of the window 
	 * 5. bs: character to be used for the bottom side of the window 
	 * 6. tl: character to be used for the top left corner of the window 
	 * 7. tr: character to be used for the top right corner of the window 
	 * 8. bl: character to be used for the bottom left corner of the window 
	 * 9. br: character to be used for the bottom right corner of the window
	 */
	wrefresh(local_win);
	delwin(local_win);

}