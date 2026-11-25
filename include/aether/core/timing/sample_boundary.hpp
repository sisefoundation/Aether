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

#pragma once

// Prototype-only timing helper.
// ADR-002 (rational time as the stored authority) is ACCEPTED — INITIAL EDITORIAL TIME AUTHORITY.
// This function stores nothing. It is the span form already stated in
// docs/TIMEBASE_AND_FRAME_ACCURACY.md. It does not call FFmpeg and it does not
// change the open 30000/1001 MXF encoder failure.

#include <cstdint>

namespace aether {

struct FrameRate {
  std::int64_t numerator = 0;
  std::int64_t denominator = 0;
};

enum class TimingError {
  Ok = 0,
  NegativePosition,
  InvalidRate,
  InvalidSpan,
  Overflow,
};

struct TimingResult {
  TimingError error = TimingError::Ok;
  std::int64_t value = 0;

  bool ok() const { return error == TimingError::Ok; }
};

// sample_boundary(n) = floor(n * sample_rate * fps_denominator / fps_numerator)
// n >= 0. Exact integer arithmetic. No floating point. Fails closed on overflow.
TimingResult sampleBoundary(std::int64_t n, std::int64_t sampleRate, FrameRate rate);

// sample_count = sample_boundary(end) - sample_boundary(start), end >= start, both >= 0.
TimingResult sampleCount(std::int64_t start, std::int64_t end, std::int64_t sampleRate, FrameRate rate);

// Source-frame offset for sequence offset k. Nearest valid timestamp.
// An exact tie stays on the earlier frame. No floating point. Fails closed.
TimingResult pictureSourceOffset(std::int64_t sequenceOffset, FrameRate sequenceRate, FrameRate sourceRate);

}  // namespace aether
