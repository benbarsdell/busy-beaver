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

#include <type_traits>

namespace detail {

// A compile-time bitmask of NBIT bits, stored in a value of type T.
template <int NBIT, typename T>
struct BitMask {
  static constexpr const T value =
      NBIT == 0 ? 0 : (T(1) << (NBIT - 1)) | ((T(1) << (NBIT - 1)) - 1);
};

// Returns the i'th NBIT-size subword within data.
template <int NBIT, typename T>
inline T get_subword(T const& data, int i) {
  static_assert(std::is_unsigned<T>::value, "T must be unsigned");
  constexpr const T MASK = BitMask<NBIT, T>::value;
  return (data >> (i * NBIT)) & MASK;
}

// Sets the i'th NBIT-size subword within *data to value.
template <int NBIT, typename T>
inline void set_subword(T* data, int i, T value) {
  constexpr const T MASK = BitMask<NBIT, T>::value;
  *data &= ~(MASK << (i * NBIT));
  *data |= value << (i * NBIT);
}

}  // namespace detail

// Forward declaration.
template <int NBIT, typename T>
class NBitArray;

// A proxy object representing a reference to an element in an NBitArray.
template <int NBIT, typename T>
class NBitArrayItemRef {
  template <int, typename>
  friend class NBitArray;
  NBitArrayItemRef(T& data, int bit) : data_(data), bit_(bit) {}

 public:
  NBitArrayItemRef& operator=(T value) {
    detail::set_subword<NBIT>(&data_, bit_, value);
    return *this;
  }
  operator T() const { return detail::get_subword<NBIT>(data_, bit_); }
  operator bool() const { return static_cast<bool>(this->operator T()); }

 private:
  T& data_;
  int bit_;
};

// An small fixed-size array, stored as a single instance of type T, where each
// element is an integer of NBIT bits.
template <int NBIT, typename T>
class __attribute__((aligned(sizeof(T)))) NBitArray {
 public:
  typedef T value_type;
  typedef NBitArrayItemRef<NBIT, T> reference;
  NBitArray() : data_() {}
  NBitArray(T value) : data_(value) {}
  T operator[](int i) const { return reference(data_, i); }
  reference operator[](int i) { return reference(data_, i); }
  operator value_type const&() const { return data_; }
  operator value_type&() { return data_; }

 private:
  T data_;
};
