#ifndef __UNN_STATE_H_
#define __UNN_STATE_H_

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>

#include <notcurses/notcurses.h>

#include "winbuf.h"
#include "err.h"
#include "bind.h"

#define FLAG_EDIT 1
#define FLAG_VIEW 2
#define FLAG_FAST 4
#define FLAG_EXIT 8

typedef struct state {
    struct notcurses *nc;
    struct ncplane *p;

    grid *grid;
    buffer_list *blist;
    window *current_window;
    window *other_window;
    window *prompts;
    binds *binds_move, *binds_edit;

    pthread_mutex_t status_flags_block;
    int flags;

    int draw_flags;
    pthread_mutex_t draw_flags_block;
    sem_t draw_request;
    int draw_windows_count, draw_windows_count2;
    window **draw_windows;
    window **draw_windows2;

    pthread_mutex_t draw_block;

    suseconds_t sim_cap;
    int input_buffer_len;
    char input_buffer[16];
    
    pthread_mutex_t status_message_block;
    wchar_t status_message[512];

    pthread_t iloop, dloop;

    char done;
} state;

int state_init(state *s, err *e) {
    if(!(s->nc = notcurses_core_init(NULL, stdout))) {
        err_set(e, -1, "notcurses_core_init failed");
        return -1;
    }

    if(!(s->p = notcurses_stdplane(s->nc))) {
        err_set(e, -2, "notcurses_stdplane returned NULL");
        return -2;
    }

    if(sem_init(&s->draw_request, 0, 0)) {
        err_set(e, -3, "sem_init for draw_request failed");
        return -3;
    }

    if(pthread_mutex_init(&s->draw_flags_block, NULL)) {
        err_set(e, -4, "pthread_mutex_init for draw_flags_block failed");
        return -4;
    }

    if(pthread_mutex_init(&s->status_message_block, NULL)) {
        err_set(e, -5, "pthread_mutex_init for status_message_block failed");
        return -5;
    }

    if(pthread_mutex_init(&s->status_flags_block, NULL)) {
        err_set(e, -6, "pthread_mutex_init for status_flags_block failed");
        return -6;
    }

    if(pthread_mutex_init(&s->draw_block, NULL)) {
        err_set(e, -7, "pthread_mutex_init for draw_block failed");
        return -7;
    }

    s->grid = (grid *)calloc(1, sizeof(*s->grid));
    s->blist = (buffer_list *)calloc(1, sizeof(*s->blist));
    s->flags = 0;

    s->binds_move = binds_empty();
    s->binds_edit = binds_empty();

    s->current_window = NULL;
    s->other_window = NULL;
    s->prompts = NULL;

    s->input_buffer_len = 0;
    memset(&s->input_buffer, 0, sizeof(s->input_buffer));

    s->sim_cap = 25000; // microseconds, max 999999

    return 0;
}

void state_deinit(state *s) {
    if(!s) return;

    while(!s->done) {
        sleep(1);
    }

    if(s->nc) notcurses_stop(s->nc);
    if(s->grid) grid_free(s->grid);
    if(s->blist) blist_free(s->blist);

    if(s->binds_move) binds_free(s->binds_move);
    if(s->binds_edit) binds_free(s->binds_edit);

    sem_destroy(&s->draw_request);
    pthread_mutex_destroy(&s->draw_flags_block);
    pthread_mutex_destroy(&s->status_message_block);
    pthread_mutex_destroy(&s->status_flags_block);
    pthread_mutex_destroy(&s->draw_block);
}

#endif