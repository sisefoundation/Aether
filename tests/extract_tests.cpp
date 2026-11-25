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

#include "extract_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/persistence/project_store.hpp"
#include "aether/ui/frame_label.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTest>
#include <QWidget>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

class Scratch {
 public:
  Scratch() : session(logger), workspace(session) {
    const auto id = aether::makeStableId();
    parent = std::filesystem::temp_directory_path() / ("aether-extract-" + (id ? *id : std::string("tmp")));
    std::filesystem::create_directories(parent);
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ignored;
    std::filesystem::remove_all(parent, ignored);
  }

  std::filesystem::path writeMedia(const std::string& name) const {
    const auto path = parent / name;
    std::ofstream output(path, std::ios::binary);
    output << "extract-bytes";
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  aether::TimelineWorkspace workspace;
  std::filesystem::path parent;
};

struct Reel {
  std::filesystem::path directory;
  std::string objectId;
  std::string representationId;
  std::string sequenceId;
  std::string trackId;
  std::string otherTrackId;
};

bool reopen(Scratch& scratch, const Reel& reel) {
  return scratch.session.closeProject().ok() && scratch.session.openProject(reel.directory).ok();
}

bool prepare(Scratch& scratch, Reel* reel, const std::string& folder) {
  scratch.session.closeProject();
  reel->directory = scratch.parent / folder;
  if (!scratch.session.createProject(reel->directory, "ExtractCut").ok()) {
    return false;
  }
  auto imported = scratch.session.importReferences({scratch.writeMedia(folder + ".mov").string()});
  if (!imported.ok() || imported.value.size() != 1) {
    return false;
  }
  reel->objectId = imported.value.front().objectId;
  reel->representationId = imported.value.front().representationId;
  auto sequence = scratch.session.createSequence("Reel", {24, 1});
  if (!sequence.ok()) {
    return false;
  }
  reel->sequenceId = sequence.value.sequenceId;
  auto track = scratch.session.createTrack(reel->sequenceId);
  auto other = scratch.session.createTrack(reel->sequenceId);
  if (!track.ok() || !other.ok()) {
    return false;
  }
  reel->trackId = track.value.trackId;
  reel->otherTrackId = other.value.trackId;
  return true;
}

bool place(Scratch& scratch, const Reel& reel, const std::string& trackId, std::int64_t sourceStart,
           std::int64_t sourceEnd, std::int64_t recordStart, std::int64_t recordEnd, std::string* id) {
  auto placed = scratch.session.placeInstance(reel.sequenceId, trackId, reel.objectId, {24, 1}, sourceStart, sourceEnd,
                                              recordStart, recordEnd);
  if (!placed.ok()) {
    return false;
  }
  *id = placed.value.instanceId;
  return true;
}

bool insertAt(Scratch& scratch, const Reel& reel, const std::string& trackId, aether::FrameRate sourceRate,
              std::int64_t sourceStart, std::int64_t sourceEnd, std::int64_t recordStart, std::int64_t recordEnd,
              std::string* id) {
  auto placed = scratch.session.insertInstance(reel.sequenceId, trackId, reel.objectId, sourceRate, sourceStart,
                                               sourceEnd, recordStart, recordEnd);
  if (!placed.ok()) {
    return false;
  }
  *id = placed.value.instanceId;
  return true;
}

std::int64_t storedVisibleEnd(Scratch& scratch, const Reel& reel) {
  std::int64_t end = 0;
  bool any = false;
  auto tracks = scratch.session.listTracks(reel.sequenceId);
  if (!tracks.ok()) {
    return -1;
  }
  for (const aether::TrackRecord& track : tracks.value) {
    auto rows = scratch.session.listInstances(track.trackId);
    if (!rows.ok()) {
      return -1;
    }
    for (const aether::TimelineInstance& row : rows.value) {
      if (!any || row.recordEnd > end) {
        end = row.recordEnd;
        any = true;
      }
    }
  }
  return any ? end : 0;
}

}  // namespace

void ExtractTests::rangeIsSessionOnly() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "range"));
  std::string id;
  QVERIFY(place(scratch, reel, reel.trackId, 0, 12, 0, 12, &id));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(scratch.session.setRecordMarkIn(reel.sequenceId, 4).ok());
  QVERIFY(scratch.session.setRecordMarkOut(reel.sequenceId, 9).ok());
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QVERIFY(!scratch.session.setExtractRange(reel.sequenceId, 12, 12).ok());
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QVERIFY(scratch.session.setExtractRange(reel.sequenceId, 2, 6).ok());
  std::string sequenceId;
  std::int64_t start = 0;
  std::int64_t end = 0;
  QVERIFY(scratch.session.extractRange(&sequenceId, &start, &end));
  QCOMPARE(sequenceId, reel.sequenceId);
  QCOMPARE(start, static_cast<std::int64_t>(2));
  QCOMPARE(end, static_cast<std::int64_t>(6));
  QVERIFY(!scratch.session.setExtractRange(reel.sequenceId, 8, 3).ok());
  QVERIFY(scratch.session.extractRange(&sequenceId, &start, &end));
  QCOMPARE(start, static_cast<std::int64_t>(2));
  QCOMPARE(end, static_cast<std::int64_t>(6));
  auto refused = scratch.session.extract(reel.sequenceId, {reel.trackId}, 8, 3);
  QVERIFY(!refused.ok());
  QVERIFY(refused.error.message.find("half-open") != std::string::npos);
  QVERIFY(scratch.session.extractRange(&sequenceId, &start, &end));
  QCOMPARE(start, static_cast<std::int64_t>(2));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(id).value.recordStart, static_cast<std::int64_t>(0));
  scratch.session.clearExtractRange();
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QVERIFY(scratch.session.setExtractRange(reel.sequenceId, 2, 6).ok());
  QVERIFY(reopen(scratch, reel));
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QVERIFY(!scratch.session.recordMarks(reel.sequenceId).value.markIn.has_value());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string(aether::kPrototypeVersion));

  Scratch closed;
  auto missing = closed.session.extract("none", {"track"}, 0, 1);
  QVERIFY(!missing.ok());
  QVERIFY(missing.error.message.find("not open") != std::string::npos);
  QVERIFY(!closed.session.canUndo());
}

