(import boolean)
(import assert)
(assert (eq? () ()))
(assert (eq? (eq? 'x ()) ()))
(assert (eq? (eq? () 'x) ()))
(assert (eq? 'x 'x))
(assert (not ()))
(assert (not (not 't)))
(assert (equal? () ()))
(assert (not (equal? 'x ())))
(assert (not (equal? () 'x)))
(assert (equal? 'x 'x))
(assert (not (equal? '(x) 'x)))
(assert (not (equal? 'x '(x))))
(assert (not (equal? '(x) ())))
(assert (not (equal? () '(x))))
(assert (equal? '(x) '(x)))
(assert (equal? '((x) (x)) '((x) (x))))