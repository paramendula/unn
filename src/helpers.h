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

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>

#include <sys/stat.h>

#include "state.h"
#include "draw.h"
#include "line.h"

wchar_t *wstr_copy(wchar_t *str) {
    if(!str) return NULL;
    size_t len = wcslen(str);
    
    wchar_t *wcs = (wchar_t *)malloc(sizeof(*wcs) * (len + 1));

    if(!wcs) return NULL;

    wcscpy(wcs, str);
    wcs[len] = 0;

    return wcs;
}

void mode_move() {
    state_flag_off(FLAG_EDIT);

    order_draw_status();
}

void mode_edit() {
    state_flag_on(FLAG_EDIT);

    order_draw_status();
}

void mode_toggle() {
    state_flag_toggle(FLAG_EDIT);

    order_draw_status();
}

window *window_with_buffer(buffer *buff) {
    window *win = (window *)calloc(1, sizeof(*win));

    if(!win) return NULL;

    win->buff = buff;
    win->cur = (offset) {
        .index = 0,
        .pos = 0,
        .l = buff->first,
    };
    win->view = win->cur;
    win->flags = WINDOW_DEFAULT;

    buff->draw = (draw_func)draw_window;

    buff->current_window = win;

    return win;
}

inline static window *window_empty(wchar_t *buff_name) {
    return window_with_buffer(buffer_empty(buff_name));
}

line *read_file_to_lines(FILE *fp, line **last_buffer, int *lc_buffer) {
    if(!fp) return NULL;

    int lines_count = 1;
    line *first = line_empty(4);

    if(!first) return NULL;

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

                if(!new_line) {
                    node_free_nexts((node *)first, (free_func)line_free);
                    return NULL;
                }

                last->next = new_line;
                new_line->prev = last;

                last = new_line;
                continue;
            }

            line_append(last, DCH(ch));
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

void _switch_current_window(window *new_window) {
    S.other_window = S.current_window;
    S.current_window = new_window;
}

void switch_current_window(window *new_window) {
    if(!new_window) return;
    if(S.current_window == new_window) return;

    S.other_window = S.current_window;
    S.current_window = new_window;

    order_draw_window(S.current_window);
    order_draw_window(S.other_window);
    order_draw_status();
}

void cursor_right();

