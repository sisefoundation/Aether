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

// Persistence for Slice 0A. SQLite stays in this target.
// Domain headers do not include this file. The UI does not include this file.

#include "aether/core/bin/bin.hpp"
#include "aether/core/media/import.hpp"
#include "aether/core/media/media.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/core/project/result.hpp"
#include "aether/persistence/logger.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace aether {

class ProjectStore {
 public:
  explicit ProjectStore(Logger& logger);
  ~ProjectStore();

  ProjectStore(const ProjectStore&) = delete;
  ProjectStore& operator=(const ProjectStore&) = delete;

  Result<ProjectInfo> create(const std::filesystem::path& directory, const std::string& name);
  Result<ProjectInfo> open(const std::filesystem::path& directory);
  Result<void> close();
  bool isOpen() const;
  const ProjectInfo& info() const;

  Result<BinRecord> insertBin(const std::string& name);
  Result<std::vector<BinRecord>> listBins();
  Result<BinRecord> readBin(const std::string& binId);
  Result<void> renameBin(const std::string& binId, const std::string& name);

  Result<MediaAsset> insertAsset(const std::string& mediaType, const std::string& metadata);
  Result<MediaAsset> readAsset(const std::string& mediaId);
  Result<Representation> insertRepresentation(const std::string& mediaId,
                                              RepresentationRole role,
                                              const std::string& recordedPath,
                                              const std::string& status);
  Result<void> updateRepresentationPath(const std::string& representationId, const std::string& recordedPath);
  Result<std::vector<Representation>> listRepresentations(const std::string& mediaId);
  Result<std::vector<RegisteredReference>> importOriginalReferences(
      const std::string& binId,
      const std::vector<ImportCheck>& files);

  Result<MasterClip> insertMasterClip(const std::string& name,
                                      const std::string& mediaId,
                                      const std::string& binId,
                                      const std::string& metadata);
  Result<MasterClip> readMasterClip(const std::string& objectId);
  Result<std::vector<MasterClip>> listMasterClips(const std::string& binId);
  Result<void> addMembership(const std::string& binId, const std::string& objectId);
  Result<void> removeMembership(const std::string& binId, const std::string& objectId);
  Result<void> moveMembership(const std::string& objectId,
                              const std::string& fromBinId,
                              const std::string& toBinId);
  Result<MasterClip> duplicateMasterClip(const std::string& objectId, const std::string& binId);
  Result<void> setObjectMetadata(const std::string& objectId,
                                 const std::string& name,
                                 const std::string& metadata);

  Result<SequenceRecord> insertSequence(const std::string& name, FrameRate rate);
  Result<SequenceRecord> reinsertSequence(const SequenceRecord& sequence);
  Result<SequenceRecord> readSequence(const std::string& sequenceId);
  Result<std::vector<SequenceRecord>> listSequences();
  Result<void> renameSequence(const std::string& sequenceId, const std::string& name);
  Result<void> deleteSequenceIfEmpty(const std::string& sequenceId);

  Result<TrackRecord> insertTrack(const std::string& sequenceId);
  Result<TrackRecord> reinsertTrack(const TrackRecord& track);
  Result<std::vector<TrackRecord>> listTracks(const std::string& sequenceId);
  Result<void> moveTrack(const std::string& trackId, int ordinal);
  Result<void> deleteTrackIfEmpty(const std::string& trackId);

  Result<TimelineInstance> insertInstance(const TimelineInstance& requested);
  Result<TimelineInstance> readInstance(const std::string& instanceId);
  Result<std::vector<TimelineInstance>> listInstances(const std::string& trackId);
  Result<void> moveInstance(const std::string& instanceId, std::int64_t recordStart);
  Result<void> trimInstance(const std::string& instanceId,
                            std::int64_t sourceStart,
                            std::int64_t sourceEnd,
                            std::int64_t recordStart,
                            std::int64_t recordEnd);
  Result<void> deleteInstance(const std::string& instanceId);
  Result<TimelineInstance> reinsertInstance(const TimelineInstance& instance);

  // One transaction. The final rows on trackId must not overlap. A failure
  // rolls back and leaves the previous commit.
  struct InstanceEditResult {
    std::vector<TimelineInstance> updated;
    std::vector<TimelineInstance> inserted;
  };
  Result<InstanceEditResult> commitInstanceEdits(const std::string& trackId,
                                                 const std::vector<std::string>& deleteIds,
                                                 const std::vector<TimelineInstance>& updates,
                                                 const std::vector<TimelineInstance>& inserts);

  // One transaction for every named track. A failure rolls the whole set back.
  struct InstanceTrackEdit {
    std::string trackId;
    std::vector<std::string> deleteIds;
    std::vector<TimelineInstance> updates;
    std::vector<TimelineInstance> inserts;
  };
  Result<InstanceEditResult> commitMultiTrackEdits(const std::vector<InstanceTrackEdit>& edits);

  // Fsync the committed database and rewrite the manifest. Does not touch undo.
  Result<void> autosave();

  Result<std::vector<std::string>> listUserTables();

  // Prototype test seams. Not used by the window.
  Result<void> debugExec(const std::string& sql);
  Result<int> debugReadSchemaVersion(const std::filesystem::path& directory);
  Result<void> debugDowngradeEmptySequenceSchema();
  void debugArmMigrationFailure();
  void debugArmNextMutationFailure();
  Result<void> debugProveRollback(const std::string& binId);
  Result<void> abandonUncommittedInsert();
  [[noreturn]] void crashMidTransactionForTest();

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace aether
