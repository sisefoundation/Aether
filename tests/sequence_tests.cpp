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

#include "sequence_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/core/timing/sample_boundary.hpp"
#include "aether/persistence/project_store.hpp"

#include <QTest>

#include <sqlite3.h>

#include <filesystem>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    directory = std::filesystem::temp_directory_path() / ("aether-slice1a-" + (id ? *id : std::string("tmp")));
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
  }

  int countRows(const char* table) {
    sqlite3* db = nullptr;
    if (sqlite3_open_v2((directory / "project.db").c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
      return -1;
    }
    const std::string sql = std::string("SELECT COUNT(*) FROM ") + table + ";";
    sqlite3_stmt* statement = nullptr;
    int count = -1;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr) == SQLITE_OK &&
        sqlite3_step(statement) == SQLITE_ROW) {
      count = sqlite3_column_int(statement, 0);
    }
    sqlite3_finalize(statement);
    sqlite3_close(db);
    return count;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path directory;
};

}  // namespace

void SequenceTests::modelRejectsUnreducedAndEmptySpans() {
  QVERIFY(aether::isAcceptedFrameRate({24, 1}));
  QVERIFY(aether::isAcceptedFrameRate({25, 1}));
  QVERIFY(aether::isAcceptedFrameRate({24000, 1001}));
  QVERIFY(aether::isAcceptedFrameRate({30000, 1001}));
  QVERIFY(aether::isAcceptedFrameRate({60000, 1001}));
  QVERIFY(!aether::isAcceptedFrameRate({48000, 2002}));
  QVERIFY(!aether::isAcceptedFrameRate({30, 1}));
  QVERIFY(aether::isHalfOpenFrameSpan(0, 8));
  QVERIFY(!aether::isHalfOpenFrameSpan(8, 8));
  QVERIFY(!aether::isHalfOpenFrameSpan(-1, 4));
  QVERIFY(!aether::recordRangesOverlap(0, 10, 10, 20));
  QVERIFY(aether::recordRangesOverlap(0, 10, 9, 12));
}