void ExtractTests::namedTracksOnly() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "tracks"));
  std::string kept;
  std::string other;
  QVERIFY(place(scratch, reel, reel.trackId, 0, 40, 0, 40, &kept));
  QVERIFY(place(scratch, reel, reel.otherTrackId, 0, 20, 0, 20, &other));
  auto second = scratch.session.createSequence("Other", {24, 1});
  QVERIFY(second.ok());
  auto foreign = scratch.session.createTrack(second.value.sequenceId);
  QVERIFY(foreign.ok());
  QVERIFY(reopen(scratch, reel));

  auto none = scratch.session.extract(reel.sequenceId, {}, 10, 20);
  QVERIFY(!none.ok());
  QVERIFY(none.error.message.find("at least one track") != std::string::npos);
  auto duplicate = scratch.session.extract(reel.sequenceId, {reel.trackId, reel.trackId}, 10, 20);
  QVERIFY(!duplicate.ok());
  QVERIFY(duplicate.error.message.find("more than once") != std::string::npos);
  auto missingSequence = scratch.session.extract("missing-sequence", {reel.trackId}, 10, 20);
  QVERIFY(!missingSequence.ok());
  auto foreignTrack = scratch.session.extract(reel.sequenceId, {foreign.value.trackId}, 10, 20);
  QVERIFY(!foreignTrack.ok());
  QVERIFY(foreignTrack.error.message.find("not in the sequence") != std::string::npos);
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(kept).value.recordEnd, static_cast<std::int64_t>(40));

  auto closed = scratch.session.extract(reel.sequenceId, {reel.trackId}, 10, 20);
  QVERIFY2(closed.ok(), closed.error.message.c_str());
  QCOMPARE(scratch.session.readInstance(other).value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(20));
  auto named = scratch.session.listInstances(reel.trackId);
  QVERIFY(named.ok());
  QCOMPARE(named.value.size(), static_cast<std::size_t>(2));
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
}

