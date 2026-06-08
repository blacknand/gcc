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
  [(HFP_REGNUM 5)
   (SP_REGNUM 13)
   (LR_REGNUM 14)
   (SC_REGNUM 15)
   (SFP_REGNUM 16)]
)