void SequenceTests::operationsMoveTrimRemoveAndRejectOverlap() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Cut").ok());
  auto bin = scratch.session.createBin("Bin");
  auto asset = scratch.session.createAsset("picture", "{}");
  QVERIFY(bin.ok());
  QVERIFY(asset.ok());
  auto clip = scratch.session.createMasterClip("Shot", asset.value.mediaId, bin.value.binId, "{}");
  QVERIFY(clip.ok());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  QVERIFY(sequence.ok());
  const std::string sequenceId = sequence.value.sequenceId;
  auto renamed = scratch.session.renameSequence(sequenceId, "Main Cut");
  QVERIFY(renamed.ok());
  auto reread = scratch.session.readSequence(sequenceId);
  QVERIFY(reread.ok());
  QCOMPARE(reread.value.sequenceId, sequenceId);
  QCOMPARE(reread.value.name, std::string("Main Cut"));
  auto badName = scratch.session.renameSequence(sequenceId, "");
  QVERIFY(!badName.ok());
  QCOMPARE(scratch.session.readSequence(sequenceId).value.name, std::string("Main Cut"));

  auto first = scratch.session.createTrack(sequenceId);
  auto second = scratch.session.createTrack(sequenceId);
  QVERIFY(first.ok());
  QVERIFY(second.ok());
  QVERIFY(scratch.session.orderTrack(second.value.trackId, 0).ok());
  auto tracks = scratch.session.listTracks(sequenceId);
  QVERIFY(tracks.ok());
  QCOMPARE(tracks.value.size(), static_cast<std::size_t>(2));
  QCOMPARE(tracks.value[0].trackId, second.value.trackId);
  QCOMPARE(tracks.value[1].trackId, first.value.trackId);

  auto placed = scratch.session.insertInstance(sequenceId, first.value.trackId, clip.value.objectId, {24, 1}, 0, 10, 0, 10);
  QVERIFY(placed.ok());
  QVERIFY(placed.value.instanceId != clip.value.objectId);
  auto abut = scratch.session.insertInstance(sequenceId, first.value.trackId, clip.value.objectId, {24, 1}, 10, 20, 10, 20);
  QVERIFY(abut.ok());
  QVERIFY(abut.value.instanceId != placed.value.instanceId);
  auto overlap = scratch.session.insertInstance(sequenceId, first.value.trackId, clip.value.objectId, {24, 1}, 0, 5, 5, 15);
  QVERIFY(!overlap.ok());
  auto still = scratch.session.readInstance(placed.value.instanceId);
  QVERIFY(still.ok());
  QCOMPARE(still.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(still.value.recordEnd, static_cast<std::int64_t>(10));

  QVERIFY(scratch.session.moveInstance(placed.value.instanceId, 20).ok());
  auto moved = scratch.session.readInstance(placed.value.instanceId);
  QVERIFY(moved.ok());
  QCOMPARE(moved.value.recordStart, static_cast<std::int64_t>(20));
  QCOMPARE(moved.value.recordEnd, static_cast<std::int64_t>(30));
  QCOMPARE(moved.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(moved.value.sourceEnd, static_cast<std::int64_t>(10));

  QVERIFY(scratch.session.trimInstance(placed.value.instanceId, 2, 8, 20, 26).ok());
  auto trimmed = scratch.session.readInstance(placed.value.instanceId);
  QVERIFY(trimmed.ok());
  QCOMPARE(trimmed.value.sourceStart, static_cast<std::int64_t>(2));
  QCOMPARE(trimmed.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(trimmed.value.recordStart, static_cast<std::int64_t>(20));
  QCOMPARE(trimmed.value.recordEnd, static_cast<std::int64_t>(26));

  QVERIFY(scratch.session.removeInstance(abut.value.instanceId).ok());
  QVERIFY(!scratch.session.readInstance(abut.value.instanceId).ok());
  QVERIFY(scratch.session.readInstance(placed.value.instanceId).ok());
}

void SequenceTests::persistenceReopenKeepsBothRanges() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Hold").ok());
  auto bin = scratch.session.createBin("Bin");
  auto asset = scratch.session.createAsset("picture", "{}");
  auto clip = scratch.session.createMasterClip("Shot", asset.value.mediaId, bin.value.binId, "{}");
  auto sequence = scratch.session.createSequence("Main", {24000, 1001});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, clip.value.objectId,
                                               {25, 1}, 0, 25, 0, 24);
  QVERIFY(placed.ok());
  const std::string instanceId = placed.value.instanceId;
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.directory).ok());
  auto reread = scratch.session.readInstance(instanceId);
  QVERIFY(reread.ok());
  QCOMPARE(reread.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(reread.value.sourceEnd, static_cast<std::int64_t>(25));
  QCOMPARE(reread.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(reread.value.recordEnd, static_cast<std::int64_t>(24));
  QCOMPARE(reread.value.sourceRate.numerator, static_cast<std::int64_t>(25));
  QCOMPARE(scratch.session.readSequence(sequence.value.sequenceId).value.rate.denominator, static_cast<std::int64_t>(1001));
}

void SequenceTests::identitySurvivesPathAndSecondRepresentation() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Identity").ok());
  auto bin = scratch.session.createBin("Bin");
  auto asset = scratch.session.createAsset("picture", "{}");
  QVERIFY(asset.ok());
  auto original = scratch.session.addRepresentation(asset.value.mediaId, aether::RepresentationRole::Original,
                                                    "/media/original.mov", "online");
  QVERIFY(original.ok());
  auto clip = scratch.session.createMasterClip("Shot", asset.value.mediaId, bin.value.binId, "{}");
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto first = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, clip.value.objectId,
                                              {24, 1}, 0, 4, 0, 4);
  auto second = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, clip.value.objectId,
                                               {24, 1}, 4, 8, 8, 12);
  QVERIFY(first.ok());
  QVERIFY(second.ok());
  QVERIFY(first.value.instanceId != second.value.instanceId);
  QCOMPARE(first.value.mediaId, asset.value.mediaId);
  QCOMPARE(second.value.mediaId, asset.value.mediaId);
  QCOMPARE(scratch.countRows("media_assets"), 1);
  QCOMPARE(scratch.countRows("master_clips"), 1);

  QVERIFY(scratch.session.setRepresentationPath(original.value.representationId, "/media/renamed.mov").ok());
  auto reread = scratch.session.readInstance(first.value.instanceId);
  QCOMPARE(reread.value.mediaId, asset.value.mediaId);
  QCOMPARE(reread.value.masterObjectId, clip.value.objectId);
  auto proxy = scratch.session.addRepresentation(asset.value.mediaId, aether::RepresentationRole::Proxy,
                                                 "/media/proxy.mov", "online");
  QVERIFY(proxy.ok());
  auto after = scratch.session.readInstance(first.value.instanceId);
  QCOMPARE(after.value.mediaId, asset.value.mediaId);
  QCOMPARE(after.value.masterObjectId, clip.value.objectId);
  QCOMPARE(scratch.session.listRepresentations(asset.value.mediaId).value.size(), static_cast<std::size_t>(2));
  QCOMPARE(scratch.countRows("media_assets"), 1);
}

