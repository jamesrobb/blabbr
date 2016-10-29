#ifndef LINE_BUFFER_H
#define LINE_BUFFER_H

#include <wchar.h>

struct input_buffer {
    wchar_t *ln;
    int length;
    int capacity;
    int cursor_x;
    int cursor_y;
    int last_rendered;
};

int line_buffer_get_line_non_blocking(struct input_buffer *buffer, wchar_t *target, int max_len);

int line_buffer_handle_input(struct input_buffer *buffer, wchar_t *target, int max_len, wint_t key, int ch_ret);

void line_buffer_add_char(struct input_buffer *buffer, wchar_t ch);

void line_buffer_make(struct input_buffer *buffer);

void line_buffer_destroy(struct input_buffer *buffer);

int line_buffer_retrieve_content(struct input_buffer *buffer, wchar_t *target, int max_len);

#endif