(define <= lequ)

(define id (lambda (x) x))

(define list?
  (lambda (x)
    (or (null? x)
        (and (pair? x)
             (list? (cdr x))))))
