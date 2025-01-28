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

#include "state.h"
#include "logic.h"

#include <pthread.h>

void clear_input_buffer_and_move() {
    S.input_buffer[0] = 0;
    S.input_buffer_len = 0;
    state_flag_off(FLAG_EDIT);
    order_draw_status();
}

void cursor_up() {
    char is_view = state_flag_is_on(FLAG_VIEW);
    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int result;
    if(is_view) {
        result = view_move(S.current_window, -times, 0);
    } else {
        result = cursor_move(S.current_window, -times, 0, 0);
    }

    if(!result) { // if something has changed
        order_draw_window(S.current_window);
    }
}

void cursor_down() {
    char is_view = state_flag_is_on(FLAG_VIEW);
    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int result;
    if(is_view) {
        result = view_move(S.current_window, times, 0);
    } else {
        result = cursor_move(S.current_window, times, 0, 0);
    }

    if(!result) { // if something has changed
        order_draw_window(S.current_window);
    }
}

void cursor_left() {
    char is_view = state_flag_is_on(FLAG_VIEW);
    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int result;
    if(is_view) {
        result = view_move(S.current_window, 0, -times);
    } else {
        result = cursor_move(S.current_window, 0, -times, 0);
        // if we want to cancel the last pos on an empty line we can just press 's'
        S.current_window->last_pos = S.current_window->cur.pos;
    }

    if(!result) { // if something has changed
        order_draw_window(S.current_window);
    }
}

void cursor_right() {
    char is_view = state_flag_is_on(FLAG_VIEW);
    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int result;
    if(is_view) {
        result = view_move(S.current_window, 0, times);
    } else {
        result = cursor_move(S.current_window, 0, times, 0);
    }

    if(!result) { // if something has changed
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

    int new = (cur->gl->len - cur->pos);

    if(flag_is_off(S.current_window->buff->flags, BUFFER_COLORED)) {
        line *l = line_empty(new + 4);

        if(new) {
            line_append_str(l, cur->l->str + cur->pos);

            cur->l->len = cur->pos;
            cur->l->str[cur->pos] = 0;
        }

        list_insert_after(BUFFER_LIST(S.current_window->buff), (node *)cur->l, (node *)l);

        cur->l = l;
    } else {
        dline *dl = dline_empty(new + 4);

        if(new) {
            dline_append_multi(dl, cur->dl->dstr, cur->dl->len);

            cur->dl->len = cur->pos;
        }

        list_insert_after(CBUFFER_LIST((cbuffer*)S.current_window->buff), (node *)cur->dl, (node *)dl);

        cur->dl = dl;
    }

    cur->pos = 0;
    cur->index++;

    adjust_view_for_cursor(S.current_window);

    pthread_mutex_unlock(&S.current_window->buff->block);

    order_draw_window(S.current_window);
}

void buffer_erase_at_cursor() {
    pthread_mutex_lock(&S.current_window->buff->block);

    offset *cur = &S.current_window->cur;


    if(flag_is_on(S.current_window->buff->flags, BUFFER_COLORED)) {
        if(cur->pos == 0) {
            dline *prev = cur->dl->prev;

            if(!prev) {
                pthread_mutex_unlock(&S.current_window->buff->block);
                return;
            }

            dline *dl = cur->dl;
            
            list_remove(CBUFFER_LIST((cbuffer *)S.current_window->buff), (node *)dl);

            cur->pos = prev->len;
            dline_append_multi(prev, dl->dstr, dl->len);
            
            cur->dl = prev;

            dline_free(dl);
        } else {
            dline_remove(cur->dl, cur->pos - 1, NULL);
            cursor_left();
        }
    } else {
        if(cur->pos == 0) {
            line *prev = cur->l->prev;

            if(!prev) {
                pthread_mutex_unlock(&S.current_window->buff->block);
                return;
            }

            line *l = cur->l;
            
            list_remove(BUFFER_LIST(S.current_window->buff), (node *)l);

            cur->pos = prev->len;
            line_append_str(prev, l->str);
            
            cur->l = prev;

            line_free(l);
        } else {
            line_remove(cur->l, cur->pos - 1, NULL);
            cursor_left();
        }
    }

    adjust_view_for_cursor(S.current_window);

    pthread_mutex_unlock(&S.current_window->buff->block);

    order_draw_window(S.current_window);
}

void current_buffer_switch_new() {
    buffer *nb = buffer_empty(L"*empty*");

    nb->draw = (draw_func)draw_window;

    blist_insert(S.blist, nb);

    S.current_window->buff = nb;
    S.current_window->cur = (offset) {
        .index = 0,
        .l = nb->first,
        .pos = 0,
    };
    S.current_window->view = S.current_window->cur;

    nb->current_window = S.current_window;
    
    order_draw_window(S.current_window);
    order_draw_status();                // not optimal, locking mutex twice in a row
}

void current_buffer_switch_new_colored() {
    cbuffer *nb = cbuffer_empty(NULL);

    blist_insert(S.blist, (buffer*)nb);

    S.current_window->buff = (buffer *)nb;
    S.current_window->cur = (offset) {
        .index = 0,
        .dl = nb->first,
        .pos = 0,
    };
    S.current_window->view = S.current_window->cur;

    nb->buff.current_window = S.current_window;
    
    order_draw_window(S.current_window);
    order_draw_status();                // not optimal, locking mutex twice in a row
}

void current_buffer_inverse_cur_color() {
    if(!S.current_window) return;
    if(!S.current_window->buff) return;

    cbuffer *cb = (cbuffer *)S.current_window->buff;

    if(flag_is_off(cb->buff.flags, BUFFER_COLORED)) return;

    dchar *dch = &(S.current_window->cur.dl->dstr[S.current_window->cur.pos]);

    rgb_pair col = dch->color;

    if(flag_is_off(dch->flags, DCHAR_COLORED)) {
        dch->flags |= DCHAR_COLORED;
        col = S.current_window->cl.focused.gen;
    }
    
    dch->color = RGB_PAIR_INVERSE(col);
}

void current_buffer_switch_from_file() {
    make_prompt(L"*file open prompt*", L"Open file: ", (callback)prompt_cb_file_open);
}

void current_buffer_save() {
    if(!S.current_window) return;
    if(!S.current_window->buff) return;

    save_buffer(S.current_window->buff);
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
    make_prompt(L"*file save prompt*", L"Save to file: ", (callback)prompt_cb_file_save);
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

void current_window_toggle_lines() {
    if(!S.current_window) return;
    flag_toggle(S.current_window->flags, WINDOW_LINES);
    order_draw_window(S.current_window);
}

void current_window_toggle_marks() {
    if(!S.current_window) return;
    flag_toggle(S.current_window->flags, WINDOW_LONG_MARKS);
    order_draw_window(S.current_window);
}

// split current window in half horizontally
void new_window_command() {
    logg("New window requested\n");
    window *w = window_empty(L"*empty*");
    blist_insert(S.blist, w->buff);
    w->buff->current_window = w;

    rect loc = { 0 };

    if(!S.current_window) {
        loc.y2 = 1;
        loc.x2 = 1;
    } else {
        // for now just constant horizontal expansion
        loc.x1 = S.grid->width;
        loc.x2 = S.grid->width + 1;
        loc.y2 = 1;
    }

    w->cl = S.colors_default;

    grid_insert_loc(S.grid, w, loc);

    S.current_window = w;

    on_resize();
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