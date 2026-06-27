;; Machine description for flint
;; Copyright (C) 2018-2026 Free Software Foundation, Inc.
;; Contributed by Nathan Blackburn
;; Contributed by Stafford Horne (OpenRISC)
;; GCC-VAM was used as a reference: https://github.com/embecosm/gcc-vam/commit/97863f37b4a844e596214f12d13f9b7e0b979b16

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
;; flint specific constraints, predicates and attributes
;; -------------------------------------------------------------------------

(include "constraints.md")
(include "predicates.md")

;; Register numbers
(define_constants
  [(RET_VAL_REGNUM 1)
   (HFP_REGNUM 5)
   (SP_REGNUM 13)
   (LR_REGNUM 14)
   (SC_REGNUM 15)
   (SFP_REGNUM 16)]
)

;; (define_attr "type"
  ;; "alu,st,ld"   
  ;; (const_string "alu"))

;; -------------------------------------------------------------------------
;; Move instructions
;; -------------------------------------------------------------------------

(define_mode_iterator I [QI HI SI])
(define_mode_attr ldst [(QI "b") (HI "h") (SI "w")])

(define_expand "mov<I:mode>"
  [(set (match_operand:I 0 "nonimmediate_operand" "")
	(match_operand:I 1 "general_operand" ""))]
  ""
{
  flint_expand_move(<MODE>mode, operands[0], operands[1]);
  DONE;
})

;; For flint, a 32-bit move can have:
  ;; register -> register: an ALU operation <add rd, rs, r0>
  ;; immediate const -> register: an ALU operation <addi rd, r0, imm>
  ;; memory -> register: a load operation <lw rd, imm(rs1)>
  ;; register -> memory: a store operation <sw rs2, imm(rs1)>

;; 8-bit, 16-bit and 32-bit moves
(define_insn "*mov<I:mode>_internal"
  ;; =r, r: reg -> reg (add rd, rs1, r0)
  ;; =r, I: small const -> reg (addi rd, r0, rs1)
  ;; =r, m: mem -> reg ()
  ;; m, r: reg -> mem
  [(set (match_operand:I 0 "nonimmediate_operand" "=r,r,r,m")
	(match_operand:I 1 "input_operand"        " r,I,m,r"))]
  "register_operand(operands[0], <I:MODE>mode)
   || reg_or_0_operand(operands[1], <I:MODE>mode)"
   "@
    add  %0, %1, r0
    addi %0, r0, %1
    l<I:ldst> %0, %a1
    s<I:ldst> %1, %a0")
   ;; NOTE: no scheduler descritpion at all
   ;; [(set_attr "type" "alu,alu,alu,alu,st,ld")]
   

;; -------------------------------------------------------------------------
;; nop instruction
;; -------------------------------------------------------------------------

(define_insn "nop"
  [(const_int 0)]
  ""
  "l.nop")

;; -------------------------------------------------------------------------
;; return instruction
;; -------------------------------------------------------------------------

(define_expand "simple_return"
  [(parallel [(simple_return) (use (match_dup 0))])]
  ""
{
  operands[0] = gen_rtx_REG (Pmode, LR_REGNUM);
})

(define_insn "*simple_return"
  [(simple_return)
   (use (match_operand:SI 0 "register_operand" "r"))]
  "@
   jalr")