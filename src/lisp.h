#ifndef __UNN_LISP_H__
#define __UNN_LISP_H__

// external Lisp bindings
// let's assume Scheme, for now?

#include <wchar.h>

// try to evaluate **ex** as proper Scheme Lisp Sexp
// returns 0 if everything is OK, any other value if ERR
extern int lisp_run(wchar_t *ex);

#endif