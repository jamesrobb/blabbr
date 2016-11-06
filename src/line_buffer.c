#include <ncursesw/curses.h>
#include <string.h>
#include <stdlib.h>
#include <wctype.h>
#include <glib.h>

#include "line_buffer.h"

int line_buffer_get_line_non_blocking(struct input_buffer *buffer, wchar_t *target, int max_len) {

    while(TRUE) {

    	wint_t key;
        int ret = get_wch(&key);
        if(ret == ERR) {
            // No more input
            return 0;
        }

        int n = line_buffer_handle_input(buffer, target, max_len, key, ret);
        if(n) {
            return n;
        }

    }

}

int line_buffer_handle_input(struct input_buffer *buffer, wchar_t *target, int max_len, wint_t key, int ch_ret) {

    if(iswprint(key) && ch_ret != KEY_CODE_YES) {
        line_buffer_add_char(buffer, key);
        return 0;
    }

    switch(key) {
	    case ERR: /* no key pressed */ break;
	    case KEY_LEFT:  if(buffer->cursor_x > 0)           { buffer->cursor_x --; } break;
	    case KEY_RIGHT: if(buffer->cursor_x < buffer->length) { buffer->cursor_x ++; } break;
	    case KEY_HOME:  buffer->cursor_x = 0;           break;
	    case KEY_END:   buffer->cursor_x = buffer->length; break;
	    case '\t':
	        line_buffer_add_char(buffer, '\t');
	        break;
	    case KEY_BACKSPACE:
	    case 127:
	    case 8:
	        if(buffer->cursor_x <= 0) {
	            break;
	        }
	        buffer->cursor_x--;
	        // Fall-through
	    case KEY_DC:
	        if(buffer->cursor_x < buffer->length) {
	            memmove(
	                &buffer->ln[buffer->cursor_x],
	                &buffer->ln[buffer->cursor_x+1],
	                (buffer->length - buffer->cursor_x - 1) * sizeof(wchar_t)
	            );
	            buffer->length--;
	        }
	        break;
	    case KEY_ENTER:
	    case '\r':
	    case '\n':
	        return line_buffer_retrieve_content(buffer, target, max_len);
    }
    return 0;
}

void line_buffer_add_char(struct input_buffer *buffer, wchar_t ch) {
    // Ensure enough space for new character
    if(buffer->length == buffer->capacity) {
        int ncap = (buffer->capacity + 128) * sizeof(wchar_t);
        wchar_t *nln = (wchar_t*) realloc(buffer->ln, ncap);
        if(!nln) {
            // Out of memory!
            return;
        }
        buffer->ln = nln;
        //buffer->capacity = ncap / sizeof(wchar_t);
        buffer->capacity += 128;
    }

    // Add new character
    memmove(
        &buffer->ln[buffer->cursor_x+1],
        &buffer->ln[buffer->cursor_x],
        (buffer->length - buffer->cursor_x) * sizeof(wchar_t)
    );

    buffer->ln[buffer->cursor_x] = ch;
    buffer->cursor_x++;
    buffer->length++;
}

void line_buffer_make(struct input_buffer *buffer) {
    buffer->ln = NULL;
    buffer->length = 0;
    buffer->capacity = 0;
    buffer->cursor_x = 0;
    buffer->cursor_y = 0;
    buffer->last_rendered = 0;
}

void line_buffer_destroy(struct input_buffer *buffer) {
    free(buffer->ln);
    line_buffer_make(buffer);
}

int line_buffer_retrieve_content(struct input_buffer *buffer, wchar_t *target, int max_len) {
    int len = buffer->length < (max_len - 1) ? buffer->length : (max_len - 1);

    memcpy(target, buffer->ln, len * sizeof(wchar_t));
    target[len] = '\0';
    buffer->cursor_x = 0;
    buffer->cursor_y = 0;
    buffer->length = 0;
    return (len);
}