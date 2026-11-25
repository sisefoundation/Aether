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

// Display counts for the timeline ruler. Nothing here is written to a project.
// A drop-frame flag is not stored, so it is not inferred and the 59.94
// skipped-number table is not used.

#include "aether/core/sequence/sequence.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace aether {

// The visible timeline uses these paint constants. They are not stored.
inline constexpr int kTimelineOriginPx = 48;
inline constexpr int kTimelinePixelsPerFrame = 4;

struct CanvasFrameHit {
  bool ok = false;
  std::int64_t frame = 0;
  const char* reason = "";
};

// x = origin + frame * pixelsPerFrame. This is the painter's conversion.
// A frame that does not fit in a widget coordinate is not given a pixel.
inline std::optional<int> timelinePaintX(std::int64_t frame, int pixelsPerFrame, int origin) {
  if (pixelsPerFrame <= 0 || origin < 0 || frame < 0) {
    return std::nullopt;
  }
  const std::int64_t room = static_cast<std::int64_t>(std::numeric_limits<int>::max()) - origin;
  if (frame > room / pixelsPerFrame) {
    return std::nullopt;
  }
  return origin + static_cast<int>(frame) * pixelsPerFrame;
}

// frame = floor((x - origin) / pixelsPerFrame). x < origin is before frame 0.
// An exact boundary is the frame that starts there. No snap and no float.
inline CanvasFrameHit frameAtCanvasX(int x, int pixelsPerFrame, int origin) {
  if (pixelsPerFrame <= 0) {
    return {false, 0, "timeline scale is not positive"};
  }
  if (x < origin) {
    return {false, 0, "the pointer is before frame 0"};
  }
  const std::int64_t delta = static_cast<std::int64_t>(x) - static_cast<std::int64_t>(origin);
  return {true, delta / static_cast<std::int64_t>(pixelsPerFrame), ""};
}

// Frames numbered inside one label second. This is not a float duration.
inline std::optional<std::int64_t> labelFramesPerSecond(FrameRate rate) {
  if (!isAcceptedFrameRate(rate)) {
    return std::nullopt;
  }
  if (rate.denominator == 1) {
    return rate.numerator;
  }
  if (rate.numerator == 24000) {
    return 24;
  }
  if (rate.numerator == 30000) {
    return 30;
  }
  if (rate.numerator == 60000) {
    return 60;
  }
  return std::nullopt;
}

inline std::string formatFrameLabel(std::int64_t frame, FrameRate rate) {
  if (frame < 0) {
    return {};
  }
  const std::optional<std::int64_t> fps = labelFramesPerSecond(rate);
  if (!fps || *fps <= 0) {
    return "frame " + std::to_string(frame);
  }
  const std::int64_t frames = frame % *fps;
  const std::int64_t totalSeconds = frame / *fps;
  const std::int64_t seconds = totalSeconds % 60;
  const std::int64_t minutes = (totalSeconds / 60) % 60;
  const std::int64_t hours = totalSeconds / 3600;
  auto two = [](std::int64_t value) {
    const std::string text = std::to_string(value);
    return text.size() >= 2 ? text : std::string(2 - text.size(), '0') + text;
  };
  return two(hours) + ":" + two(minutes) + ":" + two(seconds) + ":" + two(frames);
}

struct IntegerFrameParse {
  bool ok = false;
  std::int64_t frame = 0;
  const char* reason = "";
};

// Digits only. A decimal point, a sign, and an empty token are rejected.
inline IntegerFrameParse parseIntegerFrame(std::string_view text) {
  if (text.empty()) {
    return {false, 0, "a frame must be a non-negative integer"};
  }
  for (const char character : text) {
    if (character < '0' || character > '9') {
      return {false, 0, "a frame must be a non-negative integer"};
    }
  }
  std::int64_t value = 0;
  const char* const begin = text.data();
  const char* const end = begin + text.size();
  const std::from_chars_result parsed = std::from_chars(begin, end, value);
  if (parsed.ec != std::errc{} || parsed.ptr != end || value < 0) {
    return {false, 0, "a frame must be a non-negative integer"};
  }
  return {true, value, ""};
}

}  // namespace aether