void ExtractTests::classification() {
  const std::int64_t r0 = 20;
  const std::int64_t r1 = 30;
  struct Expectation {
    const char* folder;
    std::int64_t recordStart;
    std::int64_t recordEnd;
    std::int64_t sourceStart;
    std::int64_t sourceEnd;
    bool gone;
    bool keepId;
    std::int64_t outRecordStart;
    std::int64_t outRecordEnd;
    std::int64_t outSourceStart;
    std::int64_t outSourceEnd;
    bool right;
    std::int64_t rightRecordStart;
    std::int64_t rightRecordEnd;
    std::int64_t rightSourceStart;
    std::int64_t rightSourceEnd;
  };
  const Expectation cases[] = {
      {"before", 0, 10, 100, 110, false, true, 0, 10, 100, 110, false, 0, 0, 0, 0},
      {"touch-start", 10, 20, 100, 110, false, true, 10, 20, 100, 110, false, 0, 0, 0, 0},
      {"inside", 22, 28, 100, 106, true, false, 0, 0, 0, 0, false, 0, 0, 0, 0},
      {"cover", 20, 30, 100, 110, true, false, 0, 0, 0, 0, false, 0, 0, 0, 0},
      {"tail", 10, 25, 100, 115, false, true, 10, 20, 100, 110, false, 0, 0, 0, 0},
      {"tail-at-end", 10, 30, 100, 120, false, true, 10, 20, 100, 110, false, 0, 0, 0, 0},
      {"head-at-start", 20, 40, 100, 120, false, true, 20, 30, 110, 120, false, 0, 0, 0, 0},
      {"head-inside", 24, 40, 100, 116, true, false, 20, 30, 106, 116, false, 0, 0, 0, 0},
      {"span", 10, 40, 100, 130, false, true, 10, 20, 100, 110, true, 20, 30, 120, 130},
      {"after", 40, 55, 100, 115, false, true, 30, 45, 100, 115, false, 0, 0, 0, 0},
      {"touch-end", 30, 44, 100, 114, false, true, 20, 34, 100, 114, false, 0, 0, 0, 0},
  };
  for (const Expectation& expected : cases) {
    Scratch scratch;
    Reel reel;
    QVERIFY2(prepare(scratch, &reel, expected.folder), expected.folder);
    std::string id;
    QVERIFY2(place(scratch, reel, reel.trackId, expected.sourceStart, expected.sourceEnd, expected.recordStart,
                   expected.recordEnd, &id),
             expected.folder);
    QVERIFY2(reopen(scratch, reel), expected.folder);
    auto extracted = scratch.session.extract(reel.sequenceId, {reel.trackId}, r0, r1);
    QVERIFY2(extracted.ok(), expected.folder);
    auto rows = scratch.session.listInstances(reel.trackId);
    QVERIFY2(rows.ok(), expected.folder);
    if (expected.gone && !expected.right) {
      if (expected.folder == std::string("head-inside")) {
        QCOMPARE(rows.value.size(), static_cast<std::size_t>(1));
        const aether::TimelineInstance& piece = rows.value.front();
        QVERIFY(piece.instanceId != id);
        QCOMPARE(piece.recordStart, expected.outRecordStart);
        QCOMPARE(piece.recordEnd, expected.outRecordEnd);
        QCOMPARE(piece.sourceStart, expected.outSourceStart);
        QCOMPARE(piece.sourceEnd, expected.outSourceEnd);
        QCOMPARE(piece.trackId, reel.trackId);
        QVERIFY(!scratch.session.readInstance(id).ok());
      } else {
        QCOMPARE(rows.value.size(), static_cast<std::size_t>(0));
        QVERIFY(!scratch.session.readInstance(id).ok());
      }
      continue;
    }
    const aether::TimelineInstance* kept = nullptr;
    const aether::TimelineInstance* minted = nullptr;
    for (const aether::TimelineInstance& row : rows.value) {
      if (row.instanceId == id) {
        kept = &row;
      } else {
        minted = &row;
      }
    }
    QVERIFY2(kept != nullptr, expected.folder);
    QCOMPARE(kept->recordStart, expected.outRecordStart);
    QCOMPARE(kept->recordEnd, expected.outRecordEnd);
    QCOMPARE(kept->sourceStart, expected.outSourceStart);
    QCOMPARE(kept->sourceEnd, expected.outSourceEnd);
    QCOMPARE(kept->trackId, reel.trackId);
    QCOMPARE(kept->masterObjectId, reel.objectId);
    if (expected.right) {
      QVERIFY(minted != nullptr);
      QCOMPARE(rows.value.size(), static_cast<std::size_t>(2));
      QCOMPARE(minted->recordStart, expected.rightRecordStart);
      QCOMPARE(minted->recordEnd, expected.rightRecordEnd);
      QCOMPARE(minted->sourceStart, expected.rightSourceStart);
      QCOMPARE(minted->sourceEnd, expected.rightSourceEnd);
      QCOMPARE(minted->trackId, reel.trackId);
    } else {
      QVERIFY(minted == nullptr);
      QCOMPARE(rows.value.size(), static_cast<std::size_t>(1));
    }
  }
}

