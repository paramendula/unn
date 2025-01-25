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

#ifndef __UNN_LISP_MODE_H_
#define __UNN_LISP_MODE_H_

#include "colors.h"
#include "window.h"

// analyze a file - a sequence of symbolic expressions
// database of all imported procedures, special forms and macros should exist at any moment
// features:
// auto-complete a symbol by showing all possible choices
//      or automatically choosing the single one if it is sole
// jump between matching parentheses
// moving between Sexps:
//  jump to the next Sexp, to the prev Sexp
//  escape current Sexp, get one level above
//  get inside current Sexp (if it's a list), point to the first Sexp inside the list
// editing Sexps
//  cut current Sexp
//  move current Sexp (back, forward)
// get hint about the symbol, it's definition and documentation if is in database
// quick binds for construction of predefined Sexps such as let, lambda, cond, etc.
// running the file, or selected Sexp
// REPL inside unn?
// allow running external REPLs inside unn, requires vterm or something similar that handles
//      virtual term sequences modification for sub-drawing (does this even work like that?)

// all identation is fully automatic

// you could also call it Lisp Buffer

// at first, let's make it so we have to re-parse the whole file after a single change

typedef struct buffer_lisp {
    union {
        buffer buff; 
        cbuffer cbuff;
    }; // for convenience 
    // save current Sexp, etc.
} buffer_lisp;

// override all moving/editing keybinds
// make buff's on_destroy cb call b_lisps's on_destroy

// first, primitive lisp parser needed

#endif