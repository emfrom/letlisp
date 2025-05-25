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

;; Strict combinator
;; eta expanded Y-comb 
(define Z-combinator
  (lambda (f)
    ((lambda (x) (f (lambda (v) ((x x) v))))
     (lambda (x) (f (lambda (v) ((x x) v)))))))

(define Z-comb Z-combinator)
(define Z Z-combinator)

(define factorial
  (Z (lambda (fact)
       (lambda (n)
         (if (= n 0)
             1
             (* n (fact (- n 1))))))))


;;; Test of builtins

;; null?
(if (not (eq? #t (null? '()))) (print "Failed: (null? '())"))
(if (not (eq? #f (null? '(1 2 3)))) (print "Failed: (null? '(1 2 3))"))
(if (not (eq? #f (null? 42))) (print "Failed: (null? 42)"))
(if (not (eq? #f (null? #f))) (print "Failed: (null? #f)"))

;; and
(if (not (eq? #t (and #t #t))) (print "Failed: (and #t #t)"))
(if (not (eq? #f (and #f #t))) (print "Failed: (and #f #t)"))
(if (not (eq? #f (and #t #f))) (print "Failed: (and #t #f)"))
(if (not (eq? #f (and #f #f))) (print "Failed: (and #f #f)"))
(if (not (eq? #t (and))) (print "Failed: (and)"))

;; or 
(if (not (eq? #t (or #t #t))) (print "Failed: (or #t #t)"))
(if (not (eq? #t (or #f #t))) (print "Failed: (or #f #t)"))
(if (not (eq? #t (or #t #f))) (print "Failed: (or #t #f)"))
(if (not (eq? #f (or #f #f))) (print "Failed: (or #f #f)"))
(if (not (eq? #f (or))) (print "Failed: (or)"))

;; pair?
(if (not (eq? #f (pair? '()))) (print "Failed: (pair? '())"))
(if (not (eq? #t (pair? '(1 2)))) (print "Failed: (pair? '(1 2))"))
(if (not (eq? #f (pair? 42))) (print "Failed: (pair? 42)"))
(if (not (eq? #f (pair? #t))) (print "Failed: (pair? #t)"))

;; list?
(if (not (eq? #t (list? '()))) (print "Failed: (list? '())"))
(if (not (eq? #t (list? '(1 2 3)))) (print "Failed: (list? '(1 2 3))"))
(if (not (eq? #f (list? 42))) (print "Failed: (list? 42)"))
(if (not (eq? #f (list? #f))) (print "Failed: (list? #f)"))

