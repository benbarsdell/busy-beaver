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

#include "rule_table.hpp"

#include <sstream>

RuleTable::RuleTable(const std::string& table)
    // Initialized such that all states are STATE_NOHALT for use in printing.
    : table0_(0xFFFFFFFFu), table1_(0xFFFFFFFFu) {
  std::stringstream ss(table);
  std::string buf;
  int i = 0;
  while (ss >> buf) {
    if (i / 2 >= 6) {
      throw std::runtime_error("Rule table exceeds limit of 6 states.");
    }
    if (buf.size() != 3) {
      throw std::runtime_error("Invalid rule string: \"" + buf +
                               "\". Expected 3 characters.");
    }
    Rule rule;
    for (char c : buf) {
      c = std::toupper(c);
      // TODO: Add a check that state + symbol + move_right are all specified.
      if (c == '0' || c == '1')
        rule.symbol = (c == '1');
      else if (c == 'L' || c == 'R')
        rule.move_right = (c == 'R');
      else if (c >= 'A' && c <= 'F')
        rule.state = c - 'A';
      else if (c == 'H')
        rule.state = STATE_HALT;
      else {
        throw std::runtime_error(
            std::string("Invalid character in rule string: \"") + c + "\"");
      }
    }
    bool symbol = i % 2;
    uint32_t state = i / 2;
    ++i;
    set_rule(symbol, state, rule);
  }
}

std::ostream& operator<<(std::ostream& os, const RuleTable& rule_table) {
  // This relies on initializing the table to STATE_NOHALT.
  for (int st = 0; st < 6 && rule_table(0, st).state != STATE_NOHALT; ++st) {
    for (int sym = 0; sym < 2; ++sym) {
      Rule rule = rule_table(sym, st);
      char state_char = rule.state == STATE_HALT ? 'H' : 'A' + rule.state;
      os << state_char << rule.symbol << (rule.move_right ? 'R' : 'L') << " ";
    }
    os << " ";
  }
  return os;
}
