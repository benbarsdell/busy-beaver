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

#include "macro_machine.hpp"
#include "util.hpp"

#include <iostream>

class PatternKey : public std::tuple<uint, std::vector<MacroSym>, uint, bool> {
  typedef std::tuple<uint, std::vector<MacroSym>, uint, bool> super_type;

 public:
  explicit PatternKey(const MacroMachineState& mstate)
      : super_type(
            mstate.state, tape_symbols(mstate.tape),
            std::distance(mstate.tape.begin(),
                          static_cast<Tape::const_iterator>(mstate.cur_span)),
            mstate.moving_right) {}

  uint state() const { return std::get<0>(*this); }
  const std::vector<MacroSym>& symbols() const { return std::get<1>(*this); }
  uint cur_span_idx() const { return std::get<2>(*this); }
  bool moving_right() const { return std::get<3>(*this); }

  size_t hash() const {
    using detail::hash_combine;
    size_t symbols_hash = 0;
    for (const auto& symbol : symbols()) {
      symbols_hash = hash_combine(symbols_hash, symbol);
    }
    return hash_combine(state(), symbols_hash, cur_span_idx(), moving_right());
  }

  friend std::ostream& operator<<(std::ostream& os, const PatternKey& key);
};

namespace std {
template <>
struct hash<PatternKey> {
  size_t operator()(const PatternKey& pk) const { return pk.hash(); }
};
}  // namespace std

class Pattern {
 public:
  Pattern() = default;
  Pattern(const std::vector<std::pair<BigNum, BigNum>>& lbounds_and_deltas,
          BigNum num_micro_steps, BigNum num_macro_steps, BigNum num_iters)
      : lbounds_and_deltas_(lbounds_and_deltas),
        num_micro_steps_(num_micro_steps),
        num_macro_steps_(num_macro_steps),
        num_iters_(num_iters) {}
  size_t num_spans() const { return lbounds_and_deltas_.size(); }
  BigNum num_iters() const { return num_iters_; }
  BigNum num_micro_steps() const { return num_micro_steps_; }
  BigNum span_size_lower_bound(size_t span_idx) const {
    return lbounds_and_deltas_[span_idx].first;
  }
  void update_span_size_lower_bound(size_t span_idx, BigNum lower_bound) {
    lbounds_and_deltas_[span_idx].first = lower_bound;
  }
  void update_num_micro_steps(const BigNum& num_micro_steps) {
    num_micro_steps_ = num_micro_steps;
  }
  void update_span_num_micro_steps(size_t span_idx,
                                   BigNum num_micro_steps_per_symbol,
                                   BigNum num_micro_steps_offset) {
    // HACK Find a cleaner way to manage this metadata.
    span_num_micro_steps_.resize(lbounds_and_deltas_.size());
    span_num_micro_steps_[span_idx] =
        std::make_pair(num_micro_steps_per_symbol, num_micro_steps_offset);
  }
  BigNum span_size_delta(size_t span_idx) const {
    return lbounds_and_deltas_[span_idx].second;
  }

  // Updates all args and returns the no. times the rule was applied (which may
  // be 0, indicating that the pattern could not be applied).
  BigNum apply(MacroMachineState* mstate, BigNum* num_micro_steps,
               BigNum* num_macro_steps, BigNum* num_iters) const;

  friend std::ostream& operator<<(std::ostream& os, const Pattern& pattern);

 private:
  BigNum num_times_applicable(const MacroMachineState& mstate) const;

  std::vector<std::pair<BigNum, BigNum>> lbounds_and_deltas_;
  BigNum num_micro_steps_;
  BigNum num_macro_steps_;
  BigNum num_iters_;
  // For each span: (num_micro_steps_per_symbol, num_micro_steps_offset)
  std::vector<std::pair<BigNum, BigNum>> span_num_micro_steps_;
};

class PatternInstance {
  BigNum micro_step_num_;
  BigNum macro_pos_;
  BigNum iter_num_;
  std::vector<std::pair<BigNum, SpanID>> span_sizes_and_ids_;

 public:
  explicit PatternInstance(const Tape& tape, BigNum micro_step_num,
                           BigNum macro_pos, BigNum iter_num)
      : micro_step_num_(micro_step_num),
        macro_pos_(macro_pos),
        iter_num_(iter_num) {
    span_sizes_and_ids_.reserve(tape.size());
    for (const auto& span : tape) {
      span_sizes_and_ids_.push_back(std::make_pair(span.size, span.id));
    }
  }
  const BigNum& iter_num() const { return iter_num_; }
  size_t num_spans() const { return span_sizes_and_ids_.size(); }
  const BigNum& span_size(size_t span_idx) const {
    return span_sizes_and_ids_[span_idx].first;
  }
  SpanID span_id(size_t span_idx) const {
    return span_sizes_and_ids_[span_idx].second;
  }

  // If the transition *this -> later_instance forms a proven pattern, this
  // method writes the pattern to *pattern and returns true; otherwise it
  // returns false. If the pattern is proven, *nohalt is set to true if the
  // pattern does not shrink with time and otherwise false.
  bool confirm_pattern(const PatternInstance& later_instance, Pattern* pattern,
                       bool* nohalt) const;

  friend std::ostream& operator<<(std::ostream& os,
                                  const PatternInstance& inst);
};

class ProofMachine {
  enum { PATTERN_INSTANCE_THRESHOLD = 3 };

 public:
  ProofMachine(const RuleTable& rule_table, int macro_nbit)
      : macro_machine_(rule_table, macro_nbit) {}

  // Updates the arguments.
  void step(MacroMachineState* mstate, BigNum* num_micro_steps,
            BigNum* macro_pos, BigNum* num_iters) const;

 private:
  // Returns the number of times the pattern was applied (may be 0 if the
  // pattern was disproved).
  BigNum step_with_potential_pattern(Pattern* pattern,
                                     const PatternInstance& current_instance,
                                     MacroMachineState* mstate,
                                     BigNum* num_micro_steps, BigNum* macro_pos,
                                     BigNum* num_iters) const;

  MacroMachine macro_machine_;
  //*typedef std::list<PatternInstance> historic_instances_type;
  typedef FastList<PatternInstance> historic_instances_type;
  // **TODO: Consider moving these (along with MacroMachineState) into a
  //           ProofMachineState struct to be passed to step(). This would
  //             treat them as official parts of the machine state, instead of
  //             just caching utilities.
  // Maps tape patterns to their historic instances.
  mutable std::unordered_map<PatternKey, historic_instances_type> history_map_;
  mutable std::unordered_map<PatternKey, Pattern> proven_patterns_;
};
