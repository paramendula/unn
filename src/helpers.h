/*
    UNN - text editor with high ambitions and far-fetched goals
    Copyright (C) 2025  Sergei Igolnikov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __UNN_HELPERS_H_
#define __UNN_HELPERS_H_

#include "winbuf.h"
#include "state.h"

#include <stdio.h>
#include <wchar.h>
#include <string.h>

#include <sys/stat.h>

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

void buffer_destroy(buffer *b) {
    if(flag_is_on(b->flags, BUFFER_PROMPT)) {
        blist_remove(S.blist_prompts, b);
    } else {
        blist_remove(S.blist, b);
    }
    
    callback on_destroy = b->on_destroy;
    if(on_destroy) on_destroy(b);

    window *w = (window *)b->current_window;
    if(w) {
        w->buff = buffer_empty(L"*empty*");
        blist_insert(S.blist, w->buff);
        order_draw_window(w);
    }

    buffer_free(b);
}

void window_destroy(window *w) {
    if(S.prompt_window == w) {
        S.prompt_window = NULL;
    } else {
        grid_remove(S.grid, w);
    }

    callback on_destroy = w->on_destroy;
    if(on_destroy) on_destroy(w);

    if(S.current_window == w) {
        S.current_window = S.grid->first;
    }

    free(w);
}

void on_resize(); // this is why I will move UNN to Lisp!
                  // I could've added a ton of them in a single header,
                  // but that's even uglier!

void prompt_cb_default(buffer *b) {
    window *w = (window *)b->current_window;

    if(w) {
        buffer *nb = S.blist_prompts->last;
        if(!nb) {
            window_destroy(w);
            on_resize();
        } else {
            w->buff = nb;
            order_draw_window(w);
        }

        b->current_window = NULL; // stop buffer_destroy from modifying it
    }
}

// we assume that prompt buffer's line count is 1
void prompt_cb_file_open(buffer *b) {
    window *w = (window *)b->userdata;
    
    if(!w) {
        prompt_cb_default(b);
        return;
    }

    wchar_t *path = b->first->str;
    free(b->first);
    b->first = NULL;
    b->last = NULL;

    char raw_path[512] = { 0 };
    // unsafe
    wcstombs(raw_path, path, sizeof(raw_path) - 1);

    buffer *nb;

    struct stat s;
    if(stat(raw_path, &s) || S_ISDIR(s.st_mode)) { // also check if folder
        goto bad;
    } else {
        line *first, *last;
        int line_count;
        
        FILE *fp = fopen(raw_path, "rb");

        if(!fp) {
            goto bad;
        }

        first = read_file_to_lines(fp, &last, &line_count);

        fclose(fp);

        if(!first) {
            goto bad;
        }

        nb = buffer_from_lines(path, first, last, line_count);
        nb->path = path;
    }

    goto good;

    bad: // ***

    free(path);

    nb = buffer_empty(L"*file-open-error*");

    line *l = nb->first;
    line_insert_str(l, L"unable to open/read the file", 0);

    good: // ***

    blist_insert(S.blist, nb);

    w->buff = nb;
    w->cur = (offset) {
        .index = 0,
        .pos = 0,
        .l = nb->first,
    };
    w->view = w->cur;

    S.current_window = w;

    order_draw_window(w);
    
    prompt_cb_default(b);
}

// returns not 0 if nothing has changed
// similar to cursor_move, for comments check it out
int view_move(window *w, int dy, int dx) {
    if(!w) return -1;

    offset *view = &w->view;

    char changed = 0;

    line *l = view->l;

    if(!l) return -1;

    if(dy) {
        line *initial_line = l;
        char is_up = (dy < 0);

        line *next = (is_up) ? l->prev : l->next;
        int new_index = view->index;

        while(next && dy) {
            l = next;

            if(is_up) {
                next = l->prev;
                new_index--;
            } else {
                next = l->next;
                new_index++;
            }

            dy = dy + ((is_up) ? 1 : -1);
        }

        if(l != initial_line) {
            changed = 1;

            view->l = l;
            view->index = new_index;
        }
    }

    if(dx) {
        char is_left = (dx < 0);
        int new_pos = view->pos + dx;

        if(is_left) {
            if(new_pos < 0) {
                new_pos = 0;
            }
        } else {
            if(new_pos > l->len) {
                new_pos = l->len;
            }
        }

        if(new_pos != view->pos) {
            changed = 1;

            view->pos = new_pos;
        }
    }

    return !changed;
}

int adjust_view_for_cursor(window *w) {
    int cidx = w->cur.index;
    int cpos = w->cur.pos;

    int height = w->pos.y2 - w->pos.y1 + 1;
    int width = w->pos.x2 - w->pos.x1;

    int top_line = w->view.index;
    int bottom_line = top_line + height - 1;
    int left_border = w->view.pos;
    int right_border = left_border + width;

    int view_x = -1;
    int view_dy = 0;

    if(top_line > cidx) {  // move to the line if it is above or below
        w->view.l = w->cur.l;
        w->view.index = cidx;
    } else if(bottom_line < cidx) {
        view_dy = cidx - bottom_line;
    }

    int is_markers = !!flag_is_on(w->flags, WINDOW_LONG_MARKS);

    if(left_border > cpos) { // change the whole view window is cur is in the different section
        int attempt = cpos - width + 1 - is_markers; // -1 for side markers
        view_x = (attempt > 0) ? attempt : 0;
    } else if(right_border <= cpos + is_markers) { // + 1 for side markers
        view_x = cpos;
    }

    if(view_x > -1) {
        w->view.x = view_x;
    }

    if(view_dy) {
        view_move(w, view_dy, 0);
    }

    return !(view_x || view_dy);
}

// returns not 0 if nothing has changed
int cursor_move(window *w, int dy, int dx, char no_view) {
    if(!w) return -1;

    offset *cur = &w->cur;

    char changed = 0;

    line *l = cur->l;

    if(!l) return -1;

    if(dy) { // if we move vertically
        line *initial_line = l;
        char is_up = (dy < 0); // are we going up or down?

        line *next = (is_up) ? l->prev : l->next;
        int new_index = cur->index;

        while(next && dy) { // get to the line or to the nearest one (before NULL)
            l = next;

            if(is_up) {
                next = l->prev;
                new_index--;
            } else {
                next = l->next;
                new_index++;
            }

            dy = dy + ((is_up) ? 1 : -1);
        }

        if(l != initial_line) { // if we really have moved
            changed = 1;

            cur->l = l;
            cur->index = new_index;

            if(w->last_pos <= l->len) {
                cur->pos = w->last_pos;
            } else {
                cur->pos = l->len;
            }
        }
    }

    if(dx) { // if we move horizontally
        char is_left = (dx < 0); // are we moving to the left?
        int new_pos = cur->pos + dx;

        if(is_left) {
            if(new_pos < 0) {
                new_pos = 0; // 0 is the leftmost limit
            }
        } else {
            if(new_pos > l->len) {
                new_pos = l->len; // line of the line is the rightmost limit
            }
        }

        if(new_pos != cur->pos) { // if we really have moved
            changed = 1;

            cur->pos = new_pos;
            w->last_pos = new_pos;
        }
    }

    return !(adjust_view_for_cursor(w) || changed);
}

#endif