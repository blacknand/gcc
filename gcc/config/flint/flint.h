/* Target Definitions for flint.
   Copyright (C) 2018-2026 Free Software Foundation, Inc.
   Contributed by Nathan Blackburn.
   Contributed by Stafford Horne (or1k).
   GCC-VAM was used as a reference: https://github.com/embecosm/gcc-vam/commit/97863f37b4a844e596214f12d13f9b7e0b979b16

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

/*
  Look at /docs/GCC backend.md for a full explanation of a GCC backend
  using examples from this backend for flint.
*/

#ifndef GCC_FLINT_H
#define GCC_FLINT_H


/* Names to predefine in the preprocessor for this target machine.  */
#define TARGET_CPU_CPP_BUILTINS()		\
  do						\
    {						\
      builtin_define ("__flint__");		\
      builtin_define ("__FLINT__");	\
      builtin_assert ("cpu=flint");		\
      builtin_assert ("machine=flint");		\
    }						\
  while (0)


/* Storage layout.  */
#define DEFAULT_SIGNED_CHAR 1
#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0
#define BITS_PER_WORD 32
#define UNITS_PER_WORD 4
#define POINTER_SIZE 32
#define BIGGEST_ALIGNMENT 32
#define STRICT_ALIGNMENT 1
#define FUNCTION_BOUNDARY 32
#define PARM_BOUNDARY 32
#define STACK_BOUNDARY 32
#define PREFERRED_STACK_BOUNDARY 32
#define MAX_FIXED_MODE_SIZE 64


/* Layout of source language data types.  */
#define INT_TYPE_SIZE 32
#define SHORT_TYPE_SIZE 16
#define LONG_TYPE_SIZE 32
#define LONG_LONG_TYPE_SIZE 64
#define WCHAR_TYPE_SIZE 32

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "unsigned int"


/* Describing Relative Costs of Operations.  */
#define MOVE_MAX 4
#define SLOW_BYTE_ACCESS 1


/* Register usage, class and contents.  */

/* In flint there are 16 general purpose registers with the following
   designations:

   r0    0 (hardwired)
   r1    argument/return values (caller-saved)
   r2    argument/return values (caller-saved)
   r3    argument/return values (caller-saved)
   r4    argument/return values (caller-saved)
   r5    general purpose variables (callee-saved)
   r6    general purpose variables (callee-saved)
   r7    general purpose variables (callee-saved)
   r8    general purpose variables (callee-saved)
   r9    general purpose variables (callee-saved)
   r10   general purpose variables (callee-saved)
   r11   general purpose variables (callee-saved)
   r12   scratch (caller-saved)
   r13   stack pointer (fixed)
   r14   link register (special, also caller-saved)
   r15   scratch (caller-saved)
   r16   SFP (virtual)
*/

/* 
  Tell GCC where the hardware register numbering ends.
  flint has 16 hardware registers + one virtual register,
  so registers 17 and above are pseudo/virtual registers
  created during compilation and are not directly
  mappable to hardware 
*/
#define FIRST_PSEUDO_REGISTER 17

#define REGISTER_NAMES { \
  "r0",   "r1",   "r2",   "r3",   "r4",   "r5",   "r6",   "r7",   \
  "r8",   "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",  \
  "?sfp" }

/*
  Registers GCC is never allowed to use for general allocation.
  Index 0 is r0, index 13 is sp (must never be overwritten),
  index 16 is SFP (virtual, handled specially). 
*/
#define FIXED_REGISTERS		\
{ 1, 0, 0, 0, 0, 0, 0, 0,	\
  0, 0, 0, 0, 0, 1, 0, 0, \
  1 }

/* 
  Caller saved/temporary registers + args + fixed.
  These are registers a function is allowed to destory.
*/
#define CALL_USED_REGISTERS	\
{ 1, 1, 1, 1, 1, 0, 0, 0,	\
  0, 0, 0, 0, 1, 1, 1, 1, \
  1 }

/* 
  List the order in which to allocate registers. Each register must
  be listed once, even those in FIXED_REGISTERS.  

  Caller-saved registers are allocated before callee-saved so GCC can
  minimise the number of save/restore pairs it has to emit in prologues
  and epilogues. 

  r15 and r12 are listed before r1-r4 so that the argument regiters
  are available for their intended purposes when setting up calls. The
  pure scratch registers (r12, r15) are used up first before touching
  the argument registers.
*/
#define REG_ALLOC_ORDER { \
    15, 12,                         /* caller-saved temp */ \
    1, 2, 3, 4,                     /* caller-saved argument */ \
    5, 6, 7, 8, 9, 10, 11,          /* callee-saved */ \
    0,                              /* 0 reg (hardwired)*/ \
    13,                             /* stack pointer (fixed) */ \
    14,                             /* link register (fixed)*/ \
    16,                             /* virtual SFP (fixed) */ \
}

