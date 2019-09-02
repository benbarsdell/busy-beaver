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

// References
// [1] Alex Holkner, 2004, Acceleration Techniques for Busy Beaver Candidates.
// [2] http://www.logique.jussieu.fr/~michel/bbc.html
// [3] http://www.logique.jussieu.fr/~michel/ha.html
// [4] https://skelet.ludost.net/bb/nreg.html
// [5] http://www.logique.jussieu.fr/~michel/beh.html#tm62c
// [6] Heiner Marxen and Jurgen Buntrock, 1990, Attacking the Busy Beaver 5
// [7] https://googology.wikia.org/wiki/Forum:Sigma_project

const RuleTable best1("H1R");
const RuleTable best2("B1R B1L  A1L H1R");
const RuleTable best3("B1R H1R  C0R B1R  C1L A1L");
const RuleTable best4("B1R B1L  A1L C0L  H1R D1L  D1R A0R");
const RuleTable best5("B1R C1L C1R B1R D1R E0L A1L D1L H1R A0L");
// [reasonably fast to compute with macro_nbit=6]
const RuleTable best6("B1R E1L  C1R F1R  D1L B0R  E1R C0L  A1L D0R  H1R C1R");
// From Ref. [1].
const RuleTable bb6_1("B1L A1L  C1R B1R  F0R D1R  A1L E0R  A0L C1R  E1L H1L");
const RuleTable bb6_2("B1R A1R  C1L B1L  F0R D1L  A1R E0L  H1R F1L  A0L C0L");
// From Ref. [3], 2.0 × 10^95 steps [very slow to compute].
const RuleTable bb6_3("1RB 0RC  0LA 0RD  1RD 1RH  1LE 0LD  1RF 1LB  1RA 1RE");
// From Ref. [3], 5.3 × 10^42 steps [very slow to compute].
const RuleTable bb6_4("1RB 0LC  1LA 1RD  0LB 0LE  1RA 0RB  1LF 1LC  1RD 1RH");
// From Ref. [5], > 1.4e60 ones [very fast to compute].
const RuleTable bb6_5("1RB 0LC  1LA 1RC  1RA 0LD  1LE 1LC  1RF 1RH  1RA 1RE");
// From Ref. [5], > 6.4e462 ones [very slow to compute].
const RuleTable bb6_6("1RB 0LB  0RC 1LB  1RD 0LA  1LE 1LF  1LA 0LD  1RH 1LE");
// From Ref. [5], > 1.2e865 ones [very slow to compute].
const RuleTable bb6_7("1RB 0LF  0RC 0RD  1LD 1RE  0LE 0LD  0RA 1RC  1LA 1RH");
// From Ref. [5], > 2.5e881 ones [quite fast to compute].
const RuleTable bb6_8("1RB 0RF  0LB 1LC  1LD 0RC  1LE 1RH  1LF 0LD  1RA 0LE");
// From Ref. [5], > 4.6e1439 ones [quite fast to compute].
const RuleTable bb6_9("1RB 0LE  1LC 0RA  1LD 0RC  1LE 0LF  1LA 1LC  1LE 1RH");
// From Ref. [5], > 3.1e10566 ones [fairly fast to compute with macro_nbit=3].
const RuleTable bb6_10("1RB 0LD  1RC 0RF  1LC 1LA  0LE 1RH  1LA 0RB  0RC 0RE");
// From Ref. [4].
const RuleTable bb5nr1_1("C1L B1L  H1L A0L  D1R C1L  A0L E0R  C1R E1R");
// Note: bb5nr1_2 is extremely micro-machine intensive.
const RuleTable bb5nr1_2("C1L D1R  H1L D0R  D1R A1L  B0R E0R  A0L E1L");
const RuleTable bb5hnr1("C1L E1L  H1L D1L  D1R D0L  A1L E1R  B0L C0R");
const RuleTable bb5hnr2("C1L E0R  H1L C0R  D1R A0L  A1R D1R  A1L B0R");
const RuleTable bb5hnr3("C1L A0R  H1L E1L  D1R B0L  A1R C1R  C0L D1L");
// Ref. [7] claims these are the only remaining unproven cases as of 2019/5/5.
const RuleTable bb5hnr16("B1L H1L  C0R D1L  D1R C1R  E1L E0L  A0L B0R");
const RuleTable bb5hnr19("B1L H1L  C0L B0L  C1R D0R  A1L E0R  A0R E0R");
const RuleTable bb5hnr24("C1L A1L  E1R H1L  D1R D0R  B0R E0L  A0L C1R");
const RuleTable bb5hnr37("C1L C0L  D1L H1L  B0L D0R  E0R A1L  A1R E1R");
const RuleTable bb5hnr40("B1L A0R  C1L H1L  D0L E1R  E1L A0L  C1R A0R");
const RuleTable bb5hnr41("B1L E0R  C1L H1L  D0L C0L  D1R A0R  B0R E0R");
const RuleTable bb5hnr42("B1L A0R  C0L H1L  C1R D1L  E1L A1R  B0L D0R");
// From Ref. [6], requires MACRO_NBIT=80, which we don't support.
const RuleTable mabu90_3("B1L B0R  C1R E0L  A1L D0R  C0R A1R  C1L H1L");
// From Ref. [6], non-shrinking pattern with MACRO_NBIT=4 (confirm?).
const RuleTable mabu90_4("B1L B1R  C1R E0L  D0R A0L  A1L D0R  A1L D0R");
// From Ref. [6], requires 2-bck-bck macro machine?.
const RuleTable mabu90_5("B1L A1R  A0R C0L  A0R D1L  E0L B1R  B0R H1L");
// From Ref. [6], easy halter with MACRO_NBIT=3.
const RuleTable mabu90_7("B1L A1L  C1R B1R  F0R D1R  A1L E0R  A0L C1R E1L H1L");
// From Ref. [6], non-shrunking pattern.
const RuleTable mabu90_8("B1L A1R  C0R B1L  H1L A1R");
