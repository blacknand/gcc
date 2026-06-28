/* Target Code for flint
   Copyright (C) 2018-2026 Free Software Foundation, Inc.
   Contributed by Nathan Blackburn based on or1k and GCC-VAM patch.
   Check flint.h comment.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#define IN_TARGET_CODE 1

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "rtl.h"
#include "tree.h"
#include "stringpool.h"
#include "attribs.h"
#include "df.h"
#include "regs.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "diagnostic-core.h"
#include "output.h"
#include "stor-layout.h"
#include "varasm.h"
#include "calls.h"
#include "expr.h"
#include "builtins.h"
#include "optabs.h"
#include "explow.h"
#include "cfgrtl.h"
#include "alias.h"
#include "targhooks.h"
#include "case-cfn-macros.h"

/* These 4 are needed to allow using satisfies_constraint_J.  */
#include "insn-config.h"
#include "recog.h"
#include "tm_p.h"
#include "tm-constrs.h"

/* This file should be included last.  */
#include "target-def.h"

/*
  cfun is a GCC global pointer to the current function being compiled which is a
  struct function. It contains everything GCC knows about the function: its RTL,
  its CFG, its stack frame information.
  
  cfun->machine is a target specific extension to that struct, it is a pointer to a
  struct machine_function defined below. This is where flint stores its own per-function
  state that GCC's generic machinery doesn't know about.

  This is therefore a per-function data structure where the backend stores information
  it needs to track about each function being compiled. It gets allocated when GCC starts
  processing a function and lives until that function is done.
*/
struct GTY(()) machine_function
{
  /* Number of bytes saved on the stack for callee saved registers.  */
  HOST_WIDE_INT callee_saved_reg_size;

  /* Number of bytes saved on the stack for local variables.  */
  HOST_WIDE_INT local_vars_size;

  /* Number of bytes saved on the stack for outgoing/sub-function args.  */
  HOST_WIDE_INT args_size;

  /* The sum of sizes: locals vars, called saved regs, stack pointer
     and an optional frame pointer.
     Used in expand_prologue () and expand_epilogue ().  */
  HOST_WIDE_INT total_size;

  /* Remember where the set_got_placeholder is located.  */
  rtx_insn *set_got_insn;

  /* Remember where mcount args are stored so we can insert set_got_insn
     after.  */
  rtx_insn *set_mcount_arg_insn;
};

static struct machine_function *
flint_init_machine_status (void)
{
  return ggc_cleared_alloc<machine_function> ();
};

/* Helper for defining INITIAL_ELIMINATION_OFFSET.
   We allow the following eliminiations:
     FP -> HARD_FP or SP
     AP -> HARD_FP or SP

   HARD_FP and AP are the same which is handled below.

   The offset is the difference from the real register to the virtual register.
   
   high addresses
   ┌─────────────────┐  ← AP (incoming args live here, above this line)
   │  saved registers│
   │  local variables│
   │  outgoing args  │
   └─────────────────┘  ← SP (after prologue)   

   If I have VIRTUAL_REG + N, what constant do I add to the real register
   to get the same address? GCC uses the following formula after elimination:
   real_register + (elimination_offset + N)
   so the offset is the distance from the real register to the virtual register.
*/
HOST_WIDE_INT
flint_initial_elimination_offset(int from, int to) 
{
   HOST_WIDE_INT offset;
   if (from == ARG_POINTER_REGNUM)
      offset = cfun->machine->total_size;
   else if (from == FRAME_POINTER_REGNUM)
      offset = cfun->machine->args_size + cfun->machine->local_vars_size;
   else
      gcc_unreachable();

   /* Compute the offset assuming TO = SP, if TO is actually HFP then correct it */
   if (to == HARD_FRAME_POINTER_REGNUM) offset -= cfun->machine->total_size;

   return offset;
}

static bool
callee_saved_regno_p (int regno)
{  
   /* Check call-saved registers.  
      From the callee's perspective, if it is not a call-used register meaning it is
      a call-saved (callee saved) register AND the register is actually used then it must
      be saved and restored by the callee 
   */
   if (!call_used_or_fixed_reg_p (regno) && df_regs_ever_live_p (regno))
      return true;

   switch (regno)
      {
         /*
            GCC normally tries to eliminate SFP into SP, but falls back to
            HFP when SP isn't stable (specifically when a function uses alloca)
            or a variable length array which move SP at runtime by an unkown amount.

            when frame_pointer_needed is true,r5 gets used as a HFP and therefore
            must be saved and restored like any other callee-saved register. this must
            be done here because this happens outside of normal register allocation
            and so GCC may not see the register as live.
         */
      case HARD_FRAME_POINTER_REGNUM:
         return frame_pointer_needed;
      case LR_REGNUM:
         /* Always save LR if we are saving HFP, producing a walkable
      stack chain with -fno-omit-frame-pointer.  */
         return (frame_pointer_needed
            /* if the function is not a leaf, then LR must be saved */
            || !crtl->is_leaf
            || df_regs_ever_live_p (regno));
      default:
         return false;
      }
}

