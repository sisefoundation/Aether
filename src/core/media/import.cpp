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

#include "aether/core/media/import.hpp"

#include "aether/core/project/ids.hpp"

#include <sys/stat.h>

#include <ctime>
#include <fstream>

namespace aether {
namespace {

std::string lowerExtension(const std::filesystem::path& path) {
  std::string extension = path.extension().string();
  for (char& character : extension) {
    if (character >= 'A' && character <= 'Z') {
      character = static_cast<char>(character - 'A' + 'a');
    }
  }
  return extension;
}

const char* mediaTypeForExtension(const std::string& extension) {
  if (extension == ".mov" || extension == ".mp4" || extension == ".mxf" || extension == ".avi" ||
      extension == ".mkv" || extension == ".mpg" || extension == ".mpeg" || extension == ".m4v") {
    return "picture";
  }
  if (extension == ".wav" || extension == ".aif" || extension == ".aiff" || extension == ".mp3" ||
      extension == ".flac" || extension == ".aac" || extension == ".m4a") {
    return "sound";
  }
  return nullptr;
}

}  // namespace

ImportCheck inspectImportPath(const std::filesystem::path& path) {
  ImportCheck checked;
  if (path.empty()) {
    checked.kind = ImportCheckKind::Invalid;
    checked.reason = "path is empty";
    return checked;
  }
  std::error_code error;
  const std::filesystem::path absolute = std::filesystem::absolute(path, error).lexically_normal();
  if (error || absolute.empty()) {
    checked.kind = ImportCheckKind::Invalid;
    checked.recordedPath = path.string();
    checked.reason = "path is not usable";
    return checked;
  }
  checked.recordedPath = absolute.string();
  checked.fileName = absolute.filename().string();
  if (!std::filesystem::exists(absolute, error) || error) {
    checked.kind = ImportCheckKind::Missing;
    checked.reason = "file is missing";
    return checked;
  }
  if (!std::filesystem::is_regular_file(absolute, error) || error) {
    checked.kind = ImportCheckKind::Invalid;
    checked.reason = "path is not a media file";
    return checked;
  }
  std::ifstream input(absolute, std::ios::binary);
  if (!input) {
    checked.kind = ImportCheckKind::Unreadable;
    checked.reason = "file is unreadable";
    return checked;
  }
  const char* mediaType = mediaTypeForExtension(lowerExtension(absolute));
  if (mediaType == nullptr) {
    checked.kind = ImportCheckKind::UnknownType;
    checked.reason = "file type is unknown; technical metadata was not probed";
    return checked;
  }
  if (!validDisplayName(checked.fileName)) {
    checked.kind = ImportCheckKind::Invalid;
    checked.reason = "file name cannot be a bin display name";
    return checked;
  }
  checked.kind = ImportCheckKind::Ok;
  checked.mediaType = mediaType;
  checked.reason = "readable reference";
  return checked;
}

FilePresence inspectRecordedFile(const std::string& recordedPath) {
  FilePresence presence;
  presence.sizeText = "unavailable";
  presence.mtimeUtc = "unavailable";
  struct stat fileStat {};
  if (stat(recordedPath.c_str(), &fileStat) != 0 || !S_ISREG(fileStat.st_mode)) {
    return presence;
  }
  std::ifstream input(recordedPath, std::ios::binary);
  if (!input) {
    return presence;
  }
  presence.online = true;
  presence.sizeText = std::to_string(static_cast<std::uintmax_t>(fileStat.st_size)) + " bytes";
  std::tm utc {};
  if (gmtime_r(&fileStat.st_mtime, &utc) != nullptr) {
    char buffer[32];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc) > 0) {
      presence.mtimeUtc = buffer;
    }
  }
  return presence;
}

}  // namespace aether
