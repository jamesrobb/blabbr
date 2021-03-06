#ifndef CHAT_UI_H
#define CHAT_UI_H

#include <glib.h>
#include <ncursesw/curses.h>
#include <curses.h>
#include <wchar.h>

#include "line_buffer.h"

#define NC_UNUSED(x) (void)(x)

#define HEADER_BG_PAIR		2
#define HEADER_FG_PAIR 		3
#define WARNING_PAIR		7
#define SERVER_USER_PAIR	8
#define OTHER_USER_PAIR		6
#define CURRENT_USER_PAIR	10

#define SIDEBAR_WIDTH		15

extern wchar_t *ui_username;

void gui_clear_all();

void gui_create_header(WINDOW *window);

void gui_create_siderbar(WINDOW *window);

void gui_create_text_area(WINDOW *window, GSList *lines);

void gui_create_input_area_header(WINDOW *window);

void gui_create_input_area(WINDOW *window, struct input_buffer *buffer);

#endif