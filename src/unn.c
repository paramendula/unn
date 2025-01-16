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
        logic[helpers state draw binds]
        commands[err winbuf state logic]
        binds[bind commands]

        unn.c

    !!  Multithreading problems do currently exist,
        but certainly are going to be fixed in the
    !!  LISP version of UNN

    TODO(appropximated):
        prompting(special window located just above status, emacs and vim-like)

        ! fix cursor movement and window drawing
            cursor can go beyond the window's border(currently checks only line len)
            somehow cursor falls to the left-most window when placed on an empty line
                (in two-windows situation)

        imeplement the left binds, add more binds for MVP

        text stylizing for scripts
            colored text
            bold, italicized, pale(dimmed),
                understrike(?), throughstrike(?)

        window/buffer additionals for scripts
            pop-up windows for autocompletion selection and hints

        Scheme Lisp support/script
            Parentheses highlighting
            Parentheses jump
            Atoms highlighting
            Sexp move, delete
            Procedures, macros, special forms database
                for completion features

        Scheme Lisp integration

        - MVP. From now on Scheme code is prioritized
            Will I be using SemVer? If so, MVP is 1.0.0
            Thus, releases are needed(archives of precompiled binaries + source code?)
        - Code refactoring and various fixes are obvious

        (for Scheme Lisp)
        coroutines
        object system

        <make several Scheme pet projects for experience>
        
        LSP support? clangd is VERY needed

        rewrite unn in Scheme fully except for notcurses bindings
            (maybe leave C unn core available?)

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

    logg("Begin\n");

    fclose(fopen("logs.txt", "w")); // empty the logs file
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

    // TODO: set prompt binds

    logg("Binds set\n");

    // test windows
    line *first, *last;
    int lines_count;

    FILE *fp = fopen("other/ascii_table.c", "rb");

    first = read_file_to_lines(fp, &last, &lines_count);
    fclose(fp);

    buffer *b1 = (buffer *)buffer_from_lines(L"ascii_table.scm", first, last, lines_count);

    fp = fopen("other/little-schemes.scm", "rb");

    first = read_file_to_lines(fp, &last, &lines_count);
    fclose(fp);

    buffer *b2 = (buffer *)buffer_from_lines(L"little-schemes.scm", first, last, lines_count);

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
    S.other_window = w2;

    logg("Initial windows/buffers created\n");

    // fit and draw first time
    on_resize();
    logg("First draw request sent\n");
}

void unn_run() {
    pthread_create(&S.iloop, NULL, input_loop, NULL);
    pthread_create(&S.dloop, NULL, draw_loop, NULL);

    pthread_join(S.iloop, NULL);
    pthread_join(S.dloop, NULL);

    logg("Both threads exited\n");

    S.done = 1; // signal state_deinit that threads are done
}

int main(int argc, char **argv) {
    unn_init();
    unn_run();
    logg("Returning from main\n");
    return 0;
}