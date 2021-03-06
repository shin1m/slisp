(import assert)
(define x 'hello)
(define y '(hello world))
(print-assert-equal `() ())
(print-assert-equal `,() ())
(print-assert-equal `x 'x)
(print-assert-equal `,x 'hello)
(print-assert-equal `,'hello 'hello)
(print-assert-equal `,y '(hello world))
(print-assert-equal `(x) '(x))
(print-assert-equal `(,x) '(hello))
(print-assert-equal `(foo ,x) '(foo hello))
(print-assert-equal `(,x bar) '(hello bar))
(print-assert-equal `(foo ,x bar) '(foo hello bar))
(print-assert-equal `(,y) '((hello world)))
(print-assert-equal `(foo ,y) '(foo (hello world)))
(print-assert-equal `(,y bar) '((hello world) bar))
(print-assert-equal `(foo ,y bar) '(foo (hello world) bar))
(print-assert-equal `(foo ,(cons 'hello '(world)) bar) '(foo (hello world) bar))
(print-assert-equal `(,@y) '(hello world))
(print-assert-equal `(foo ,@y) '(foo hello world))
(print-assert-equal `(,@y bar) '(hello world bar))
(print-assert-equal `(foo ,@y bar) '(foo hello world bar))
(print-assert-equal `(foo ,@(cons 'hello '(world)) bar) '(foo hello world bar))
(define z '(world))
(print-assert-equal `(hello ,@z) '(hello world))
((lambda ()
  (define-macro xx ()
    (define x 'world)
    `(cons 'hello '(,x))
  )
  (print-assert-equal (xx) '(hello world))
))
