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

#include "turing_machine.hpp"

#include "proof_machine.hpp"

#include <sys/sysinfo.h>  // For querying available RAM.

#include <bitset>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;

namespace {

double get_free_ram_fraction() {
  struct sysinfo si;
  ::sysinfo(&si);
  return double(si.freeram) / si.totalram;
}

std::string symbol_binary_string(int macro_nbit, MacroSym symbol) {
  std::stringstream ss;
  ss << std::bitset<MAX_MACRO_NBIT>(symbol);
  std::string s = ss.str().substr(MAX_MACRO_NBIT - macro_nbit, macro_nbit);
  std::reverse(s.begin(), s.end());
  return s;
}

void print_status(int macro_nbit, uint state, const Tape& tape,
                  Tape::iterator cur_span, bool moving_right,
                  bool uncompressed = false) {
  cout << state_char(state) << ": ";
  const char* const sep = uncompressed ? "" : "|";
  for (auto span = tape.begin(); span != tape.end(); ++span) {
    if (moving_right) {
      cout << (span == cur_span ? ">" : sep);
    } else {
      cout << (span == std::next(cur_span) ? "<" : sep);
    }
    if (!uncompressed) {
      if (macro_nbit <= 8) {
        cout << symbol_binary_string(macro_nbit, span->symbol) << "(@"
             << span->id << ")";
      } else {
        cout << symbol_string(span->symbol) << "(@" << span->id << ")";
      }
    }
    if (span != tape.begin() && std::next(span) != tape.end()) {
      if (uncompressed) {
        for (int i = 0; i < span->size; ++i) {
          cout << symbol_binary_string(macro_nbit, span->symbol);
        }
      } else {
        cout << "*" << ConcisePrintBigNum(span->size);
      }
    }
  }
  cout << endl;
}

class comma_numpunct : public std::numpunct<char> {
 protected:
  virtual char do_thousands_sep() const { return ','; }
  virtual std::string do_grouping() const { return "\03"; }
};

}  // end namespace

TMResult run_turing_machine(RuleTable rule_table, int macro_nbit,
                            size_t max_num_spans) {
  cout << "-----------------------------------------" << endl;
  cout << "Running Turing machine with macro_nbit=" << macro_nbit << endl;
  cout << "-----------------------------------------" << endl;
  static const std::locale c_locale("C");
  static const std::locale comma_locale(std::locale(), new comma_numpunct());
  ProofMachine proof_machine(rule_table, macro_nbit);
  MacroMachineState mstate;
  BigNum num_micro_steps = 0;
  BigNum old_num_micro_steps = 0;
  BigNum avg_num_micro_steps_per_sec = -1;
  BigNum macro_pos = 0;
  // TODO: Need better names for these.
  BigNum num_iters = 0;
  BigNum num_proof_steps = 0;
  auto print_interval = std::chrono::seconds(1);
  auto last_print_time = std::chrono::steady_clock::now();

  while (mstate.state != STATE_HALT && mstate.state != STATE_NOHALT) {
    proof_machine.step(&mstate, &num_micro_steps, &macro_pos, &num_iters);
    ++num_proof_steps;

    auto elapsed_time = std::chrono::steady_clock::now() - last_print_time;
    if (  // true || num_proof_steps < num_iters || // HACK TESTING added first
          // condition(s) for debugging
        elapsed_time >= print_interval) {
      last_print_time = std::chrono::steady_clock::now();
      // Print large numbers with thousands separators
      cout.imbue(comma_locale);
      cout << "Proof steps: " << ConcisePrintBigNum(num_proof_steps) << endl;
      cout << "Macro steps: " << ConcisePrintBigNum(num_iters) << endl;
      // std::chrono::duration<double> elapsed_secs = elapsed_time;
      auto elapsed_us =
          std::chrono::duration_cast<std::chrono::microseconds>(elapsed_time);
      BigNum num_micro_steps_per_sec =
          //(num_micro_steps - old_num_micro_steps) / elapsed_secs.count();
          (num_micro_steps - old_num_micro_steps) * 1000000 /
          elapsed_us.count();
      if (avg_num_micro_steps_per_sec == -1) {
        avg_num_micro_steps_per_sec = num_micro_steps_per_sec;
      } else {
        avg_num_micro_steps_per_sec = avg_num_micro_steps_per_sec * 95 / 100;
        avg_num_micro_steps_per_sec += num_micro_steps_per_sec * 5 / 100;
      }
      cout << "Micro steps: " << ConcisePrintBigNum(num_micro_steps)
           << " (avg speed=" << ConcisePrintBigNum(avg_num_micro_steps_per_sec)
           << "/s)" << endl;
      old_num_micro_steps = num_micro_steps;
      cout << "Num spans:   " << mstate.tape.size() << endl;
      BigNum tape_len = tape_num_macro_symbols(mstate.tape) * macro_nbit;
      cout << "Tape size:   " << ConcisePrintBigNum(tape_len) << endl;
      BigNum tape_pop = tape_population(mstate.tape);
      cout << "Num ones:    " << ConcisePrintBigNum(tape_pop) << " ("
           << (100. * tape_pop / tape_len) << "%)" << endl;
      BigNum tape_pos = macro_pos * macro_nbit;
      cout << "Head pos:    " << ConcisePrintBigNum(tape_pos) << " ("
           << (100. * tape_pos / tape_len) << "%)" << endl;
      cout.imbue(c_locale);
      cout << ConcisePrintBigNum(num_micro_steps) << ": ";
      print_status(macro_nbit, mstate.state, mstate.tape, mstate.cur_span,
                   mstate.moving_right);
      cout << endl;
      // cout << "PAUSED" << endl;
      // std::cin.get();

      if ((size_t)mstate.tape.size() >= max_num_spans) {
        mstate.state = STATE_INCOMPLETE;
        break;
      }

      if (get_free_ram_fraction() < 0.05) {
        std::cerr << "********************" << endl;
        std::cerr << "Error: RAM exhausted" << endl;
        std::cerr << "********************" << endl;
        mstate.state = STATE_INCOMPLETE;
        break;
      }
    }
  }
  cout.imbue(comma_locale);
  cout << "Proof steps: " << ConcisePrintBigNum(num_proof_steps) << endl;
  cout << "Macro steps: " << ConcisePrintBigNum(num_iters) << endl;
  cout << "Micro steps: " << ConcisePrintBigNum(num_micro_steps) << endl;
  cout << "Num spans:   " << mstate.tape.size() << endl;
  cout.imbue(c_locale);
  print_status(macro_nbit, mstate.state, mstate.tape, mstate.cur_span,
               mstate.moving_right);
  BigNum num_ones = -1;
  if (mstate.state == STATE_HALT) {
    num_ones = tape_population(mstate.tape);
  }
  return TMResult{num_ones, num_micro_steps, mstate.state};
}
