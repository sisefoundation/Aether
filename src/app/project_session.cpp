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

#include "aether/app/project_session.hpp"

#include "aether/core/media/import.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/persistence/project_store.hpp"

#include <filesystem>

namespace aether {
namespace {

const TimelineInstance* findInstance(const std::vector<TimelineInstance>& rows, const std::string& instanceId) {
  for (const TimelineInstance& row : rows) {
    if (row.instanceId == instanceId) {
      return &row;
    }
  }
  return nullptr;
}

struct RowEdit {
  std::vector<std::string> deleteIds;
  std::vector<TimelineInstance> updates;
  std::vector<TimelineInstance> inserts;
};

RowEdit editToward(const std::vector<TimelineInstance>& current, const std::vector<TimelineInstance>& target) {
  RowEdit edit;
  for (const TimelineInstance& row : current) {
    const TimelineInstance* next = findInstance(target, row.instanceId);
    if (next == nullptr) {
      edit.deleteIds.push_back(row.instanceId);
    } else {
      edit.updates.push_back(*next);
    }
  }
  for (const TimelineInstance& row : target) {
    if (findInstance(current, row.instanceId) == nullptr) {
      edit.inserts.push_back(row);
    }
  }
  return edit;
}

bool sameStoredSpan(const TimelineInstance& left, const TimelineInstance& right) {
  return left.instanceId == right.instanceId && left.sequenceId == right.sequenceId && left.trackId == right.trackId &&
         left.masterObjectId == right.masterObjectId && left.mediaId == right.mediaId &&
         left.sourceRate.numerator == right.sourceRate.numerator &&
         left.sourceRate.denominator == right.sourceRate.denominator && left.sourceStart == right.sourceStart &&
         left.sourceEnd == right.sourceEnd && left.recordStart == right.recordStart && left.recordEnd == right.recordEnd;
}

std::vector<ProjectStore::InstanceTrackEdit> editsByTrack(const std::vector<TimelineInstance>& current,
                                                         const std::vector<TimelineInstance>& target) {
  std::vector<std::string> tracks;
  auto note = [&](const std::string& trackId) {
    for (const std::string& have : tracks) {
      if (have == trackId) {
        return;
      }
    }
    tracks.push_back(trackId);
  };
  for (const TimelineInstance& row : current) {
    note(row.trackId);
  }
  for (const TimelineInstance& row : target) {
    note(row.trackId);
  }
  std::vector<ProjectStore::InstanceTrackEdit> edits;
  for (const std::string& trackId : tracks) {
    std::vector<TimelineInstance> from;
    std::vector<TimelineInstance> to;
    for (const TimelineInstance& row : current) {
      if (row.trackId == trackId) {
        from.push_back(row);
      }
    }
    for (const TimelineInstance& row : target) {
      if (row.trackId == trackId) {
        to.push_back(row);
      }
    }
    const RowEdit diff = editToward(from, to);
    ProjectStore::InstanceTrackEdit edit;
    edit.trackId = trackId;
    edit.deleteIds = diff.deleteIds;
    for (const TimelineInstance& update : diff.updates) {
      const TimelineInstance* previous = findInstance(from, update.instanceId);
      if (previous == nullptr || !sameStoredSpan(*previous, update)) {
        edit.updates.push_back(update);
      }
    }
    edit.inserts = diff.inserts;
    if (edit.deleteIds.empty() && edit.updates.empty() && edit.inserts.empty()) {
      continue;
    }
    edits.push_back(std::move(edit));
  }
  return edits;
}

}  // namespace

ProjectSession::ProjectSession(Logger& logger) : logger_(logger), store_(std::make_unique<ProjectStore>(logger)) {}

ProjectSession::~ProjectSession() { closeProject(); }

void ProjectSession::resetSessionState() {
  undo_.clear();
  redo_.clear();
  sourceMarks_.clear();
  recordMarks_.clear();
  selectedBinId_.clear();
  extractRangeSet_ = false;
  extractRangeSequenceId_.clear();
  extractRangeStart_ = 0;
  extractRangeEnd_ = 0;
  armedExtractId_.clear();
}

Result<ProjectInfo> ProjectSession::createProject(const std::filesystem::path& directory, const std::string& name) {
  auto created = store_->create(directory, name);
  if (created.ok()) {
    resetSessionState();
  }
  return created;
}

Result<ProjectInfo> ProjectSession::openProject(const std::filesystem::path& directory) {
  auto opened = store_->open(directory);
  if (opened.ok()) {
    resetSessionState();
  }
  return opened;
}

Result<void> ProjectSession::closeProject() {
  resetSessionState();
  return store_->close();
}

bool ProjectSession::isOpen() const { return store_->isOpen(); }

const ProjectInfo& ProjectSession::info() const {
  if (!store_->isOpen()) {
    return empty_;
  }
  return store_->info();
}

Result<BinRecord> ProjectSession::createBin(const std::string& name) { return store_->insertBin(name); }

Result<std::vector<BinRecord>> ProjectSession::listBins() { return store_->listBins(); }

Result<void> ProjectSession::renameBin(const std::string& binId, const std::string& name) {
  auto before = store_->readBin(binId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto renamed = store_->renameBin(binId, name);
  if (!renamed.ok()) {
    return renamed;
  }
  UndoEntry entry;
  entry.kind = UndoKind::RenameBin;
  entry.binId = binId;
  entry.beforeName = before.value.name;
  entry.afterName = name;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<void> ProjectSession::selectBin(const std::string& binId) {
  auto bin = store_->readBin(binId);
  if (!bin.ok()) {
    return Result<void>::failure(bin.error.code, bin.error.message);
  }
  selectedBinId_ = binId;
  return Result<void>::success();
}

const std::string& ProjectSession::selectedBinId() const { return selectedBinId_; }

Result<MediaAsset> ProjectSession::createAsset(const std::string& mediaType, const std::string& metadata) {
  return store_->insertAsset(mediaType, metadata);
}

Result<MediaAsset> ProjectSession::readAsset(const std::string& mediaId) { return store_->readAsset(mediaId); }

Result<Representation> ProjectSession::addRepresentation(const std::string& mediaId,
                                                        RepresentationRole role,
                                                        const std::string& recordedPath,
                                                        const std::string& status) {
  return store_->insertRepresentation(mediaId, role, recordedPath, status);
}

Result<void> ProjectSession::setRepresentationPath(const std::string& representationId,
                                                  const std::string& recordedPath) {
  auto updated = store_->updateRepresentationPath(representationId, recordedPath);
  if (updated.ok()) {
    logger_.info("project", "representation path updated for " + representationId);
  }
  return updated;
}

Result<std::vector<Representation>> ProjectSession::listRepresentations(const std::string& mediaId) {
  return store_->listRepresentations(mediaId);
}

Result<BinRecord> ProjectSession::ensureRootBin() {
  auto bins = store_->listBins();
  if (!bins.ok()) {
    return Result<BinRecord>::failure(bins.error.code, bins.error.message);
  }
  for (const BinRecord& bin : bins.value) {
    if (bin.name == "Root") {
      return Result<BinRecord>::success(bin);
    }
  }
  return store_->insertBin("Root");
}

Result<std::vector<RegisteredReference>> ProjectSession::importReferences(const std::vector<std::string>& paths) {
  if (paths.empty()) {
    return Result<std::vector<RegisteredReference>>::failure(ErrorCode::InvalidArgument, "no files were requested");
  }
  auto root = ensureRootBin();
  if (!root.ok()) {
    return Result<std::vector<RegisteredReference>>::failure(root.error.code, root.error.message);
  }
  std::vector<ImportCheck> accepted;
  accepted.reserve(paths.size());
  for (const std::string& path : paths) {
    ImportCheck checked = inspectImportPath(path);
    if (checked.kind != ImportCheckKind::Ok) {
      const std::string detail = checked.recordedPath.empty() ? checked.reason
                                                              : checked.recordedPath + ": " + checked.reason;
      return Result<std::vector<RegisteredReference>>::failure(
          checked.kind == ImportCheckKind::Missing || checked.kind == ImportCheckKind::Unreadable ? ErrorCode::Io
                                                                                                  : ErrorCode::InvalidArgument,
          detail + "; no records were written");
    }
    for (const ImportCheck& earlier : accepted) {
      if (earlier.recordedPath == checked.recordedPath) {
        return Result<std::vector<RegisteredReference>>::failure(
            ErrorCode::Constraint,
            checked.recordedPath +
                ": exact recorded path is already in this import; no policy merges it or creates another Media ID; "
                "no records were written");
      }
    }
    accepted.push_back(std::move(checked));
  }
  return store_->importOriginalReferences(root.value.binId, accepted);
}

Result<std::vector<BinAssetFact>> ProjectSession::listBinAssets(const std::string& binId) {
  auto clips = store_->listMasterClips(binId);
  if (!clips.ok()) {
    return Result<std::vector<BinAssetFact>>::failure(clips.error.code, clips.error.message);
  }
  std::vector<BinAssetFact> facts;
  for (const MasterClip& clip : clips.value) {
    auto representations = store_->listRepresentations(clip.mediaId);
    if (!representations.ok()) {
      return Result<std::vector<BinAssetFact>>::failure(representations.error.code, representations.error.message);
    }
    if (representations.value.empty()) {
      continue;
    }
    const Representation* chosen = &representations.value.front();
    for (const Representation& representation : representations.value) {
      if (representation.role == RepresentationRole::Original) {
        chosen = &representation;
        break;
      }
    }
    const FilePresence file = inspectRecordedFile(chosen->recordedPath);
    BinAssetFact fact;
    fact.objectId = clip.objectId;
    fact.name = clip.name;
    fact.mediaId = clip.mediaId;
    fact.representationId = chosen->representationId;
    fact.recordedPath = chosen->recordedPath;
    fact.fileName = std::filesystem::path(chosen->recordedPath).filename().string();
    auto asset = store_->readAsset(clip.mediaId);
    fact.mediaType = asset.ok() ? asset.value.mediaType : std::string("unknown");
    fact.registrationStatus = chosen->status;
    fact.presence = file.online ? "online" : "offline";
    fact.sizeText = file.sizeText;
    fact.mtimeUtc = file.mtimeUtc;
    fact.technicalMetadata = "Not probed";
    facts.push_back(std::move(fact));
  }
  return Result<std::vector<BinAssetFact>>::success(std::move(facts));
}

Result<MasterClip> ProjectSession::createMasterClip(const std::string& name,
                                                   const std::string& mediaId,
                                                   const std::string& binId,
                                                   const std::string& metadata) {
  return store_->insertMasterClip(name, mediaId, binId, metadata);
}

Result<MasterClip> ProjectSession::readMasterClip(const std::string& objectId) {
  return store_->readMasterClip(objectId);
}

Result<std::vector<MasterClip>> ProjectSession::listClips(const std::string& binId) {
  return store_->listMasterClips(binId);
}

Result<void> ProjectSession::addMembership(const std::string& binId, const std::string& objectId) {
  auto added = store_->addMembership(binId, objectId);
  if (!added.ok()) {
    return added;
  }
  UndoEntry entry;
  entry.kind = UndoKind::AddMembership;
  entry.binId = binId;
  entry.objectId = objectId;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<void> ProjectSession::removeMembership(const std::string& binId, const std::string& objectId) {
  return store_->removeMembership(binId, objectId);
}

Result<void> ProjectSession::moveMembership(const std::string& objectId,
                                           const std::string& fromBinId,
                                           const std::string& toBinId) {
  return store_->moveMembership(objectId, fromBinId, toBinId);
}

Result<void> ProjectSession::referenceObject(const std::string& objectId, const std::string& binId) {
  return addMembership(binId, objectId);
}

Result<MasterClip> ProjectSession::duplicateMasterClip(const std::string& objectId, const std::string& binId) {
  return store_->duplicateMasterClip(objectId, binId);
}

Result<void> ProjectSession::setObjectMetadata(const std::string& objectId,
                                              const std::string& name,
                                              const std::string& metadata) {
  auto before = store_->readMasterClip(objectId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto updated = store_->setObjectMetadata(objectId, name, metadata);
  if (!updated.ok()) {
    return updated;
  }
  UndoEntry entry;
  entry.kind = UndoKind::SetMetadata;
  entry.objectId = objectId;
  entry.beforeName = before.value.name;
  entry.afterName = name;
  entry.beforeMeta = before.value.metadata;
  entry.afterMeta = metadata.empty() ? "{}" : metadata;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<void> ProjectSession::apply(const UndoEntry& entry, bool undoing) {
  switch (entry.kind) {
    case UndoKind::RenameBin:
      return store_->renameBin(entry.binId, undoing ? entry.beforeName : entry.afterName);
    case UndoKind::AddMembership:
      if (undoing) {
        return store_->removeMembership(entry.binId, entry.objectId);
      }
      return store_->addMembership(entry.binId, entry.objectId);
    case UndoKind::SetMetadata:
      return store_->setObjectMetadata(entry.objectId,
                                      undoing ? entry.beforeName : entry.afterName,
                                      undoing ? entry.beforeMeta : entry.afterMeta);
    case UndoKind::RenameSequence:
      return store_->renameSequence(entry.sequenceId, undoing ? entry.beforeName : entry.afterName);
    case UndoKind::CreateSequence: {
      if (undoing) {
        return store_->deleteSequenceIfEmpty(entry.sequenceId);
      }
      SequenceRecord sequence;
      sequence.sequenceId = entry.sequenceId;
      sequence.name = entry.afterName;
      sequence.rate = entry.instance.sourceRate;
      sequence.createdUtc = entry.instance.createdUtc;
      auto restored = store_->reinsertSequence(sequence);
      if (!restored.ok()) {
        return Result<void>::failure(restored.error.code, restored.error.message);
      }
      return Result<void>::success();
    }
    case UndoKind::CreateTrack:
      if (undoing) {
        return store_->deleteTrackIfEmpty(entry.trackId);
      }
      return store_->reinsertTrack(TrackRecord{entry.trackId, entry.sequenceId, entry.afterOrdinal, {}, {}}).ok()
                 ? Result<void>::success()
                 : Result<void>::failure(ErrorCode::Database, "track could not be restored");
    case UndoKind::OrderTrack:
      return store_->moveTrack(entry.trackId, undoing ? entry.beforeOrdinal : entry.afterOrdinal);
    case UndoKind::InsertInstance:
      if (undoing) {
        auto removed = store_->deleteInstance(entry.instance.instanceId);
        if (!removed.ok()) {
          return removed;
        }
        if (!entry.trackId.empty()) {
          return store_->deleteTrackIfEmpty(entry.trackId);
        }
        return Result<void>::success();
      }
      if (!entry.trackId.empty()) {
        auto restored = store_->reinsertTrack(
            TrackRecord{entry.trackId, entry.sequenceId, entry.afterOrdinal, {}, {}});
        if (!restored.ok()) {
          return Result<void>::failure(restored.error.code, restored.error.message);
        }
      }
      return store_->reinsertInstance(entry.instance).ok()
                 ? Result<void>::success()
                 : Result<void>::failure(ErrorCode::Database, "instance could not be restored");
    case UndoKind::RemoveInstance:
      if (undoing) {
        return store_->reinsertInstance(entry.instance).ok()
                   ? Result<void>::success()
                   : Result<void>::failure(ErrorCode::Database, "instance could not be restored");
      }
      return store_->deleteInstance(entry.instance.instanceId);
    case UndoKind::Lift:
      if (undoing) {
        return store_->reinsertInstance(entry.instance).ok()
                   ? Result<void>::success()
                   : Result<void>::failure(ErrorCode::Database, "instance could not be restored");
      }
      return store_->deleteInstance(entry.instance.instanceId);
    case UndoKind::MoveInstance:
      return store_->moveInstance(entry.instance.instanceId, undoing ? entry.beforeRecordStart : entry.afterRecordStart);
    case UndoKind::TrimInstance:
      if (undoing) {
        return store_->trimInstance(entry.instance.instanceId, entry.beforeSourceStart, entry.beforeSourceEnd,
                                    entry.beforeRecordStart, entry.beforeRecordEnd);
      }
      return store_->trimInstance(entry.instance.instanceId, entry.afterSourceStart, entry.afterSourceEnd,
                                  entry.afterRecordStart, entry.afterRecordEnd);
    case UndoKind::Overwrite: {
      const RowEdit edit = undoing ? editToward(entry.afterInstances, entry.beforeInstances)
                                   : editToward(entry.beforeInstances, entry.afterInstances);
      auto wrote = store_->commitInstanceEdits(entry.trackId, edit.deleteIds, edit.updates, edit.inserts);
      if (!wrote.ok()) {
        return Result<void>::failure(wrote.error.code, wrote.error.message);
      }
      return Result<void>::success();
    }
    case UndoKind::Split: {
      const RowEdit edit = undoing ? editToward(entry.afterInstances, entry.beforeInstances)
                                   : editToward(entry.beforeInstances, entry.afterInstances);
      auto wrote = store_->commitInstanceEdits(entry.trackId, edit.deleteIds, edit.updates, edit.inserts);
      if (!wrote.ok()) {
        return Result<void>::failure(wrote.error.code, wrote.error.message);
      }
      return Result<void>::success();
    }
    case UndoKind::Extract: {
      const std::vector<ProjectStore::InstanceTrackEdit> edits =
          undoing ? editsByTrack(entry.afterInstances, entry.beforeInstances)
                  : editsByTrack(entry.beforeInstances, entry.afterInstances);
      auto wrote = store_->commitMultiTrackEdits(edits);
      if (!wrote.ok()) {
        return Result<void>::failure(wrote.error.code, wrote.error.message);
      }
      return Result<void>::success();
    }
  }
  return Result<void>::failure(ErrorCode::InvalidArgument, "undo entry is not recognised");
}

Result<SequenceRecord> ProjectSession::createSequence(const std::string& name, FrameRate rate) {
  auto created = store_->insertSequence(name, rate);
  if (!created.ok()) {
    return created;
  }
  UndoEntry entry;
  entry.kind = UndoKind::CreateSequence;
  entry.sequenceId = created.value.sequenceId;
  entry.afterName = created.value.name;
  entry.instance.sourceRate = created.value.rate;
  entry.instance.createdUtc = created.value.createdUtc;
  entry.instance.modifiedUtc = created.value.modifiedUtc;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return created;
}

Result<SequenceRecord> ProjectSession::readSequence(const std::string& sequenceId) {
  return store_->readSequence(sequenceId);
}

Result<std::vector<SequenceRecord>> ProjectSession::listSequences() {
  return store_->listSequences();
}

Result<void> ProjectSession::renameSequence(const std::string& sequenceId, const std::string& name) {
  auto before = store_->readSequence(sequenceId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto renamed = store_->renameSequence(sequenceId, name);
  if (!renamed.ok()) {
    return renamed;
  }
  UndoEntry entry;
  entry.kind = UndoKind::RenameSequence;
  entry.sequenceId = sequenceId;
  entry.beforeName = before.value.name;
  entry.afterName = name;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<TrackRecord> ProjectSession::createTrack(const std::string& sequenceId) {
  auto created = store_->insertTrack(sequenceId);
  if (!created.ok()) {
    return created;
  }
  UndoEntry entry;
  entry.kind = UndoKind::CreateTrack;
  entry.sequenceId = sequenceId;
  entry.trackId = created.value.trackId;
  entry.afterOrdinal = created.value.ordinal;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return created;
}

Result<std::vector<TrackRecord>> ProjectSession::listTracks(const std::string& sequenceId) {
  return store_->listTracks(sequenceId);
}

Result<void> ProjectSession::orderTrack(const std::string& trackId, int ordinal) {
  std::string sequenceId;
  int before = -1;
  auto sequences = store_->listSequences();
  if (!sequences.ok()) {
    return Result<void>::failure(sequences.error.code, sequences.error.message);
  }
  for (const SequenceRecord& sequence : sequences.value) {
    auto tracks = store_->listTracks(sequence.sequenceId);
    if (!tracks.ok()) {
      return Result<void>::failure(tracks.error.code, tracks.error.message);
    }
    for (const TrackRecord& track : tracks.value) {
      if (track.trackId == trackId) {
        sequenceId = sequence.sequenceId;
        before = track.ordinal;
      }
    }
  }
  if (before < 0) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "track was not found");
  }
  auto moved = store_->moveTrack(trackId, ordinal);
  if (!moved.ok()) {
    return moved;
  }
  UndoEntry entry;
  entry.kind = UndoKind::OrderTrack;
  entry.sequenceId = sequenceId;
  entry.trackId = trackId;
  entry.beforeOrdinal = before;
  entry.afterOrdinal = ordinal;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<TimelineInstance> ProjectSession::insertInstance(const std::string& sequenceId,
                                                       const std::string& trackId,
                                                       const std::string& masterObjectId,
                                                       FrameRate sourceRate,
                                                       std::int64_t sourceStart,
                                                       std::int64_t sourceEnd,
                                                       std::int64_t recordStart,
                                                       std::int64_t recordEnd) {
  TimelineInstance requested;
  requested.sequenceId = sequenceId;
  requested.trackId = trackId;
  requested.masterObjectId = masterObjectId;
  requested.sourceRate = sourceRate;
  requested.sourceStart = sourceStart;
  requested.sourceEnd = sourceEnd;
  requested.recordStart = recordStart;
  requested.recordEnd = recordEnd;
  auto created = store_->insertInstance(requested);
  if (!created.ok()) {
    return created;
  }
  UndoEntry entry;
  entry.kind = UndoKind::InsertInstance;
  entry.instance = created.value;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return created;
}

Result<TimelineInstance> ProjectSession::placeInstance(const std::string& sequenceId,
                                                      const std::string& trackId,
                                                      const std::string& masterObjectId,
                                                      FrameRate sourceRate,
                                                      std::int64_t sourceStart,
                                                      std::int64_t sourceEnd,
                                                      std::int64_t recordStart,
                                                      std::int64_t recordEnd) {
  auto sequence = store_->readSequence(sequenceId);
  if (!sequence.ok()) {
    return Result<TimelineInstance>::failure(sequence.error.code, sequence.error.message);
  }
  auto mapped = identityRecordEnd(sequence.value.rate, sourceRate, sourceStart, sourceEnd, recordStart);
  if (!mapped.ok()) {
    return Result<TimelineInstance>::failure(mapped.error.code, mapped.error.message);
  }
  if (mapped.value != recordEnd) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument,
                                            "record end is not the same-rate source span");
  }
  std::string track = trackId;
  int ordinal = 0;
  const bool makeTrack = track.empty();
  if (makeTrack) {
    auto createdTrack = store_->insertTrack(sequenceId);
    if (!createdTrack.ok()) {
      return Result<TimelineInstance>::failure(createdTrack.error.code, createdTrack.error.message);
    }
    track = createdTrack.value.trackId;
    ordinal = createdTrack.value.ordinal;
  }
  TimelineInstance requested;
  requested.sequenceId = sequenceId;
  requested.trackId = track;
  requested.masterObjectId = masterObjectId;
  requested.sourceRate = sourceRate;
  requested.sourceStart = sourceStart;
  requested.sourceEnd = sourceEnd;
  requested.recordStart = recordStart;
  requested.recordEnd = recordEnd;
  auto created = store_->insertInstance(requested);
  if (!created.ok()) {
    if (makeTrack) {
      store_->deleteTrackIfEmpty(track);
    }
    return created;
  }
  UndoEntry entry;
  entry.kind = UndoKind::InsertInstance;
  entry.instance = created.value;
  if (makeTrack) {
    entry.trackId = track;
    entry.sequenceId = sequenceId;
    entry.afterOrdinal = ordinal;
  }
  redo_.clear();
  undo_.push_back(std::move(entry));
  return created;
}

Result<TimelineInstance> ProjectSession::overwrite(const std::string& sequenceId,
                                                  const std::string& trackId,
                                                  const std::string& masterObjectId,
                                                  const std::string& representationId,
                                                  FrameRate sourceRate) {
  if (!isOpen()) {
    return Result<TimelineInstance>::failure(ErrorCode::NotOpen, "the project is not open");
  }
  auto sequence = store_->readSequence(sequenceId);
  if (!sequence.ok()) {
    return Result<TimelineInstance>::failure(sequence.error.code, sequence.error.message);
  }
  if (trackId.empty()) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "choose a track");
  }
  auto tracks = store_->listTracks(sequenceId);
  if (!tracks.ok()) {
    return Result<TimelineInstance>::failure(tracks.error.code, tracks.error.message);
  }
  bool trackFound = false;
  for (const TrackRecord& track : tracks.value) {
    if (track.trackId == trackId) {
      trackFound = true;
    }
  }
  if (!trackFound) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
  }
  auto master = store_->readMasterClip(masterObjectId);
  if (!master.ok()) {
    return Result<TimelineInstance>::failure(master.error.code, master.error.message);
  }
  auto representations = store_->listRepresentations(master.value.mediaId);
  if (!representations.ok()) {
    return Result<TimelineInstance>::failure(representations.error.code, representations.error.message);
  }
  bool representationFound = false;
  for (const Representation& representation : representations.value) {
    if (representation.representationId == representationId) {
      representationFound = true;
    }
  }
  if (!representationFound) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "the representation is not on this asset");
  }
  auto sourcePoints = sourceMarks(representationId);
  if (!sourcePoints.ok()) {
    return Result<TimelineInstance>::failure(sourcePoints.error.code, sourcePoints.error.message);
  }
  auto recordPoints = recordMarks(sequenceId);
  if (!recordPoints.ok()) {
    return Result<TimelineInstance>::failure(recordPoints.error.code, recordPoints.error.message);
  }
  if (!sourcePoints.value.markIn.has_value() || !sourcePoints.value.markOut.has_value() ||
      !recordPoints.value.markIn.has_value() || !recordPoints.value.markOut.has_value()) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "Mark In and Mark Out are required");
  }
  auto sourceEnd = exclusiveEndFromInclusiveMark(*sourcePoints.value.markIn, *sourcePoints.value.markOut);
  if (!sourceEnd.ok()) {
    return Result<TimelineInstance>::failure(sourceEnd.error.code, sourceEnd.error.message);
  }
  auto recordEnd = exclusiveEndFromInclusiveMark(*recordPoints.value.markIn, *recordPoints.value.markOut);
  if (!recordEnd.ok()) {
    return Result<TimelineInstance>::failure(recordEnd.error.code, recordEnd.error.message);
  }
  const std::int64_t sourceStart = *sourcePoints.value.markIn;
  const std::int64_t recordStart = *recordPoints.value.markIn;
  auto mapped = identityRecordEnd(sequence.value.rate, sourceRate, sourceStart, sourceEnd.value, recordStart);
  if (!mapped.ok()) {
    return Result<TimelineInstance>::failure(mapped.error.code, mapped.error.message);
  }
  if (mapped.value != recordEnd.value) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "source duration and record duration differ");
  }
  auto existing = store_->listInstances(trackId);
  if (!existing.ok()) {
    return Result<TimelineInstance>::failure(existing.error.code, existing.error.message);
  }
  if (existing.value.empty()) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument, "the destination track has no record end");
  }
  std::int64_t maximumRecordEnd = 0;
  for (const TimelineInstance& instance : existing.value) {
    if (instance.recordEnd > maximumRecordEnd) {
      maximumRecordEnd = instance.recordEnd;
    }
  }
  if (recordEnd.value > maximumRecordEnd) {
    return Result<TimelineInstance>::failure(ErrorCode::InvalidArgument,
                                            "record end is past the current maximum record end");
  }
  TimelineInstance replacement;
  replacement.sequenceId = sequenceId;
  replacement.trackId = trackId;
  replacement.masterObjectId = master.value.objectId;
  replacement.mediaId = master.value.mediaId;
  replacement.sourceRate = sourceRate;
  replacement.sourceStart = sourceStart;
  replacement.sourceEnd = sourceEnd.value;
  replacement.recordStart = recordStart;
  replacement.recordEnd = recordEnd.value;
  auto plan = planSameRateOverwrite(replacement, existing.value);
  if (!plan.ok()) {
    return Result<TimelineInstance>::failure(plan.error.code, plan.error.message);
  }
  std::vector<TimelineInstance> before;
  for (const TimelineInstance& instance : existing.value) {
    const bool removed = findInstance(plan.value.removed, instance.instanceId) != nullptr;
    const bool updated = findInstance(plan.value.updated, instance.instanceId) != nullptr;
    if (removed || updated) {
      before.push_back(instance);
    }
  }
  for (TimelineInstance& row : plan.value.inserted) {
    if (!row.instanceId.empty()) {
      continue;
    }
    const auto minted = makeStableId();
    if (!minted) {
      return Result<TimelineInstance>::failure(ErrorCode::Io, "instance id could not be minted");
    }
    row.instanceId = *minted;
    row.sequenceId = sequenceId;
    row.trackId = trackId;
  }
  std::vector<std::string> deleteIds;
  deleteIds.reserve(plan.value.removed.size());
  for (const TimelineInstance& row : plan.value.removed) {
    deleteIds.push_back(row.instanceId);
  }
  auto wrote = store_->commitInstanceEdits(trackId, deleteIds, plan.value.updated, plan.value.inserted);
  if (!wrote.ok()) {
    return Result<TimelineInstance>::failure(wrote.error.code, wrote.error.message);
  }
  TimelineInstance created;
  bool foundReplacement = false;
  for (const TimelineInstance& row : wrote.value.inserted) {
    if (row.recordStart == recordStart && row.recordEnd == recordEnd.value && row.sourceStart == sourceStart &&
        row.sourceEnd == sourceEnd.value && row.masterObjectId == master.value.objectId) {
      created = row;
      foundReplacement = true;
    }
  }
  if (!foundReplacement) {
    return Result<TimelineInstance>::failure(ErrorCode::Database, "overwrite replacement was not written");
  }
  UndoEntry entry;
  entry.kind = UndoKind::Overwrite;
  entry.sequenceId = sequenceId;
  entry.trackId = trackId;
  entry.instance = created;
  entry.beforeInstances = std::move(before);
  entry.afterInstances = wrote.value.updated;
  entry.afterInstances.insert(entry.afterInstances.end(), wrote.value.inserted.begin(), wrote.value.inserted.end());
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<TimelineInstance>::success(created);
}

