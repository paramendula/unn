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
    struct ncplane *p; // stdplane

    grid *grid; // all windows(excluding prompt) grid
    buffer_list *blist; // list of all current buffers
    
    window *current_window;
    window *other_window; // window that was current before switched
    window *prompt_window;

    binds *binds_move, *binds_edit; // binds for two modes

    int flags;
    int draw_flags; // flags for draw loop
    
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

int state_init(state *s, err *e) {
    notcurses_options opt = { 0 };

    if(!(s->nc = notcurses_core_init(&opt, stdout))) {
        err_set(e, -1, "notcurses_core_init failed");
        return -1;
    }

    notcurses_linesigs_disable(s->nc); // disable Ctrl+C quit

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

    if(pthread_mutex_init(&s->state_flags_block, NULL)) {
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

    s->binds_move = binds_empty(); // to be filled after initialization
    s->binds_edit = binds_empty();

    s->current_window = NULL;
    s->other_window = NULL;
    s->prompt_window = NULL;

    s->draw_windows = (window **)malloc(sizeof(*s->draw_windows) * 16);
    s->draw_windows2 = (window **)malloc(sizeof(*s->draw_windows2) * 16);

    s->input_buffer_len = 0;
    memset(&s->input_buffer, 0, sizeof(s->input_buffer));

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
    grid_free(s->grid);
    blist_free(s->blist);

    binds_free(s->binds_move);
    binds_free(s->binds_edit);

    free(s->draw_windows);
    free(s->draw_windows2);

    sem_destroy(&s->draw_request);
    pthread_mutex_destroy(&s->draw_flags_block);
    pthread_mutex_destroy(&s->status_message_block);
    pthread_mutex_destroy(&s->state_flags_block);
    pthread_mutex_destroy(&s->draw_block);

    logg("State deinitialized\n");
}

#endif