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

// Prototype application façade for Slice 0A.
// ADR-006 is PROPOSED and is not accepted here. This type is the service the
// window calls. It does not join decoders, playback, or exchange.

#include "aether/core/bin/bin.hpp"
#include "aether/core/media/import.hpp"
#include "aether/core/media/media.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/project/result.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/persistence/logger.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace aether {

class ProjectStore;

// Two frame points in one domain. Not a half-open edit range.
struct MarkPoints {
  std::optional<std::int64_t> markIn;
  std::optional<std::int64_t> markOut;
};

class ProjectSession {
 public:
  explicit ProjectSession(Logger& logger);
  ~ProjectSession();

  ProjectSession(const ProjectSession&) = delete;
  ProjectSession& operator=(const ProjectSession&) = delete;

  Result<ProjectInfo> createProject(const std::filesystem::path& directory, const std::string& name);
  Result<ProjectInfo> openProject(const std::filesystem::path& directory);
  Result<void> closeProject();
  bool isOpen() const;
  const ProjectInfo& info() const;

  Result<BinRecord> createBin(const std::string& name);
  Result<std::vector<BinRecord>> listBins();
  Result<void> renameBin(const std::string& binId, const std::string& name);
  Result<void> selectBin(const std::string& binId);
  const std::string& selectedBinId() const;

  Result<MediaAsset> createAsset(const std::string& mediaType, const std::string& metadata);
  Result<MediaAsset> readAsset(const std::string& mediaId);
  Result<Representation> addRepresentation(const std::string& mediaId,
                                           RepresentationRole role,
                                           const std::string& recordedPath,
                                           const std::string& status);
  Result<void> setRepresentationPath(const std::string& representationId, const std::string& recordedPath);
  Result<std::vector<Representation>> listRepresentations(const std::string& mediaId);
  Result<BinRecord> ensureRootBin();
  Result<std::vector<RegisteredReference>> importReferences(const std::vector<std::string>& paths);
  Result<std::vector<BinAssetFact>> listBinAssets(const std::string& binId);

  Result<MasterClip> createMasterClip(const std::string& name,
                                      const std::string& mediaId,
                                      const std::string& binId,
                                      const std::string& metadata);
  Result<MasterClip> readMasterClip(const std::string& objectId);
  Result<std::vector<MasterClip>> listClips(const std::string& binId);
  Result<void> addMembership(const std::string& binId, const std::string& objectId);
  Result<void> removeMembership(const std::string& binId, const std::string& objectId);
  Result<void> moveMembership(const std::string& objectId,
                              const std::string& fromBinId,
                              const std::string& toBinId);
  Result<void> referenceObject(const std::string& objectId, const std::string& binId);
  Result<MasterClip> duplicateMasterClip(const std::string& objectId, const std::string& binId);
  Result<void> setObjectMetadata(const std::string& objectId,
                                 const std::string& name,
                                 const std::string& metadata);

  Result<SequenceRecord> createSequence(const std::string& name, FrameRate rate);
  Result<SequenceRecord> readSequence(const std::string& sequenceId);
  Result<std::vector<SequenceRecord>> listSequences();
  Result<void> renameSequence(const std::string& sequenceId, const std::string& name);
  Result<TrackRecord> createTrack(const std::string& sequenceId);
  Result<std::vector<TrackRecord>> listTracks(const std::string& sequenceId);
  Result<void> orderTrack(const std::string& trackId, int ordinal);
  Result<TimelineInstance> insertInstance(const std::string& sequenceId,
                                          const std::string& trackId,
                                          const std::string& masterObjectId,
                                          FrameRate sourceRate,
                                          std::int64_t sourceStart,
                                          std::int64_t sourceEnd,
                                          std::int64_t recordStart,
                                          std::int64_t recordEnd);
  // Same insert as insertInstance. An empty trackId creates one track first.
  // Failure leaves no new track and no undo entry. Success is one undo.
  Result<TimelineInstance> placeInstance(const std::string& sequenceId,
                                         const std::string& trackId,
                                         const std::string& masterObjectId,
                                         FrameRate sourceRate,
                                         std::int64_t sourceStart,
                                         std::int64_t sourceEnd,
                                         std::int64_t recordStart,
                                         std::int64_t recordEnd);
  // Same-rate overwrite. Reads session marks and derives [In, Out + 1).
  // Does not store a range, does not create a track, and is not placeInstance.
  // Success is one transaction and one undo entry. Failure writes nothing.
  Result<TimelineInstance> overwrite(const std::string& sequenceId,
                                     const std::string& trackId,
                                     const std::string& masterObjectId,
                                     const std::string& representationId,
                                     FrameRate sourceRate);
  // One selected instance and one integer sequence frame. Not move, trim, or overwrite.
  // Success is one transaction and one undo entry. The returned row starts at the cut.
  // Failure writes nothing and adds no undo entry.
  Result<TimelineInstance> split(const std::string& instanceId, std::int64_t frame);
  // One selected instance. Deletes that row and leaves [recordStart, recordEnd) empty.
  // Not split, extract, overwrite, move, trim, or removeInstance. Does not read marks.
  // Success is one transaction and one undo entry. Failure writes nothing.
  Result<void> lift(const std::string& instanceId);
  // Session-only [recordStart, recordEnd). Not a project.db row and not marks.
  // A non-half-open span is refused and does not replace a range already stored.
  Result<void> setExtractRange(const std::string& sequenceId, std::int64_t recordStart, std::int64_t recordEnd);
  void clearExtractRange();
  bool extractRange(std::string* sequenceId, std::int64_t* recordStart, std::int64_t* recordEnd) const;
  // One command. Closes [recordStart, recordEnd) on the named tracks only.
  // Not a chain of lift, split, move, or trim. Success is one transaction and
  // one undo entry, and clears the session range. Failure writes nothing,
  // adds no undo entry, and keeps the session range. Marks are not read.
  Result<void> extract(const std::string& sequenceId,
                       const std::vector<std::string>& trackIds,
                       std::int64_t recordStart,
                       std::int64_t recordEnd);
  Result<void> moveInstance(const std::string& instanceId, std::int64_t recordStart);
  Result<void> trimInstance(const std::string& instanceId,
                            std::int64_t sourceStart,
                            std::int64_t sourceEnd,
                            std::int64_t recordStart,
                            std::int64_t recordEnd);
  Result<void> removeInstance(const std::string& instanceId);
  Result<TimelineInstance> readInstance(const std::string& instanceId);
  Result<std::vector<TimelineInstance>> listInstances(const std::string& trackId);

