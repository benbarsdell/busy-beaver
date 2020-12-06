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

#include "proof_machine.hpp"

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

std::ostream& operator<<(std::ostream& os, const PatternKey& key) {
  os << state_char(key.state()) << ": ";
  const char* const sep = "|";
  for (uint i = 0; i < key.symbols().size(); ++i) {
    if (key.moving_right()) {
      os << (i == key.cur_span_idx() ? ">" : sep);
    } else {
      os << (i == key.cur_span_idx() + 1 ? "<" : sep);
    }
    os << symbol_string(key.symbols()[i]);
  }
  return os;
}

BigNum Pattern::num_times_applicable(const MacroMachineState& mstate) const {
  BigNum min_num_times = -1;
  int64_t span_idx = 0;
  for (const TapeSpan& span : mstate.tape) {
    const auto& lbound_and_delta = lbounds_and_deltas_[span_idx++];
    if (lbound_and_delta.second == 0) {
      // Fixed spans must not change in size.
      if (span.size != lbound_and_delta.first) return 0;
    } else if (lbound_and_delta.second < 0) {
      // Shrinking spans must meet or exceed the lower bound.
      if (span.size < lbound_and_delta.first) return 0;
      BigNum num_times =
          1 + (span.size - lbound_and_delta.first) / -lbound_and_delta.second;
      if (min_num_times == -1 || num_times < min_num_times) {
        min_num_times = num_times;
      }
    }
  }
  return min_num_times;
}

BigNum Pattern::apply(MacroMachineState* mstate, BigNum* num_micro_steps,
                      BigNum* num_macro_steps, BigNum* num_iters) const {
  assert((size_t)mstate->tape.size() == lbounds_and_deltas_.size());
  BigNum num_times = num_times_applicable(*mstate);
  if (num_times == 0) return 0;
  *num_micro_steps += num_micro_steps_ * num_times;
  int64_t span_idx = 0;
  for (TapeSpan& span : mstate->tape) {
    const auto& lbound_and_delta = lbounds_and_deltas_[span_idx];
    // TODO: Try to clean this up a bit.
    const BigNum& m = span_num_micro_steps_[span_idx].first;
    const BigNum& c = span_num_micro_steps_[span_idx].second;
    const BigNum& s0 = span.size;
    const BigNum& delta = lbound_and_delta.second;
    BigNum s1 = s0 + delta * (num_times - 1);
    if (abs(delta) > 0) {
      BigNum x = num_times * (s0 + s1) / 2;
      *num_micro_steps += m * x;
    }
    *num_micro_steps += c * num_times;
    span.size += delta * num_times;
    span_idx++;
  }
  *num_macro_steps += num_macro_steps_ * num_times;
  *num_iters += num_iters_ * num_times;
  return num_times;
}

std::ostream& operator<<(std::ostream& os, const Pattern& pattern) {
  os << "num_iters=+" << pattern.num_iters_ << " ";
  os << "|";
  // Note: Skips first and last "infinite" spans.
  for (int i = 1; i < (int)pattern.lbounds_and_deltas_.size() - 1; ++i) {
    const auto& lbound_and_delta = pattern.lbounds_and_deltas_[i];
    if (lbound_and_delta.second == 0) {
      os << lbound_and_delta.first;
    } else if (lbound_and_delta.second > 0) {
      os << "+" << lbound_and_delta.second;
    } else {
      os << lbound_and_delta.second << "(>=" << lbound_and_delta.first << ")";
    }
    os << "|";
  }
  return os;
}

