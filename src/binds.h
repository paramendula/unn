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

#ifndef __UNN_BINDS_H_
#define __UNN_BINDS_H_

#include "bind.h"
#include "commands.h"

// compile-time edit only, for runtime set and del use commands

ubind WINDOW_BINDINGS[] = {
    { 0, 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, 0, "o", { current_window_switch_other } }, // switch focus from current window to the other one
    { 0, 0, "n", { new_window_command } }, // create new empty window, focus on it
    { 0, 0, "g", { current_window_switch_prev } }, // switch focus from current window to the previous one
    { 0, 0, "h", { current_window_switch_next } }, // switch focus from current window to the next one
    { 0, 0, "w", { current_window_switch_up } }, // switch focus from current window to the nearest above one
    { 0, 0, "s", { current_window_switch_left } }, // switch focus from current window to the nearest left one
    { 0, 0, "k", { current_window_switch_right } }, // switch focus from current window to the nearest right one
    { 0, 0, "m", { current_window_switch_down } }, // switch focus from current window to the nearest below one
    { 0, 0, "p", { current_window_switch_prompt } }, // switch to the prompt window
    { 0, 0, "dd", { current_window_destroy } }, // try to destroy current window
    { 0, 0, "do", { window_other_destroy } }, // try to destroy other window, selected from help window
    { 0, 0, NULL, { NULL } },
};

ubind FILE_BINDINGS[] = {
    { 0, 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, 0, "o", { current_buffer_switch_from_file } }, // open file in new buffer, switch window's buffer to the new one
    { 0, 0, "ss", { current_buffer_save } }, // save buffer's content to it's path
    { 0, 0, "so", { current_buffer_save_other } }, // change buffer's path to new one, save buffer's content to it's path
    { 0, 0, NULL, { NULL } },
};

ubind BUFFER_BINDINGS[] = {
    { 0, 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, 0, "n", { current_buffer_switch_new } }, // create new empty buffer, switch window's buffer to the new one
    { 0, 0, "dd", { current_buffer_destroy } }, // try to destroy current buffer
    { 0, 0, "do", { buffer_other_destroy } }, // try to destroy other buffer, selected from help window
    { 0, 0, "s", { current_buffer_switch_other } }, // switch current window's buffer to the new one, selected from help window
    { 0, 0, NULL, { NULL } },
};

ubind TOGGLE_BINDINGS[] = {
    { 0, 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, 0, "n", { current_window_toggle_lines } }, // toggle line numbering
    { 0, 0, "m", { current_window_toggle_marks } }, // toggle long line marks
    { 0, 0, "w", { current_window_toggle_wrap } }, // toggle line wrapping
    { 0, 0, "l", { current_window_toggle_lisp } }, // toggle lisp mode
    { 0, 0, NULL, { NULL } },
};

ubind CONTROL_BINDINGS[] = { // 'c' in MOVE or Ctrl+
    { 1, 0, "w", { .cont = WINDOW_BINDINGS } },
    { 1, 0, "f", { .cont = FILE_BINDINGS } },
    { 1, 0, "b", { .cont = BUFFER_BINDINGS } },
    { 1, 0, "t", { .cont = TOGGLE_BINDINGS } },
    { 0, 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, 0, "^c", { clear_input_buffer_and_move } },
    { 0, 0, "mi", { current_buffer_inverse_cur_color }}, // temp
    { 0, 0, "q", { try_exit } },
    { 0, 0, NULL, { NULL } },
};

ubind LINE_BINDINGS[] = {
    { 0, 0, "s", { cursor_line_beg } },
    { 0, 0, "k", { cursor_line_end } },
    { 0, 0, NULL, { NULL } },
};

ubind MOVE_BINDINGS[] = {
    { 1, 0, "c", { .cont = CONTROL_BINDINGS } },
    { 1, 0, "^", { .cont = CONTROL_BINDINGS } },
    { 1, 0, "l", { .cont = LINE_BINDINGS } },
    { 0, 0, "w", { cursor_up } }, // cursor_up
    { 0, 0, "s", { cursor_left } }, // cursor_left
    { 0, 0, "k", { cursor_right } }, // cursor_right
    { 0, 0, "m", { cursor_down } }, // cursor_down
    { 0, 0, "f", { cursor_leap_word } }, // move cursor to the char after the end of a word
    { 0, 0, "b", { cursor_leap_word_back } }, // move cursor to the char before the beginning of a word
    // maybe move those to control?
    { 0, 0, "z", { cursor_fastmode_toggle } }, // move 5 units instead of 1
    { 0, 0, "x", { cursor_viewmode_toggle } }, // move view insted of cursor
    // move to beg of buffer
    // move to end of buffer
    // move to beg of view rect
    // move to end of view rect
    // move to line N
    { 0, 0, "o", { cursor_rotate_view } }, // dislocate view around cursor(cur at top, cur at mid, cur at bot)
    // enable selection
    // copy selected to unn's clipboard and system clipboard
    // paste selection from unn's clipboard cursor (consequent activations move clipboard's cursor)
    // copy selection and then erase it
    { 0, 0, "i", { mode_toggle } }, // switch mode to EDIT
    { 0, 0, "a", { enter_append } }, // move right once, switch mode to EDIT
    { 0, 0, NULL, { NULL } },
};

ubind EDIT_BINDINGS[] = {
    { 1, 0, "^", { .cont = MOVE_BINDINGS } }, // Ctrl is move bindings 
    { 0, 0, "^n", { cursor_down } }, // cursor_down, ^m is newline on the most tty's
    { 0, 0, "(sk)", { mode_move } }, // switch mode to MOVE
    { 0, 0, "(sm)", { mode_move } }, // switch mode to MOVE
    { 0, 0, "(sw)", { mode_move } }, // switch mode to MOVE
    { 0, 0, "(sk)", { mode_move } }, // switch mode to MOVE
    { 0, 0, "(wk)", { mode_move } }, // switch mode to MOVE
    { 0, 0, "(wm)", { mode_move } }, // switch mode to MOVE
    { 0, 0, "(km)", { mode_move } }, // switch mode to MOVE
    { 0, 0, NULL, { NULL } }, 
}; // all other printable characters are input

#endif