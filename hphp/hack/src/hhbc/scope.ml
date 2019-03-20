(**
 * Copyright (c) 2019, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
*)

open Core_kernel
open Instruction_sequence
open Local

(* Run emit () in a new unnamed local scope, which produces three instruction
 * blocks -- before, inner, after. If emit () registered any unnamed locals, the
 * inner block will be wrapped in a try/fault that will unset these unnamed
 * locals upon exception. *)
let with_unnamed_locals emit =
  let current_next_local = !next_local in
  let current_temp_local_map = !temp_local_map in
  let before, inner, after = emit () in
  if current_next_local = !next_local then gather [ before; inner; after ] else
  let local_unsets = gather @@
    List.init (!next_local - current_next_local)
      ~f:(fun idx -> instr_unsetl (Unnamed (idx + current_next_local))) in
  next_local := current_next_local;
  temp_local_map := current_temp_local_map;
  let fault_block = gather [ local_unsets; instr_unwind ] in
  let fault_label = Label.next_fault () in
  gather [
    before;
    instr_try_fault fault_label inner fault_block;
    after
  ]

(* Pop the top of the stack into an unnamed local, run emit (), then push the
 * stashed value to the top of the stack. *)
let stash_top_in_unnamed_local emit = with_unnamed_locals @@ fun () ->
  let tmp = Local.get_unnamed_local () in
  instr_popl tmp, emit (), instr_pushl tmp