void ExtractTests::emptyIntervalShiftsLaterRows() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "empty"));
  std::string before;
  std::string after;
  std::string other;
  QVERIFY(place(scratch, reel, reel.trackId, 0, 10, 0, 10, &before));
  QVERIFY(place(scratch, reel, reel.trackId, 0, 10, 40, 50, &after));
  QVERIFY(place(scratch, reel, reel.otherTrackId, 0, 16, 0, 16, &other));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(scratch.session.setRecordMarkIn(reel.sequenceId, 1).ok());
  QVERIFY(scratch.session.extract(reel.sequenceId, {reel.trackId}, 20, 30).ok());
  const aether::TimelineInstance kept = scratch.session.readInstance(before).value;
  QCOMPARE(kept.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(kept.recordEnd, static_cast<std::int64_t>(10));
  QCOMPARE(kept.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(kept.sourceEnd, static_cast<std::int64_t>(10));
  const aether::TimelineInstance shifted = scratch.session.readInstance(after).value;
  QCOMPARE(shifted.instanceId, after);
  QCOMPARE(shifted.recordStart, static_cast<std::int64_t>(30));
  QCOMPARE(shifted.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(shifted.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(shifted.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(shifted.mediaId, kept.mediaId);
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(16));
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(2));
  QCOMPARE(*scratch.session.recordMarks(reel.sequenceId).value.markIn, static_cast<std::int64_t>(1));
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(40));
}

void ExtractTests::unequalUncutAndCrossing() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "unequal"));
  std::string deleted;
  std::string shifted;
  std::string crossing;
  QVERIFY(insertAt(scratch, reel, reel.trackId, {24, 1}, 0, 10, 20, 30, &deleted));
  QVERIFY(insertAt(scratch, reel, reel.trackId, {24, 1}, 0, 10, 40, 60, &shifted));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(scratch.session.extract(reel.sequenceId, {reel.trackId}, 20, 30).ok());
  QVERIFY(!scratch.session.readInstance(deleted).ok());
  const aether::TimelineInstance moved = scratch.session.readInstance(shifted).value;
  QCOMPARE(moved.instanceId, shifted);
  QCOMPARE(moved.recordStart, static_cast<std::int64_t>(30));
  QCOMPARE(moved.recordEnd, static_cast<std::int64_t>(50));
  QCOMPARE(moved.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(moved.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(moved.sourceRate.numerator, static_cast<std::int64_t>(24));

  Scratch mixed;
  Reel mixedReel;
  QVERIFY(prepare(mixed, &mixedReel, "mixed"));
  std::string mixedId;
  QVERIFY(insertAt(mixed, mixedReel, mixedReel.trackId, {25, 1}, 0, 25, 10, 40, &mixedId));
  std::string safe;
  QVERIFY(place(mixed, mixedReel, mixedReel.otherTrackId, 0, 12, 0, 12, &safe));
  QVERIFY(reopen(mixed, mixedReel));
  QVERIFY(mixed.session.setExtractRange(mixedReel.sequenceId, 20, 30).ok());
  const std::int64_t beforeEnd = storedVisibleEnd(mixed, mixedReel);
  auto refused = mixed.session.extract(mixedReel.sequenceId, {mixedReel.trackId, mixedReel.otherTrackId}, 20, 30);
  QVERIFY(!refused.ok());
  QVERIFY(refused.error.message.find(mixedReel.trackId) != std::string::npos);
  QVERIFY(refused.error.message.find(mixedId) != std::string::npos);
  QVERIFY(refused.error.message.find("not one frame per record frame") != std::string::npos);
  QVERIFY(!mixed.session.canUndo());
  QCOMPARE(mixed.session.readInstance(mixedId).value.recordStart, static_cast<std::int64_t>(10));
  QCOMPARE(mixed.session.readInstance(mixedId).value.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(mixed.session.readInstance(safe).value.recordEnd, static_cast<std::int64_t>(12));
  QCOMPARE(storedVisibleEnd(mixed, mixedReel), beforeEnd);
  std::string sequenceId;
  std::int64_t start = 0;
  std::int64_t end = 0;
  QVERIFY(mixed.session.extractRange(&sequenceId, &start, &end));
  QCOMPARE(start, static_cast<std::int64_t>(20));
  QCOMPARE(end, static_cast<std::int64_t>(30));

  Scratch sameRate;
  Reel sameReel;
  QVERIFY(prepare(sameRate, &sameReel, "cross-unequal"));
  std::string unequalId;
  QVERIFY(insertAt(sameRate, sameReel, sameReel.trackId, {24, 1}, 0, 10, 10, 40, &unequalId));
  QVERIFY(reopen(sameRate, sameReel));
  auto sameRefused = sameRate.session.extract(sameReel.sequenceId, {sameReel.trackId}, 20, 30);
  QVERIFY(!sameRefused.ok());
  QVERIFY(sameRefused.error.message.find(unequalId) != std::string::npos);
  QVERIFY(!sameRate.session.canUndo());
  QCOMPARE(sameRate.session.readInstance(unequalId).value.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(sameRate.session.readInstance(unequalId).value.recordEnd, static_cast<std::int64_t>(40));
}

void ExtractTests::visibleEnd() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "visible"));
  std::string span;
  std::string later;
  std::string other;
  QVERIFY(place(scratch, reel, reel.trackId, 0, 40, 8, 48, &span));
  QVERIFY(place(scratch, reel, reel.trackId, 0, 16, 56, 72, &later));
  QVERIFY(place(scratch, reel, reel.otherTrackId, 0, 30, 0, 30, &other));
  QVERIFY(reopen(scratch, reel));
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(72));
  QVERIFY(scratch.session.extract(reel.sequenceId, {reel.trackId}, 24, 40).ok());
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(56));
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(30));
  const aether::TimelineInstance left = scratch.session.readInstance(span).value;
  QCOMPARE(left.recordStart, static_cast<std::int64_t>(8));
  QCOMPARE(left.recordEnd, static_cast<std::int64_t>(24));
  QCOMPARE(left.sourceEnd, static_cast<std::int64_t>(16));
  const aether::TimelineInstance shifted = scratch.session.readInstance(later).value;
  QCOMPARE(shifted.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(shifted.recordEnd, static_cast<std::int64_t>(56));
  QCOMPARE(shifted.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(shifted.sourceEnd, static_cast<std::int64_t>(16));

  Scratch blocked;
  Reel blockedReel;
  QVERIFY(prepare(blocked, &blockedReel, "blocked-end"));
  std::string shortRow;
  std::string holding;
  QVERIFY(place(blocked, blockedReel, blockedReel.trackId, 0, 20, 0, 20, &shortRow));
  QVERIFY(place(blocked, blockedReel, blockedReel.otherTrackId, 0, 40, 0, 40, &holding));
  QVERIFY(reopen(blocked, blockedReel));
  auto refused = blocked.session.extract(blockedReel.sequenceId, {blockedReel.trackId}, 4, 8);
  QVERIFY(!refused.ok());
  QVERIFY(refused.error.message.find(blockedReel.otherTrackId) != std::string::npos);
  QVERIFY(refused.error.message.find(holding) != std::string::npos);
  QVERIFY(refused.error.message.find("does not move or truncate") != std::string::npos);
  QVERIFY(!blocked.session.canUndo());
  QCOMPARE(blocked.session.readInstance(shortRow).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(blocked.session.readInstance(holding).value.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(storedVisibleEnd(blocked, blockedReel), static_cast<std::int64_t>(40));
}

void ExtractTests::candidateEndAndStoredEnd() {
  Scratch past;
  Reel pastReel;
  QVERIFY(prepare(past, &pastReel, "past-candidate"));
  std::string namedEarly;
  std::string namedLate;
  std::string holding;
  QVERIFY(place(past, pastReel, pastReel.trackId, 0, 20, 0, 20, &namedEarly));
  QVERIFY(place(past, pastReel, pastReel.trackId, 0, 20, 30, 50, &namedLate));
  QVERIFY(place(past, pastReel, pastReel.otherTrackId, 0, 48, 0, 48, &holding));
  QVERIFY(reopen(past, pastReel));
  QVERIFY(past.session.setExtractRange(pastReel.sequenceId, 10, 20).ok());
  auto refused = past.session.extract(pastReel.sequenceId, {pastReel.trackId}, 10, 20);
  QVERIFY(!refused.ok());
  QVERIFY(refused.error.message.find(pastReel.otherTrackId) != std::string::npos);
  QVERIFY(refused.error.message.find(holding) != std::string::npos);
  QVERIFY(refused.error.message.find("candidate end") != std::string::npos);
  QVERIFY(refused.error.message.find("does not move or truncate") != std::string::npos);
  QVERIFY(!past.session.canUndo());
  QCOMPARE(past.session.readInstance(namedEarly).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(past.session.readInstance(namedLate).value.recordStart, static_cast<std::int64_t>(30));
  QCOMPARE(past.session.readInstance(holding).value.recordEnd, static_cast<std::int64_t>(48));
  QCOMPARE(storedVisibleEnd(past, pastReel), static_cast<std::int64_t>(50));
  std::int64_t keptStart = 0;
  std::int64_t keptEnd = 0;
  QVERIFY(past.session.extractRange(nullptr, &keptStart, &keptEnd));
  QCOMPARE(keptStart, static_cast<std::int64_t>(10));
  QCOMPARE(keptEnd, static_cast<std::int64_t>(20));

  Scratch exact;
  Reel exactReel;
  QVERIFY(prepare(exact, &exactReel, "exact-candidate"));
  std::string early;
  std::string later;
  std::string other;
  QVERIFY(place(exact, exactReel, exactReel.trackId, 0, 20, 0, 20, &early));
  QVERIFY(place(exact, exactReel, exactReel.trackId, 0, 20, 50, 70, &later));
  QVERIFY(place(exact, exactReel, exactReel.otherTrackId, 0, 60, 0, 60, &other));
  QVERIFY(reopen(exact, exactReel));
  QVERIFY(exact.session.setRecordMarkIn(exactReel.sequenceId, 2).ok());
  QVERIFY(exact.session.setExtractRange(exactReel.sequenceId, 30, 40).ok());
  QVERIFY(exact.session.extract(exactReel.sequenceId, {exactReel.trackId}, 30, 40).ok());
  QVERIFY(!exact.session.extractRange(nullptr, nullptr, nullptr));
  const aether::TimelineInstance stayed = exact.session.readInstance(early).value;
  QCOMPARE(stayed.instanceId, early);
  QCOMPARE(stayed.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(stayed.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(stayed.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(stayed.sourceEnd, static_cast<std::int64_t>(20));
  const aether::TimelineInstance shifted = exact.session.readInstance(later).value;
  QCOMPARE(shifted.instanceId, later);
  QCOMPARE(shifted.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(shifted.recordEnd, static_cast<std::int64_t>(60));
  QCOMPARE(shifted.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(shifted.sourceEnd, static_cast<std::int64_t>(20));
  QCOMPARE(exact.session.readInstance(other).value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(exact.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(60));
  QCOMPARE(exact.session.readInstance(other).value.sourceEnd, static_cast<std::int64_t>(60));
  QCOMPARE(storedVisibleEnd(exact, exactReel), static_cast<std::int64_t>(60));

  Scratch before;
  Reel beforeReel;
  QVERIFY(prepare(before, &beforeReel, "before-candidate"));
  std::string moving;
  std::string earlierOther;
  QVERIFY(place(before, beforeReel, beforeReel.trackId, 0, 20, 40, 60, &moving));
  QVERIFY(place(before, beforeReel, beforeReel.otherTrackId, 0, 20, 0, 20, &earlierOther));
  QVERIFY(reopen(before, beforeReel));
  QVERIFY(before.session.extract(beforeReel.sequenceId, {beforeReel.trackId}, 10, 20).ok());
  QCOMPARE(before.session.readInstance(moving).value.instanceId, moving);
  QCOMPARE(before.session.readInstance(moving).value.recordStart, static_cast<std::int64_t>(30));
  QCOMPARE(before.session.readInstance(moving).value.recordEnd, static_cast<std::int64_t>(50));
  QCOMPARE(before.session.readInstance(moving).value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(before.session.readInstance(moving).value.sourceEnd, static_cast<std::int64_t>(20));
  QCOMPARE(before.session.readInstance(earlierOther).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(storedVisibleEnd(before, beforeReel), static_cast<std::int64_t>(50));

  Scratch furthest;
  Reel furthestReel;
  QVERIFY(prepare(furthest, &furthestReel, "furthest-before"));
  std::string whole;
  std::string quiet;
  QVERIFY(place(furthest, furthestReel, furthestReel.trackId, 0, 100, 0, 100, &whole));
  QVERIFY(place(furthest, furthestReel, furthestReel.otherTrackId, 0, 80, 0, 80, &quiet));
  QVERIFY(reopen(furthest, furthestReel));
  QVERIFY(furthest.session.setExtractRange(furthestReel.sequenceId, 100, 108).ok());
  QVERIFY(furthest.session.extract(furthestReel.sequenceId, {furthestReel.trackId}, 100, 108).ok());
  const aether::TimelineInstance untouchedBefore = furthest.session.readInstance(whole).value;
  QCOMPARE(untouchedBefore.instanceId, whole);
  QCOMPARE(untouchedBefore.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(untouchedBefore.recordEnd, static_cast<std::int64_t>(100));
  QCOMPARE(untouchedBefore.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(untouchedBefore.sourceEnd, static_cast<std::int64_t>(100));
  QCOMPARE(furthest.session.readInstance(quiet).value.recordEnd, static_cast<std::int64_t>(80));
  QCOMPARE(furthest.session.readInstance(quiet).value.sourceEnd, static_cast<std::int64_t>(80));
  QCOMPARE(storedVisibleEnd(furthest, furthestReel), static_cast<std::int64_t>(100));
  QVERIFY(!furthest.session.extractRange(nullptr, nullptr, nullptr));

  Scratch empty;
  Reel emptyReel;
  QVERIFY(prepare(empty, &emptyReel, "empty-again"));
  std::string head;
  std::string tail;
  QVERIFY(place(empty, emptyReel, emptyReel.trackId, 0, 10, 0, 10, &head));
  QVERIFY(place(empty, emptyReel, emptyReel.trackId, 0, 10, 40, 50, &tail));
  QVERIFY(reopen(empty, emptyReel));
  QVERIFY(empty.session.extract(emptyReel.sequenceId, {emptyReel.trackId}, 20, 30).ok());
  QCOMPARE(empty.session.readInstance(head).value.recordEnd, static_cast<std::int64_t>(10));
  QCOMPARE(empty.session.readInstance(head).value.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(empty.session.readInstance(tail).value.instanceId, tail);
  QCOMPARE(empty.session.readInstance(tail).value.recordStart, static_cast<std::int64_t>(30));
  QCOMPARE(empty.session.readInstance(tail).value.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(empty.session.readInstance(tail).value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(empty.session.readInstance(tail).value.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(storedVisibleEnd(empty, emptyReel), static_cast<std::int64_t>(40));

  Scratch unequal;
  Reel unequalReel;
  QVERIFY(prepare(unequal, &unequalReel, "cross-again"));
  std::string crossing;
  std::string beside;
  QVERIFY(insertAt(unequal, unequalReel, unequalReel.trackId, {24, 1}, 0, 10, 10, 40, &crossing));
  QVERIFY(place(unequal, unequalReel, unequalReel.otherTrackId, 0, 12, 0, 12, &beside));
  QVERIFY(reopen(unequal, unequalReel));
  QVERIFY(unequal.session.setExtractRange(unequalReel.sequenceId, 20, 30).ok());
  const std::int64_t unequalEnd = storedVisibleEnd(unequal, unequalReel);
  auto unequalRefused = unequal.session.extract(unequalReel.sequenceId, {unequalReel.trackId}, 20, 30);
  QVERIFY(!unequalRefused.ok());
  QVERIFY(unequalRefused.error.message.find(unequalReel.trackId) != std::string::npos);
  QVERIFY(unequalRefused.error.message.find(crossing) != std::string::npos);
  QVERIFY(unequalRefused.error.message.find("not one frame per record frame") != std::string::npos);
  QVERIFY(!unequal.session.canUndo());
  QCOMPARE(unequal.session.readInstance(crossing).value.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(unequal.session.readInstance(crossing).value.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(unequal.session.readInstance(beside).value.recordEnd, static_cast<std::int64_t>(12));
  QCOMPARE(storedVisibleEnd(unequal, unequalReel), unequalEnd);
  QVERIFY(unequal.session.extractRange(nullptr, &keptStart, &keptEnd));
  QCOMPARE(keptStart, static_cast<std::int64_t>(20));
  QCOMPARE(keptEnd, static_cast<std::int64_t>(30));

  QVERIFY(exact.session.undo().ok());
  QCOMPARE(exact.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(50));
  QCOMPARE(exact.session.readInstance(later).value.recordEnd, static_cast<std::int64_t>(70));
  QCOMPARE(exact.session.readInstance(later).value.sourceEnd, static_cast<std::int64_t>(20));
  QCOMPARE(exact.session.readInstance(early).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(exact.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(60));
  QCOMPARE(storedVisibleEnd(exact, exactReel), static_cast<std::int64_t>(70));
  QVERIFY(exact.session.redo().ok());
  QCOMPARE(exact.session.readInstance(later).value.instanceId, later);
  QCOMPARE(exact.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(exact.session.readInstance(later).value.recordEnd, static_cast<std::int64_t>(60));
  QCOMPARE(exact.session.readInstance(later).value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(exact.session.readInstance(later).value.sourceEnd, static_cast<std::int64_t>(20));
  QCOMPARE(storedVisibleEnd(exact, exactReel), static_cast<std::int64_t>(60));
  QCOMPARE(*exact.session.recordMarks(exactReel.sequenceId).value.markIn, static_cast<std::int64_t>(2));
  QVERIFY(reopen(exact, exactReel));
  QVERIFY(!exact.session.canUndo());
  QVERIFY(!exact.session.extractRange(nullptr, nullptr, nullptr));
  QVERIFY(!exact.session.recordMarks(exactReel.sequenceId).value.markIn.has_value());
  QCOMPARE(exact.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(exact.session.readInstance(early).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(exact.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(60));
  QCOMPARE(storedVisibleEnd(exact, exactReel), static_cast<std::int64_t>(60));
  QCOMPARE(exact.session.info().schemaVersion, 2);
  QCOMPARE(exact.session.info().prototypeVersion, std::string("slice-0a"));
}

void ExtractTests::identityUndoRedoAndPersistence() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "identity"));
  std::string span;
  std::string later;
  std::string other;
  QVERIFY(place(scratch, reel, reel.trackId, 100, 130, 10, 40, &span));
  QVERIFY(place(scratch, reel, reel.trackId, 0, 16, 50, 66, &later));
  QVERIFY(place(scratch, reel, reel.otherTrackId, 0, 12, 0, 12, &other));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(scratch.session.setSourceMarkIn(reel.representationId, 3).ok());
  QVERIFY(scratch.session.setExtractRange(reel.sequenceId, 20, 30).ok());
  QVERIFY(scratch.session.extract(reel.sequenceId, {reel.trackId}, 20, 30).ok());
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QVERIFY(scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  const aether::TimelineInstance left = scratch.session.readInstance(span).value;
  QCOMPARE(left.recordStart, static_cast<std::int64_t>(10));
  QCOMPARE(left.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(left.sourceStart, static_cast<std::int64_t>(100));
  QCOMPARE(left.sourceEnd, static_cast<std::int64_t>(110));
  auto rows = scratch.session.listInstances(reel.trackId);
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(3));
  std::string minted;
  for (const aether::TimelineInstance& row : rows.value) {
    if (row.instanceId != span && row.instanceId != later) {
      minted = row.instanceId;
      QCOMPARE(row.recordStart, static_cast<std::int64_t>(20));
      QCOMPARE(row.recordEnd, static_cast<std::int64_t>(30));
      QCOMPARE(row.sourceStart, static_cast<std::int64_t>(120));
      QCOMPARE(row.sourceEnd, static_cast<std::int64_t>(130));
      QCOMPARE(row.trackId, reel.trackId);
    }
  }
  QVERIFY(!minted.empty());
  QCOMPARE(scratch.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.readInstance(later).value.recordEnd, static_cast<std::int64_t>(56));
  QCOMPARE(scratch.session.readInstance(later).value.instanceId, later);
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(12));
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(56));

  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  QCOMPARE(scratch.session.readInstance(span).value.recordStart, static_cast<std::int64_t>(10));
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.readInstance(span).value.sourceEnd, static_cast<std::int64_t>(130));
  QCOMPARE(scratch.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(50));
  QVERIFY(!scratch.session.readInstance(minted).ok());
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(2));
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(66));

  QVERIFY(scratch.session.redo().ok());
  QVERIFY(scratch.session.readInstance(minted).ok());
  QCOMPARE(scratch.session.readInstance(minted).value.recordStart, static_cast<std::int64_t>(20));
  QCOMPARE(scratch.session.readInstance(minted).value.sourceStart, static_cast<std::int64_t>(120));
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(56));
  QVERIFY(!scratch.session.canRedo());

  QCOMPARE(*scratch.session.sourceMarks(reel.representationId).value.markIn, static_cast<std::int64_t>(3));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.readInstance(minted).ok());
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(20));
  QCOMPARE(scratch.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(12));
  QCOMPARE(storedVisibleEnd(scratch, reel), static_cast<std::int64_t>(56));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
}

void ExtractTests::collisionAndFailedWrite() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "fail"));
  std::string span;
  QVERIFY(place(scratch, reel, reel.trackId, 0, 30, 10, 40, &span));
  QVERIFY(reopen(scratch, reel));
  scratch.session.storeForTest().debugArmNextMutationFailure();
  QVERIFY(scratch.session.setExtractRange(reel.sequenceId, 20, 30).ok());
  auto failed = scratch.session.extract(reel.sequenceId, {reel.trackId}, 20, 30);
  QVERIFY(!failed.ok());
  QVERIFY(failed.error.message.find("rolled back") != std::string::npos);
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(40));
  std::int64_t start = 0;
  std::int64_t end = 0;
  QVERIFY(scratch.session.extractRange(nullptr, &start, &end));
  QCOMPARE(start, static_cast<std::int64_t>(20));
  QCOMPARE(end, static_cast<std::int64_t>(30));

  scratch.session.debugArmExtractIdentityCollision(span);
  auto collided = scratch.session.extract(reel.sequenceId, {reel.trackId}, 20, 30);
  QVERIFY(!collided.ok());
  QVERIFY(collided.error.message.find("collides") != std::string::npos);
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(span).value.recordStart, static_cast<std::int64_t>(10));
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(1));
}

void ExtractTests::windowExtractsNamedTrack() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepare(scratch, &reel, "window"));
  std::string span;
  std::string later;
  std::string other;
  QVERIFY(place(scratch, reel, reel.trackId, 0, 40, 8, 48, &span));
  QVERIFY(place(scratch, reel, reel.trackId, 0, 16, 56, 72, &later));
  QVERIFY(place(scratch, reel, reel.otherTrackId, 0, 30, 0, 30, &other));
  auto second = scratch.session.createSequence("Second", {24, 1});
  QVERIFY(second.ok());
  QVERIFY(scratch.session.createTrack(second.value.sequenceId).ok());
  QVERIFY(reopen(scratch, reel));
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(reel.sequenceId)));
  scratch.workspace.resize(1280, 860);
  scratch.workspace.show();
  QVERIFY(QTest::qWaitForWindowExposed(&scratch.workspace));

  QVERIFY(scratch.workspace.findChild<QListWidget*>("extractTrackList") == nullptr);
  auto* headers = scratch.workspace.findChild<QWidget*>("timelineTrackHeaders");
  QVERIFY(headers != nullptr);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList());
  auto* visible = scratch.workspace.findChild<QLabel*>("visibleEnd");
  QCOMPARE(visible->text(), QString("Visible end 72"));

  auto* mode = scratch.workspace.findChild<QPushButton*>("extractRangeMode");
  auto* canvas = scratch.workspace.findChild<QWidget*>("timelineCanvas");
  auto* start = scratch.workspace.findChild<QLineEdit*>("extractRangeStart");
  auto* end = scratch.workspace.findChild<QLineEdit*>("extractRangeEnd");
  auto* readout = scratch.workspace.findChild<QLabel*>("extractRangeReadout");
  QVERIFY(mode != nullptr && canvas != nullptr && start != nullptr && end != nullptr && readout != nullptr);
  QTest::mouseClick(mode, Qt::LeftButton);
  QVERIFY(mode->isChecked());
  const QPoint first(aether::kTimelineOriginPx + 24 * aether::kTimelinePixelsPerFrame, 8);
  const QPoint secondClick(aether::kTimelineOriginPx + 40 * aether::kTimelinePixelsPerFrame, 40);
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, first);
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, secondClick);
  QCOMPARE(start->text(), QString("24"));
  QCOMPARE(end->text(), QString("40"));
  QVERIFY(readout->text().contains("Start 24 inclusive. End 40 exclusive. D 16."));
  QVERIFY(scratch.workspace.findChild<QLineEdit*>("recordMarkFrame")->text().isEmpty());
  QCOMPARE(scratch.workspace.selectedInstanceId(), QString());
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList());
  std::string sequenceId;
  std::int64_t storedStart = 0;
  std::int64_t storedEnd = 0;
  QVERIFY(scratch.session.extractRange(&sequenceId, &storedStart, &storedEnd));
  QCOMPARE(storedStart, static_cast<std::int64_t>(24));
  QCOMPARE(storedEnd, static_cast<std::int64_t>(40));

  auto* extract = scratch.workspace.findChild<QPushButton*>("extractApply");
  QTest::mouseClick(extract, Qt::LeftButton);
  QVERIFY(scratch.workspace.rejectionText().contains("at least one track"));
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(48));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.extractRange(nullptr, &storedStart, &storedEnd));
  QCOMPARE(storedStart, static_cast<std::int64_t>(24));
  QCOMPARE(storedEnd, static_cast<std::int64_t>(40));

  QTest::mouseClick(mode, Qt::LeftButton);
  QVERIFY(!mode->isChecked());
  const QPoint block = scratch.workspace.blockCenter(QString::fromStdString(span));
  QVERIFY(!block.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, block);
  QCOMPARE(scratch.workspace.selectedInstanceId(), QString::fromStdString(span));
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList());

  // Name text is painted at x 46..142. The first row starts under the 28px ruler.
  const QPoint trackName(94, 52);
  const QPoint otherName(94, 100);
  const QPoint viewMark(20, 52);
  const QPoint enableMark(160, 52);
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, viewMark);
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, enableMark);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList());
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, trackName);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList{QString::fromStdString(reel.trackId)});
  QCOMPARE(scratch.workspace.selectedInstanceId(), QString::fromStdString(span));
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, otherName);
  const QStringList bothTracks{QString::fromStdString(reel.trackId), QString::fromStdString(reel.otherTrackId)};
  QCOMPARE(scratch.workspace.extractParticipantIds(), bothTracks);
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, otherName);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList{QString::fromStdString(reel.trackId)});
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, block);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList{QString::fromStdString(reel.trackId)});
  QTest::mouseClick(extract, Qt::LeftButton);
  QCOMPARE(scratch.workspace.rejectionText(), QString());
  QVERIFY(readout->text().contains("cleared"));
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(24));
  QCOMPARE(scratch.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.readInstance(later).value.instanceId, later);
  QCOMPARE(scratch.session.readInstance(other).value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(30));
  QCOMPARE(visible->text(), QString("Visible end 56"));
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(3));

  QVERIFY(scratch.session.undo().ok());
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(reel.sequenceId)));
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(48));
  QCOMPARE(visible->text(), QString("Visible end 72"));
  QVERIFY(scratch.session.redo().ok());
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(reel.sequenceId)));
  QCOMPARE(scratch.session.readInstance(later).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(visible->text(), QString("Visible end 56"));

  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(second.value.sequenceId)));
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QCOMPARE(start->text(), QString());
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList());

  QVERIFY(reopen(scratch, reel));
  scratch.workspace.reload();
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(span).value.recordEnd, static_cast<std::int64_t>(24));
  QCOMPARE(scratch.session.readInstance(other).value.recordEnd, static_cast<std::int64_t>(30));
}
