;;; Tests

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

;; =
(if (not (eq? #t (= 3 3))) (print "Failed: (= 3 3)"))
(if (not (eq? #t (= 3 3 3))) (print "Failed: (= 3 3 3)"))
(if (not (eq? #f (= 3 4))) (print "Failed: (= 3 4)"))
(if (not (eq? #f (= 3 3 4))) (print "Failed: (= 3 3 4)"))

;; <=
(if (not (eq? #t (<= 1 2))) (print "Failed: (<= 1 2)"))
(if (not (eq? #t (<= 2 2))) (print "Failed: (<= 2 2)"))
(if (not (eq? #t (<= 1 2 3 3 3))) (print "Failed: (<= 1 2 3 3 3)"))
(if (not (eq? #f (<= 2 1))) (print "Failed: (<= 2 1)"))
(if (not (eq? #f (<= 1 2 1))) (print "Failed: (<= 1 2 1)"))

;; cond 
(if (not (= 42 (cond (#t 42)))) (print "Failed: (cond (#t 42))"))
(if (not (= 99 (cond (#f 1) (#t 99)))) (print "Failed: (cond (#f 1) (#t 99))"))
(if (not (eq? 'cond (cond (#f 1) (else 'cond)))) (print "Failed: (cond (#f 1) (else 'cond))"))
(if (not (= 3 (cond (#t 1 2 3)))) (print "Failed: (cond (#t 1 2 3))"))
(if (not (eq? '() (cond (#f 1) (#f 2)))) (print "Failed: (cond (#f 1) (#f 2))"))

;; let
(if (not (= 3 (let ((x 1) (y 2)) (+ x y)))) (print "Failed: let basic binding"))
(if (not (= 4 (let ((x 4)) x))) (print "Failed: let single binding"))
(if (not (= 10 (let ((a 1) (b 2) (c 5)) (* a b c)))) (print "Failed: let multiple bindings"))
(if (not (= 7 (let ((x 3)) (let ((y 4)) (+ x y))))) (print "Failed: nested let"))
(if (not (= 6 (let ((x 3)) (let ((x 6)) x)))) (print "Failed: let shadowing"))