/*
  The absoulete minimum classes GCC will allow. flint *currently* has no
  architectural register specialisation because every GPR can do everything.
*/
enum reg_class
{
  NO_REGS,
  ALL_REGS,
  LIM_REG_CLASSES   // Sentinel value to mark num of reg classes
};

#define N_REG_CLASSES (int) LIM_REG_CLASSES

#define REG_CLASS_NAMES {	\
  "NO_REGS", 			\
  "ALL_REGS" }

/*
  Each entry is a bitmask over the register file, 
  Bit N is set if N is in that class.

  0x0001FFFF is 17 ones for registers 0 through 16,
  which is r0-15 + SFP.
*/
#define REG_CLASS_CONTENTS      \
{                                 
    { 0x00000000 },	  \   // 0 -
    { 0x0001FFFF },   \   // 17
}

#define REGNO_REG_CLASS(REGNO) ALL_REGS

#define PROMOTE_MODE(MODE,UNSIGNEDP,TYPE)               \
do {                                                    \
  if (GET_MODE_CLASS (MODE) == MODE_INT                 \
      && GET_MODE_SIZE (MODE) < UNITS_PER_WORD)         \
    (MODE) = word_mode;                                 \
} while (0)

/* A macro whose definition is the name of the class to which a valid
   base register must belong. A base register is one used in an
   address which is the register value plus a displacement. INDEX_REG_CLASS
   is the macro telling GCC that there is no addressing mode using two registers.
    
   Because flint has only two register classes: NO_REGS and ALL_REGS,
   BASE_REG_CLASS is simply ALL_REGS and INDEX_REG_CLASS is NO_REGS as
   flint only has a single addressing mode: reg + immediate.
   */
#define BASE_REG_CLASS ALL_REGS
#define INDEX_REG_CLASS NO_REGS


/* Assembly definitions.  */
#define ASM_APP_ON ""
#define ASM_APP_OFF ""

#define ASM_COMMENT_START ";; "

#define GLOBAL_ASM_OP "\t.global\t"
#define TEXT_SECTION_ASM_OP "\t.section\t.text"
#define DATA_SECTION_ASM_OP "\t.section\t.data"
#define BSS_SECTION_ASM_OP "\t.section\t.bss"
// NOTE: may add in future
// #define READONLY_DATA_SECTION_ASM_OP	"\t.section\t.rodata"
// NOTE: flint does not have a GP reg; kept for reference
// #define SBSS_SECTION_ASM_OP "\t.section\t.sbss"

/* This is how to output an assembler line
   that says to advance the location counter
   to a multiple of 2**LOG bytes.  */
#define ASM_OUTPUT_ALIGN(FILE,LOG)			\
  do							\
    {							\
      if ((LOG) != 0)					\
	fprintf (FILE, "\t.balign %d\n", 1 << (LOG));	\
    }							\
  while (0)

/* Calling convention definitions.  */
#define CUMULATIVE_ARGS int   // The type of the counter

/* 
  INIT_CUMULATIVE_ARGS initialises the CUMULATIVE_ARGS counter
  to zero at the start of processing each function's argument list.
  GCC then calls target hooks to process each argument in turn,
  incrementing the counter each time.
*/
#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS) \
  do { (CUM) = 0; } while (0)

/* Trampolines, for nested functions.
   2 instructions to load the static chain pointer into r15,
   3 instructions to construct the target address and jump (JALR)
   5 instructions * 4 bytes = 20 bytes. */
#define TRAMPOLINE_SIZE      20
#define TRAMPOLINE_ALIGNMENT 32     // 4-byte aligned

/* Pointer mode */
#define Pmode SImode
#define FUNCTION_MODE SImode
#define STACK_POINTER_REGNUM SP_REGNUM
/* GCC uses r16 as the reference point for both local
   and incoming arguments during compilation. The elimination
   pass then sorts out the correct offsets for each case differently. */
#define FRAME_POINTER_REGNUM SFP_REGNUM
#define HARD_FRAME_POINTER_REGNUM HFP_REGNUM
/* r15, the register used to pass the static chain pointer for nested
   function calls. */
#define STATIC_CHAIN_REGNUM SC_REGNUM

