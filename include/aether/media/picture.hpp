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

// Display pictures from a local file. This header has no decoder types.
// A media index is a decoded picture count. It is not a sequence frame,
// not a source rate, and it is not written to a project.

#include <cstdint>
#include <string>
#include <vector>

namespace aether {

struct DisplayPicture {
  bool present = false;
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgb;
  std::int64_t mediaIndex = -1;
  std::string note;
};

class PictureReader {
 public:
  PictureReader();
  ~PictureReader();
  PictureReader(PictureReader&& other) noexcept;
  PictureReader& operator=(PictureReader&& other) noexcept;
  PictureReader(const PictureReader&) = delete;
  PictureReader& operator=(const PictureReader&) = delete;

  bool open(const std::string& path, std::string* note);
  void close();
  bool isOpen() const;

  // Spacing for a display timer only. Not an editorial rate and not stored.
  // On success, note names the container field that supplied it.
  bool reportedDisplayIntervalUs(std::int64_t* microseconds, std::string* note) const;

  DisplayPicture pictureAt(std::int64_t index);

 private:
  struct State;
  State* state_ = nullptr;
};

}  // namespace aether
