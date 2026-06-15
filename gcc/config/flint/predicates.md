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