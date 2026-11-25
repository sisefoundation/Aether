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

#include "aether/persistence/logger.hpp"

#include <fstream>
#include <iostream>

namespace aether {
namespace {

const char* levelName(LogLevel level) {
  return level == LogLevel::Error ? "error" : "info";
}

}  // namespace

void FileLogger::setLogDirectory(const std::string& directory) { directory_ = directory; }

void FileLogger::write(LogLevel level, std::string_view category, std::string_view message) {
  const std::string line = std::string(levelName(level)) + " " + std::string(category) + " " +
                           std::string(message);
  std::cerr << line << '\n';
  if (directory_.empty()) {
    return;
  }
  const std::string path = directory_ + "/aether.log";
  std::ofstream out(path, std::ios::app);
  if (!out) {
    return;
  }
  out << line << '\n';
}

void MemoryLogger::write(LogLevel level, std::string_view category, std::string_view message) {
  lines_.push_back(std::string(levelName(level)) + " " + std::string(category) + " " +
                   std::string(message));
}

bool MemoryLogger::contains(std::string_view text) const {
  for (const std::string& line : lines_) {
    if (line.find(text) != std::string::npos) {
      return true;
    }
  }
  return false;
}

}  // namespace aether
