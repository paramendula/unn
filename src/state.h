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

#ifndef __UNN_STATE_H_
#define __UNN_STATE_H_

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>

#include <notcurses/notcurses.h>

#include "colors.h"
#include "buffer.h"
#include "window.h"
#include "err.h"
#include "bind.h"

#define FLAG_EDIT 1
#define FLAG_VIEW 2
#define FLAG_FAST 4
#define FLAG_EXIT 8

typedef struct state {
    struct notcurses *nc;
    struct ncplane *p; // stdplane

    grid *grid; // all windows(excluding prompt) grid
    buffer_list *blist; // list of all current buffers(except prompt buffers)
    buffer_list *blist_prompts; // prompts buffers
    
    window *current_window;
    window *other_window; // window that was current before switched
    window *prompt_window;

    window *tmp_window;

    binds *binds_move, *binds_edit; // binds for two modes
    binds *binds_prompt; // special overriding binds for prompt window/buffers

    int flags;
    int draw_flags; // flags for draw loop

    rgb_pair colors_status;
    win_colors colors_default;
    win_colors colors_prompt;
    
    sem_t draw_request;
    int draw_windows_count, draw_windows_count2; // two queues to not make other thread wait while
    window **draw_windows;                       // draw loop processes it
    window **draw_windows2;

    wchar_t status_message[512];

    suseconds_t sim_cap; // microseconds cap for two keys pressed to count as simultaneous
    int input_buffer_len;
    char input_buffer[16]; // current keybind input
    
    pthread_mutex_t draw_block; // block drawing loop
    pthread_mutex_t draw_flags_block; // lock drawing flags for editing/reading
    pthread_mutex_t status_message_block; // lock status_message for editing/reading
    pthread_mutex_t state_flags_block; // lock state flags for editing/reading
    
    pthread_t iloop, dloop;
    char done; // if != 0 then we wait for iloop, dloop to end, deinit everything and exit
} state;

// currently, memory allocations are either OK or panic
int state_init(state *s, err *e) {
    *s = (state) { 0 };
    notcurses_options opt = { 0 };

    if(!(s->nc = notcurses_core_init(&opt, stdout))) {
        err_set(e, -1, L"notcurses_core_init failed");
        return -1;
    }

    notcurses_linesigs_disable(s->nc); // disable Ctrl+C quit

    if(!(s->p = notcurses_stdplane(s->nc))) {
        err_set(e, -2, L"notcurses_stdplane returned NULL");
        return -2;
    }

    if(sem_init(&s->draw_request, 0, 0)) {
        err_set(e, -3, L"sem_init for draw_request failed");
        return -3;
    }

    // always return 0
    pthread_mutex_init(&s->draw_flags_block, NULL);
    pthread_mutex_init(&s->status_message_block, NULL);
    pthread_mutex_init(&s->state_flags_block, NULL);
    pthread_mutex_init(&s->draw_block, NULL);

    s->grid = (grid *)calloc(1, sizeof(*s->grid));
    if(!s->grid) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->blist = (buffer_list *)calloc(1, sizeof(*s->blist));
    if(!s->blist) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->blist_prompts = (buffer_list *)calloc(1, sizeof(*s->blist_prompts));
    if(!s->blist_prompts) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->flags = 0;

    s->binds_move = binds_empty(); // to be filled after initialization
    if(!s->binds_move) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->binds_edit = binds_empty();
    if(!s->binds_edit) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }
    
    s->binds_prompt = binds_empty();
    if(!s->binds_prompt) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->draw_windows = (window **)malloc(sizeof(*s->draw_windows) * 16);
    if(!s->draw_windows) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->draw_windows2 = (window **)malloc(sizeof(*s->draw_windows2) * 16);
    if(!s->draw_windows2) {
        err_set(e, -4, L"not enough memory");
        return -4;
    }

    s->input_buffer_len = 0;

    s->sim_cap = 25000; // microseconds, max 999999

    s->done = 0;

    logg("State initialized\n");

    return 0;
}

void state_deinit(state *s) {
    if(!s) return;

    while(!s->done) {
        logg("Waiting for both threads to join...\n");
        sleep(1);
    }

    if(s->nc) notcurses_stop(s->nc);

    // no need to check if NULL, free functions do it already
    grid_free(s->grid);
    blist_free(s->blist);
    blist_free(s->blist_prompts);

    binds_free(s->binds_move);
    binds_free(s->binds_edit);
    binds_free(s->binds_prompt);

    if(s->draw_windows)
        free(s->draw_windows);
    
    if(s->draw_windows2)
        free(s->draw_windows2);

    // not sure if I should check if those are init'd
    sem_destroy(&s->draw_request);
    pthread_mutex_destroy(&s->draw_flags_block);
    pthread_mutex_destroy(&s->status_message_block);
    pthread_mutex_destroy(&s->state_flags_block);
    pthread_mutex_destroy(&s->draw_block);

    logg("State deinitialized\n");
}

#endif