/* The register number of the arg pointer register, which is used to
   access the function's argument list.  */
#define ARG_POINTER_REGNUM SFP_REGNUM
#define FUNCTION_ARG_REGNO_P(r) (r >= 1 && r <= 4)
#define MAX_REGS_PER_ADDRESS 1

/* The ELIMINABLE_REGS macro specifies a table of register pairs used to
   eliminate unneeded registers that point into the stack frame. Note,
   the only elimination attempted by the compiler is to replace references
   to the frame pointer with references to the stack pointer.  */
#define ELIMINABLE_REGS					\
{{ FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM },	\
 { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM },	\
 { ARG_POINTER_REGNUM,   STACK_POINTER_REGNUM },	\
 { ARG_POINTER_REGNUM,   HARD_FRAME_POINTER_REGNUM }}

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  do {							\
    (OFFSET) = flint_initial_elimination_offset ((FROM), (TO)); \
  } while (0)

#define REGNO_OK_FOR_INDEX_P(REGNO) 0
// Up to and including SFP_REGNUM which is 16
#define REGNO_OK_FOR_BASE_P(REGNO)  ((REGNO) <= SFP_REGNUM)     // SFP_REGNUM = 16

/* If defined, the maximum amount of space required for outgoing
   arguments will be computed and placed into the variable
   'crtl->outgoing_args_size'.  No space will be pushed
   onto the stack for each call; instead, the function prologue
   should increase the stack frame size by this amount.  
   
   In other words, GCC will pre-allocate space in the frame for all
   outgoing arguments rather than pushing arguments onto the stack
   one by one. */
#define ACCUMULATE_OUTGOING_ARGS 1


/* Stack layout and stack pointer usage.  */

/* This plus ARG_POINTER_REGNUM points to the first word of incoming args.  

   This is the offset from the ARG pointer to the first incoming argument. 
   Zero means the first argument lives exactly at the address AP points to, with no gap. 
   In flint, incoming arguments sit directly above the frame boundary.
*/
#define FIRST_PARM_OFFSET(FNDECL) (0)

/* This plus STACK_POINTER_REGNUM points to the first work of outgoing args.  

   On some architectures sp doesn't point to the last used word but to the next free word, 
   or there's a reserved slot at the top of the frame. This offset accounts for that. 

   Zero means sp points exactly to the last allocated word with no reserved gap. 
   flint has no such convention so this is zero.
*/
#define STACK_POINTER_OFFSET (0)

/* Define this macro if pushing a word onto the stack moves the stack
   pointer to a smaller address.  */
#define STACK_GROWS_DOWNWARD 1

#define FRAME_GROWS_DOWNWARD 1

/* An alias for a machine mode name.  This is the machine mode that
   elements of a jump-table should have.  */
#define CASE_VECTOR_MODE SImode

#define STORE_FLAG_VALUE 1

/* Indicates how loads of narrow mode values are loaded into words.  */
#define LOAD_EXTEND_OP(MODE) (ZERO_EXTEND)

/* TODO: include when exception handling added  */
// #define INITIAL_FRAME_ADDRESS_RTX  or1k_initial_frame_addr ()
// #define DYNAMIC_CHAIN_ADDRESS      or1k_dynamic_chain_addr
// #define RETURN_ADDR_RTX            or1k_return_addr

/* EXIT_IGNORE_STACK should be nonzero if, when returning from a function,
   the stack pointer does not matter.  */
#define EXIT_IGNORE_STACK 1

/* Always pass the SYMBOL_REF for direct calls to the expanders.

   Prevents GCC from caching function addresses in registers before calls. */
#define NO_FUNCTION_CSE 1

/* Emit rtl for profiling.  We don't support this, so can be empty  */
#define NO_PROFILE_COUNTERS 1

/* Emit rtl for profiling.  We don't support this, so can be empty  */
#define PROFILE_HOOK(LABEL)

/* All the work is done in PROFILE_HOOK, but this is still required.  */
#define FUNCTION_PROFILER(STREAM, LABELNO) do { } while (0)

/* Dwarf 2 Support */
// NOTE: should work with GDB but not really sure
#define DWARF2_DEBUGGING_INFO 1
#define INCOMING_RETURN_ADDR_RTX gen_rtx_REG (Pmode, LR_REGNUM)
#define DWARF_FRAME_RETURN_COLUMN LR_REGNUM

// NOTE: no exception handling support. C++ not supported. Only basic C programs.

#endif // GCC_FLINT_H