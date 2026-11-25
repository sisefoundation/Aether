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

// Prototype media records. A path is a location hint, not an id.
// File existence is not required and this slice does not create media files.

struct MediaAsset {
  std::string mediaId;
  std::string projectId;
  std::string mediaType;
  std::string metadata;
  std::string createdUtc;
  std::string modifiedUtc;
};

enum class RepresentationRole {
  Original,
  Proxy,
  Transcode,
  Consolidated,
};

inline const char* representationRoleName(RepresentationRole role) {
  switch (role) {
    case RepresentationRole::Original:
      return "original";
    case RepresentationRole::Proxy:
      return "proxy";
    case RepresentationRole::Transcode:
      return "transcode";
    case RepresentationRole::Consolidated:
      return "consolidated";
  }
  return "";
}

inline bool representationRoleFromName(const std::string& name, RepresentationRole& role) {
  if (name == "original") {
    role = RepresentationRole::Original;
    return true;
  }
  if (name == "proxy") {
    role = RepresentationRole::Proxy;
    return true;
  }
  if (name == "transcode") {
    role = RepresentationRole::Transcode;
    return true;
  }
  if (name == "consolidated") {
    role = RepresentationRole::Consolidated;
    return true;
  }
  return false;
}

struct Representation {
  std::string representationId;
  std::string mediaId;
  RepresentationRole role = RepresentationRole::Original;
  std::string recordedPath;
  std::string status;
  std::string createdUtc;
  std::string modifiedUtc;
};

struct MasterClip {
  std::string objectId;
  std::string projectId;
  std::string name;
  std::string mediaId;
  std::string metadata;
  std::string createdUtc;
  std::string modifiedUtc;
};

}  // namespace aether
