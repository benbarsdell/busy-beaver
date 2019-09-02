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

  if (rule.state == STATE_NOHALT) {
    // TODO: Return this via a msg or similar instead of printing.
    cout << "INFINITE MICROLOOP" << endl;
    mstate->state = STATE_NOHALT;
    return;
  }
  BigNum this_num_macro_steps;
  if (rule.state == mstate->state && rule.move_right == mstate->moving_right) {
    // No state change, can jump.
    // Check for infinite walk at end of tape.
    if ((rule.move_right && mstate->cur_span == --mstate->tape.end()) ||
        (!rule.move_right && mstate->cur_span == mstate->tape.begin())) {
      cout << "INFINITE WALK" << endl;
      mstate->state = STATE_NOHALT;
      return;
    }
    BigNum jump = mstate->cur_span->size;
    if (did_jump) *did_jump = true;
    this_num_micro_steps *= jump;
    this_num_macro_steps = rule.move_right ? jump : -jump;
    if (rule.move_right && rule.symbol == std::prev(mstate->cur_span)->symbol) {
      // Keep the older span, erase the newer one (enables more proofs).
      if (std::prev(mstate->cur_span)->id < mstate->cur_span->id) {
        // Extend the prev span to encompass the current span.
        std::prev(mstate->cur_span)->size += mstate->cur_span->size;
        if (deleted_span_id) *deleted_span_id = mstate->cur_span->id;
        mstate->cur_span = mstate->tape.erase(mstate->cur_span);
      } else {
        // Extend the current span to encompass the previous one.
        mstate->cur_span->symbol = rule.symbol;
        mstate->cur_span->size += std::prev(mstate->cur_span)->size;
        if (deleted_span_id) *deleted_span_id = std::prev(mstate->cur_span)->id;
        mstate->tape.erase(std::prev(mstate->cur_span));
        mstate->cur_span = std::next(mstate->cur_span);
      }
    } else if (!rule.move_right &&
               rule.symbol == std::next(mstate->cur_span)->symbol) {
      // Keep the older span, erase the newer one (enables more proofs).
      if (std::next(mstate->cur_span)->id < mstate->cur_span->id) {
        // Extend the next span to encompass the current span.
        std::next(mstate->cur_span)->size += mstate->cur_span->size;
        if (deleted_span_id) *deleted_span_id = mstate->cur_span->id;
        mstate->cur_span = --mstate->tape.erase(mstate->cur_span);
      } else {
        // Extend the current span to encompass the next one.
        mstate->cur_span->symbol = rule.symbol;
        mstate->cur_span->size += std::next(mstate->cur_span)->size;
        if (deleted_span_id) *deleted_span_id = std::next(mstate->cur_span)->id;
        mstate->tape.erase(std::next(mstate->cur_span));
        mstate->cur_span = std::prev(mstate->cur_span);
      }
    } else {
      // Change current span's symbol (it may also stay the same).
      mstate->cur_span->symbol = rule.symbol;
      if (rule.move_right) {
        mstate->cur_span = std::next(mstate->cur_span);
      } else {
        mstate->cur_span = std::prev(mstate->cur_span);
      }
    }
  } else {  // Can only take a single macro step.
    this_num_macro_steps = rule.move_right ? 1 : -1;
    if (rule.move_right &&
        (mstate->moving_right || (mstate->cur_span->size == 1 &&
                                  mstate->cur_span != mstate->tape.begin())) &&
        rule.symbol == std::prev(mstate->cur_span)->symbol) {
      // Extend the prev span forward by 1.
      std::prev(mstate->cur_span)->size += 1;
      if (mstate->cur_span !=
          std::prev(
              mstate->tape.end())) {  // TODO: These guards are a bit hacky; not
                                      // sure how necessary they are.
        mstate->cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = mstate->cur_span;
        if (mstate->cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = mstate->cur_span->id;
          mstate->cur_span = mstate->tape.erase(mstate->cur_span);
        }
      }
    } else if (!rule.move_right &&
               (!mstate->moving_right ||
                (mstate->cur_span->size == 1 &&
                 mstate->cur_span != std::prev(mstate->tape.end()))) &&
               rule.symbol == std::next(mstate->cur_span)->symbol) {
      // Extend the next span backward by 1.
      std::next(mstate->cur_span)->size += 1;
      if (mstate->cur_span != mstate->tape.begin()) {
        mstate->cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = mstate->cur_span;
        if (mstate->cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = mstate->cur_span->id;
          mstate->cur_span = --mstate->tape.erase(mstate->cur_span);
        }
      }
    } else if (rule.move_right && mstate->moving_right) {
      // Insert new size-1 span before current span.
      mstate->tape.insert(mstate->cur_span,
                          TapeSpan{rule.symbol, 1, mstate->span_id_counter++});
      if (mstate->cur_span != std::prev(mstate->tape.end())) {
        mstate->cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = mstate->cur_span;
        if (mstate->cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = mstate->cur_span->id;
          mstate->cur_span = mstate->tape.erase(mstate->cur_span);
        }
      }
    } else if (!rule.move_right && !mstate->moving_right) {
      // Insert new size-1 span after current span.
      mstate->tape.insert(std::next(mstate->cur_span),
                          TapeSpan{rule.symbol, 1, mstate->span_id_counter++});
      if (mstate->cur_span != mstate->tape.begin()) {
        mstate->cur_span->size -= 1;
        if (shrunk_span) *shrunk_span = mstate->cur_span;
        if (mstate->cur_span->size == 0) {
          if (deleted_span_id) *deleted_span_id = mstate->cur_span->id;
          mstate->cur_span = --mstate->tape.erase(mstate->cur_span);
        }
      }
    } else if (rule.move_right && !mstate->moving_right) {
      if (rule.symbol != mstate->cur_span->symbol) {
        auto old_span = mstate->cur_span;
        mstate->cur_span = mstate->tape.insert(
            std::next(mstate->cur_span),
            TapeSpan{rule.symbol, 1, mstate->span_id_counter++});
        if (old_span != mstate->tape.begin()) {
          old_span->size -= 1;
          if (shrunk_span) *shrunk_span = old_span;
          if (old_span->size == 0) {
            if (deleted_span_id) *deleted_span_id = old_span->id;
            mstate->tape.erase(old_span);
          }
        }
      }
      mstate->cur_span = std::next(mstate->cur_span);
    } else {  // !rule.move_right && mstate->moving_right
      if (rule.symbol != mstate->cur_span->symbol) {
        auto old_span = mstate->cur_span;
        mstate->cur_span = mstate->tape.insert(
            mstate->cur_span,
            TapeSpan{rule.symbol, 1, mstate->span_id_counter++});
        if (old_span != std::prev(mstate->tape.end())) {
          old_span->size -= 1;
          if (shrunk_span) *shrunk_span = old_span;
          if (old_span->size == 0) {
            if (deleted_span_id) *deleted_span_id = old_span->id;
            mstate->tape.erase(old_span);
          }
        }
      }
      mstate->cur_span = std::prev(mstate->cur_span);
    }
    // Update state.
    mstate->state = rule.state;
    mstate->moving_right = rule.move_right;
  }
  *num_micro_steps += this_num_micro_steps;
  *num_macro_steps += this_num_macro_steps;
}
