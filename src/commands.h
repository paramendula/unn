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

#ifndef __UNN_COMMANDS_H_
#define __UNN_COMMANDS_H_

#include "winbuf.h"
#include "err.h"
#include "state.h"
#include "logic.h"

#include <pthread.h>

void clear_input_buffer_and_move() {
    S.input_buffer[0] = 0;
    S.input_buffer_len = 0;
    state_flag_off(FLAG_EDIT);
    order_draw_status();
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

void cursor_up() {
    offset *off = (state_flag_is_on(FLAG_VIEW)) ?
    &S.current_window->view : &S.current_window->cur;

    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    line *l = off->l;
    line *n = l;

    for(int i = 0; i < times; i++) {
        if(!n->prev) break;
        off->index--;
        n = n->prev;
    }

    if(n != l) {
        off->l = n;

        if(n->len < l->len) {
            if(off->pos >= n->len) {
                off->pos = (n->len) ? (n->len - 1) : 0;
            }
        }

        order_draw_window(S.current_window);
    }
}

void cursor_down() {
    offset *off = (state_flag_is_on(FLAG_VIEW)) ?
    &S.current_window->view : &S.current_window->cur;

    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    line *l = off->l;
    line *n = l;

    for(int i = 0; i < times; i++) {
        if(!n->next) break;
        off->index++;
        n = n->next;
    }

    if(n != l) {
        off->index -= times;
        off->l = n;

        if(n->len < l->len) {
            if(off->pos >= n->len) {
                off->pos = (n->len) ? (n->len - 1) : 0;
            }
        }

        order_draw_window(S.current_window);
    }
}

void cursor_left() {
    offset *off = (state_flag_is_on(FLAG_VIEW)) ?
    &S.current_window->view : &S.current_window->cur;

    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int n = off->pos - times;

    if(n >= 0) {
        off->pos = n;
        order_draw_window(S.current_window);
    }
}

void cursor_right() {
    char is_view = state_flag_is_on(FLAG_VIEW);

    offset *off = (is_view) ?
    &S.current_window->view : &S.current_window->cur;

    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int width = (S.current_window->pos.x2 - S.current_window->pos.x1)
     + S.current_window->view.pos;

    int cpos = off->pos;
    int n = times;
    int npos = cpos + n;

    if(npos > off->l->len) {
        n -= npos - off->l->len;
    }

    if(!is_view && npos >= width) {
        n -= npos - width + 1;
    }

    if(n > 0) {
        off->index += n;
        off->pos += n;
        order_draw_window(S.current_window);
    }

}

void cursor_leap_word() {

}

void cursor_leap_word_back() {
    
}

void cursor_fastmode_toggle() {
    state_flag_toggle(FLAG_FAST);
}

void cursor_viewmode_toggle() {
    state_flag_toggle(FLAG_VIEW);
}

void enter_append() {
    cursor_right();
    mode_edit();
}

void try_exit() {
    state_flag_on(FLAG_EXIT);
    order_draw(0); // to process the exit flags
}

void buffer_newline_at_cursor() {
    pthread_mutex_lock(&S.current_window->buff->block);

    if(flag_is_on(S.current_window->buff->flags, BUFFER_PROMPT)) {
        pthread_mutex_unlock(&S.current_window->buff->block);
        buffer_destroy(S.current_window->buff); // call the prompt cb
        return;
    }

    offset *cur = &S.current_window->cur;

    int new = (cur->l->len - cur->pos);

    line *l = line_empty(new + 4);

    if(new) {
        line_append_str(l, cur->l->str + cur->pos);

        cur->l->len = cur->pos;
        cur->l->str[cur->pos] = 0;
    }

    list_insert_after((list *)S.current_window->buff, (node *)cur->l, (node *)l);

    cur->pos = 0;
    cur->l = l;

    pthread_mutex_unlock(&S.current_window->buff->block);

    order_draw_window(S.current_window);
}

void buffer_erase_at_cursor() {
    pthread_mutex_lock(&S.current_window->buff->block);

    offset *cur = &S.current_window->cur;

    if(cur->pos == 0) {
        line *prev = cur->l->prev;

        if(!prev) {
            pthread_mutex_unlock(&S.current_window->buff->block);
            return;
        }

        line *l = cur->l;
        
        list_remove((list *)S.current_window->buff, (node *)l);

        cur->pos = prev->len;
        line_append_str(prev, l->str);
        
        cur->l = prev;

        line_free(l);
    } else {
        line_remove(cur->l, cur->pos - 1, NULL);
        cursor_left();
    }

    pthread_mutex_unlock(&S.current_window->buff->block);

    order_draw_window(S.current_window);
}

void current_buffer_switch_new() {
    buffer *nb = buffer_empty(L"*empty*");
    blist_insert(S.blist, nb);

    S.current_window->buff = nb;
    S.current_window->cur = (offset) {
        .index = 0,
        .l = nb->first,
        .pos = 0,
    };
    S.current_window->view = S.current_window->cur;
    
    order_draw_window(S.current_window);
    order_draw_status();                // not optimal, locking mutex twice in a row
}

void current_buffer_switch_from_file() {
    buffer *pb = buffer_empty(L"*file open prompt*");
    pb->path = L"Open file: ";
    pb->move_binds = S.binds_prompt;
    pb->edit_binds = S.binds_prompt;
    pb->flags = BUFFER_PROMPT;

    if(!S.current_window) {
        // TODO
    }

    pb->userdata = S.current_window; // save current window as userdata

    pb->on_destroy = (callback)prompt_cb_file_open; // passes an entered filename

    blist_insert(S.blist_prompts, pb);

    window *pw;
    if(!S.prompt_window) {
        pw = window_with_buffer(pb);

        pb->current_window = pw;

        S.prompt_window = pw;
        S.tmp_window = S.current_window;

        S.current_window = pw;

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

        S.current_window = pw;

        order_draw_window(S.tmp_window);
        order_draw_window(S.prompt_window);
        order_draw_status();
    }
}

void current_buffer_save() {
    // dangerous for now, backing up needed before the saving
}

void current_buffer_destroy() {
    if(!S.current_window) return;
    buffer_destroy(S.current_window->buff);
}

void buffer_other_destroy() {
    // TODO
}

void current_buffer_switch_other() {
    // TODO
}

void current_buffer_save_other() {
    // TODO
}

void current_window_switch_other() {
    if(!S.other_window) return;

    window *other = S.other_window;
    S.other_window = S.current_window;
    S.current_window = other;

    if(S.other_window) order_draw_window(S.other_window);
    order_draw_window(S.current_window);
    order_draw_status();
}

void new_window_command() {
    // TODO
}

void current_window_switch_prev() {
    if(!S.current_window) return;
    window *prev = S.current_window->prev;

    if(!prev) return;

    S.current_window = prev;

    order_draw_window(S.current_window); // redraw both windows
    order_draw_window(S.current_window->next); // TODO: order multiple draws sim-ly
    order_draw_status();
}

void current_window_switch_next() {
    if(!S.current_window) return;
    window *next = S.current_window->next;

    if(!next) return;

    S.current_window = next;

    order_draw_window(S.current_window); // redraw both windows
    order_draw_window(S.current_window->prev); // TODO: order multiple draws sim-ly
    order_draw_status();
}

void current_window_switch_up() {
    if(!S.current_window) return;

    for(window *w = S.grid->first; w != NULL; w = w->next) {
        if(w == S.current_window) continue;
        if(w->loc.y2 >= S.current_window->loc.y2) continue; // possibly use pos instead of loc?
        if((S.current_window->loc.x1 > w->loc.x1)) continue;

        S.other_window = S.current_window;
        S.current_window = w;
        
        order_draw_window(S.other_window);
        order_draw_window(S.current_window);
        order_draw_status();
        return;
    }
}

// grid needs to be modified for implementing those

void current_window_switch_left() {
    // TODO
}

void current_window_switch_right() {
    // TODO
}

void current_window_switch_down() {
    if(!S.current_window) return;

    for(window *w = S.grid->first; w != NULL; w = w->next) {
        if(w == S.current_window) continue;
        if(w->loc.y2 <= S.current_window->loc.y2) continue; // possibly use pos instead of loc?
        if((S.current_window->loc.x1 > w->loc.x1)) continue;

        S.other_window = S.current_window;
        S.current_window = w;

        order_draw_window(S.other_window);
        order_draw_window(S.current_window);
        order_draw_status();

        return;
    }
}

void current_window_destroy() {

}

void window_other_destroy() {

}

// sorry for that
void _process_edit(wchar_t wch) {
    if(!S.current_window) return;
    if(!S.current_window->buff) return;

    if(wch == NCKEY_BACKSPACE) {
        buffer_erase_at_cursor();
        return;
    } else if(wch == NCKEY_ENTER || wch == L'\n') {
        buffer_newline_at_cursor();
        return;
    }

    buffer_insert_at_cursor(S.current_window, wch);
}

#endif