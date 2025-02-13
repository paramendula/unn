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

// sometimes I wonder why I'd chosen C over Zig for this project

/*
    FILES:
        bind.h - bindings primitive hash map implementation
        binds.h - arrays of default bindings
        buffer.h - UNN's general buffer implementation
        colors.h - useful structures and definitions for coloring, colored buffer, etc.
        commands.h - general commands represented by thunks (no-arg, no-ret) used by bindings
        draw.h - window, status, grid drawing functions
        err.h - simple error handling structure and functions, mainly forgotten about
        flags.h - primitive bitwise manipulation definitions for flagging
        helpers.h - misc. functions mainly used by commands.h
        lparse.h - crude Scheme Lisp one-step parser
        lmode.h - an implementation of a special mode that helps coding in Lisp greatly
        lisp.h - header for functions that some Lisp implementation should export for UNN to use
        list.h - simple doubly-linked list implementation
        line.h - mutable attributed wide char string implementation
        logic.h - main logic implemented in functions, draw/input loop functions
        misc.h - miscallenous types and definitios
        panic.h - exposes a single function that simply panics (aborts)
        state.h - general UNN state expressed by a single structure and it's helper functions
        window.h - general definitions for window, grid, etc. and it's helper functions
        unn.c - state + logic glue and bootstrapper

    !!  Multithreading problems do currently exist,
    !!  but certainly are going to be fixed after
    !!  some time.

    TODO (appropximated):
        * add togglable movement options, such as view adjustment flags, etc.

        * focused/unfocused for colored buffers
        * dline, line <-> wstr interop
        
        * make colors focused and unfocsed separate instances of one struct for usability

        * headers hell, is this codebase even at least a bit comprehensible for anyone except me??

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
            Sexp move, delete, jump
            Procedures, macros, special forms database
                for completion, query features

        * Scheme Lisp integration

        - MVP. From now on Scheme code is prioritized
            Will I be using SemVer? If so, MVP is 1.0.0
            Thus, releases are needed(archives of precompiled binaries + source code)
        - Code refactoring and various fixes are obvious
        - Most probably, I will begin rewriting UNN from scratch, but in Scheme this time

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
#include <locale.h>

#include <pthread.h>

pthread_mutex_t log_mutex;

// #define DEBUG 1

#define DEBUG_FILE 1
#define DEBUG_BUFFER 1

#define DEBUG_BUFFER_LIMIT 1000

#ifdef DEBUG
void logg(char *fmt, ...) {
    static FILE *errfp;
    va_list ap; va_start(ap, fmt);

    pthread_mutex_lock(&log_mutex);

    #ifdef DEBUG_FILE
    errfp = fopen("logs.txt", "a");

    vfprintf(errfp, fmt, ap);

    fclose(errfp);
    #endif
    

    #ifdef DEBUG_BUFFER
    // TODO
    #endif

    va_end(ap);
    
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
    S.colors_default.focused = (colors) {
        .cur = RGB_PAIR(255, 255, 255, 0, 0, 0),
        .cur_line = RGB_PAIR(10, 10, 10, 240, 240, 240),
        .gen = RGB_PAIR(0, 0, 0, 255, 255, 255),
    };

    S.colors_default.unfocused = (colors) {
        .cur = RGB_PAIR(250, 250, 250, 5, 5, 5),
        .cur_line = RGB_PAIR(5, 5, 5, 245, 245, 245),
        .gen = RGB_PAIR(5, 5, 5, 250, 250, 250),
    };
    
    S.colors_prompt.focused = (colors) {
        .cur = RGB_PAIR(255, 255, 230, 0, 0, 25),
        .cur_line = RGB_PAIR(0, 0, 25, 255, 255, 230), // same as gen
        .gen = RGB_PAIR(0, 0, 25, 255, 255, 230),
    };

    S.colors_prompt.unfocused = (colors) {
        .cur = RGB_PAIR(250, 250, 225, 5, 5, 30),
        .cur_line = RGB_PAIR(5, 5, 30, 250, 250, 225), // same as gen
        .gen = RGB_PAIR(5, 5, 30, 250, 250, 225),
    };

    S.colors_status = S.colors_default.focused.cur;

    // binds
    for(int i = 0; i < sizeof(MOVE_BINDINGS) / sizeof(*MOVE_BINDINGS) - 1; i++) {
        binds_set(S.binds_move, NULL, MOVE_BINDINGS + i);
    }

    for(int i = 0; i < sizeof(EDIT_BINDINGS) / sizeof(*EDIT_BINDINGS) - 1; i++) {
        binds_set(S.binds_edit, NULL, EDIT_BINDINGS + i);
    }

    // TODO: set prompt binds
    // currently unneeded, one bind(enter, newline) is hardcoded

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
    setlocale(LC_ALL, "");
    
    unn_init();
    unn_run();
    logg("Returning from main\n");
    return 0;
}