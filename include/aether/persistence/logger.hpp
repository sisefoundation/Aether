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

#include <string>
#include <string_view>
#include <vector>

namespace aether {

enum class LogLevel { Info, Error };

// Startup, shutdown, project, database, transaction, schema, manifest, autosave, recovery.
// Messages name operations and ids. They do not include representation paths.
class Logger {
 public:
  virtual ~Logger() = default;
  virtual void write(LogLevel level, std::string_view category, std::string_view message) = 0;
  virtual void setLogDirectory(const std::string&) {}

  void info(std::string_view category, std::string_view message) {
    write(LogLevel::Info, category, message);
  }
  void error(std::string_view category, std::string_view message) {
    write(LogLevel::Error, category, message);
  }
};

class FileLogger final : public Logger {
 public:
  void write(LogLevel level, std::string_view category, std::string_view message) override;
  void setLogDirectory(const std::string& directory) override;

 private:
  std::string directory_;
};

class MemoryLogger final : public Logger {
 public:
  void write(LogLevel level, std::string_view category, std::string_view message) override;
  const std::vector<std::string>& lines() const { return lines_; }
  bool contains(std::string_view text) const;

 private:
  std::vector<std::string> lines_;
};

}  // namespace aether
