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
#include <stdarg.h>

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

    // additional data
    int line, symbol;

    // for wchar_t* datums
    int len, cap;

    union {
        wchar_t *str;

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

typedef struct perr_node {
    struct perr_node *prev, *next;
    union {
        struct {
            int code;
            wchar_t message[512];
        };
        err e;
    };
    int line, symbol;
} perr_node;

typedef struct perr_list {
    int len;
    perr_node *first, *last;
} perr_list;

int pe_append(perr_list *pe, int code, int line, int symbol, wchar_t *fmt, ...) {
    perr_node *n = (perr_node *)malloc(sizeof(*n));

    if(!n) return -1;

    va_list ap; va_start(ap, fmt);

    vswprintf(n->message, ERRMSG_LEN, fmt, ap);

    va_end(ap);

    n->code = code;
    n->line = line;
    n->symbol = symbol;

    list_append((list *)pe, (node *)n);

    return 0;
}

// last list in the stack receives all parsed datums until it encounters a list datum
// beginning a list datum parsing appends it to the stack
// finishing a list datum parsing pops it from the stack
typedef struct lparse {
    int stack_size;
    datum *first, *last; // lists, vectors only

    perr_list pe; // parse errors list
} lparse;

// we only use this function for 'raw' datums that are essentialy w-strings
// allocated block is always cap + 1 sized
int datum_append(datum *d, wchar_t ch) {
    if(!d) return -1;

    if(!d->str) {
        d->str = (wchar_t *)malloc(sizeof(*d->str) * 5);

        if(!d->str) return -2;

        d->cap = 4;
        d->len = 1;
    } else {
        if(d->len == d->cap) {
            int new_cap = d->cap * 2;

            wchar_t *new_str = (wchar_t *)realloc(d->str, sizeof(*d->str) * (new_cap + 1));

            if(!new_str) {
                return -2;
            }

            d->str = new_str;
            d->cap = new_cap;
        }
    }

    d->str[d->len++] = ch;
    d->str[d->len] = 0;

    return 0;
}

void datum_free(datum *d) {
    if(!d) return;

}

void lp_free(lparse *state) {
    if(!state) return;

    node_free_nexts((node *)state->first, (free_func)datum_free);
    node_free_nexts((node *)state->pe.first, free);

    free(state);
}

inline static int is_ident_symbol(wchar_t ch) {
    return ((ch != ',') && (ch != '`') && (ch != '\'') && iswprint(ch) && !iswspace(ch));
}

// state, rfunc and e can't be NULL
// bad lisp != error; state should handle bad lisp info propagation
// rfunc should infinitely send ERROR_READ_EOF when it's exhausted
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

    memset(&state->pe, 0, sizeof(state->pe));


    int line = 0;
    int symbol = 0;

    perr_list *pe = &state->pe;


    char flag_hash = 0;
    char flag_comma = 0;
    char flag_com_sin = 0;
    char flag_directive = 0;

    char flag_bool_check = 0;

    datum *new_datum = NULL;
    char datum_done = 0;

    char keep_last = 0;
    wchar_t last_ch = 0;

    while (1) {
        if(datum_done && new_datum) {
            list_append((list *)state->last, (node *)new_datum);

            // append to the stack if we're entering a list or a vector
            if(new_datum->t = tList || new_datum->t == tVector) { 
                list_append((list *)state, (node *)new_datum);
            }

            new_datum = NULL;
            datum_done = 0;
        }

        if(!keep_last) {
            last_ch = ch;
            ch = rfunc(data, e);
        } else {
            keep_last = 0;
        }

        if(e->code) {
            if(e->code == ERR_READ_EOF) {
                // done parsing
                e->code = 0;

                if(new_datum) {
                    ch = 0;
                    // new_datum should be done with in this iteration
                    // otherwise it's a bug
                } else break;
            } else {
                lp_free(state);
                return e->code;
            }
        }
        
        symbol++;

        if(flag_hash) {
            flag_hash = 0;

            if(ch == 0) {
                pe_append(pe, -1, line, symbol, L"unexpected char before EOF: '#'");
            } else if(ch == L'f' || ch == L'F') {
                new_datum->t = tBoolFalse;
                flag_bool_check = 1;
            } else if(ch == L't' || ch == L'T') {
                new_datum->t = tBoolTrue;
                flag_bool_check = 1;
            } else if(ch == L'!') {
                flag_directive = 1;
            }

            continue;
        } else if(flag_comma) {
            if(ch == L'@') {
                new_datum->t = tSeqComma;
            } else {
                new_datum->t = tComma;
            }
            
            datum_done = 1;
            flag_comma = 0;

            continue;
        } else if(flag_com_sin) {
            if(ch == L'\n' || ch == 0) {
                datum_done = 1;
                flag_com_sin = 0;
            } else {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, com_sin");
                }
            }

            continue;
        } else if(flag_bool_check) {
            flag_bool_check = 0;

            if(!is_ident_symbol(ch)) {
                datum_done = 1;
            } else {
                free(new_datum);
                new_datum = NULL;

                pe_append(pe, -3, line, symbol, L"invalid token, starts as bool");
            }
        } else if(flag_directive) {
            if(is_ident_symbol(ch)) {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, com_sin");
                }
            } else {
                keep_last = 1;

                if(new_datum->len == 0) {
                    free(new_datum);
                    pe_append(pe, -1, line, symbol, L"unfinished directive '#!'");
                } else {
                    datum_done = 1;
                }
            }
        }

        // skipping whitespace
        if(ch == L'\n') {
            symbol = 0;
            line++;
            continue;
        } else if(iswblank(ch)) {
            continue;
        }

        // finishing a list or a vector
        if(ch == L')') {
            if(state->stack_size < 2) {
                pe_append(pe, -1, line, symbol, L"unexpected token: ')'");
                continue;
            }

            // pop from the stack
            list_remove((list *)state, (node *)state->last);

            continue;
        }

        new_datum = (datum *)calloc(1, sizeof(*new_datum));

        if(!new_datum) {
            lp_free(state);
            return err_set(e, -2, L"new_datum: not enough memory");
        }

        // TODO: account for all beginnings

        if(ch == L'\'') {           // tApost
            datum_done = 1;
            new_datum->t = tApost;
        } else if(ch == L'`') {     // tGrave
            datum_done = 1;
            new_datum->t = tGrave;
        } else if(ch == L',') {     // tComma, tSeqComma
            flag_comma = 1;
        } else if(ch == L'(') {     // tList
            dat_list *l = (dat_list *)calloc(1, sizeof(*l));

            if(!l) {
                lp_free(state);
                return err_set(e, -2, L"dat_list: not enough memory");
            }

            datum_done = 1;

            new_datum->t = tList;
            new_datum->list = l;
        } else if(ch == L';') {     // tComSingle
            flag_com_sin = 1;
            new_datum->t = tComSingle;
        } else if(ch == L'#') {
            flag_hash = 1;
        }
    }

    // at this point, function finished successfully, 
    //  only parsing errors are possible

    if(state->stack_size != 1) {
        pe_append(pe, -2, line, symbol, L"unfinished list or excessive '(', stack_size != 1");
    }

    return 0;
}

#endif