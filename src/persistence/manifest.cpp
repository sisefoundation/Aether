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

#include "aether/persistence/manifest.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <utility>

namespace aether {
namespace {

std::string jsonEscape(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (unsigned char c : text) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c < 0x20) {
          const char* hex = "0123456789abcdef";
          out += "\\u00";
          out += hex[c >> 4];
          out += hex[c & 0x0f];
        } else {
          out += static_cast<char>(c);
        }
        break;
    }
  }
  return out;
}

class Parser {
 public:
  explicit Parser(std::string text) : text_(std::move(text)) {}

  Result<ManifestDocument> parse() {
    skip();
    if (!consume('{')) {
      return fail("manifest must be one JSON object");
    }
    ManifestDocument document;
    bool seenId = false;
    bool seenName = false;
    bool seenFormat = false;
    bool seenDatabase = false;
    bool seenSchema = false;
    bool seenCreated = false;
    bool seenModified = false;
    bool seenPrototype = false;
    bool seenApp = false;
    skip();
    if (peek() == '}') {
      return fail("manifest is missing package fields");
    }
    while (true) {
      skip();
      std::string key;
      if (!parseString(key)) {
        return fail("manifest key must be a string");
      }
      skip();
      if (!consume(':')) {
        return fail("manifest key is missing a colon");
      }
      skip();
      if (key == "project_id" || key == "name" || key == "database_filename" || key == "created" ||
          key == "modified" || key == "prototype_version" || key == "app_version") {
        std::string value;
        if (!parseString(value)) {
          return fail("manifest field " + key + " must be a string");
        }
        if (key == "project_id") {
          document.info.projectId = value;
          seenId = true;
        } else if (key == "name") {
          document.info.name = value;
          seenName = true;
        } else if (key == "database_filename") {
          document.info.databaseFilename = value;
          seenDatabase = true;
        } else if (key == "created") {
          document.info.createdUtc = value;
          seenCreated = true;
        } else if (key == "modified") {
          document.info.modifiedUtc = value;
          seenModified = true;
        } else if (key == "prototype_version") {
          document.info.prototypeVersion = value;
          seenPrototype = true;
        } else {
          document.info.appVersion = value;
          seenApp = true;
        }
      } else if (key == "format_version" || key == "schema_version") {
        std::int64_t number = 0;
        if (!parseInt(number)) {
          return fail("manifest field " + key + " must be an integer");
        }
        if (key == "format_version") {
          document.info.formatVersion = static_cast<int>(number);
          seenFormat = true;
        } else {
          document.info.schemaVersion = static_cast<int>(number);
          seenSchema = true;
        }
      } else {
        return fail("manifest contains an unsupported field");
      }
      skip();
      if (consume('}')) {
        break;
      }
      if (!consume(',')) {
        return fail("manifest object is not closed");
      }
    }
    skip();
    if (index_ != text_.size()) {
      return fail("manifest has trailing data");
    }
    if (!seenId || !seenName || !seenFormat || !seenDatabase || !seenSchema || !seenCreated ||
        !seenModified || !seenPrototype || !seenApp) {
      return fail("manifest is missing a required package field");
    }
    if (document.info.projectId.empty() || document.info.name.empty() ||
        document.info.databaseFilename.empty() || document.info.createdUtc.empty() ||
        document.info.modifiedUtc.empty() || document.info.prototypeVersion.empty() ||
        document.info.appVersion.empty()) {
      return fail("manifest has an empty package field");
    }
    return Result<ManifestDocument>::success(std::move(document));
  }

 private:
  Result<ManifestDocument> fail(const std::string& message) const {
    return Result<ManifestDocument>::failure(ErrorCode::ManifestInvalid, message);
  }

  void skip() {
    while (index_ < text_.size() &&
           std::isspace(static_cast<unsigned char>(text_[index_])) != 0) {
      ++index_;
    }
  }

  char peek() const { return index_ < text_.size() ? text_[index_] : '\0'; }

  bool consume(char expected) {
    if (peek() != expected) {
      return false;
    }
    ++index_;
    return true;
  }