static bool
flint_legitimate_address_p(machine_mode, rtx x, bool strict_p, code_helper = ERROR_MARK)
{
   rtx base, imm;
   switch (GET_CODE(x))
   {
      case REG:
         base = x;
         break;
      case PLUS:
         base = XEXP(x, 0);
         imm = XEXP(x, 1);
         // If the base is not a bare register
         if (!REG_P(base))
            return false;
         /*
            Before register allocation (!strict_p), if the base is one of the virtual
            registers, accept the address as long as the immedaite is a constant integer 
         */
         if (!strict_p && virtual_frame_reg_operand (base, VOIDmode))
            return CONST_INT_P(imm);
         // For real register, the immediate must fit into flint's signed 14 bit range
         if (!satisfies_constraint_I(imm))
            return false;
         break;
      // Any other RTX from directly as an address is invalid
      default:
         return false;
   }

   // Register validity check
   unsigned regno = REGNO(base);
   if (regno >= FIRST_PSEUDO_REGISTER) {
      if (strict_p)
         regno = reg_renumber[regno];
      else
         return true;
   }

   if (strict_p)
      return regno <= 15;
   else
      return REGNO_OK_FOR_BASE_P(regno);
}

/* Tells GCC what RTX represents the return value of a function.
   In other words, return an RTX representing the place where a function returns a value. */
static rtx
flint_function_value (const_tree valtype,
		     const_tree /* fn_decl_or_type */,
		     bool /* outgoing */)
{
  return gen_rtx_REG(TYPE_MODE(valtype), RET_VAL_REG);
}

#if 0
static void
or1k_save_reg (int regno, HOST_WIDE_INT offset)
{

}

static rtx
or1k_restore_reg (int regno, HOST_WIDE_INT offset, rtx cfa_restores)
{

}

void
or1k_expand_prologue (void)
{

}

void
or1k_expand_epilogue (void)
{

}

rtx
or1k_initial_frame_addr ()
{

}

rtx
or1k_dynamic_chain_addr (rtx frame)
{

}

rtx
or1k_return_addr (int, rtx frame)
{

}

static bool
or1k_legitimate_address_p (machine_mode, rtx x, bool strict_p,
			   code_helper = ERROR_MARK)
{

}

static bool
or1k_pass_by_reference (cumulative_args_t, const function_arg_info &arg)
{

}

static rtx
or1k_function_value (const_tree valtype,
		     const_tree /* fn_decl_or_type */,
		     bool /* outgoing */)
{

}

static bool
or1k_function_value_regno_p (const unsigned int regno)
{

}

static bool
or1k_return_in_memory (const_tree type, const_tree /* fntype */)
{

}
#endif

/* Updates the summariser variable pointer to by cum_v to advance past argument
   arg in the list. Once this is done,t he variable cum is suitable for analysing 
   the following argument with TARGET_FUNCTION_ARG, etc. */
static void
flint_function_arg_advance (cumulative_args_t cum_v,
			   const function_arg_info &arg)
{
   CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
   int nreg = CEIL (GET_MODE_SIZE (arg.mode), UNITS_PER_WORD);

  /* Note that all large arguments are passed by reference.
      Check flint_function_arg. */
  gcc_assert (nreg <= 2);
  if (arg.named)
    *cum += nreg;
}

/* Return an RTX indicating whether function arg is passed in a register and  if so,
   which register. Argument cum_v (ca) summarises all the pervious arguments. */
static rtx
flint_function_arg (cumulative_args_t cum_v, const function_arg_info &arg)
{
   /* Handle the special marker for the end of the arguments.  */
   if (arg.end_marker_p ())
      return NULL_RTX;     

   /* How many registers this argument will consume. On flint, UNITS_PER_WORD
      is 4 (32-bit) meaning a 4-byte int is nreg=1 and an 8-byte long long is nreg=2. */
   CUMULATIVE_ARGS *cum = get_cumulative_args (cum_v);
   int nreg = CEIL (GET_MODE_SIZE (arg.mode), UNITS_PER_WORD);

   /* No argument takes more than 2 registers */
   gcc_assert (nreg <= 2);
   /* Only named arguments go in registers. There is no varargs support in flint. */
   if (arg.named && *cum + nreg <= 4)
      /* The offset of 1 maps count to register number. */
      return gen_rtx_REG (arg.mode, *cum + 1);     // Register number is *cum + 1
   else
      return NULL_RTX;
}

