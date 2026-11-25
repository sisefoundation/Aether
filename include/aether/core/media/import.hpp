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

// File checks for a reference import. A path is not a Media ID.
// This header does not include SQLite and does not copy a file.

#include <cstdint>
#include <filesystem>
#include <string>

namespace aether {

enum class ImportCheckKind {
  Ok,
  Invalid,
  Missing,
  Unreadable,
  UnknownType,
  Duplicate
};

struct ImportCheck {
  ImportCheckKind kind = ImportCheckKind::Invalid;
  std::string recordedPath;
  std::string fileName;
  std::string mediaType;
  std::string reason;
};

// Absolute, lexically normal path. Does not resolve symlinks and does not read bytes.
ImportCheck inspectImportPath(const std::filesystem::path& path);

// Presence for the inspector. Not stored as identity.
struct FilePresence {
  bool online = false;
  std::string sizeText;
  std::string mtimeUtc;
};

FilePresence inspectRecordedFile(const std::string& recordedPath);

struct RegisteredReference {
  std::string objectId;
  std::string mediaId;
  std::string representationId;
  std::string recordedPath;
  std::string fileName;
  std::string mediaType;
};

struct BinAssetFact {
  std::string objectId;
  std::string name;
  std::string mediaId;
  std::string representationId;
  std::string recordedPath;
  std::string fileName;
  std::string mediaType;
  std::string registrationStatus;
  std::string presence;
  std::string sizeText;
  std::string mtimeUtc;
  std::string technicalMetadata;
};

}  // namespace aether
