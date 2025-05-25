;; Welcome to minilisp
(display "Welcome to minilisp")
(newline)

;; Simple aliasing
(define <= lequ)


;; Functions as lambdas
(define not
    (lambda (x)
      (if x #f #t)))

(define print
    (lambda (x)
      (display x)
      (newline)))

(define id
    (lambda (x)
      x))

(define list?
  (lambda (x)
    (or (null? x)
        (and (pair? x)
             (list? (cdr x))))))


;; Test of builtins
(if (not (eq? #t (null? '())))
    (print "Failed: (null? '())"))
(if (not (eq? #f (null? '(1 2 3))))
    (print "Failed: (null? '(1 2 3))"))
(if (not (eq? #f (null? 42)))
    (print "Failed: (null? 42)"))
(if (not (eq? #f (null? #f)))
    (print "Failed: (null? #f)"))
