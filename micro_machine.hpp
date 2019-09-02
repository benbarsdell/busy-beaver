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

#include "rule_table.hpp"
#include "util.hpp"

#include <cstdint>
#include <unordered_map>

// Note that this implementation only support MACRO_NBIT <= 60.
enum { MAX_MACRO_NBIT = 60 };
typedef uint64_t MacroSym;

inline static std::string symbol_string(MacroSym symbol) {
  // Map the N'th unique symbol to the number N.
  static std::unordered_map<uint64_t, uint64_t> symbol_cache;
  auto it = symbol_cache.find(symbol);
  if (it == symbol_cache.end()) {
    it = symbol_cache.emplace(symbol, symbol_cache.size()).first;
  }
  return "$" + std::to_string(it->second);
}

struct MicroMachineState {
  uint32_t state : 3;
  MacroSym symbol : MAX_MACRO_NBIT;
  bool move_right : 1;

  bool operator==(MicroMachineState other) const {
    using detail::bit_cast;
    return bit_cast<uint64_t>(*this) == bit_cast<uint64_t>(other);
  }
  bool operator<(MicroMachineState other) const {
    using detail::bit_cast;
    return bit_cast<uint64_t>(*this) < bit_cast<uint64_t>(other);
  }
  size_t hash() const {
    using detail::bit_cast;
    return std::hash<uint64_t>()(bit_cast<uint64_t>(*this));
  }
};

namespace std {
template <>
struct hash<MicroMachineState> {
  size_t operator()(const MicroMachineState& ms) const { return ms.hash(); }
};
}  // namespace std

// Performs step-by-step simulation within a single macro-symbol.
class MicroMachine {
 public:
  MicroMachine(const RuleTable& rule_table, int macro_nbit)
      : rule_table_(rule_table), macro_nbit_(macro_nbit) {
    assert(macro_nbit_ <= MAX_MACRO_NBIT);
  }

  // Updates *mstate and returns the number of micro steps that were taken.
  int64_t step(MicroMachineState* mstate) const;

 private:
  RuleTable rule_table_;
  int macro_nbit_;
  // Caches the results of the step() method, mapping mstate -> (mstate,
  // num_micro_steps).
  mutable std::unordered_map<MicroMachineState,
                             std::pair<MicroMachineState, int64_t>>
      _cache;
};
