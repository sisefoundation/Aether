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

#include "aether/core/project/project_info.hpp"
#include "aether/core/project/result.hpp"

#include <filesystem>
#include <string>

namespace aether {

struct ManifestDocument {
  ProjectInfo info;
};

Result<ManifestDocument> readManifest(const std::filesystem::path& path);
Result<void> writeManifestAtomic(const std::filesystem::path& path, const ProjectInfo& info);

}  // namespace aether
