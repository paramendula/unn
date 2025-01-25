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

        bind.h - bindings primitive hash map implementation
        binds.h - arrays of default bindings
        buffer.h - UNN's general buffer implementation
        colors.h - useful structures and definitions for coloring, colored buffer, etc.
        commands.h - general commands represented by thunks (no-arg, no-ret) used by bindings
        draw.h - window, status, grid drawing functions
        err.h - simple error handling structure and functions, mainly forgotten about
        flags.h - primitive bitwise manipulation definitions for flagging
        helpers.h - misc. functions mainly used by commands.h
        lisp-mode.h - an implementation of a special mode that hels coding in Lisp greatly
        lisp.h - header for functions that some Lisp implementation should export for UNN to use
        list.h - simple doubly-linked list implementation
        logic.h - main logic implemented in functions, draw/input loop functions
        misc.h - miscallenous types and definitios
        panic.h - exposes a single function that simply panics (aborts)
        state.h - general UNN state expressed by a single structure and it's helper functions
        window.h - general definitions for window, grid, etc. and it's helper functions
        unn.c - state + logic glue and bootstrapper

    !!  Multithreading problems do currently exist,
    !!  but certainly are going to be fixed after
    !!  some time.

    Daily minimum task list (move it somewhere else later):
        26.01.25: Colored buffer, separate drawing functions for buffers
        27.01.25: Optimize windows drawing to only draw changed lines
        28.01.25: Small Scheme parser, also something for Lisp Buffer
        29.01.25: Line wrapping, make colored buffers default
        30.01.25: Grid matrix for win movement commands, overlay windows
        31.01.25: Implement some commented about movement commands
        01.02.25: Implement selection

    TODO (appropximated):

        * add matrix of cells to grid for window switching commands
        make grid more flexible, so that static windows like prompt can be made easily
            * error propagation needs to be implemented with error windows
            * error and help windows depend on flexible grid
                also it must be possible to create one big "helper window"
                that logs important events, errors, etc.

        * buffer list and prompt buffer list indexes are separate

        * window switching needs to be simplified, general functions implemented
        
        * bind system must be more complex to be able to handle non-hardcoded
            non-printable characters, numerical non-hardcoded input without prompting
        * implement the left binds, add more binds for MVP
        * refactor binds for better usability

        * draw ordering functions are not flexible, more general ordering functions needed
        * status drawing alogrithm is crude and inflexible
        * window drawing function fully redraws a window, lazy methods needed for optimization
            (draw only the modified line, half of a window etc.)
            * save pre-drawn notcurses buffer, only draw changed lines/dlines into it
        
        * make buffers abstract along with it's drawing and processing functions

        * parameterizing buffers, windows and state is used by flags, which is inflexible and crude

        * add mutexes for data used in multiple threads
            + use unused mutexes for shared data

        * add marks for left border if chars are going out of view
            (do we need it??)

        * text stylizing for scripts
            colored text, blinking,
            bold, italicized, pale(dimmed),
                understrike(?), throughstrike(?)

        * wrap lines feature

        * window/buffer additionals for scripts
            pop-up windows for autocompletion selection and hints

        * Scheme Lisp support/script
            Parentheses highlighting
            Parentheses jump
            Atoms highlighting
            Sexp move, delete
            Procedures, macros, special forms database
                for completion features

        * Scheme Lisp integration

        - MVP. From now on Scheme code is prioritized
            Will I be using SemVer? If so, MVP is 1.0.0
            Thus, releases are needed(archives of precompiled binaries + source code)
        - Code refactoring and various fixes are obvious

        (for Scheme Lisp)
        coroutines
        object system

        <make several Scheme side projects related to UNN>
        
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

    if(e.code) {
        printf("UNN exited with an error[%d]: %ls\n", e.code, e.message);
    }
}

void unn_init() {
    atexit(unn_cleanup);

    logg("Begin\n");

    fclose(fopen("logs.txt", "w")); // empty the logs file
    pthread_mutex_init(&log_mutex, NULL);

    state_init(&S, &e);
    
    if(e.code) {
        return;
    }

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
    if(e.code) return;

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