  bool parseString(std::string& out) {
    if (!consume('"')) {
      return false;
    }
    out.clear();
    while (index_ < text_.size()) {
      const char c = text_[index_++];
      if (c == '"') {
        return true;
      }
      if (c == '\\') {
        if (index_ >= text_.size()) {
          return false;
        }
        const char esc = text_[index_++];
        switch (esc) {
          case '"':
          case '\\':
          case '/':
            out += esc;
            break;
          case 'b':
            out += '\b';
            break;
          case 'f':
            out += '\f';
            break;
          case 'n':
            out += '\n';
            break;
          case 'r':
            out += '\r';
            break;
          case 't':
            out += '\t';
            break;
          case 'u':
            if (index_ + 4 > text_.size()) {
              return false;
            }
            index_ += 4;
            out += '?';
            break;
          default:
            return false;
        }
      } else if (static_cast<unsigned char>(c) < 0x20) {
        return false;
      } else {
        out += c;
      }
    }
    return false;
  }

  bool parseInt(std::int64_t& out) {
    if (index_ >= text_.size() || !std::isdigit(static_cast<unsigned char>(text_[index_]))) {
      return false;
    }
    std::int64_t value = 0;
    while (index_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[index_]))) {
      const int digit = text_[index_] - '0';
      if (value > (INT64_MAX - digit) / 10) {
        return false;
      }
      value = value * 10 + digit;
      ++index_;
    }
    out = value;
    return true;
  }

  std::string text_;
  std::size_t index_ = 0;
};

std::string render(const ProjectInfo& info) {
  std::ostringstream out;
  out << "{\n"
      << "  \"project_id\": \"" << jsonEscape(info.projectId) << "\",\n"
      << "  \"name\": \"" << jsonEscape(info.name) << "\",\n"
      << "  \"format_version\": " << info.formatVersion << ",\n"
      << "  \"database_filename\": \"" << jsonEscape(info.databaseFilename) << "\",\n"
      << "  \"schema_version\": " << info.schemaVersion << ",\n"
      << "  \"created\": \"" << jsonEscape(info.createdUtc) << "\",\n"
      << "  \"modified\": \"" << jsonEscape(info.modifiedUtc) << "\",\n"
      << "  \"prototype_version\": \"" << jsonEscape(info.prototypeVersion) << "\",\n"
      << "  \"app_version\": \"" << jsonEscape(info.appVersion) << "\"\n"
      << "}\n";
  return out.str();
}

}  // namespace

Result<ManifestDocument> readManifest(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) {
    return Result<ManifestDocument>::failure(ErrorCode::ManifestInvalid, "manifest could not be read");
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  if (!in && !in.eof()) {
    return Result<ManifestDocument>::failure(ErrorCode::ManifestInvalid, "manifest could not be read");
  }
  Parser parser(buffer.str());
  return parser.parse();
}

Result<void> writeManifestAtomic(const std::filesystem::path& path, const ProjectInfo& info) {
  const std::filesystem::path temporary = path.string() + ".tmp";
  const std::string body = render(info);
  const int fd = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0) {
    return Result<void>::failure(ErrorCode::Io, "manifest temporary file could not be created");
  }
  std::size_t written = 0;
  while (written < body.size()) {
    const ssize_t count = ::write(fd, body.data() + written, body.size() - written);
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      ::close(fd);
      ::unlink(temporary.c_str());
      return Result<void>::failure(ErrorCode::Io, "manifest temporary file could not be written");
    }
    written += static_cast<std::size_t>(count);
  }
  if (::fsync(fd) != 0) {
    ::close(fd);
    ::unlink(temporary.c_str());
    return Result<void>::failure(ErrorCode::Io, "manifest temporary file could not be flushed");
  }
  if (::close(fd) != 0) {
    ::unlink(temporary.c_str());
    return Result<void>::failure(ErrorCode::Io, "manifest temporary file could not be closed");
  }
  if (::rename(temporary.c_str(), path.c_str()) != 0) {
    ::unlink(temporary.c_str());
    return Result<void>::failure(ErrorCode::Io, "manifest could not be replaced");
  }
  const int dirfd = ::open(path.parent_path().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (dirfd >= 0) {
    ::fsync(dirfd);
    ::close(dirfd);
  }
  return Result<void>::success();
}

}  // namespace aether
