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

(define (abs x) (if (<= x 0) (- x) x))
(define (average x y) 
  (/ (+ x y) 2))


;;; Tests
(load "test.lsp")
