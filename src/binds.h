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
    { 0, "o", { current_window_switch_other } }, // switch focus from current window to the other one
    { 0, "n", { new_window_command } }, // create new empty window, focus on it
    { 0, "b", { current_window_switch_prev } }, // switch focus from current window to the previous one
    { 0, "n", { current_window_switch_next } }, // switch focus from current window to the next one
    { 0, "w", { current_window_switch_up } }, // switch focus from current window to the nearest above one
    { 0, "s", { current_window_switch_left } }, // switch focus from current window to the nearest left one
    { 0, "k", { current_window_switch_right } }, // switch focus from current window to the nearest right one
    { 0, "m", { current_window_switch_down } }, // switch focus from current window to the nearest below one
    { 0, NULL, { NULL } },
};

ubind FILE_BINDINGS[] = {
    { 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, "o", { current_buffer_switch_from_file } }, // open file in new buffer, switch window's buffer to the new one
    { 0, "ss", { current_buffer_save } }, // save buffer's content to it's path
    { 0, "so", { current_buffer_save_other } }, // change buffer's path to new one, save buffer's content to it's path
    { 0, NULL, { NULL } },
};

ubind BUFFER_BINDINGS[] = {
    { 0, "c", { clear_input_buffer_and_move } }, // exit this continuation
    { 0, "n", { current_buffer_switch_new } }, // create new empty buffer, switch window's buffer to the new one
    { 0, "dd", { current_buffer_destroy } }, // try to destroy current buffer
    { 0, "do", { buffer_other_destroy } }, // try to destroy other buffer, selected from help window
    { 0, "s", { current_buffer_switch_other } }, // switch current window's buffer to the new one, selected from help window
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