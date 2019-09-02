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

#include <gmpxx.h>

#include <cmath>
#include <iostream>

// Note that any integer-like class will work here (with the exception of the
// below utilities that are specific to GMP).
typedef mpz_class BigNum;

namespace detail {

inline static void summarize_bignum(const BigNum& bn, double* base10_mantissa,
                                    uint32_t* lo_9_digits,
                                    uint32_t* base10_num_digits) {
  BigNum bn_lo_9_digits = bn % 1000000000;
  *lo_9_digits = mpz_get_si(bn_lo_9_digits.get_mpz_t());

  long e;
  double d = mpz_get_d_2exp(&e, bn.get_mpz_t());
  double sign = d < 0 ? -1 : 1;
  double e2 = double(e) + log(fabs(d)) / log(2.);
  double e10 = e2 * (log(2) / log(10));
  double e10_ipart;
  double fpart = modf(e10, &e10_ipart);
  *base10_mantissa = sign * pow(10, fpart);
  *base10_num_digits = (uint32_t)e10_ipart;
}

}  // namespace detail

// Utility wrapper to print large BigNum values in scientific notation.
class ConcisePrintBigNum {
 public:
  ConcisePrintBigNum(const BigNum& bn) : bn_(bn) {}
  friend std::ostream& operator<<(std::ostream& os,
                                  const ConcisePrintBigNum& cbn) {
    const BigNum& bn = cbn.bn_;
    if (mpz_fits_slong_p(bn.get_mpz_t())) {
      os << mpz_get_si(bn.get_mpz_t());
    } else {
      double mantissa;
      uint32_t lo_9_digits, num_digits;
      detail::summarize_bignum(bn, &mantissa, &lo_9_digits, &num_digits);
      unsigned lsbs = lo_9_digits % 1000;
      os << mantissa << ".." << lsbs << "e" << num_digits;
    }
    return os;
  }

 private:
  const BigNum& bn_;
};

// Utility wrapper for comparing BigNum values without the need to store all
// digits (used for implementing tests). This class compares only the highest
// and lowest digits and the total number of digits.
class ConciseCompareBigNum {
 public:
  ConciseCompareBigNum(int32_t hi_9_digits, uint32_t lo_9_digits,
                       uint32_t num_digits)
      : hi9_(hi_9_digits), lo9_(lo_9_digits), numdig_(num_digits) {}

  bool operator==(const BigNum& bn) const {
    double mantissa;
    uint32_t lo_9_digits, num_digits;
    detail::summarize_bignum(bn, &mantissa, &lo_9_digits, &num_digits);
    int32_t hi_9_digits = (int32_t)(mantissa * 100000000);
    return hi_9_digits == hi9_ && lo_9_digits == lo9_ && num_digits == numdig_;
  }

  bool operator!=(const BigNum& bn) const { return !(*this == bn); }

  friend std::ostream& operator<<(std::ostream& os,
                                  const ConciseCompareBigNum& cbn) {
    os << cbn.hi9_ / 100000000 << "." << cbn.hi9_ % 100000000 << ".."
       << cbn.lo9_ << "e" << cbn.numdig_;
    return os;
  }

 private:
  int32_t hi9_;
  uint32_t lo9_;
  uint32_t numdig_;
};

static inline bool operator==(const BigNum& bn,
                              const ConciseCompareBigNum& cbn) {
  return cbn == bn;
}

static inline bool operator!=(const BigNum& bn,
                              const ConciseCompareBigNum& cbn) {
  return cbn != bn;
}