Result<TimelineInstance> ProjectSession::split(const std::string& instanceId, std::int64_t frame) {
  if (!isOpen()) {
    return Result<TimelineInstance>::failure(ErrorCode::NotOpen, "the project is not open");
  }
  auto before = store_->readInstance(instanceId);
  if (!before.ok()) {
    return Result<TimelineInstance>::failure(before.error.code, before.error.message);
  }
  auto sequence = store_->readSequence(before.value.sequenceId);
  if (!sequence.ok()) {
    return Result<TimelineInstance>::failure(sequence.error.code, sequence.error.message);
  }
  auto plan = planSplit(before.value, sequence.value.rate, frame);
  if (!plan.ok()) {
    return Result<TimelineInstance>::failure(plan.error.code, plan.error.message);
  }
  const auto minted = makeStableId();
  if (!minted) {
    return Result<TimelineInstance>::failure(ErrorCode::Io, "instance id could not be minted");
  }
  plan.value.right.instanceId = *minted;
  auto wrote = store_->commitInstanceEdits(before.value.trackId, {}, {plan.value.left}, {plan.value.right});
  if (!wrote.ok()) {
    return Result<TimelineInstance>::failure(wrote.error.code, wrote.error.message);
  }
  TimelineInstance created = plan.value.right;
  for (const TimelineInstance& row : wrote.value.inserted) {
    if (row.instanceId == *minted) {
      created = row;
    }
  }
  UndoEntry entry;
  entry.kind = UndoKind::Split;
  entry.sequenceId = before.value.sequenceId;
  entry.trackId = before.value.trackId;
  entry.instance = created;
  entry.beforeInstances.push_back(before.value);
  entry.afterInstances = wrote.value.updated;
  if (wrote.value.inserted.empty()) {
    entry.afterInstances.push_back(created);
  } else {
    entry.afterInstances.insert(entry.afterInstances.end(), wrote.value.inserted.begin(), wrote.value.inserted.end());
  }
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<TimelineInstance>::success(created);
}

