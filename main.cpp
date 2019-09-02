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

#include "builtin_rule_tables.hpp"
#include "tests.hpp"
#include "turing_machine.hpp"

#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

using std::cerr;
using std::cout;
using std::endl;

class ArgParser {
 public:
  ArgParser(int argc, char* argv[], int argi = 1)
      : argc_(argc), argv_(argv), argi_(argi) {}

  const char* const symbol() const { return argv_[argi_]; }

  void next() { ++argi_; }

  bool has_symbol() const { return argi_ < argc_; }

  bool accept(const std::unordered_set<std::string>& symbols) {
    if (!symbols.count(symbol())) return false;
    next();
    return true;
  }

  bool expect(std::string* s) {
    if (!expect_argument()) return false;
    *s = symbol();
    next();
    return true;
  }

  bool expect(int* i) {
    if (!expect_argument()) return false;
    long val = strtol(symbol(), nullptr, 0);
    if (!val) {
      cerr << "Invalid command line: expected an integer value, got "
           << symbol() << endl;
      return false;
    }
    *i = val;
    next();
    return true;
  }

 private:
  bool expect_argument() {
    if (!has_symbol()) {
      cerr << "Invalid command line: expected an argument";
      return false;
    }
    return true;
  }

  const int argc_;
  const char* const* argv_;
  int argi_;
};

int main(int argc, char* argv[]) {
  bool do_test = false;
  bool do_test_long = false;
  bool verbose = false;
  int macro_nbit = 60;
  std::string builtin_rule_table_code = "";
  bool list_builtins = false;
  std::string rule_table_str;
  ArgParser arg_parser(argc, argv);
  while (arg_parser.has_symbol()) {
    if (arg_parser.accept({"-h", "--help"})) {
      cout << "Usage: " << argv[0] << " [options] [rule_table]" << endl;
      cout << "Options:" << endl;
      cout << "  -h --help                 Display this help information."
           << endl;
      cout << "  -t --test                 Run quick tests." << endl;
      cout << "  -T --test_long            Run long tests." << endl;
      cout
          << "  -k --macro_nbit <int=60>  Set the no. bits per macro symbol to "
             "<int>."
          << endl;
      cout
          << "  -b --builtin <name>       Run the builtin rule table with name "
             "<name>."
          << endl;
      cout << "  -l --list_builtins        List the builtin rule tables."
           << endl;
      cout << "  -v --verbose              Print all digits of the final "
              "results."
           << endl;
      return -1;
    } else if (arg_parser.accept({"-t", "--test"})) {
      do_test = true;
    } else if (arg_parser.accept({"-T", "--test_long"})) {
      do_test_long = true;
    } else if (arg_parser.accept({"-k", "--macro_nbit"})) {
      if (!arg_parser.expect(&macro_nbit)) return -1;
      // HACK TODO: Use MAX_MACRO_NBIT instead of hard-coding 60 here.
      if (macro_nbit <= 0 || macro_nbit > 60) {
        cerr << "Invalid macro_nbit (" << macro_nbit
             << "), must be in the range [1, 60]." << endl;
        return -1;
      }
    } else if (arg_parser.accept({"-b", "--builtin"})) {
      if (!arg_parser.expect(&builtin_rule_table_code)) return -1;
    } else if (arg_parser.accept({"-l", "--list_builtins"})) {
      list_builtins = true;
    } else if (arg_parser.accept({"-v", "--verbose"})) {
      verbose = true;
    } else {
      std::string arg;
      arg_parser.expect(&arg);
      rule_table_str += arg + " ";
    }
  }

  if (do_test && !test()) {
    return -1;
  }
  if (do_test_long && !test_long()) {
    return -1;
  }
  if (do_test || do_test_long) {
    cout << "All tests PASSED" << endl;
    return 0;
  }

  const std::map<std::string, RuleTable> builtin_rule_tables = {
      {"bb1", best1},          {"bb2", best2},          {"bb3", best3},
      {"bb4", best4},          {"bb5", best5},          {"bb6", best6},
      {"bb6_1", bb6_1},        {"bb6_2", bb6_2},        {"bb6_3", bb6_3},
      {"bb6_4", bb6_4},        {"bb6_5", bb6_5},        {"bb6_6", bb6_6},
      {"bb6_7", bb6_7},        {"bb6_8", bb6_8},        {"bb6_9", bb6_9},
      {"bb6_A", bb6_10},       {"mabu90_3", mabu90_3},  {"mabu90_4", mabu90_4},
      {"mabu90_5", mabu90_5},  {"mabu90_7", mabu90_7},  {"mabu90_8", mabu90_8},
      {"bb5_hnr1", bb5hnr1},   {"bb5_hnr2", bb5hnr2},   {"bb5_hnr3", bb5hnr3},
      {"bb5_hnr16", bb5hnr16}, {"bb5_hnr19", bb5hnr19}, {"bb5_hnr24", bb5hnr24},
      {"bb5_hnr37", bb5hnr37}, {"bb5_hnr40", bb5hnr40}, {"bb5_hnr41", bb5hnr41},
      {"bb5_hnr42", bb5hnr42}, {"bb5_nr1_1", bb5nr1_1}, {"bb5_nr1_2", bb5nr1_2},
  };

  if (list_builtins) {
    cout << "Builtin rule tables:" << endl;
    for (const auto& item : builtin_rule_tables) {
      cout << item.first << std::string(16 - item.first.size(), ' ')
           << item.second << endl;
    }
    return 0;
  }

  RuleTable rule_table;
  if (!builtin_rule_table_code.empty()) {
    if (!rule_table_str.empty()) {
      cerr << "Cannot specify both --builtin and a custom rule table" << endl;
      return -1;
    }
    auto it = builtin_rule_tables.find(builtin_rule_table_code);
    if (it == builtin_rule_tables.end()) {
      cerr << "Invalid builtin rule table: " << builtin_rule_table_code
           << endl;
      cerr
          << "Use the --list_builtins flag to see a list of available programs."
          << endl;
      return -1;
    }
    rule_table = it->second;
  } else if (!rule_table_str.empty()) {
    try {
      rule_table = RuleTable(rule_table_str);
    } catch (const std::runtime_error& e) {
      cerr << "Invalid argument: " << e.what() << endl;
      return -1;
    }
  } else {
    // Default to BB5
    rule_table = RuleTable(best5);
  }

  cout << rule_table << endl;

  TMResult result = run_turing_machine(rule_table, macro_nbit);
  if (result.state == STATE_INCOMPLETE) {
    cout << "Program execution did not complete" << endl;
  } else if (result.state == STATE_NOHALT) {
    cout << "Program does not halt" << endl;
  } else {
    cout << ConcisePrintBigNum(result.num_ones) << " ones in "
         << ConcisePrintBigNum(result.num_steps) << " steps,"
         << " ending in state ";
    if (result.state == STATE_HALT) {
      cout << "HALT" << endl;
    } else {
      cout << result.state << endl;
    }
  }
  if (verbose) {
    cout << "Num ones:" << endl;
    cout << result.num_ones << endl;
    cout << "Num steps:" << endl;
    cout << result.num_steps << endl;
  }
  return 0;
}