  Result<void> undo();
  Result<void> redo();
  bool canUndo() const;
  bool canRedo() const;

  // Session context only. Not a project.db row and not an undo entry.
  // A stored pair is two frame points. It is not converted to [start, end).
  Result<MarkPoints> sourceMarks(const std::string& representationId);
  Result<void> setSourceMarkIn(const std::string& representationId, std::int64_t frame);
  Result<void> setSourceMarkOut(const std::string& representationId, std::int64_t frame);
  Result<void> clearSourceMarkIn(const std::string& representationId);
  Result<void> clearSourceMarkOut(const std::string& representationId);
  Result<void> clearSourceMarks(const std::string& representationId);
  Result<MarkPoints> recordMarks(const std::string& sequenceId);
  Result<void> setRecordMarkIn(const std::string& sequenceId, std::int64_t frame);
  Result<void> setRecordMarkOut(const std::string& sequenceId, std::int64_t frame);
  Result<void> clearRecordMarkIn(const std::string& sequenceId);
  Result<void> clearRecordMarkOut(const std::string& sequenceId);
  Result<void> clearRecordMarks(const std::string& sequenceId);

  Result<void> autosave();

  ProjectStore& storeForTest();
  // Test seam. The next new extract fragment uses this id and then the seam clears.
  // Not used by the window.
  void debugArmExtractIdentityCollision(const std::string& instanceId);

 private:
  enum class UndoKind {
    RenameBin,
    AddMembership,
    SetMetadata,
    RenameSequence,
    CreateSequence,
    CreateTrack,
    OrderTrack,
    InsertInstance,
    MoveInstance,
    TrimInstance,
    RemoveInstance,
    Overwrite,
    Split,
    Lift,
    Extract
  };

  struct UndoEntry {
    UndoKind kind = UndoKind::RenameBin;
    std::string binId;
    std::string objectId;
    std::string beforeName;
    std::string afterName;
    std::string beforeMeta;
    std::string afterMeta;
    std::string sequenceId;
    std::string trackId;
    int beforeOrdinal = 0;
    int afterOrdinal = 0;
    std::int64_t beforeSourceStart = 0;
    std::int64_t beforeSourceEnd = 0;
    std::int64_t afterSourceStart = 0;
    std::int64_t afterSourceEnd = 0;
    std::int64_t beforeRecordStart = 0;
    std::int64_t beforeRecordEnd = 0;
    std::int64_t afterRecordStart = 0;
    std::int64_t afterRecordEnd = 0;
    TimelineInstance instance;
    std::vector<TimelineInstance> beforeInstances;
    std::vector<TimelineInstance> afterInstances;
  };

  void resetSessionState();
  Result<void> apply(const UndoEntry& entry, bool undoing);
  bool representationInProject(const std::string& representationId);
  bool sequenceInProject(const std::string& sequenceId);
  Result<MarkPoints> readPoints(const std::map<std::string, MarkPoints>& table, const std::string& id, bool owner,
                                const char* missing) const;
  Result<void> storePoint(std::map<std::string, MarkPoints>& table, const std::string& id, bool owner, bool isIn,
                          std::int64_t frame, const char* missing);
  Result<void> clearPoints(std::map<std::string, MarkPoints>& table, const std::string& id, bool owner, bool clearIn,
                           bool clearOut, const char* missing);

  Logger& logger_;
  std::unique_ptr<ProjectStore> store_;
  std::vector<UndoEntry> undo_;
  std::vector<UndoEntry> redo_;
  std::map<std::string, MarkPoints> sourceMarks_;
  std::map<std::string, MarkPoints> recordMarks_;
  std::string selectedBinId_;
  bool extractRangeSet_ = false;
  std::string extractRangeSequenceId_;
  std::int64_t extractRangeStart_ = 0;
  std::int64_t extractRangeEnd_ = 0;
  std::string armedExtractId_;
  ProjectInfo empty_;
};

}  // namespace aether
