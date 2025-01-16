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

#ifndef __UNN_LISP_H__
#define __UNN_LISP_H__

// external Lisp bindings
// let's assume Scheme, for now?

#include <wchar.h>

// try to evaluate **ex** as proper Scheme Lisp Sexp
// returns 0 if everything is OK, any other value if ERR
extern int lisp_run(wchar_t *ex);

#endif