#ifndef __UNN_BINDS_H_
#define __UNN_BINDS_H_

#include "bind.h"
#include "commands.h"

// compile-time edit only, for run-time set and del use commands

ubind WINDOW_BINDINGS[] = {
    { 0, NULL, { NULL } },
};

ubind FILE_BINDINGS[] = {
    { 0, "o", { NULL } }, // open file in new buffer, switch window's buffer to the new one
    { 0, "ss", { NULL } }, // save buffer's content to it's path
    { 0, "so", { NULL } }, // change buffer's path to new one, save buffer's content to it's path
    { 0, NULL, { NULL } },
};

ubind BUFFER_BINDINGS[] = {
    { 0, "n", { NULL } }, // create new empty buffer, switch window's buffer to the new one
    { 0, "dd", { NULL } }, // try to destroy current buffer
    { 0, "do", { NULL } }, // try to destroy other buffer, selected from help window
    { 0, "s", { NULL } }, // switch current window's buffer to the new one, selected from help window
    { 0, NULL, { NULL } },
};

ubind CONTROL_BINDINGS[] = { // 'c' in MOVE or Ctrl+
    { 1, "w", { .cont = WINDOW_BINDINGS } },
    { 1, "f", { .cont = FILE_BINDINGS } },
    { 1, "b", { .cont = BUFFER_BINDINGS } },
    { 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, "^c", { clear_input_buffer_and_move } },
    { 0, "q", { try_exit } },
    { 0, NULL, { NULL } },
};

ubind MOVE_BINDINGS[] = {
    { 1, "c", { .cont = CONTROL_BINDINGS } },
    { 1, "^", { .cont = CONTROL_BINDINGS } },
    { 0, "w", { cursor_up } }, // cursor_up
    { 0, "s", { cursor_left } }, // cursor_left
    { 0, "k", { cursor_right } }, // cursor_right
    { 0, "m", { cursor_down } }, // cursor_down
    { 0, "z", { cursor_fastmode_toggle } }, 
    { 0, "x", { cursor_viewmode_toggle } }, 
    { 0, "i", { mode_toggle } }, // switch mode to EDIT
    { 0, "a", { enter_append } }, // move right once, switch mode to EDIT
    { 0, NULL, { NULL } },
};

ubind EDIT_BINDINGS[] = {
    { 1, "^", { .cont = MOVE_BINDINGS } }, // Ctrl is move bindings 
    { 0, "^n", { cursor_down } }, // cursor_down, ^m is newline on most tty's
    { 0, "(sk)", { mode_move } }, // switch mode to MOVE
    { 0, "(sm)", { mode_move } }, // switch mode to MOVE
    { 0, "(sw)", { mode_move } }, // switch mode to MOVE
    { 0, "(sk)", { mode_move } }, // switch mode to MOVE
    { 0, "(wk)", { mode_move } }, // switch mode to MOVE
    { 0, "(wm)", { mode_move } }, // switch mode to MOVE
    { 0, "(km)", { mode_move } }, // switch mode to MOVE
    { 0, NULL, { NULL } }, 
}; // all other printable characters are input

#endif