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
    tByteVector,
    tChar,
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

// practically, same as dat_list
typedef struct dat_bvec {
    int len;
    struct datum *first, *last;
} dat_bvec;

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
        dat_bvec *bvec;
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
    int len;
    int line, symbol;
} perr_node;

typedef struct perr_list {
    int len;
    perr_node *first, *last;
} perr_list;

int pe_append(perr_list *pe, int code, int len, int line, int symbol, wchar_t *fmt, ...) {
    perr_node *n = (perr_node *)malloc(sizeof(*n));

    if(!n) return -1;

    va_list ap; va_start(ap, fmt);

    vswprintf(n->message, ERRMSG_LEN, fmt, ap);

    va_end(ap);

    n->code = code;
    n->len = len;
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
    return ((ch != ',') && 
            (ch != '`') && 
            (ch != '\'') && 
            (ch != '(') && 
            (ch != ')') && 
            iswprint(ch) && 
            !iswspace(ch));
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
    char flag_com_blk = 0;
    char flag_directive = 0;
    char flag_number = 0;
    char flag_ident = 0;
    char flag_str = 0;
    char flag_char = 0;
    char flag_label = 0;

    char flag_bool_check = 0;
    char flag_bytevector_check = 0;
    char flag_sign_check = 0;

    char flag_number_nosym = 0;
    char flag_number_dot = 0;
    char flag_number_exp = 0;
    char flag_number_complex = 0;

    char flag_exactness = 0; // #e #i
    char flag_base = 0; // #b #o #d #x

    char is_bytevector = 0;

    datum *new_datum = NULL;
    char datum_done = 0;

    char keep_last = 0;
    wchar_t last_ch = 0;

    while (1) {
        if(datum_done && new_datum) {
            list_append((list *)state->last, (node *)new_datum);

            if(is_bytevector) {
                // TODO: check if is u8
                if(new_datum->t != tNumber) {
                    pe_append(pe, -5, 0, new_datum->line, new_datum->symbol, 
                        L"not an u8 in a bytevector");
                }
            }

            // append to the stack if we're entering a list or a vector
            if(new_datum->t = tList || new_datum->t == tVector) { 
                list_append((list *)state, (node *)new_datum);
            } else if(new_datum->t == tByteVector) {
                is_bytevector = 1;
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

        if(flag_hash) { // tokens beginning with '#'
            flag_hash = 0;

            char new_exactness = 0;
            char new_base = 0;
            char append = 0;

            if(flag_base || flag_exactness) {
                if((ch != L'e') ||
                   (ch != L'i') ||
                   (ch != L'b') ||
                   (ch != L'o') ||
                   (ch != L'd') ||
                   (ch != L'x')) {
                    flag_base = 0;
                    flag_exactness = 0;

                    datum_free(new_datum);
                    new_datum = NULL;

                    pe_append(pe, -6, 0, line, symbol, L"wrong token, began as spec. number");

                    continue;
                }
            }

            if(ch == 0) {
                pe_append(pe, -1, 0, line, symbol, L"unexpected char before EOF: '#'");
            } else if(ch == L'f' || ch == L'F') { // booleans
                new_datum->t = tBoolFalse;
                flag_bool_check = 1;
            } else if(ch == L't' || ch == L'T') {
                new_datum->t = tBoolTrue;
                flag_bool_check = 1;
            } else if(ch == L'!') { // directive
                flag_directive = 1;
            } else if(ch == L';') { // datum comment
                datum_done = 1;
                new_datum->t = tComDatum;
            } else if(ch == L'|') { // block comment
                flag_com_blk = 1;
            } else if(ch == L'\\') { // tChar
                flag_char = 1;
            } else if(ch == L'(') { // tVector
                dat_vec *l = (dat_vec *)calloc(1, sizeof(*l));

                if(!l) {
                    lp_free(state);
                    return err_set(e, -2, L"dat_list: not enough memory");
                }

                datum_done = 1;

                new_datum->t = tVector;
                new_datum->vec = l;
            } else if(ch == L'u') {
                flag_bytevector_check = 1;
            } else if(ch == L'e' || ch == L'E') {
                new_exactness = 1;
            } else if(ch == L'i' || ch == L'I') {
                new_exactness = 2;
            } else if(ch == L'b' || ch == L'B') {
                new_base = 1;
            } else if(ch == L'o' || ch == L'O') {
                new_base = 2;
            } else if(ch == L'd' || ch == L'D') {
                new_base = 3;
            } else if(ch == L'x' || ch == L'X') {
                new_base = 4;
            } else if(iswdigit(ch)) {
                flag_label = 1;
                append = 1;
            }
 
            if(new_exactness) {
                if(flag_exactness) {
                    flag_exactness = 0;
                    flag_base = 0;
                    datum_free(new_datum);
                    new_datum = NULL;

                    pe_append(pe, -6, 0, line, symbol, L"multiple number exactness spec.");
                } else {
                    append = 1;
                    flag_exactness = new_exactness;
                }
            } else if(new_base) {
                if(flag_base) {
                    flag_exactness = 0;
                    flag_base = 0;
                    datum_free(new_datum);
                    new_datum = NULL;

                    pe_append(pe, -6, 0, new_datum->len, line, symbol, L"multiple number base spec.");
                } else {
                    append = 1;
                    flag_base = new_base;
                }
            }

            if(append) {
                if(datum_append(new_datum, L'#') || datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, hash");
                }
            }

            continue;
        } else if(flag_comma) { // ',' or ',@'
            if(ch == L'@') {
                new_datum->t = tSeqComma;
            } else {
                new_datum->t = tComma;
            }
            
            datum_done = 1;
            flag_comma = 0;

            continue;
        } else if(flag_com_sin) { // a single comment ';'
            if(ch == L'\n' || ch == 0) {
                datum_done = 1;
                flag_com_sin = 0;
                new_datum->t = tComSingle;
            } else {
                if(datum_append(new_datum, ch)) { // memory-safe w-string append
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, com_sin");
                }
            }

            continue;
        } else if(flag_bool_check) { // e.g. is it '#f' or '#fabc', which is wrong
            flag_bool_check = 0;

            if(!is_ident_symbol(ch)) {
                datum_done = 1;
            } else {
                free(new_datum);
                new_datum = NULL;

                pe_append(pe, -3, 0, line, symbol, L"invalid token, starts as bool literal");
            }

            continue;
        } else if(flag_directive) {
            if(is_ident_symbol(ch)) {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, flag_directive");
                }
            } else {
                keep_last = 1;

                if(new_datum->len == 0) { // empty directive
                    free(new_datum);
                    pe_append(pe, -1, 0, line, symbol, L"unfinished directive '#!'");
                } else {
                    datum_done = 1;
                }
            }

            continue;
        } else if(flag_bytevector_check) {
            if(flag_bytevector_check == 1) {
                if(ch == L'8') {
                    flag_bytevector_check = 2;
                } else {
                    flag_bytevector_check = 0;

                    pe_append(pe, -3, 0, line, symbol, L"invalid token");
                }
            } else {
                flag_bytevector_check = 0;

                if(ch == L'(') {
                    dat_bvec *l = (dat_bvec *)calloc(1, sizeof(*l));

                    if(!l) {
                        lp_free(state);
                        return err_set(e, -2, L"dat_bvec: not enough memory");
                    }

                    datum_done = 1;

                    new_datum->t = tByteVector;
                    new_datum->bvec = l;
                } else {
                    pe_append(pe, -3, 0, line, symbol, L"invalid token");
                }
            }

            continue;
        } else if(flag_ident) {
            if(is_ident_symbol(ch)) {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, flag_ident");
                }
            } else {
                keep_last = 1;

                if(new_datum->len == 0) { // empty directive
                    free(new_datum);
                    pe_append(pe, -1, 0, line, symbol, L"unfinished directive '#!'");
                } else {
                    datum_done = 1;
                    new_datum->t = tIdent;
                }
            }

            continue;
        } else if(flag_sign_check) { // please ignore this part
            char as_symbol = 0;
            char as_number = 0;

            // i -> n -> f
            // n -> a -> n
            if(!is_ident_symbol(ch)) {
                if(flag_sign_check == 9) {
                    as_number = 1;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 1) {
                if(ch == L'i') {
                    flag_sign_check = 2;
                } else if(ch == L'n') {
                    flag_sign_check = 4;
                } else if(iswdigit(ch)) {
                    as_number = 1;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 2) {
                if(ch == L'n') {
                    flag_sign_check = 3;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 3) {
                if(ch == L'f') {
                    flag_sign_check = 6;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 4) {
                if(ch == L'a') {
                    flag_sign_check = 5;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 5) {
                if(ch == L'n') {
                    flag_sign_check = 6;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 6) {
                if(ch == L'.') {
                    flag_sign_check = 7;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 7) {
                if(ch == L'0') {
                    flag_sign_check = 8;
                } else {
                    as_symbol = 1;
                }
            } else if(flag_sign_check == 9) {
                // not ended past (+|-)(inf|nan).0
                as_symbol = 1;
            }

            if(as_symbol) {
                keep_last = 1;
                flag_sign_check = 0;

                if(flag_number_nosym) {
                    datum_free(new_datum);
                    new_datum = NULL;
                    pe_append(pe, -6, 0, line, symbol, L"wrong token, began as spec. number");

                    flag_number_nosym = 0;

                    continue;
                }

                flag_ident = 1; // continue parsing as an identifier
            } else if(as_number) {
                keep_last = 1;
                flag_sign_check = 0;

                flag_number = 1;
            } else {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, flag_ident");
                }
            }
        } else if(flag_number) {
            char append = 0;
            char as_symbol = 0;

            if(!is_ident_symbol(ch)) {
                keep_last = 1;

                flag_number = 0;
                flag_number_dot = 0;
                flag_number_exp = 0;
                flag_number_nosym = 0;
                flag_number_complex = 0;
                flag_base = 0;
                flag_exactness = 0;

                char cpx_good = 0;

                if(flag_number_complex) {
                    wchar_t lch = new_datum->num[new_datum->len - 1];
                    if(lch != L'i' && lch != L'I') {
                        as_symbol = 1;
                    } else {
                        cpx_good = 1;
                    }
                }

                if(flag_number_exp && cpx_good) {
                    if(!iswdigit(new_datum->num[new_datum->len - 
                        ((flag_number_complex) ? 2 : 1)])) {
                        as_symbol = 1;
                    } else {
                        datum_done = 1;
                        new_datum->t = tNumber;
                    }
                } else if (cpx_good) {
                    datum_done = 1;
                    new_datum->t = tNumber;
                }
            } else if(iswdigit(ch)) {
                append = 1;
            } else if(ch == L'.') {
                if(flag_number_dot) {
                    as_symbol = 1;
                } else {
                    append = 1;
                    flag_number_dot = 1;
                }
            } else if(ch == L's' || ch == L'S' ||
                      ch == L'f' || ch == L'F' ||
                      ch == L'd' || ch == L'D' ||
                      ch == L'l' || ch == L'L' ||
                      ch == L'e' || ch == L'E') {
                if(flag_number_exp) {
                    as_symbol = 1;
                } else {
                    flag_number_exp = 1;
                }
            } else if(ch == L'-' || ch == L'+') {
                if(flag_number_complex) {
                    as_symbol = 1;
                } else {
                    flag_number_dot = 0;
                    flag_number_exp = 0;
                    flag_number_complex = 1;
                    append = 1;
                }
            } else if(flag_number_complex && (ch == L'i' || ch == L'I')) {
                if(flag_number_complex == 2) {
                    as_symbol = 1;
                } else {
                    append = 1;
                    flag_number_complex = 2;
                }
            }

            if(append) {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, number");
                }
            } else if(as_symbol) {
                if(flag_base || flag_exactness) {
                    datum_free(new_datum);
                    new_datum = NULL;

                    pe_append(pe, -6, 0, line, symbol, L"wrong token, began as spec. number");

                    continue;
                }

                flag_number = 0;
                flag_number_dot = 0;
                flag_number_exp = 0;
                flag_number_nosym = 0;
                flag_number_complex = 0;
                flag_base = 0;
                flag_exactness = 0;

                keep_last = 1;
                flag_ident = 1;
            }

            continue;
        } else if(flag_exactness || flag_base) {
            if(ch == L'#') {
                flag_hash = 1;
            } else if(ch == L'+' || ch == L'-') {
                flag_number_nosym = 1;
                flag_sign_check = 1;
            } else if(iswdigit(ch) || ch == L'.') {
                flag_number_nosym = 1;
                flag_number = 1;
            } else {
                keep_last = 1;

                datum_free(new_datum);
                new_datum = NULL;
                pe_append(pe, -6, 0, line, symbol, L"wrong token, began as spec. number");
            }

            continue;
        } else if(flag_label) {
            if(ch == L'=') {
                new_datum->t = tLabel;
            } else if(ch == L'#') {
                new_datum->t = tLabelRef;
            } else if(!is_ident_symbol(ch) || !iswdigit(ch)) {
                datum_free(new_datum);
                new_datum = NULL;

                pe_append(pe, -6, 0, line, symbol, L"wrong token");

                continue;
            } else {
                if(datum_append(new_datum, ch)) {
                    datum_free(new_datum);
                    lp_free(state);
                    return err_set(e, -2, L"datum_append: not enough memory, label");
                }

                continue;
            }

            flag_label = 0;
            datum_done = 1;  

            continue;
        } else if(flag_com_blk) {
            if(flag_com_blk == 2) {
                if(ch == L'#') {
                    flag_com_blk = 0;
                    datum_done = 1;
                } else {
                    keep_last = 1;
                    if(datum_append(new_datum, L'|')) {
                        datum_free(new_datum);
                        lp_free(state);
                        return err_set(e, -2, L"datum_append: not enough memory, com_blk|");
                    }
                }

                continue;
            }

            if(ch == L'|') {
                flag_com_blk = 2;

                continue;
            }

            if(datum_append(new_datum, ch)) {
                datum_free(new_datum);
                lp_free(state);
                return err_set(e, -2, L"datum_append: not enough memory, com_blk");
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

        // ')' means finishing a list or a vector, bytevector
        if(ch == L')') {
            if(state->stack_size < 2) {
                pe_append(pe, -1, 0, line, symbol, L"unexpected token: ')'");
                continue;
            }

            // pop from the stack
            list_remove((list *)state, (node *)state->last);

            if(is_bytevector) {
                is_bytevector = state->last->t == tByteVector;   
            }

            continue;
        }

        // ALLOCATE A NEW DATUM

        if(!new_datum) {
            new_datum = (datum *)calloc(1, sizeof(*new_datum));

            lp_free(state);
            return err_set(e, -2, L"new_datum: not enough memory");
        }

        // ACTIVATE A FLAG DEPENDING ON THE FIRST CHARACTER OF A TOKEN
        // OR IF A TOKEN IS TRIVIAL, FINISH THE DATUM   
        
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
        } else if(ch == L'#') { // tVector, tBoolFalse|True, tDirective, ...
            flag_hash = 1;
        } else if(ch == L'"') {     // tString
            flag_str = 1;
        } else if(ch == L'+' || ch == L'-') {
            if(datum_append(new_datum, ch)) {
                datum_free(new_datum);
                lp_free(state);
                return err_set(e, -2, L"datum_append: not enough memory, +, -");
            }

            flag_sign_check = 1;
        } else if(iswdigit(ch)) {   // tNumber
            flag_number = 1;
        } else if(is_ident_symbol(ch)) { // tIdent
            flag_ident = 1;
        } else { // wow! // tStupid
            pe_append(pe, -3, line, symbol, L"unknown character [%d]", ch);
        }
    }

    // at this point, function has finished successfully, 
    //  only parsing errors are possible

    if(state->stack_size != 1) {
        pe_append(pe, -2, 0, line, symbol, L"unfinished list or excessive '(', stack_size != 1");
    }

    return 0;
}

#endif