Result<void> ProjectSession::setExtractRange(const std::string& sequenceId,
                                            std::int64_t recordStart,
                                            std::int64_t recordEnd) {
  if (!isOpen()) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  auto sequence = store_->readSequence(sequenceId);
  if (!sequence.ok()) {
    return Result<void>::failure(sequence.error.code, sequence.error.message);
  }
  if (!isHalfOpenFrameSpan(recordStart, recordEnd)) {
    return Result<void>::failure(ErrorCode::InvalidArgument,
                                "the record range must be a half-open interval of integer sequence frames");
  }
  extractRangeSet_ = true;
  extractRangeSequenceId_ = sequenceId;
  extractRangeStart_ = recordStart;
  extractRangeEnd_ = recordEnd;
  return Result<void>::success();
}

void ProjectSession::clearExtractRange() {
  extractRangeSet_ = false;
  extractRangeSequenceId_.clear();
  extractRangeStart_ = 0;
  extractRangeEnd_ = 0;
}

bool ProjectSession::extractRange(std::string* sequenceId, std::int64_t* recordStart, std::int64_t* recordEnd) const {
  if (!extractRangeSet_) {
    return false;
  }
  if (sequenceId != nullptr) {
    *sequenceId = extractRangeSequenceId_;
  }
  if (recordStart != nullptr) {
    *recordStart = extractRangeStart_;
  }
  if (recordEnd != nullptr) {
    *recordEnd = extractRangeEnd_;
  }
  return true;
}

