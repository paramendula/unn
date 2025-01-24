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

// UNN's Not Named
// (or Unnamed's Not Named, same thing)
// a newly-born not-yet-finished "text editor"
// with high amibitions and far-fetched goals

/*
    HEADERS(hdr[deps]):

        panic flags list lisp
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

        add matrix of cells to grid for window switching commands
        make grid more flexible, so that static windows like prompt can be made easily
        buffer list and prompt buffer list indexes are separate
        ~ cursor and view moving needs to be interconnected
        text rendering zone in windows shouldn't take all available space
            because we need additions such as line numbering, markers, etc.
        window switching needs to be simplified, general functions implemented
        error propagation needs to be implemented with error windows
        error and help windows depend on flexible grid
            also it must be possible to create one big "helper window"
            that logs important events, errors, etc.
        bind system must be more complex to be able to handle non-hardcoded
            non-printable characters, numerical non-hardcoded input without prompting
        draw ordering functions are not flexible, more general ordering functions needed
        status drawing alogrithm is crude and inflexible
        parameterizing buffers and state is used by flags, which is inflexible and crude
        (half of those are solvable by using Lisp... But I'm a hard worker and love C! jk)

        window drawing functions fully redraws a window, lazy methods needed for optimization
            (draw only the modified line, half of a window etc.)

        prompting(special window located just above status, emacs and vim-like)

        add marks for left border if chars are going out of view

        implement the left binds, add more binds for MVP

        text stylizing for scripts
            colored text, blinking,
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

        also current bindings must be refactored for better usability

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

//#define DEBUG 1

#ifdef DEBUG
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
#else
void logg(char *fmt, ...) {
    return;
}
#endif

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

    // default colors
    S.colors_default = (colors) {
        .cur = RGB_PAIR(255, 255, 255, 0, 0, 0),
        .cur_unfocused = RGB_PAIR(200, 200, 200, 55, 55, 55),
        .cur_line = RGB_PAIR(10, 10, 10, 240, 240, 240),
        .cur_line_unfocused = RGB_PAIR(2, 2, 2, 253, 253, 253),
        .gen = RGB_PAIR(0, 0, 0, 255, 255, 255),
        .gen_unfocused = RGB_PAIR(2, 2, 2, 253, 253, 253),
    };
    
    S.colors_prompt = (colors) {
        .cur = RGB_PAIR(255, 255, 230, 0, 0, 25),
        .cur_unfocused = RGB_PAIR(245, 245, 220, 10, 10, 35),
        .cur_line = RGB_PAIR(0, 0, 25, 255, 255, 230), // same as gen
        .cur_line_unfocused = RGB_PAIR(10, 10, 35, 245, 245, 220), // same as gen_unfocused
        .gen = RGB_PAIR(0, 0, 25, 255, 255, 230),
        .gen_unfocused = RGB_PAIR(10, 10, 35, 245, 245, 220),
    };

    S.colors_status = S.colors_default.cur;
    // binds
    for(int i = 0; i < sizeof(MOVE_BINDINGS) / sizeof(*MOVE_BINDINGS) - 1; i++) {
        binds_set(S.binds_move, NULL, MOVE_BINDINGS + i);
    }

    for(int i = 0; i < sizeof(EDIT_BINDINGS) / sizeof(*EDIT_BINDINGS) - 1; i++) {
        binds_set(S.binds_edit, NULL, EDIT_BINDINGS + i);
    }

    // TODO: set prompt binds
    // currently unneeded, one binds(enter, newline) is hardcoded

    logg("Binds set\n");

    // new window and draw first time
    new_window_command();
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