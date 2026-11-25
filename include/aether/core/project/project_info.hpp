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

namespace aether {

// Package identity. Editorial rows live in the database, not in this record's manifest role.
struct ProjectInfo {
  std::string projectId;
  std::string name;
  int formatVersion = 1;
  std::string databaseFilename = "project.db";
  int schemaVersion = 1;
  std::string createdUtc;
  std::string modifiedUtc;
  std::string prototypeVersion;
  std::string appVersion;
  std::string directory;
};

inline constexpr int kSchemaVersionSlice0A = 1;
inline constexpr int kPrototypeSchemaVersion = 2;
inline constexpr int kPrototypeFormatVersion = 1;
inline const char* kPrototypeVersion = "slice-0a";
inline const char* kPrototypeAppVersion = "0.0.0-slice-0a";
inline const char* kDatabaseFilename = "project.db";
inline const char* kManifestFilename = "project.manifest.json";

}  // namespace aether