Result<void> ProjectSession::extract(const std::string& sequenceId,
                                     const std::vector<std::string>& trackIds,
                                     std::int64_t recordStart,
                                     std::int64_t recordEnd) {
  if (!isOpen()) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  auto sequence = store_->readSequence(sequenceId);
  if (!sequence.ok()) {
    return Result<void>::failure(sequence.error.code, sequence.error.message);
  }
  if (!isHalfOpenFrameSpan(recordStart, recordEnd)) {
    return Result<void>::failure(ErrorCode::InvalidArgument,
                                "the record range must be a half-open interval of integer sequence frames");
  }
  if (trackIds.empty()) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "name at least one track");
  }
  for (std::size_t left = 0; left < trackIds.size(); ++left) {
    if (trackIds[left].empty()) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "choose a track");
    }
    for (std::size_t right = left + 1; right < trackIds.size(); ++right) {
      if (trackIds[left] == trackIds[right]) {
        return Result<void>::failure(ErrorCode::InvalidArgument, "track " + trackIds[left] + " is named more than once");
      }
    }
  }
  auto tracks = store_->listTracks(sequenceId);
  if (!tracks.ok()) {
    return Result<void>::failure(tracks.error.code, tracks.error.message);
  }
  auto named = [&](const std::string& trackId) {
    for (const std::string& id : trackIds) {
      if (id == trackId) {
        return true;
      }
    }
    return false;
  };
  for (const std::string& trackId : trackIds) {
    bool found = false;
    for (const TrackRecord& track : tracks.value) {
      if (track.trackId == trackId) {
        found = true;
      }
    }
    if (!found) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "track is not in the sequence");
    }
  }
  std::vector<ExtractTrackInput> inputs;
  std::vector<TimelineInstance> untouched;
  std::int64_t oldEnd = 0;
  bool anyInstance = false;
  for (const TrackRecord& track : tracks.value) {
    auto rows = store_->listInstances(track.trackId);
    if (!rows.ok()) {
      return Result<void>::failure(rows.error.code, rows.error.message);
    }
    for (const TimelineInstance& row : rows.value) {
      if (!anyInstance || row.recordEnd > oldEnd) {
        oldEnd = row.recordEnd;
        anyInstance = true;
      }
    }
    if (named(track.trackId)) {
      ExtractTrackInput input;
      input.trackId = track.trackId;
      input.instances = std::move(rows.value);
      inputs.push_back(std::move(input));
    } else {
      untouched.insert(untouched.end(), rows.value.begin(), rows.value.end());
    }
  }
  auto planned = planExtract(sequence.value.rate, recordStart, recordEnd, inputs);
  if (!planned.ok()) {
    return Result<void>::failure(planned.error.code, planned.error.message);
  }
  const std::int64_t distance = recordEnd - recordStart;
  // candidateEnd is only the overflow line for tracks the editor did not name.
  // The stored visible end stays the furthest record_end after the write. A
  // participating row that ends at or before r0 is not moved to force oldEnd - D.
  const bool candidateBelowZero = oldEnd < distance;
  const std::int64_t candidateEnd = candidateBelowZero ? 0 : oldEnd - distance;
  const TimelineInstance* blocker = nullptr;
  for (const TimelineInstance& row : untouched) {
    const bool past = candidateBelowZero || row.recordEnd > candidateEnd;
    if (past && (blocker == nullptr || row.recordEnd > blocker->recordEnd)) {
      blocker = &row;
    }
  }
  if (blocker != nullptr) {
    const std::string candidateText = candidateBelowZero ? "below zero" : std::to_string(candidateEnd);
    return Result<void>::failure(
        ErrorCode::InvalidArgument,
        "Track " + blocker->trackId + " instance " + blocker->instanceId + " ends at " +
            std::to_string(blocker->recordEnd) + ", past candidate end " + candidateText + ". Visible end is " +
            std::to_string(oldEnd) + " and D is " + std::to_string(distance) +
            ". Candidate end checks unselected overflow. It is not the stored visible end. Extract does not move "
            "or truncate a track that was not named.");
  }
  for (ExtractTrackResult& track : planned.value) {
    for (TimelineInstance& row : track.after) {
      if (!row.instanceId.empty()) {
        continue;
      }
      std::string minted;
      if (!armedExtractId_.empty()) {
        minted = armedExtractId_;
        armedExtractId_.clear();
      } else {
        auto id = makeStableId();
        if (!id.has_value()) {
          return Result<void>::failure(ErrorCode::Io, "instance id could not be minted");
        }
        minted = *id;
      }
      for (const ExtractTrackResult& other : planned.value) {
        for (const TimelineInstance& have : other.after) {
          if (have.instanceId == minted) {
            return Result<void>::failure(ErrorCode::InvalidArgument,
                                        "instance id " + minted + " collides with an existing row");
          }
        }
      }
      auto existing = store_->readInstance(minted);
      if (existing.ok()) {
        return Result<void>::failure(ErrorCode::InvalidArgument,
                                    "instance id " + minted + " collides with an existing row");
      }
      if (existing.error.message != "instance was not found") {
        return Result<void>::failure(existing.error.code, existing.error.message);
      }
      row.instanceId = minted;
    }
  }
  std::vector<TimelineInstance> beforeRows;
  std::vector<TimelineInstance> afterRows;
  for (const ExtractTrackResult& track : planned.value) {
    beforeRows.insert(beforeRows.end(), track.before.begin(), track.before.end());
    afterRows.insert(afterRows.end(), track.after.begin(), track.after.end());
  }
  auto wrote = store_->commitMultiTrackEdits(editsByTrack(beforeRows, afterRows));
  if (!wrote.ok()) {
    return Result<void>::failure(wrote.error.code, wrote.error.message);
  }
  UndoEntry entry;
  entry.kind = UndoKind::Extract;
  entry.sequenceId = sequenceId;
  entry.beforeInstances = std::move(beforeRows);
  entry.afterInstances = std::move(afterRows);
  redo_.clear();
  undo_.push_back(std::move(entry));
  clearExtractRange();
  return Result<void>::success();
}

