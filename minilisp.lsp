;; Welcome to minilisp
(display "Welcome to minilisp")
(newline)

;; Simple aliasing


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

;; Functions on (define (func a1 a2 ..) form
(define (foldl proc init lst)
  (if (null? lst)
      init
      (fold proc (proc
		  init
		  (car lst))
	    (cdr lst))))

(define (foldr proc init lst)
  (if (null? lst)
      init
      (proc (car lst) (foldr proc init (cdr lst)))))

(define fold foldl)


;; Recursive function defines
(define (length lst)
  (define (len-iter lst acc)
    (if (null? lst)
        acc
        (len-iter (cdr lst) (+ acc 1))))
  (len-iter lst 0))

;; ;; Testing let*q
;; (define (let* bindings body)
;;     (if (null? bindings)
;; 	(begin body)
;; 	(let ((first-binding (car bindings))
;;               (rest-bindings (cdr bindings)))
;; 	  (begin (debug rest-bindings)
;; 		 (let ((var (car first-binding))
;; 		       (val (cadr first-binding)))
;; 		   (let ((var val))
;; 		     (let* rest-bindings body)))))))

;; Strict combinator
;; (eta expanded Y-comb)
(define Z-combinator
  (lambda (f)
    ((lambda (x) (f (lambda (v) ((x x) v))))
     (lambda (x) (f (lambda (v) ((x x) v)))))))

(define Z-comb Z-combinator)
(define Z Z-combinator)


;; Mathy stuff

(define factorial
  (Z (lambda (fact)
       (lambda (n)
         (if (= n 0)
             1
             (* n (fact (- n 1))))))))

(define (abs x)
    (if (<= x 0)
	(- x)
	x))

(define (sum lst)
  (fold + 0 lst))
(define (average lst)
    (/ (sum lst)
       (length lst)))

;;; Tests
(load "test.lsp")
