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

#include "tests.hpp"

#include "builtin_rule_tables.hpp"
#include "turing_machine.hpp"

#include <iostream>

namespace {

using std::cerr;
using std::cout;
using std::endl;

template <typename BN1, typename BN2>
bool test_case(RuleTable rule_table, int macro_nbit,
               const BN1& expected_num_ones, const BN2& expected_num_steps,
               uint32_t expected_state) {
  cerr << "====================================================" << endl;
  cerr << "Testing the following rule table with macro_nbit=" << macro_nbit
       << ":" << endl;
  cerr << rule_table << endl;
  cerr << "====================================================" << endl;
  bool passed = true;
  TMResult result = run_turing_machine(rule_table, macro_nbit);
  if (result.num_ones != expected_num_ones) {
    passed = false;
    cerr << "Expected " << expected_num_ones << " ones on tape, got "
         << ConcisePrintBigNum(result.num_ones) << endl;
  }
  if (result.num_steps != expected_num_steps) {
    passed = false;
    cerr << "Expected " << expected_num_steps << " steps on tape, got "
         << ConcisePrintBigNum(result.num_steps) << endl;
  }
  if (result.state != expected_state) {
    passed = false;
    cerr << "Expected to end in state " << expected_state << ", got "
         << result.state << endl;
  }
  if (passed) {
    cerr << "Test PASSED" << endl;
  } else {
    cerr << "Test FAILED" << endl;
  }
  return passed;
}

}  // namespace

bool test() {
  bool passed = true;
  for (int macro_nbit : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 60}) {
    passed &= test_case(best1, macro_nbit, 1, 1, STATE_HALT);
    passed &= test_case(best2, macro_nbit, 4, 6, STATE_HALT);
    passed &= test_case(best3, macro_nbit, 6, 14, STATE_HALT);
    passed &= test_case(best4, macro_nbit, 13, 107, STATE_HALT);
  }
  for (int macro_nbit : {3, 6, 9, 12, 30, 60}) {
    passed &= test_case(best5, macro_nbit, 4098, 47176870, STATE_HALT);
    passed &= test_case(bb6_1, macro_nbit, 136612, 13122572797LU, STATE_HALT);
  }
  for (int macro_nbit : {2, 4, 6, 8, 40, 60}) {
    passed &=
        test_case(bb6_2, macro_nbit, 95524079, 8690333381690951LU, STATE_HALT);
    passed &= test_case(
        bb6_5, macro_nbit, ConciseCompareBigNum(142869590, 17928251, 60),
        ConciseCompareBigNum(612351597, 788910538, 119), STATE_HALT);
  }
  passed &= test_case(mabu90_8, 3, -1, 155, STATE_NOHALT);
  passed &=
      test_case(bb6_8, 4, ConciseCompareBigNum(250010283, 232693664, 881),
                ConciseCompareBigNum(892930596, 430817336, 1762), STATE_HALT);
  passed &=
      test_case(bb6_9, 4, ConciseCompareBigNum(464098470, 543758576, 1439),
                ConciseCompareBigNum(258464867, 609889227, 2879), STATE_HALT);
  return passed;
}

bool test_long() {
  bool passed = true;
  passed &=
      test_case(bb6_10, 3, ConciseCompareBigNum(318711900, 928090906, 10566),
                ConciseCompareBigNum(380914784, 483559719, 21132), STATE_HALT);
  passed &=
      test_case(best6, 6, ConciseCompareBigNum(351474952, 618690847, 18267),
                ConciseCompareBigNum(741207853, 260478608, 36534), STATE_HALT);
  return passed;
}