Result<void> ProjectSession::lift(const std::string& instanceId) {
  auto before = store_->readInstance(instanceId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto removed = store_->deleteInstance(instanceId);
  if (!removed.ok()) {
    return removed;
  }
  UndoEntry entry;
  entry.kind = UndoKind::Lift;
  entry.instance = before.value;
  entry.sequenceId = before.value.sequenceId;
  entry.trackId = before.value.trackId;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<void> ProjectSession::moveInstance(const std::string& instanceId, std::int64_t recordStart) {
  auto before = store_->readInstance(instanceId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto moved = store_->moveInstance(instanceId, recordStart);
  if (!moved.ok()) {
    return moved;
  }
  auto after = store_->readInstance(instanceId);
  if (!after.ok()) {
    return Result<void>::failure(after.error.code, after.error.message);
  }
  UndoEntry entry;
  entry.kind = UndoKind::MoveInstance;
  entry.instance = after.value;
  entry.beforeRecordStart = before.value.recordStart;
  entry.afterRecordStart = after.value.recordStart;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<void> ProjectSession::trimInstance(const std::string& instanceId,
                                          std::int64_t sourceStart,
                                          std::int64_t sourceEnd,
                                          std::int64_t recordStart,
                                          std::int64_t recordEnd) {
  auto before = store_->readInstance(instanceId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto trimmed = store_->trimInstance(instanceId, sourceStart, sourceEnd, recordStart, recordEnd);
  if (!trimmed.ok()) {
    return trimmed;
  }
  UndoEntry entry;
  entry.kind = UndoKind::TrimInstance;
  entry.instance = before.value;
  entry.beforeSourceStart = before.value.sourceStart;
  entry.beforeSourceEnd = before.value.sourceEnd;
  entry.beforeRecordStart = before.value.recordStart;
  entry.beforeRecordEnd = before.value.recordEnd;
  entry.afterSourceStart = sourceStart;
  entry.afterSourceEnd = sourceEnd;
  entry.afterRecordStart = recordStart;
  entry.afterRecordEnd = recordEnd;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<void> ProjectSession::removeInstance(const std::string& instanceId) {
  auto before = store_->readInstance(instanceId);
  if (!before.ok()) {
    return Result<void>::failure(before.error.code, before.error.message);
  }
  auto removed = store_->deleteInstance(instanceId);
  if (!removed.ok()) {
    return removed;
  }
  UndoEntry entry;
  entry.kind = UndoKind::RemoveInstance;
  entry.instance = before.value;
  redo_.clear();
  undo_.push_back(std::move(entry));
  return Result<void>::success();
}

Result<TimelineInstance> ProjectSession::readInstance(const std::string& instanceId) {
  return store_->readInstance(instanceId);
}

Result<std::vector<TimelineInstance>> ProjectSession::listInstances(const std::string& trackId) {
  return store_->listInstances(trackId);
}

Result<void> ProjectSession::undo() {
  if (!store_->isOpen()) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  if (undo_.empty()) {
    return Result<void>::failure(ErrorCode::NothingToUndo, "nothing to undo");
  }
  const UndoEntry entry = undo_.back();
  auto applied = apply(entry, true);
  if (!applied.ok()) {
    logger_.error("transaction", "undo failed and the stack was left in place");
    return applied;
  }
  undo_.pop_back();
  redo_.push_back(entry);
  return Result<void>::success();
}

Result<void> ProjectSession::redo() {
  if (!store_->isOpen()) {
    return Result<void>::failure(ErrorCode::NotOpen, "project is not open");
  }
  if (redo_.empty()) {
    return Result<void>::failure(ErrorCode::NothingToRedo, "nothing to redo");
  }
  const UndoEntry entry = redo_.back();
  auto applied = apply(entry, false);
  if (!applied.ok()) {
    logger_.error("transaction", "redo failed and the stack was left in place");
    return applied;
  }
  redo_.pop_back();
  undo_.push_back(entry);
  return Result<void>::success();
}

bool ProjectSession::canUndo() const { return store_->isOpen() && !undo_.empty(); }

bool ProjectSession::canRedo() const { return store_->isOpen() && !redo_.empty(); }

Result<void> ProjectSession::autosave() {
  auto flushed = store_->autosave();
  if (!flushed.ok()) {
    logger_.error("autosave", flushed.error.message);
    return flushed;
  }
  logger_.info("autosave", "autosave finished without touching the undo stack");
  return flushed;
}

ProjectStore& ProjectSession::storeForTest() { return *store_; }

void ProjectSession::debugArmExtractIdentityCollision(const std::string& instanceId) {
  armedExtractId_ = instanceId;
}

bool ProjectSession::representationInProject(const std::string& representationId) {
  if (!isOpen() || representationId.empty()) {
    return false;
  }
  auto bins = listBins();
  if (!bins.ok()) {
    return false;
  }
  for (const BinRecord& bin : bins.value) {
    auto assets = listBinAssets(bin.binId);
    if (!assets.ok()) {
      return false;
    }
    for (const BinAssetFact& asset : assets.value) {
      if (asset.representationId == representationId) {
        return true;
      }
    }
  }
  return false;
}

bool ProjectSession::sequenceInProject(const std::string& sequenceId) {
  if (!isOpen() || sequenceId.empty()) {
    return false;
  }
  return readSequence(sequenceId).ok();
}

Result<MarkPoints> ProjectSession::readPoints(const std::map<std::string, MarkPoints>& table, const std::string& id,
                                             bool owner, const char* missing) const {
  if (!isOpen()) {
    return Result<MarkPoints>::failure(ErrorCode::NotOpen, "the project is not open");
  }
  if (!owner) {
    return Result<MarkPoints>::failure(ErrorCode::InvalidArgument, missing);
  }
  const auto found = table.find(id);
  if (found == table.end()) {
    return Result<MarkPoints>::success(MarkPoints{});
  }
  return Result<MarkPoints>::success(found->second);
}

Result<void> ProjectSession::storePoint(std::map<std::string, MarkPoints>& table, const std::string& id, bool owner,
                                        bool isIn, std::int64_t frame, const char* missing) {
  if (!isOpen()) {
    return Result<void>::failure(ErrorCode::NotOpen, "the project is not open");
  }
  if (!owner) {
    return Result<void>::failure(ErrorCode::InvalidArgument, missing);
  }
  if (frame < 0) {
    return Result<void>::failure(ErrorCode::InvalidArgument, "a frame must be a non-negative integer");
  }
  MarkPoints next;
  const auto found = table.find(id);
  if (found != table.end()) {
    next = found->second;
  }
  if (isIn) {
    next.markIn = frame;
  } else {
    next.markOut = frame;
  }
  if (next.markIn.has_value() && next.markOut.has_value()) {
    if (*next.markOut < *next.markIn) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "Mark Out is before Mark In");
    }
    if (*next.markOut == *next.markIn) {
      return Result<void>::failure(ErrorCode::InvalidArgument, "equal Mark In and Mark Out are an incomplete range");
    }
  }
  table[id] = next;
  return Result<void>::success();
}

Result<void> ProjectSession::clearPoints(std::map<std::string, MarkPoints>& table, const std::string& id, bool owner,
                                         bool clearIn, bool clearOut, const char* missing) {
  if (!isOpen()) {
    return Result<void>::failure(ErrorCode::NotOpen, "the project is not open");
  }
  if (!owner) {
    return Result<void>::failure(ErrorCode::InvalidArgument, missing);
  }
  const auto found = table.find(id);
  if (found == table.end()) {
    return Result<void>::success();
  }
  if (clearIn) {
    found->second.markIn.reset();
  }
  if (clearOut) {
    found->second.markOut.reset();
  }
  if (!found->second.markIn.has_value() && !found->second.markOut.has_value()) {
    table.erase(found);
  }
  return Result<void>::success();
}

Result<MarkPoints> ProjectSession::sourceMarks(const std::string& representationId) {
  return readPoints(sourceMarks_, representationId, representationInProject(representationId),
                    "the representation is not in this project");
}

Result<void> ProjectSession::setSourceMarkIn(const std::string& representationId, std::int64_t frame) {
  return storePoint(sourceMarks_, representationId, representationInProject(representationId), true, frame,
                    "the representation is not in this project");
}

Result<void> ProjectSession::setSourceMarkOut(const std::string& representationId, std::int64_t frame) {
  return storePoint(sourceMarks_, representationId, representationInProject(representationId), false, frame,
                    "the representation is not in this project");
}

Result<void> ProjectSession::clearSourceMarkIn(const std::string& representationId) {
  return clearPoints(sourceMarks_, representationId, representationInProject(representationId), true, false,
                     "the representation is not in this project");
}

Result<void> ProjectSession::clearSourceMarkOut(const std::string& representationId) {
  return clearPoints(sourceMarks_, representationId, representationInProject(representationId), false, true,
                     "the representation is not in this project");
}

Result<void> ProjectSession::clearSourceMarks(const std::string& representationId) {
  return clearPoints(sourceMarks_, representationId, representationInProject(representationId), true, true,
                     "the representation is not in this project");
}

Result<MarkPoints> ProjectSession::recordMarks(const std::string& sequenceId) {
  return readPoints(recordMarks_, sequenceId, sequenceInProject(sequenceId), "the sequence is not in this project");
}

Result<void> ProjectSession::setRecordMarkIn(const std::string& sequenceId, std::int64_t frame) {
  return storePoint(recordMarks_, sequenceId, sequenceInProject(sequenceId), true, frame,
                    "the sequence is not in this project");
}

Result<void> ProjectSession::setRecordMarkOut(const std::string& sequenceId, std::int64_t frame) {
  return storePoint(recordMarks_, sequenceId, sequenceInProject(sequenceId), false, frame,
                    "the sequence is not in this project");
}

Result<void> ProjectSession::clearRecordMarkIn(const std::string& sequenceId) {
  return clearPoints(recordMarks_, sequenceId, sequenceInProject(sequenceId), true, false,
                     "the sequence is not in this project");
}

Result<void> ProjectSession::clearRecordMarkOut(const std::string& sequenceId) {
  return clearPoints(recordMarks_, sequenceId, sequenceInProject(sequenceId), false, true,
                     "the sequence is not in this project");
}

Result<void> ProjectSession::clearRecordMarks(const std::string& sequenceId) {
  return clearPoints(recordMarks_, sequenceId, sequenceInProject(sequenceId), true, true,
                     "the sequence is not in this project");
}

}  // namespace aether
