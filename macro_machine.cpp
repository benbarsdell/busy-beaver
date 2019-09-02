/*
 * Copyright (c) 2019, Ben Barsdell. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "macro_machine.hpp"

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

void MacroMachine::step(MacroMachineState* mstate, BigNum* num_micro_steps,
                        BigNum* num_macro_steps, SpanID* deleted_span_id,
                        Tape::iterator* shrunk_span,
                        // HACK TODO: Clean up this interface. Maybe just return
                        // deltas instead of updating absolutes?
                        BigNum* this_num_micro_steps_ptr,
                        bool* did_jump) const {
  MicroMachineState rule{mstate->state, mstate->cur_span->symbol,
                         mstate->moving_right};
  BigNum this_num_micro_steps = micro_machine_.step(&rule);
  if (this_num_micro_steps_ptr)
    *this_num_micro_steps_ptr = this_num_micro_steps;
  if (did_jump) *did_jump = false;
  // HACK TODO: Replace these with direct access (e.g., mstate->cur_span).
  auto& state = mstate->state;
  auto& tape = mstate->tape;
  auto& cur_span = mstate->cur_span;
  auto& moving_right = mstate->moving_right;
  auto& span_id_counter = mstate->span_id_counter;

  if (rule.state == STATE_NOHALT) {
    // TODO: Return this via a msg or similar instead of printing.
    cout << "INFINITE MICROLOOP" << endl;
    state = STATE_NOHALT;
    return;
  }
  BigNum this_num_macro_steps;
  if (rule.state == state && rule.move_right == moving_right) {
    // No state change, can jump.
    // Check for infinite walk at end of tape.
    if ((rule.move_right && cur_span == --tape.end()) ||
        (!rule.move_right && cur_span == tape.begin())) {
      cout << "INFINITE WALK" << endl;
      state = STATE_NOHALT;
      return;
    }
    BigNum jump = cur_span->size;
    if (did_jump) *did_jump = true;
    this_num_micro_steps *= jump;
    this_num_macro_steps = rule.move_right ? jump : -jump;
    if (rule.move_right && rule.symbol == std::prev(cur_span)->symbol) {
      // Keep the older span, erase the newer one (enables more proofs).
      if (std::prev(cur_span)->id < cur_span->id) {
        // Extend the prev span to encompass the current span.
        std::prev(cur_span)->size += cur_span->size;
        if (deleted_span_id) *deleted_span_id = cur_span->id;
        cur_span = tape.erase(cur_span);
      } else {
        // Extend the current span to encompass the previous one.
        cur_span->symbol = rule.symbol;
        cur_span->size += std::prev(cur_span)->size;
        if (deleted_span_id) *deleted_span_id = std::prev(cur_span)->id;
        tape.erase(std::prev(cur_span));
        cur_span = std::next(cur_span);
      }
    } else if (!rule.move_right && rule.symbol == std::next(cur_span)->symbol) {
      // Keep the older span, erase the newer one (enables more proofs).
      if (std::next(cur_span)->id < cur_span->id) {
        // Extend the next span to encompass the current span.
        std::next(cur_span)->size += cur_span->size;
        if (deleted_span_id) *deleted_span_id = cur_span->id;
        cur_span = --tape.erase(cur_span);
      } else {
        // Extend the current span to encompass the next one.
        cur_span->symbol = rule.symbol;
        cur_span->size += std::next(cur_span)->size;
        if (deleted_span_id) *deleted_span_id = std::next(cur_span)->id;
        tape.erase(std::next(cur_span));
        cur_span = std::prev(cur_span);
      }
    } else {
      // Change current span's symbol (it may also stay the same).
      cur_span->symbol = rule.symbol;
      if (rule.move_right) {
        cur_span = std::next(cur_span);
      } else {
        cur_span = std::prev(cur_span);
      }
    }
  } else {  // Can only take a single macro step.
    this_num_macro_steps = rule.move_right ? 1 : -1;
    if (rule.move_right &&
        (moving_right || (cur_span->size == 1 && cur_span != tape.begin())) &&
        rule.symbol == std::prev(cur_span)->symbol) {
      // Extend the prev span forward by 1.
      std::prev(cur_span)->size += 1;
      if (cur_span !=
          std::prev(tape.end())) {  // TODO: These guards are a bit hacky; not
                                    // sure how necessary they are.
        cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = cur_span;
        if (cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = cur_span->id;
          cur_span = tape.erase(cur_span);
        }
      }
    } else if (!rule.move_right &&
               (!moving_right ||
                (cur_span->size == 1 && cur_span != std::prev(tape.end()))) &&
               rule.symbol == std::next(cur_span)->symbol) {
      // Extend the next span backward by 1.
      std::next(cur_span)->size += 1;
      if (cur_span != tape.begin()) {
        cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = cur_span;
        if (cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = cur_span->id;
          cur_span = --tape.erase(cur_span);
        }
      }
    } else if (rule.move_right && moving_right) {
      // Insert new size-1 span before current span.
      tape.insert(cur_span, TapeSpan{rule.symbol, 1, span_id_counter++});
      if (cur_span != std::prev(tape.end())) {
        cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = cur_span;
        if (cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = cur_span->id;
          cur_span = tape.erase(cur_span);
        }
      }
    } else if (!rule.move_right && !moving_right) {
      // Insert new size-1 span after current span.
      tape.insert(std::next(cur_span),
                  TapeSpan{rule.symbol, 1, span_id_counter++});
      if (cur_span != tape.begin()) {
        cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = cur_span;
        if (cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = cur_span->id;
          cur_span = --tape.erase(cur_span);
        }
      }
    } else if (rule.move_right && !moving_right) {
      if (rule.symbol != cur_span->symbol) {
        auto old_span = cur_span;
        cur_span = tape.insert(std::next(cur_span),
                               TapeSpan{rule.symbol, 1, span_id_counter++});
        if (old_span != tape.begin()) {
          old_span->size -= 1;
          if (shrunk_span) *shrunk_span = old_span;
          if (old_span->size == 0) {
            if (deleted_span_id) *deleted_span_id = old_span->id;
            tape.erase(old_span);
          }
        }
      }
      cur_span = std::next(cur_span);
    } else {  // !rule.move_right && moving_right
      if (rule.symbol != cur_span->symbol) {
        auto old_span = cur_span;
        cur_span =
            tape.insert(cur_span, TapeSpan{rule.symbol, 1, span_id_counter++});
        if (old_span != std::prev(tape.end())) {
          old_span->size -= 1;
          if (shrunk_span) *shrunk_span = old_span;
          if (old_span->size == 0) {
            if (deleted_span_id) *deleted_span_id = old_span->id;
            tape.erase(old_span);
          }
        }
      }
      cur_span = std::prev(cur_span);
    }
    // Update state.
    state = rule.state;
    moving_right = rule.move_right;
  }
  *num_micro_steps += this_num_micro_steps;
  *num_macro_steps += this_num_macro_steps;
}
