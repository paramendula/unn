#ifndef __UNN_COMMANDS_H_
#define __UNN_COMMANDS_H_

#include "winbuf.h"
#include "err.h"
#include "state.h"
#include "logic.h"

#include <pthread.h>

void buffer_destroy(buffer *b) {
    blist_remove(S.blist, b);
    
    b->on_destroy(b);

    if(b->current_window) {
        window *w = (window *)b->current_window;
        w->buff = NULL;
        order_draw_window(w);
    }

    buffer_free(b);
}

void window_destroy(window *w) {
    if(S.prompts == w) {
        S.prompts = w->next;
    } else {
        grid_remove(S.grid, w);
    }

    // TODO
    
    if(S.current_window == w) {
        S.current_window = S.grid->first;
    }

    free(w);
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
        off->l = n;
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

    int n = off->pos - times;

    if(n >= 0) {
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
    order_draw(0);
}

#endif