void SequenceTests::timeSelectionAndPublishedSampleCounts() {
  const aether::FrameRate sequence{24000, 1001};
  const aether::FrameRate source{25, 1};
  auto tie = aether::pictureSourceOffset(480, sequence, source);
  QVERIFY(tie.ok());
  QCOMPARE(tie.value, static_cast<std::int64_t>(500));
  const aether::FrameRate rate{30000, 1001};
  auto eight = aether::sampleCount(0, 8, 48000, rate);
  auto next = aether::sampleCount(8, 16, 48000, rate);
  auto longSpan = aether::sampleCount(0, 200000, 48000, rate);
  QVERIFY(eight.ok());
  QVERIFY(next.ok());
  QVERIFY(longSpan.ok());
  QCOMPARE(eight.value, static_cast<std::int64_t>(12812));
  QCOMPARE(next.value, static_cast<std::int64_t>(12813));
  QCOMPARE(longSpan.value, static_cast<std::int64_t>(320320000));
}

void SequenceTests::undoCloseReopenKeepsCommittedEdit() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Undo").ok());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  QVERIFY(sequence.ok());
  QVERIFY(scratch.session.renameSequence(sequence.value.sequenceId, "Renamed").ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readSequence(sequence.value.sequenceId).value.name, std::string("Renamed"));
  QVERIFY(scratch.session.undo().error.code == aether::ErrorCode::NothingToUndo);
  QVERIFY(scratch.session.renameSequence(sequence.value.sequenceId, "Again").ok());
  QVERIFY(scratch.session.undo().ok());
  QCOMPARE(scratch.session.readSequence(sequence.value.sequenceId).value.name, std::string("Renamed"));
  QVERIFY(scratch.session.redo().ok());
  QCOMPARE(scratch.session.readSequence(sequence.value.sequenceId).value.name, std::string("Again"));
}

void SequenceTests::migrateVersionOnePreservesBinsAndMedia() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Legacy").ok());
  auto bin = scratch.session.createBin("Bin");
  auto asset = scratch.session.createAsset("picture", "{}");
  auto clip = scratch.session.createMasterClip("Shot", asset.value.mediaId, bin.value.binId, "{}");
  QVERIFY(bin.ok());
  QVERIFY(clip.ok());
  QVERIFY(scratch.session.storeForTest().debugDowngradeEmptySequenceSchema().ok());
  QVERIFY(scratch.session.closeProject().ok());
  auto version = scratch.session.storeForTest().debugReadSchemaVersion(scratch.directory);
  QVERIFY(version.ok());
  QCOMPARE(version.value, 1);
  auto opened = scratch.session.openProject(scratch.directory);
  QVERIFY(opened.ok());
  QCOMPARE(opened.value.schemaVersion, 2);
  auto bins = scratch.session.listBins();
  QVERIFY(bins.ok());
  QCOMPARE(bins.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(bins.value[0].name, std::string("Bin"));
  QCOMPARE(scratch.session.readMasterClip(clip.value.objectId).value.mediaId, asset.value.mediaId);
  QVERIFY(scratch.session.listSequences().value.empty());
}

void SequenceTests::migrationFailureRollsBackWithoutDeleting() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.directory, "Rollback").ok());
  auto bin = scratch.session.createBin("Kept");
  QVERIFY(bin.ok());
  QVERIFY(scratch.session.storeForTest().debugDowngradeEmptySequenceSchema().ok());
  scratch.session.storeForTest().debugArmMigrationFailure();
  QVERIFY(scratch.session.closeProject().ok());
  auto refused = scratch.session.openProject(scratch.directory);
  QVERIFY(!refused.ok());
  QCOMPARE(static_cast<int>(refused.error.code), static_cast<int>(aether::ErrorCode::Database));
  QVERIFY(std::filesystem::is_regular_file(scratch.directory / "project.db"));
  auto version = scratch.session.storeForTest().debugReadSchemaVersion(scratch.directory);
  QVERIFY(version.ok());
  QCOMPARE(version.value, 1);
  auto opened = scratch.session.openProject(scratch.directory);
  QVERIFY(opened.ok());
  QCOMPARE(scratch.session.listBins().value.size(), static_cast<std::size_t>(1));
  QCOMPARE(scratch.session.listBins().value[0].name, std::string("Kept"));
}
