;; Constraint definitions for flint
;; Copyright (C) 2018-2026 Free Software Foundation, Inc.
;; Contributed by Nathan Blackburn
;; Modified from or1k (Stafford Horne)

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
;; Constraints
;; -------------------------------------------------------------------------

(define_constraint "I"
    "A signed 14-bit immediate within the range of -8192 to 8191."
    (and (match_code "const_int")
         (match_test "IN_RANGE(ival, -8192, 8191)")))

(define_constraint "0"
    "Constant 0."
    (and (match_code "const_int")
         (match_test "ival == 0")))