static void
flint_compute_frame_layout(void)
{
  HOST_WIDE_INT local_vars_size, args_size, save_reg_size;

  local_vars_size = get_frame_size ();
  local_vars_size = ROUND_UP (local_vars_size, UNITS_PER_WORD);

  args_size = crtl->outgoing_args_size;
  args_size = ROUND_UP (args_size, UNITS_PER_WORD);

  save_reg_size = 0;
  for (int regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
    if (callee_saved_regno_p (regno))
      save_reg_size += UNITS_PER_WORD;

  cfun->machine->local_vars_size = local_vars_size;
  cfun->machine->args_size = args_size;
  cfun->machine->callee_saved_reg_size = save_reg_size;
  cfun->machine->total_size = save_reg_size + local_vars_size + args_size;
}

/*
   In flint, S-type instructions cannot store constants into addresses in a single instruction
   because there is no immediate field. The imm field is only for offsets. I-type instructions
   use an immediate field of 14 bits for a range of -8192 to +8191 so any constant outside that
   range needs two instructions: lui followed by addi. flint also only has a single addressing 
   mode: EA = rs1 + sext(imm); every load and store uses this form. 

   Expand the patterns movqi, movhi and movsi.
   - op0 = destination
   - op1 = source
*/
void
flint_expand_move(machine_mode mode, rtx op0, rtx op1)
{
   /* 
      reg -> reg: ADD rd, rs, r0
      mem -> reg: LW rd, imm(rs1)
      reg -> mem: SW rs2, imm(rs1)
      small const -> reg: ADDI rd, r0, imm
      large const -> reg: [LUI rd, upper] then [ADDI rd, rd, lower]
      const -> mem: [LW rd, imm(rs1)] then [ADD rd, rs, r0]
   */

   if (MEM_P(op0)) {
      /* If destination is memory, check if source if a constant 0.
         If not 0, then force it into a register. This is because you cannot 
         store a constant integer directly into memory in a single instruction. */
      if (!const0_operand(op1, mode))
         op1 = force_reg(mode, op1);
   } else if (mode == QImode || mode == HImode) {
      /* Have not implemented byte/half-word byte moves here, all just fall
         through to emit_insn at the bottom. */
   } else {
      switch (GET_CODE(op1)) {
         case CONST_INT:
            /* Check if const fits in the immediate field. If too large, special handling: */
            if (!input_operand(op1, mode)) {
	            HOST_WIDE_INT i = INTVAL(op1);
               /* Extract low 14 bits and high 22 bits from source reg op1. */
	            HOST_WIDE_INT lo = (i >> 14) & 0x3fff;
	            HOST_WIDE_INT hi = i ^ lo;
               /* If bit 13 of lo is 1, ADDI will sign extend as negative, 
                  so add 1 to hi to compensate */
               if (lo & 0x2000) hi += 1;

               /* Check if it is safe to use new pseudos to 
                  give the optimiser more freedom? */
	            rtx subtarget = op0;
	            if (!cse_not_expected && can_create_pseudo_p())
                  subtarget = gen_reg_rtx(SImode);
               
               /* Emit RTL insn subtarget = hi bits */
	            emit_insn(gen_rtx_SET(subtarget, GEN_INT(hi)));
               /* Emit RTL insn subtarget += lo bits */
	            emit_insn(
                  gen_rtx_SET(
                     op0, 
                     (gen_rtx_PLUS(SImode, subtarget, GEN_INT(lo)))
                  )
               );
	            return;
            }
            break;
         default:
            break;
      }
   }
   emit_insn(gen_rtx_SET (op0, op1));
}

/* Worker for TARGET_PRINT_OPERAND_ADDRESS.
   Prints the argument ADDR, an address RTX, to the file FILE.  The output is
   formed as expected by the (non-existent) flint assembler.  Examples:

     RTX							      OUTPUT
     (reg:SI 3)							       0(r3)
     (plus:SI (reg:SI 3) (const_int 4))				     0x4(r3)
     (lo_sum:SI (reg:SI 3) (symbol_ref:SI ("x"))))		   lo(x)(r3)  */
static void
flint_print_operand_address (FILE *file, machine_mode, rtx addr)
{
  rtx offset;

  switch (GET_CODE (addr)) {
      case REG:
         /* Output 0 for the offset (because there is no offset). */
         fputc('0', file);
         break;
      case PLUS:
         offset = XEXP(addr, 1);
         addr = XEXP(addr, 0);
         gcc_assert(CONST_INT_P (offset));
         output_addr_const(file, offset);
         break;
    default:
      output_addr_const (file, addr);
      return;
   }

  fprintf (file, "(%s)", reg_names[REGNO (addr)]);
}

static rtx
flint_libcall_value (machine_mode mode,
		    const_rtx /* fun */)
{
  return gen_rtx_REG(mode, RET_VAL_REGNUM);
}

#define TARGET_HAVE_TLS false

#undef TARGET_FUNCTION_ARG
#define TARGET_FUNCTION_ARG flint_function_arg

#undef TARGET_FUNCTION_ARG_ADVANCE
#define TARGET_FUNCTION_ARG_ADVANCE flint_function_arg_advance

#undef TARGET_FUNCTION_VALUE
#define TARGET_FUNCTION_VALUE flint_function_value

#undef  TARGET_COMPUTE_FRAME_LAYOUT
#define TARGET_COMPUTE_FRAME_LAYOUT flint_compute_frame_layout

#undef  TARGET_LEGITIMATE_ADDRESS_P
#define TARGET_LEGITIMATE_ADDRESS_P flint_legitimate_address_p

#undef TARGET_PRINT_OPERAND_ADDRESS 
#define TARGET_PRINT_OPERAND_ADDRESS flint_print_operand_address

#undef TARGET_LIBCALL_VALUE
#define TARGET_LIBCALL_VALUE flint_libcall_value

struct gcc_target targetm = TARGET_INITIALIZER;

#include "gt-flint.h"