// this technically needs to be moved to commands.h, but who cares?
void buffer_insert_at_cursor(window *w, wchar_t ch) {
    pthread_mutex_lock(&w->buff->block);

    line_insert(w->cur.l, DCH(ch), w->cur.pos);

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

void save_buffer(buffer *b) {
    if(!b) return;
    if(!b->path) return;

    // unsafe TODO
    char raw_path[512] = { 0 };
    wcstombs(raw_path, b->path, sizeof(raw_path) - 1);

    FILE *fp = fopen(raw_path, "wb");

    if(!fp) return;

    for(line *l = b->first; l != NULL; l = l->next) {
        if(ferror(fp)) {
            break;
        }

        dchar *dstr = l->dstr; // just in case
        int len = l->len;

        if(dstr) {
            for(int i = 0; i < len; i++) {
                fprintf(fp, "%lc", dstr[i].wch);
            }

            if(l->next != NULL) { // add \n to each line, except for the last one
                fprintf(fp, "\n");
            }
        }
    }

    fclose(fp);
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
                  // I could've added a ton of them in a sinle header,
                  // but that's even ulier!

void prompt_cb_default(buffer *b) {
    window *w = (window *)b->current_window;

    b->move_binds = NULL;
    b->edit_binds = NULL;

    if(w) {
        buffer *nb = S.blist_prompts->last;
        if(!nb) {
            window_destroy(w);
            mode_move();
            on_resize();
        } else {
            w->buff = nb;
            order_draw_window(w);
            order_draw_status();
        }

        b->current_window = NULL; // stop buffer_destroy from modifying it
    }
}

int copy_file(FILE *from, FILE *to) {
    if(!from) return -1;
    if(!to) return -1;

    while(1) {
        if(ferror(from) || ferror(to)) {
            return -1;
        }

        if(feof(from)) break;

        char buf[256];
        int readen = fread(buf, 1, sizeof(buf), from);

        if(readen <= 0) { // can it be 0 at this point?? no feof
            return -1;
        }

        fwrite(buf, 1, readen, to);
    }

    return 0;
}

// we assume that prompt buffer's line count is 1
void prompt_cb_file_open(buffer *b) {
    window *w = (window *)b->userdata;
    
    if(!w) {
        prompt_cb_default(b);
        return;
    }

    wchar_t *path = NULL;

    if(line_to_wstr(b->first, &path)) {
        prompt_cb_default(b);
        return;
    }

    free(b->first);
    free(b->first->dstr);
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
        
        // safety backup
        char backup_path[562] = { 0 };
        strcpy(backup_path, raw_path);
        strncat(backup_path, "_$backup", sizeof(backup_path) - 1 - strlen(raw_path));

        FILE *backup_fp = fopen(backup_path, "wb");
        copy_file(fp, backup_fp);
        fclose(backup_fp);

        fseek(fp, 0L, SEEK_SET); // get back to the start
        // !!

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
    line_insert_wcs(l, L"unable to open/read the file", 0);

    good: // ***

    nb->draw = (draw_func)draw_window;

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

void prompt_cb_file_save(buffer *b) {
    window *w = (window *)b->userdata;

    if(!w) {
        prompt_cb_default(b);
        return;
    }

    wchar_t *path = NULL;

    if(line_to_wstr(b->first, &path)) {
        prompt_cb_default(b);
        return;
    }

    free(b->first->dstr);
    free(b->first);

    b->first = NULL;
    b->last = NULL;

    if(w->buff->path)
        free(w->buff->path);

    w->buff->path = path;

    if(w->buff->name)
        free(w->buff->name);

    w->buff->name = wstr_copy(path);

    save_buffer(w->buff);

    prompt_cb_default(b);

    order_draw_status();
}

// returns not 0 if nothing has changed
// similar to cursor_move, for comments check it out
int view_move(window *w, int dy, int dx) {
    if(!w) return -1;

    int height = w->pos.y2 - w->pos.y1;
    offset *view = &w->view;

    char changed = 0;

    line *l = (line *)view->l;

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
            char good = 0;
            int limit = 0;

            int y = 0;
            for(line *l = view->l; l != NULL; l = l->next) {
                if(y++ >= height) break;

                if(l->len > new_pos) {
                    good = 1;
                    break;
                } else {
                    if(l->len > limit)
                        limit = l->len;
                }
            }

            if(!good) {
                new_pos = (limit) ? (limit - 1) : 0;
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

    int is_markers = !!flag_is_on(w->flags, WINDOW_LONG_MARKS);
    int dc = (w->dc) ? (w->dc + 1) : 0;

    int height = w->pos.y2 - w->pos.y1 + 1;
    int width = w->pos.x2 - w->pos.x1 + 1;

    int top_line = w->view.index;
    int bottom_line = top_line + height - 1;
    int left_border = w->view.pos;
    int right_border = left_border + width - is_markers - dc;

    int view_x = -1;
    int view_dy = 0;
    int view_dx = 0;

    if(top_line > cidx) {  // move to the line if it is above or below
        w->view.l = w->cur.l;
        w->view.index = cidx;
    } else if(bottom_line < cidx) {
        view_dy = cidx - bottom_line;
    }

    logg("border: %d; cpos: %d\n", right_border, cpos);

    if(left_border > cpos) { // change the whole view window if cur is in the different section
        int attempt = cpos - width + 1 + dc + is_markers;
        view_x = (attempt > 0) ? attempt : 0;
    } else if(right_border <= cpos) { // + 1 for side markers
        view_x = cpos;
    }

    if(view_x > -1) {
        w->view.x = view_x;
    }

    if(view_dy || view_dx) {
        view_move(w, view_dy, view_dx);
    }

    return !(view_x || view_dy);
}

int cursor_set(window *w, line *l, int y, int x, char no_view) {
    if(!w) return -1;
    if(!l) return -1;

    char changed = 0;

    offset *cur = &w->cur;

    if(cur->l != l) {
        changed = 1;
    }

    if(cur->index != y) {
        changed = 1;

        if(y < 0 || y >= w->buff->lines_count) {
            return -2;
        }
    }
    
    if(cur->x != y) {
        changed = 1;

        if (x < 0 || x > l->len) {
            return -3;
        }
    }

    *cur = (offset) {
        .l = l,
        .pos = x,
        .index = y,
    };

    if(changed && !no_view) {
        adjust_view_for_cursor(w);
    }

    return !changed;
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

void make_prompt(wchar_t *name, wchar_t *msg, callback prompt_cb) {
    if(!S.current_window) {
        // TODO
        return;
    }

    buffer *pb = buffer_empty(name);
    pb->path = wstr_copy(msg);
    // set prompt binds
    pb->flags = BUFFER_PROMPT;

    pb->userdata = S.current_window; // save current window as userdata

    pb->on_destroy = prompt_cb; // passes an entered filename

    blist_insert(S.blist_prompts, pb);

    window *pw;
    if(!S.prompt_window) {
        pw = window_with_buffer(pb);

        S.prompt_window = pw;

        pw->cl = S.colors_prompt;

        state_flag_on(FLAG_EDIT);

        _switch_current_window(pw);

        on_resize();
    } else {
        pw = S.prompt_window;

        pb->current_window = pw;

        pw->buff = pb;
        pw->cur = (offset) {
            .index = 0,
            .pos = 0,
            .l = pb->first,
        };
        pw->view = pw->cur;

        state_flag_on(FLAG_EDIT);

        switch_current_window(S.prompt_window);
    }
}

#endif