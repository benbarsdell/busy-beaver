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
#include "util.hpp"

#include <unordered_set>

namespace {

struct Situation : public std::tuple<int, MacroSym, uint> {
  typedef std::tuple<int, MacroSym, uint> super_type;
  using super_type::super_type;
};

struct HashSituation {
  size_t operator()(const Situation& s) const {
    return detail::hash_combine(0, std::get<0>(s), std::get<1>(s),
                                std::get<2>(s));
  }
};

}  // namespace

int64_t MicroMachine::step(MicroMachineState* mstate) const {
  auto cache_iter = _cache.find(*mstate);
  if (cache_iter != _cache.end()) {
    *mstate = cache_iter->second.first;
    return cache_iter->second.second;
  }
  typedef NBitArray<1, MacroSym> MicroTape;
  uint state = mstate->state;
  MicroTape tape = mstate->symbol;
  int pos = mstate->move_right ? 0 : macro_nbit_ - 1;
  std::unordered_set<Situation, HashSituation> visited;
  bool final_move_right = true;
  int64_t num_steps = 0;
  while (state != STATE_HALT) {
    Situation situation(pos, tape, state);
    if (visited.count(situation)) {
      state = STATE_NOHALT;
      break;  // Loop found
    }
    visited.insert(situation);

    Rule rule = rule_table_(tape[pos], state);
    state = rule.state;
    tape[pos] = rule.symbol;
    pos += rule.move_right ? 1 : -1;
    num_steps += 1;
    if (pos == -1 || pos == macro_nbit_) {
      final_move_right = (pos == macro_nbit_);
      break;  // Ran off edge of the macro symbol
    }
  }
  MicroMachineState result{state, tape, final_move_right};
  _cache.emplace(*mstate, std::make_pair(result, num_steps));
  *mstate = result;
  return num_steps;
}
