(define + (lambda (x y) (if x (cons (car x) (+ (cdr x) y)) y)))
(define - (lambda (x y) (if y (- (cdr x) (cdr y)) x)))
(define * (lambda (x y) (if x (+ y (* (cdr x) y)) ())))
(define > (lambda (x y) (if x (if y (> (cdr x) (cdr y)) 't))))
(export +)
(export -)
(export *)
(export >)
