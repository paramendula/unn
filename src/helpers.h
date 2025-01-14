#ifndef __UNN_HELPERS_H_
#define __UNN_HELPERS_H_

#include "winbuf.h"
#include "state.h"

#include <stdio.h>
#include <wchar.h>
#include <string.h>

inline static void _line_check(line *l, int amount) {
    if((l->len + amount) <= l->cap) return;

    int new_cap = l->cap * 2;
    while((l->len + amount) > new_cap) new_cap *= 2;

    wchar_t *str = (wchar_t *)unn_realloc(l->str, (new_cap + 1) * sizeof(*str));

    l->cap = new_cap;
    l->str = str;
}

int line_insert(line *l, wchar_t ch, int index) {
    _line_check(l, 1);

    int to_move = l->len - index;

    if(to_move)
        memmove(l->str + index + 1, l->str + index, sizeof(*l->str) * to_move);

    l->str[index] = ch;
    l->str[++(l->len)] = 0;

    return 0;
}

int line_insert_multi(line *l, wchar_t *wbuff, int len, int index) {
    _line_check(l, len);

    int to_move = l->len - index;

    if(to_move)
        memmove(l->str + index + 1, l->str + index, sizeof(*l->str) * to_move);

    memcpy(l->str + index, wbuff, sizeof(*wbuff) * len);

    l->len += len;
    l->str[l->len] = 0;

    return 0;
}

inline static int line_insert_str(line *l, wchar_t *str, int index) {
    return line_insert_multi(l, str, wcslen(str), index);
}

inline static int line_append(line *l, wchar_t ch) {
    return line_insert(l, ch, l->len);
}

inline static int line_append_multi(line *l, wchar_t *ch, int len) {
    return line_insert_multi(l, ch, l->len, l->len);
}

inline static int line_append_str(line *l, wchar_t *str) {
    return line_insert_multi(l, str, wcslen(str), l->len);
}

int line_remove(line *l, int index, wchar_t *buff) {
    wchar_t ch = l->str[index];

    int to_move = sizeof(*l->str) * (l->len - index - 1);

    if(to_move)
        memmove(l->str + index, l->str + index + 1, to_move);

    l->str[--(l->len)] = 0;

    if(buff)
        *buff = ch;

    return 0;
}

int line_remove_multi(line *l, int index, int amount, wchar_t *buff) {
    if(buff) {
        for(int i = index; i < index + amount; i++) {
            *buff = l->str[i];
        }
    }

    int to_move = sizeof(*l->str) * (l->len - index - amount);

    if(to_move)
        memmove(l->str + index, l->str + index + amount, to_move);

    l->len -= amount;

    l->str[l->len] = 0;

    return 0;
}

line *read_file_to_lines(FILE *fp, line **last_buffer, int *lc_buffer) {
    if(!fp) return NULL;

    int lines_count = 1;
    line *first = line_empty(4);
    line *last = first;

    char raw_buffer[512];
    wchar_t ch;
    mbstate_t mb = { 0 };
    int save = 0;

    while(1) {
        int readen = fread(raw_buffer + save, 1, sizeof(raw_buffer) - save, fp) + save;

        if(ferror(fp)) {
            node_free_nexts((node *)first, (free_func)line_free);
            return NULL;
        }

        int from = 0;

        while (1) {
            if(from == readen) break;

            int wclen = mbrtowc(&ch, raw_buffer + from, readen - from, &mb);

            if(wclen == -1) {
                node_free_nexts((node *)first, (free_func)line_free);
                return NULL;
            } else if(wclen == -2) {
                save = readen - from;
                break;
            }

            from += wclen;

            if(ch == L'\n') {
                line *new_line = line_empty(4);
                last->next = new_line;
                new_line->prev = last;

                last = new_line;
                continue;
            }

            line_append(last, ch);
        }

        if(feof(fp)) {
            break;
        }
    }

    if(last) {
        *last_buffer = last;
    }

    if(lc_buffer) {
        *lc_buffer = lines_count;
    }

    return first;
}

void cursor_right();

// this technically needs to be moved to commands.h, but who cares?
void buffer_insert_at_cursor(window *w, wchar_t ch) {
    pthread_mutex_lock(&w->buff->block);

    line_insert(w->cur.l, ch, w->cur.pos);
    cursor_right();

    pthread_mutex_unlock(&w->buff->block);

    order_draw_window(w);
}

void prompt_cb_file_open(buffer *) {

}

#endif