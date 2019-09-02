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

#pragma once

#include "fast_list.hpp"
#include "micro_machine.hpp"
#include "bignum.hpp"

#include <vector>

typedef uint64_t SpanID;

struct TapeSpan {
  MacroSym symbol;  // The macro symbol.
  BigNum size;      // No. times the macro symbol is repeated.
  SpanID id;        // Unique ID for this span (unique for lifetime of tape).
};

typedef FastList<TapeSpan> Tape;  // ~13% faster than with std::list.

// TODO: Consider refactoring these for performance and/or cleaner code.
inline static BigNum tape_population(const Tape& tape) {
  BigNum num_ones = 0;
  for (auto span = tape.begin(); span != tape.end(); ++span) {
    if (span->symbol) {
      num_ones += span->size * __builtin_popcountll(span->symbol);
    }
  }
  return num_ones;
}

inline static BigNum tape_num_macro_symbols(const Tape& tape) {
  // TODO: This needs to exclude the leading/trailing zeros of the first/last
  // non-zero spans.
  BigNum length = 0;
  for (auto span = tape.begin(); span != tape.end(); ++span) {
    if (span != tape.begin() && std::next(span) != tape.end()) {
      length += span->size;
    }
  }
  return length;
}

inline static std::vector<MacroSym> tape_symbols(const Tape& tape) {
  std::vector<MacroSym> result;
  result.reserve(tape.size());
  for (const TapeSpan& span : tape) {
    result.push_back(span.symbol);
  }
  return result;
}

inline static std::vector<BigNum> tape_sizes(const Tape& tape) {
  std::vector<BigNum> result;
  result.reserve(tape.size());
  for (const TapeSpan& span : tape) {
    result.push_back(span.size);
  }
  return result;
}

struct MacroMachineState {
  uint32_t state;
  Tape tape;
  Tape::iterator cur_span;
  bool moving_right;
  SpanID span_id_counter;

  MacroMachineState()
      : state(0),
        tape(),
        cur_span(tape.end()),
        moving_right(true),
        span_id_counter(0) {
    // Note that moving_right=true => start at left edge of current span.
    // These first and last spans represent the infinite empty tape ends and
    // are never modified during processing.
    tape.push_back(TapeSpan{0, 0, span_id_counter++});
    tape.push_back(TapeSpan{0, 0, span_id_counter++});
    cur_span = std::next(tape.begin());
  }
};

class MacroMachine {
 public:
  MacroMachine(const RuleTable& rule_table, int macro_nbit)
      : micro_machine_(rule_table, macro_nbit) {}

  // Performs one update step on the tape, updating the arguments, and returns
  // a pair (num_micro_steps, num_macro_steps).
  void step(MacroMachineState* mstate, BigNum* num_micro_steps,
            BigNum* num_macro_steps, SpanID* deleted_span_id = nullptr,
            Tape::iterator* shrunk_span = nullptr,
            // HACK TODO: Clean up this interface. Maybe just return deltas
            // instead of updating absolutes?
            BigNum* this_num_micro_steps_ptr = nullptr,
            bool* did_jump = nullptr) const;

 private:
  MicroMachine micro_machine_;
};
