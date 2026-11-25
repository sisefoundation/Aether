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

#include "aether/core/project/ids.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <random>

namespace aether {
namespace {

char hexDigit(unsigned int value) {
  return static_cast<char>(value < 10 ? '0' + value : 'a' + (value - 10));
}

}  // namespace

std::optional<std::string> makeStableId() {
  std::array<unsigned char, 16> bytes{};
  try {
    std::random_device device;
    for (std::size_t i = 0; i < bytes.size();) {
      const unsigned int value = device();
      const std::size_t remaining = bytes.size() - i;
      const std::size_t chunk = remaining < 4 ? remaining : 4;
      for (std::size_t b = 0; b < chunk; ++b) {
        bytes[i + b] = static_cast<unsigned char>((value >> (8 * b)) & 0xffu);
      }
      i += chunk;
    }
  } catch (...) {
    return std::nullopt;
  }

  bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0fu) | 0x40u);
  bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3fu) | 0x80u);

  std::string text;
  text.resize(36);
  std::size_t out = 0;
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      text[out++] = '-';
    }
    text[out++] = hexDigit(bytes[i] >> 4);
    text[out++] = hexDigit(bytes[i] & 0x0fu);
  }
  return text;
}

std::string utcTimestamp() {
  const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm parts{};
  if (gmtime_r(&now, &parts) == nullptr) {
    return "1970-01-01T00:00:00Z";
  }
  char buffer[32];
  if (std::strftime(buffer, sizeof buffer, "%Y-%m-%dT%H:%M:%SZ", &parts) == 0) {
    return "1970-01-01T00:00:00Z";
  }
  return buffer;
}

bool validDisplayName(const std::string& name) {
  if (name.empty() || name.size() > 200) {
    return false;
  }
  if (name == "." || name == "..") {
    return false;
  }
  for (unsigned char c : name) {
    if (c < 0x20 || c == 0x7f || c == '/' || c == '\\') {
      return false;
    }
  }
  return true;
}

}  // namespace aether
