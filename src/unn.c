// Unnamed is Not Named
// a newly-born not-yet-finished "text editor"
// with high amibitions and far-fetched goals

/*
    HEADERS(hdr[deps]):

        panic flags list
        err[panic] mem[panic] bind[mem]
        winbuf[mem flags list]
        state[err bind winbuf]
        helpers[state winbuf]
        commands[err winbuf state]
        binds[bind commands]
        logic[state draw binds]

        unn.c

    TODO(appropximated):
        prompting
        implement the binds
        add more binds for MVP
        text stylizing for scripts
        window/buffer additionals for scripts

        Scheme Lisp support/script
            Parentheses highlighting
            Parentheses jump
            Atoms highlighting
            Sexp move, delete
            Procedures, macros, special forms database
                for completion features

        Scheme Lisp integration

        - MVP. From now on Scheme code is prioritized

        coroutines
        object system

        <make several Scheme pet projects for experience>
        
        LSP support? clangd is VERY needed

        rewrite unn in Scheme fully except for notcurses bindings
        unn modularity
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <pthread.h>

pthread_mutex_t log_mutex;

void logg(char *fmt, ...) {
    static FILE *errfp;
    va_list ap; va_start(ap, fmt);

    pthread_mutex_lock(&log_mutex);

    errfp = fopen("logs.txt", "a");

    vfprintf(errfp, fmt, ap);

    va_end(ap);

    fclose(errfp);
    
    pthread_mutex_unlock(&log_mutex);
}

#include "err.h"
#include "state.h"

state S;
err e = { 0 };

#include "logic.h"
#include "binds.h"

void unn_cleanup() {
    state_deinit(&S);

    pthread_mutex_destroy(&log_mutex);
}

void unn_init() {
    atexit(unn_cleanup);

    pthread_mutex_init(&log_mutex, NULL);

    state_init(&S, &e);
    err_fatal(&e);

    // binds
    for(int i = 0; i < sizeof(MOVE_BINDINGS) / sizeof(*MOVE_BINDINGS) - 1; i++) {
        binds_set(S.binds_move, NULL, MOVE_BINDINGS + i);
    }

    for(int i = 0; i < sizeof(EDIT_BINDINGS) / sizeof(*EDIT_BINDINGS) - 1; i++) {
        binds_set(S.binds_edit, NULL, EDIT_BINDINGS + i);
    }

    // test windows
    line *first, *last;
    int lines_count;

    FILE *fp = fopen("other/ascii_table.c", "rb");

    first = read_file_to_lines(fp, &last, &lines_count);
    fclose(fp);

    buffer *b1 = (buffer *)buffer_from_lines("ascii_table.scm", first, last, lines_count);

    fp = fopen("other/little-schemes.scm", "rb");

    first = read_file_to_lines(fp, &last, &lines_count);
    fclose(fp);

    buffer *b2 = (buffer *)buffer_from_lines("little-schemes.scm", first, last, lines_count);

    blist_insert(S.blist, b1);
    blist_insert(S.blist, b2);

    window *w1 = window_with_buffer(b1);
    w1->loc = (rect) {
        .y1 = 0,
        .x1 = 0,
        .y2 = 1,
        .x2 = 1,
    };

    window *w2 = window_with_buffer(b2);
    w2->loc = (rect) {
        .y1 = 0,
        .x1 = 1,
        .y2 = 1,
        .x2 = 2,
    };

    grid_insert(S.grid, w1);
    grid_insert(S.grid, w2);
    S.current_window = w1;

    // fit and draw first time
    on_resize();
}

void unn_run() {
    pthread_create(&S.iloop, NULL, input_loop, NULL);
    pthread_create(&S.dloop, NULL, draw_loop, NULL);

    pthread_join(S.iloop, NULL);
    pthread_join(S.dloop, NULL);

    S.done = 1; // signal state_deinit that threads are done
}

int main(int argc, char **argv) {
    unn_init();
    unn_run();
    return 0;
}