bool PatternInstance::confirm_pattern(const PatternInstance& later_instance,
                                      Pattern* pattern, bool* nohalt) const {
  assert(later_instance.num_spans() == num_spans());
  std::vector<std::pair<BigNum, BigNum>> lbounds_and_deltas;
  lbounds_and_deltas.reserve(num_spans());
  bool any_decreasing = false;
  for (size_t i = 0; i < num_spans(); ++i) {
    // Note: The IDs matching means the span's size never reached 0
    // between the historic state and the current state. This is
    // sufficient to prove that the same transition will also happen
    // for any greater initial span size, because the simulation is
    // independent of the absolute sizes of the spans as long as they
    // don't reach 0 (and get erased).
    if (later_instance.span_id(i) != span_id(i) &&
        later_instance.span_size(i) != span_size(i)) {
      return false;
    }
    const BigNum& size_lbound = span_size(i);
    BigNum size_delta = later_instance.span_size(i) - span_size(i);
    lbounds_and_deltas.emplace_back(size_lbound, size_delta);
    if (size_delta < 0) {
      any_decreasing = true;
    }
  }
  *nohalt = !any_decreasing;  // Indicates pattern does not shrink with time.
  BigNum num_micro_steps = later_instance.micro_step_num_ - micro_step_num_;
  BigNum num_macro_steps = later_instance.macro_pos_ - macro_pos_;
  BigNum num_iters = later_instance.iter_num_ - iter_num_;
  *pattern =
      Pattern(lbounds_and_deltas, num_micro_steps, num_macro_steps, num_iters);
  return true;
}

std::ostream& operator<<(std::ostream& os, const PatternInstance& inst) {
  os << "iter_num=" << inst.iter_num_ << " ";
  os << "|";
  // Note: Skips first and last "infinite" spans.
  for (int i = 1; i < (int)inst.span_sizes_and_ids_.size() - 1; ++i) {
    const auto& size_and_id = inst.span_sizes_and_ids_[i];
    os << "@" << size_and_id.second << "*" << size_and_id.first;
    os << "|";
  }
  return os;
}

BigNum ProofMachine::step_with_potential_pattern(
    Pattern* pattern, const PatternInstance& current_instance,
    MacroMachineState* mstate, BigNum* num_micro_steps, BigNum* macro_pos,
    BigNum* num_iters) const {
  // At this point the pattern has only been proven for span sizes larger than
  // the current ones.
  struct SpanInfo {
    int idx;
    BigNum min_size;
    BigNum num_micro_steps_per_symbol;
    BigNum num_micro_steps_offset;
  };
  using SpanInfoMap = std::unordered_map<SpanID, SpanInfo>;
  SpanInfoMap pattern_span_info;
  for (int span_idx = 0; span_idx < (int)pattern->num_spans(); ++span_idx) {
    if (pattern->span_size_delta(span_idx) != 0) {
      pattern_span_info.emplace(
          current_instance.span_id(span_idx),
          SpanInfo{span_idx, current_instance.span_size(span_idx), BigNum(0),
                   BigNum(0)});
    }
  }
  // Run forward for another round of the pattern while keeping track of the
  // min size of each span. The min sizes will determine the starting size
  // lower-bounds for which the pattern is proven to work. We can then use
  // these lower-bounds to derive the number of times the pattern can be
  // applied starting from the current sizes.
  BigNum pattern_num_micro_steps0 = 0;
  for (BigNum i = 0; i < pattern->num_iters(); ++i) {
    SpanID deleted_span_id = 0;
    Tape::iterator shrunk_span = mstate->tape.end();
    BigNum num_micro_steps0 = *num_micro_steps;
    BigNum macro_pos0 = *macro_pos;
    // auto old_cur_span = mstate->cur_span;
    BigNum old_cur_span_size = mstate->cur_span->size;
    SpanID old_cur_span_id = mstate->cur_span->id;
    BigNum this_num_micro_steps;
    bool did_jump;
    macro_machine_.step(mstate, num_micro_steps, macro_pos, &deleted_span_id,
                        &shrunk_span, &this_num_micro_steps, &did_jump);
    ++*num_iters;
    // Check for the pattern breaking.
    if (deleted_span_id && pattern_span_info.count(deleted_span_id)) {
      // The pattern no longer applies.
      // cout << "Pattern no longer applies " << pattern->num_iters() << endl;
      return 0;
    }
    // Track the min size of each span.
    if (shrunk_span != mstate->tape.end()) {
      auto it = pattern_span_info.find(shrunk_span->id);
      if (it != pattern_span_info.end()) {
        it->second.min_size = std::min(it->second.min_size, shrunk_span->size);
      }
    }
    // Track the no. micro steps as a function of the span sizes.
    SpanInfoMap::iterator old_cur_span_info_iter;
    if (did_jump && (old_cur_span_info_iter = pattern_span_info.find(
                         old_cur_span_id)) != pattern_span_info.end()) {
      auto& old_cur_span_info = old_cur_span_info_iter->second;
      old_cur_span_info.num_micro_steps_per_symbol += this_num_micro_steps;
      const BigNum& size0 = current_instance.span_size(old_cur_span_info.idx);
      old_cur_span_info.num_micro_steps_offset +=
          this_num_micro_steps * (old_cur_span_size - size0);
    } else {
      pattern_num_micro_steps0 += this_num_micro_steps;
    }
  }
  // Update the pattern's lower bounds based on the min span sizes encountered
  // since the beginning of the pattern.
  // TODO: Try to clean this up a bit; pattern.num_micro_steps isn't really
  // meaningful until here.
  pattern->update_num_micro_steps(pattern_num_micro_steps0);
  for (const auto& item : pattern_span_info) {
    auto& span_info = item.second;
    const BigNum& span_start_size = current_instance.span_size(span_info.idx);
    BigNum span_size_lower_bound = span_start_size - span_info.min_size + 1;
    pattern->update_span_size_lower_bound(span_info.idx, span_size_lower_bound);
    pattern->update_span_num_micro_steps(span_info.idx,
                                         span_info.num_micro_steps_per_symbol,
                                         span_info.num_micro_steps_offset);
  }
  return pattern->apply(mstate, num_micro_steps, macro_pos, num_iters);
}

