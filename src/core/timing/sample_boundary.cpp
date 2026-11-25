// Copyright 2026 Society of Indian Screen Editors Foundation
//
// AETHER
// Architect: Subhajit Singha, SISE
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "aether/core/timing/sample_boundary.hpp"

#include <cstdint>

namespace aether {
namespace {

#if !defined(__SIZEOF_INT128__)
#error "Slice 0A prototype timing needs a 128-bit unsigned integer so products cannot wrap."
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

bool multiply(unsigned __int128 left, unsigned __int128 right, unsigned __int128& out) {
  if (left == 0 || right == 0) {
    out = 0;
    return true;
  }
  if (left > (~static_cast<unsigned __int128>(0)) / right) {
    return false;
  }
  out = left * right;
  return true;
}

}  // namespace

TimingResult sampleBoundary(std::int64_t n, std::int64_t sampleRate, FrameRate rate) {
  if (n < 0) {
    return {TimingError::NegativePosition, 0};
  }
  if (sampleRate <= 0 || rate.numerator <= 0 || rate.denominator <= 0) {
    return {TimingError::InvalidRate, 0};
  }

  unsigned __int128 product = 0;
  if (!multiply(static_cast<unsigned __int128>(n),
                static_cast<unsigned __int128>(sampleRate),
                product)) {
    return {TimingError::Overflow, 0};
  }
  unsigned __int128 full = 0;
  if (!multiply(product, static_cast<unsigned __int128>(rate.denominator), full)) {
    return {TimingError::Overflow, 0};
  }

  const unsigned __int128 quotient = full / static_cast<unsigned __int128>(rate.numerator);
  if (quotient > static_cast<unsigned __int128>(INT64_MAX)) {
    return {TimingError::Overflow, 0};
  }
  return {TimingError::Ok, static_cast<std::int64_t>(quotient)};
}

TimingResult sampleCount(std::int64_t start, std::int64_t end, std::int64_t sampleRate, FrameRate rate) {
  if (start < 0 || end < 0) {
    return {TimingError::NegativePosition, 0};
  }
  if (end < start) {
    return {TimingError::InvalidSpan, 0};
  }
  const TimingResult left = sampleBoundary(start, sampleRate, rate);
  if (!left.ok()) {
    return left;
  }
  const TimingResult right = sampleBoundary(end, sampleRate, rate);
  if (!right.ok()) {
    return right;
  }
  return {TimingError::Ok, right.value - left.value};
}

TimingResult pictureSourceOffset(std::int64_t sequenceOffset, FrameRate sequenceRate, FrameRate sourceRate) {
  if (sequenceOffset < 0) {
    return {TimingError::NegativePosition, 0};
  }
  if (sequenceRate.numerator <= 0 || sequenceRate.denominator <= 0 || sourceRate.numerator <= 0 ||
      sourceRate.denominator <= 0) {
    return {TimingError::InvalidRate, 0};
  }
  // numerator = k * Nr * Ds; denominator = Dr * Ns. Tie stays on the earlier frame.
  unsigned __int128 step = 0;
  if (!multiply(static_cast<unsigned __int128>(sequenceOffset),
                static_cast<unsigned __int128>(sourceRate.numerator),
                step)) {
    return {TimingError::Overflow, 0};
  }
  unsigned __int128 numerator = 0;
  if (!multiply(step, static_cast<unsigned __int128>(sequenceRate.denominator), numerator)) {
    return {TimingError::Overflow, 0};
  }
  unsigned __int128 denominator = 0;
  if (!multiply(static_cast<unsigned __int128>(sourceRate.denominator),
                static_cast<unsigned __int128>(sequenceRate.numerator),
                denominator) ||
      denominator == 0) {
    return {TimingError::Overflow, 0};
  }
  const unsigned __int128 quotient = numerator / denominator;
  const unsigned __int128 remainder = numerator % denominator;
  unsigned __int128 selected = quotient;
  if (remainder != 0) {
    unsigned __int128 twice = 0;
    if (!multiply(remainder, 2, twice)) {
      return {TimingError::Overflow, 0};
    }
    if (twice > denominator) {
      selected = quotient + 1;
    }
  }
  if (selected > static_cast<unsigned __int128>(INT64_MAX)) {
    return {TimingError::Overflow, 0};
  }
  return {TimingError::Ok, static_cast<std::int64_t>(selected)};
}

#pragma GCC diagnostic pop

}  // namespace aether
