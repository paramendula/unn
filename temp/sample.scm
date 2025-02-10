; a single line comment

'("hello"
1 2 3 4 52 6 7 8 9 1000
#!abc
"lol")

#|
a block comment
that spans multiple lines
|#

#\space
#\ 
#\abc
#\x42

(define (fib n)
    (cond ((= n 1) 0)
          ((= n 2) 1)
          (else (+ (fib (- n 2)) (fib (- n 1))))))

(define (occur a lat)
   (cond ((null? lat) 0)
         ((eqan? (car lat) a) (add1 (occur a (cdr lat))))
         (else (occur a (cdr lat)))))

(define (subst* old new l)
   (cond ((null? l) '())
         ((atom? (car l)) (cond ((eq? (car l) old)
                                 (cons new (subst* old new (cdr l))))
                                (else (cons (car l) (subst* old new (cdr l))))))
         (else (cons (subst* old new (car l)) (subst* old new (cdr l))))))

(define (one? n)
   (o= n 1))

(define (one?2 n)
   (zero? (sub1 n)))

(define (rember* a l)
   (cond ((null? l) '())
         ((atom? (car l))
          (cond ((eq? (car l) a) (rember* a (cdr l)))
                (else (cons (car l) (rember* a (cdr l))))))
         (else (cond ((null? (car l)) '())
                     (else (cons (rember* a (car l)) (rember* a (cdr l))))))))

(member char '(#\space #\newline))