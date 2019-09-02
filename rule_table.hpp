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

#include "nbit_array.hpp"
#include "util.hpp"

#include <iostream>

// Note that this implementation only supports single-tape, 2-symbol (binary),
// <=6-state machines.
struct Rule {
  uint32_t state : 3;       // State to change to.
  uint32_t symbol : 1;      // Symbol to write.
  uint32_t move_right : 1;  // Whether to move right or left.
  enum { NBIT = 5 };
};

enum {
  // Values 0-5 represent states A-F.
  STATE_HALT = 6,
  STATE_NOHALT = 7,
  STATE_INCOMPLETE = 8,
};

inline static char state_char(uint32_t state) {
  switch (state) {
    case STATE_HALT:
      return 'H';
    case STATE_NOHALT:
      return 'N';
    default:
      return 'A' + state;
  }
}

// A table of num_symbols(2) * num_states(<=6) rules that define a Turing
// machine program.
class __attribute__((aligned(8))) RuleTable {
 public:
  typedef uint32_t type;
  typedef NBitArray<Rule::NBIT, type> table_type;

 private:
  table_type table0_;
  table_type table1_;

 public:
  // Construct a rule table by parsing a string representation.
  // table: List of transitions A0 A1 ... E0 E1 separated by whitespace,
  //        where each transition contains the characters 0/1 L/R A/B/C/...
  //        E.g., "1RB 1LC 1RC 1RB 1RD 0LE 1LA 1LD 1RH 0LA".
  explicit RuleTable(const std::string& table = "H1R");

  // Print out a rule table.
  friend std::ostream& operator<<(std::ostream& os,
                                  const RuleTable& rule_table);

  // Returns a rule in the table.
  Rule operator()(bool symbol, int state) const {
    table_type table = symbol ? table1_ : table0_;
    return detail::bit_cast<Rule>(static_cast<type>(table[state]));
  }

 private:
  void set_rule(bool symbol, int state, Rule rule) {
    table_type& table = symbol ? table1_ : table0_;
    table[state] = detail::bit_cast<type>(rule);
  }
};
