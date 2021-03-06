;; these are taken and adapted from 
;; https://github.com/gypified/chibi-scheme/blob/master/tests/r7rs-tests.scm
;; XXX this should be converted to the "test" framework used by chibi once
;; we can actually run that (needs define-syntax et al)

; XXX incomplete section!

(cond ((> 3 2) 'greater)
        ((< 3 2) 'less))

(cond ((> 3 3) 'greater)
        ((< 3 3) 'less)
        (else 'equal))

(cond ((assv 'b '((a 1) (b 2))) => cadr)
      (else #f))

(case (* 2 3)
      ((2 3 5 7) 'prime)
      ((1 4 6 8 9) 'composite))

(case (car '(c d))
      ((a e i o u) 'vowel)
      ((w y) 'semivowel)
      (else => (lambda (x) x)))

(map (lambda (x)
        (case x
            ((a e i o u) => (lambda (w) (cons 'vowel w)))
            ((w y) (cons 'semivowel x))
            (else => (lambda (w) (cons 'other w)))))
        '(z y x w u))


(and (= 2 2) (> 2 1))
(and (= 2 2) (< 2 1))
(and 1 2 'c '(f g))
(and)
(or (= 2 2) (> 2 1))
(or (= 2 2) (< 2 1))
(or #f #f #f)
(or (memq 'b '(a b c))
    (/ 3 0))
(let ((x 2) (y 3))
  (* x y))
(let ((x 2) (y 3))
  (let ((x 7)
        (z (+ x y)))
    (* z x)))
(let ((x 2) (y 3))
  (let* ((x 7)
         (z (+ x y)))
    (* z x)))
(letrec [(even?
            (lambda (n)
                (if (zero? n)
                    #t
                    (odd? (- n 1)))))
         (odd?
            (lambda (n)
                (if (zero? n)
                    #f
                    (even? (- n 1)))))]
    (even? 88))

(letrec* [(p (lambda (x)
                (+ 1 (q (- x 1)))))
          (q (lambda (y)
                (if (zero? y)
                    0
                    (+ 1 (p (- y 1))))))
          (x (p 5))
          (y x)]
            y)
(do ((vec (make-vector 5))
     (i 0 (+ i 1)))
    ((= i 5) vec)
  (vector-set! vec i i))

(let ((x '(1 3 5 7 9)))
  (do ((x x (cdr x))
       (sum 0 (+ sum (car x))))
      ((null? x) sum)))

(let ()
  (define x 0)
  (set! x 5)
    (+ x 1))
===
greater
equal
2
composite
c
((other . z) (semivowel . y) (other . x) (semivowel . w) (vowel . u))
#t
#f
(f g)
#t
#t
#t
#f
(b c)
6
35
70
#t
5
#(0 1 2 3 4)
25
6