void ProofMachine::step(MacroMachineState* mstate, BigNum* num_micro_steps,
                        BigNum* macro_pos, BigNum* num_iters) const {
  PatternKey pattern_key(*mstate);

  //// HACK TESTING (seems to only be useful for one or two machines?)
  // auto pattern_it = proven_patterns_.find(pattern_key);
  // if (pattern_it != proven_patterns_.end()) {
  //  //cout << "******* Applying existing pattern to skip "
  //  //  << pattern_it->second.num_iters() << " iterations" << endl;
  //  pattern_it->second.apply(mstate, num_micro_steps, macro_pos, num_iters);
  //  //cout << proven_patterns_.size() << endl;
  //}

  historic_instances_type* historic_instances = &history_map_[pattern_key];
  PatternInstance current_instance(mstate->tape, *num_micro_steps, *macro_pos,
                                   *num_iters);

  if (historic_instances->size() >= PATTERN_INSTANCE_THRESHOLD) {
    const PatternInstance& historic_instance = historic_instances->back();
    Pattern pattern;
    bool nohalt;
    if (historic_instance.confirm_pattern(current_instance, &pattern,
                                          &nohalt)) {
      if (nohalt) {
        // TODO: Return this via a msg or similar instead of printing.
        cout << "NON-SHRINKING PATTERN" << endl;
        mstate->state = STATE_NOHALT;
        return;
      }

      // TODO: Consider not doing this if the pattern could only be applied
      // a small no. times (e.g., once) anyway.
      BigNum num_pattern_repeats =
          step_with_potential_pattern(&pattern, current_instance, mstate,
                                      num_micro_steps, macro_pos, num_iters);
      //*proven_patterns_.emplace(pattern_key, pattern);
      // if (double(rand()) / RAND_MAX < 1e-2) { // HACK TESTING
      // cout << "***************** Applied pattern " << num_pattern_repeats
      //     << " times jumping " << num_pattern_repeats * pattern.num_iters()
      //     << " iterations and "
      //     << num_pattern_repeats * pattern.num_micro_steps() << " steps"
      //     << endl;
      //}
      // return;

      history_map_.clear();
      return;
    }
  }
  historic_instances->push_back(current_instance);
  macro_machine_.step(mstate, num_micro_steps, macro_pos);
  ++*num_iters;
}
