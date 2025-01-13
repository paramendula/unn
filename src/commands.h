#ifndef __UNN_COMMANDS_H_
#define __UNN_COMMANDS_H_

#include "winbuf.h"
#include "err.h"
#include "state.h"
#include "logic.h"

#include <pthread.h>

void buffer_destroy(buffer *b) {
    blist_remove(S.blist, b);
    
    callback on_destroy = b->on_destroy;
    if(on_destroy) on_destroy(b);

    window *w = (window *)b->current_window;
    if(w) {
        w->buff = NULL;
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
    offset *off = (state_flag_is_on(FLAG_VIEW)) ?
    &S.current_window->view : &S.current_window->cur;

    int times = (state_flag_is_on(FLAG_FAST)) ? 5 : 1;

    int n = off->pos + times;

    if(n <= off->l->len) {
        off->index += times;
        off->pos = n;
        order_draw_window(S.current_window);
    }

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

        cur->index--;
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