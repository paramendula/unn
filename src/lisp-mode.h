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

#include <wchar.h>
#include <wctype.h>

#include "colors.h"
#include "window.h"
#include "err.h"

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
// also let's make so that lparse* won't be needlessly free'd and malloc'd
//      as we'd have to do this a lot if impl written wrongly

// everything is assumed to be a Symbolic Expression: sexp
// a sexp can only be either a list or an atom
// a list consists of 1 or more sexps
// null is a list with 0 elements and is virtual (exists only conceptually, and, in fact, just a null ptr)

// parser logic and context

// Lisp Parse Datum Type
// not-so-compliant to the ref.
typedef enum lpdatype {
    tList,
    tVector,
    tNumber,
    tString,
    tBoolFalse, // no mem
    tBoolTrue, // no mem
    tIdent,
    tDirective,
    tComSingle,
    tComDatum,
    tComBlock,
    tApost, // ' no mem
    tGrave, // ` no mem
    tComma, // , no mem
    tSeqComma, // ,@ no mem
    tLabel, // #n=
    tLabelRef, // #n#
} lpdatype;

typedef struct dat_list {
    int len;
    struct datum *first, *last;
} dat_list;

// practically, same as dat_list
typedef struct dat_vec {
    int len;
    struct datum *first, *last;
} dat_vec;

// TODO: process "raw" datums(wchar_t *) for possible future usage

// tagged union
typedef struct datum {
    struct datum *prev, *next;

    lpdatype t;
    union {
        dat_list *list;
        dat_vec *vec;
        wchar_t *num;
        wchar_t *str;
        wchar_t *ident;
        wchar_t *dir;
        wchar_t *com_sin;
        wchar_t *com_dat;
        wchar_t *com_blk;
        wchar_t *lab;
        wchar_t *lab_ref;
    };
} datum;

// last list in the stack receives all parsed datums until it encounters a list datum
// beginning a list datum parsing appends it to the stack
// finishing a list datum parsing pops it from the stack
typedef struct lparse {
    int stack_size;
    datum *first, *last; // lists, vectors only

    int line, symbol;
    err pe; // parse error
} lparse;

void datum_free(datum *d) {
    if(!d) return;

}

void lp_free(lparse *state) {
    if(!state) return;

    node_free_nexts((node *)state->first, (free_func)datum_free);

    free(state);
}

// state, rfunc and e can't be NULL
// bad lisp != error; state should handle bad lisp info propagation
int lp_parse(lparse *state, read_func rfunc, void *data, err *e) {
    wchar_t ch;

    if(!state)
        return err_set(e, -1, L"lparse *state is NULL");

    if(!rfunc)
        return err_set(e, -1, L"read_func rfunc is NULL");

    if(!e)
        return err_set(e, -1, L"err *e is NULL");


    datum *top_level = (datum *)malloc(sizeof(*top_level));

    if(!top_level)
        return err_set(e, -2, L"top_level: not enough memory");
    

    dat_list *top_level_list = (dat_list *)calloc(1, sizeof(*top_level_list));
    
    if(!top_level_list) {
        free(top_level);
        return err_set(e, -2, L"top_level_list: not enough memory");
    }


    top_level->t = tList;
    top_level->list = top_level_list;

    list_append((list *)state, (node *)top_level);

    state->pe.code = 0;
    memset(&state->pe.message, 0, sizeof(state->pe.message));

    state->line = 0;
    state->symbol = 0;


    char flag_hash = 0;

    while (1) {
        ch = rfunc(data, e);

        if(e->code) {
            if(e->code == ERR_READ_EOF) {
                // done parsing
                e->code = 0;
                break;
            } else {
                lp_free(state);
                return e->code;
            }
        }

        state->symbol++;

        // finishing a list or a vector
        if(ch == L')') {
            if(state->stack_size < 2) {
                err_set(&state->pe, 2, L"unexpected token ')'");
                lp_free(state);
                return 0;
            }

            // pop from the stack
            list_remove((list *)state, (node *)state->last);

            continue;
        }

        if(ch == L'#') {
            flag_hash = 1;
            continue;
        }

        // skip whitespace, only if no flags on
        if(!flag_hash) {
            if(ch == L'\n') {
                state->symbol = 0;
                state->line++;
                continue;
            } else if(iswblank(ch)) {
                continue;
            }

        }

        datum *new_datum = (datum *)calloc(1, sizeof(*new_datum));

        if(!new_datum) {
            lp_free(state);
            return err_set(e, -2, L"new_datum: not enough memory");
        }

        if(flag_hash) {

            continue;
        }

        if(ch == L'\'') {           // tApost
            new_datum->t = tApost;
        } else if(ch == L'(') {     // tList
            dat_list *l = (dat_list *)calloc(1, sizeof(*l));

            if(!l) {
                lp_free(state);
                return err_set(e, -2, L"dat_list: not enough memory");
            }

            new_datum->t = tList;
            new_datum->list = l;
        }
        
        // at this point, new datum is ready

        list_append((list *)state->last, (node *)new_datum);

        // append to the stack if we're entering a list or a vector
        if(new_datum->t = tList || new_datum->t == tVector) { 
            list_append((list *)state, (node *)new_datum);
        }
    }

    // at this point, function finished successfully, 
    //  only parsing errors are possible

    // if no error ATM
    if(!state->pe.code) {
        if(state->stack_size != 1) {
            err_set(&state->pe, 1, L"unfinished list or excessive '(', stack_size != 1");
        } else if(flag_hash) {
            err_set(&state->pe, 3, L"unfinished '#'-starting datum");
        }
    }

    return 0;
}

#endif