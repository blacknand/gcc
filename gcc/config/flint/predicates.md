;; Predicate definitions for Nathan Blackburn
;; Copyright (C) 2018-2026 Free Software Foundation, Inc.
;; Contributed by Nathan Blackburn 
;; Contributed by Stafford Horne (OpenRISC)

;; This file is part of GCC.

;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 3, or (at your
;; option) any later version.

;; GCC is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
;; License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; -------------------------------------------------------------------------
;; Predicates
;; -------------------------------------------------------------------------

;; Return true for a "virtual" or "soft" register that will be
;; adjusted to a "soft" or "hard" register during elimination.
(define_predicate "virtual_frame_reg_operand"
  (match_code "reg")
{
  unsigned regno = REGNO (op);
  return (regno != STACK_POINTER_REGNUM
	  && regno != HARD_FRAME_POINTER_REGNUM
	  && REGNO_PTR_FRAME_P (regno));
})

(define_predicate "const0_operand"
  (and (match_code "const_int,const_wide_int")
       (match_test "op == CONST0_RTX (mode)")))

(define_predicate "reg_or_0_operand"
  (ior (match_operand 0 "register_operand")
       (match_operand 0 "const0_operand")))

(define_predicate "input_operand"
  (ior (match_operand 0 "register_operand") 
       (match_operand 0 "memory_operand")   
       (and (match_code "const_int")
	          (match_test "satisfies_constraint_I (op)"))
  ))