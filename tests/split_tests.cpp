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

#include "split_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/persistence/project_store.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTest>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <set>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    parent = std::filesystem::temp_directory_path() / ("aether-split-" + token);
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
    output << "split-bytes";
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
};

struct Cut {
  std::filesystem::path directory;
  std::string representationId;
  std::string objectId;
  std::string mediaId;
  std::string sequenceId;
  std::string trackId;
  std::string otherTrackId;
  std::string instanceId;
  std::string otherInstanceId;
  std::string laterId;
};

bool prepare(Scratch& scratch, Cut* cut) {
  cut->directory = scratch.parent / "Cut";
  if (!scratch.session.createProject(cut->directory, "Cut").ok()) {
    return false;
  }
  const auto media = scratch.writeMedia("shot.mov");
  auto imported = scratch.session.importReferences({media.string()});
  if (!imported.ok() || imported.value.size() != 1) {
    return false;
  }
  cut->representationId = imported.value.front().representationId;
  cut->objectId = imported.value.front().objectId;
  cut->mediaId = imported.value.front().mediaId;
  auto sequence = scratch.session.createSequence("Reel", {24, 1});
  if (!sequence.ok()) {
    return false;
  }
  cut->sequenceId = sequence.value.sequenceId;
  auto track = scratch.session.createTrack(cut->sequenceId);
  auto other = scratch.session.createTrack(cut->sequenceId);
  if (!track.ok() || !other.ok()) {
    return false;
  }
  cut->trackId = track.value.trackId;
  cut->otherTrackId = other.value.trackId;
  auto placed = scratch.session.placeInstance(cut->sequenceId, cut->trackId, cut->objectId, {24, 1}, 10, 58, 0, 48);
  auto held = scratch.session.placeInstance(cut->sequenceId, cut->otherTrackId, cut->objectId, {24, 1}, 5, 53, 0, 48);
  auto later = scratch.session.placeInstance(cut->sequenceId, cut->trackId, cut->objectId, {24, 1}, 0, 12, 60, 72);
  if (!placed.ok() || !held.ok() || !later.ok()) {
    return false;
  }
  cut->instanceId = placed.value.instanceId;
  cut->otherInstanceId = held.value.instanceId;
  cut->laterId = later.value.instanceId;
  return true;
}

const aether::TimelineInstance* findRecord(const std::vector<aether::TimelineInstance>& rows, std::int64_t start) {
  for (const aether::TimelineInstance& row : rows) {
    if (row.recordStart == start) {
      return &row;
    }
  }
  return nullptr;
}

std::set<std::int64_t> frames(std::int64_t start, std::int64_t end) {
  std::set<std::int64_t> values;
  for (std::int64_t frame = start; frame < end; ++frame) {
    values.insert(frame);
  }
  return values;
}

}  // namespace

void SplitTests::boundaryAndMappingRefused() {
  aether::TimelineInstance row;
  row.instanceId = "kept";
  row.sourceRate = {48, 2};
  row.sourceStart = 10;
  row.sourceEnd = 58;
  row.recordStart = 0;
  row.recordEnd = 48;
  auto reduced = aether::planSplit(row, {24, 1}, 16);
  QVERIFY(reduced.ok());
  QCOMPARE(reduced.value.left.instanceId, std::string("kept"));
  QVERIFY(reduced.value.right.instanceId.empty());
  QCOMPARE(reduced.value.left.sourceEnd, static_cast<std::int64_t>(26));
  QCOMPARE(reduced.value.right.sourceStart, static_cast<std::int64_t>(26));
  QCOMPARE(reduced.value.left.recordEnd, static_cast<std::int64_t>(16));
  QCOMPARE(reduced.value.right.recordStart, static_cast<std::int64_t>(16));

  auto atStart = aether::planSplit(row, {24, 1}, 0);
  QVERIFY(!atStart.ok());
  QCOMPARE(atStart.error.message, std::string("a split at the start or end is rejected"));
  auto atEnd = aether::planSplit(row, {24, 1}, 48);
  QVERIFY(!atEnd.ok());
  QCOMPARE(atEnd.error.message, std::string("a split at the start or end is rejected"));
  auto outside = aether::planSplit(row, {24, 1}, 49);
  QVERIFY(!outside.ok());
  QCOMPARE(outside.error.message, std::string("split frame is not inside the record span"));

  row.sourceRate = {24, 1};
  row.sourceEnd = 20;
  auto unequal = aether::planSplit(row, {24, 1}, 16);
  QVERIFY(!unequal.ok());
  QCOMPARE(unequal.error.message, std::string("source span is not one frame per record frame"));
  row.sourceEnd = 58;
  row.sourceRate = {25, 1};
  auto mixed = aether::planSplit(row, {24, 1}, 16);
  QVERIFY(!mixed.ok());
  QCOMPARE(mixed.error.message, std::string("source span is not one frame per record frame"));

  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  const auto before = scratch.session.readInstance(cut.instanceId);
  QVERIFY(before.ok());

  auto start = scratch.session.split(cut.instanceId, 0);
  QVERIFY(!start.ok());
  QCOMPARE(start.error.message, std::string("a split at the start or end is rejected"));
  auto end = scratch.session.split(cut.instanceId, 48);
  QVERIFY(!end.ok());
  auto missing = scratch.session.split("missing-instance", 16);
  QVERIFY(!missing.ok());
  QVERIFY(!scratch.session.canUndo());
  auto still = scratch.session.readInstance(cut.instanceId);
  QCOMPARE(still.value.sourceStart, before.value.sourceStart);
  QCOMPARE(still.value.sourceEnd, before.value.sourceEnd);
  QCOMPARE(still.value.recordStart, before.value.recordStart);
  QCOMPARE(still.value.recordEnd, before.value.recordEnd);
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(2));

  QVERIFY(scratch.session.trimInstance(cut.instanceId, 0, 10, 0, 20).ok());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  auto trimmed = scratch.session.split(cut.instanceId, 10);
  QVERIFY(!trimmed.ok());
  QCOMPARE(trimmed.error.message, std::string("source span is not one frame per record frame"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.sourceEnd, static_cast<std::int64_t>(10));
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.recordEnd, static_cast<std::int64_t>(20));

  auto mixedSequence = scratch.session.createSequence("Mixed", {24000, 1001});
  auto mixedTrack = scratch.session.createTrack(mixedSequence.value.sequenceId);
  auto mixedRow = scratch.session.insertInstance(mixedSequence.value.sequenceId, mixedTrack.value.trackId, cut.objectId,
                                                 {25, 1}, 0, 25, 0, 24);
  QVERIFY(mixedRow.ok());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  auto refused = scratch.session.split(mixedRow.value.instanceId, 12);
  QVERIFY(!refused.ok());
  QCOMPARE(refused.error.message, std::string("source span is not one frame per record frame"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(mixedRow.value.instanceId).value.sourceEnd, static_cast<std::int64_t>(25));
  QCOMPARE(scratch.session.readInstance(mixedRow.value.instanceId).value.recordEnd, static_cast<std::int64_t>(24));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));
}

void SplitTests::interiorContinuityIdentity() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  auto created = scratch.session.split(cut.instanceId, 16);
  QVERIFY(created.ok());
  QCOMPARE(created.value.instanceId == cut.instanceId, false);
  QCOMPARE(created.value.recordStart, static_cast<std::int64_t>(16));
  auto rows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(3));
  const auto* left = findRecord(rows.value, 0);
  const auto* right = findRecord(rows.value, 16);
  const auto* later = findRecord(rows.value, 60);
  QVERIFY(left != nullptr);
  QVERIFY(right != nullptr);
  QVERIFY(later != nullptr);
  QCOMPARE(left->instanceId, cut.instanceId);
  QCOMPARE(right->instanceId, created.value.instanceId);
  QCOMPARE(left->instanceId == right->instanceId, false);
  QCOMPARE(left->sourceStart, static_cast<std::int64_t>(10));
  QCOMPARE(left->sourceEnd, static_cast<std::int64_t>(26));
  QCOMPARE(left->recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(left->recordEnd, static_cast<std::int64_t>(16));
  QCOMPARE(right->sourceStart, static_cast<std::int64_t>(26));
  QCOMPARE(right->sourceEnd, static_cast<std::int64_t>(58));
  QCOMPARE(right->recordStart, static_cast<std::int64_t>(16));
  QCOMPARE(right->recordEnd, static_cast<std::int64_t>(48));
  QCOMPARE(left->recordEnd, right->recordStart);
  QCOMPARE(left->sourceEnd, right->sourceStart);
  QCOMPARE(left->mediaId, cut.mediaId);
  QCOMPARE(right->mediaId, cut.mediaId);
  QCOMPARE(left->masterObjectId, cut.objectId);
  QCOMPARE(right->masterObjectId, cut.objectId);
  QCOMPARE(left->trackId, cut.trackId);
  QCOMPARE(right->trackId, cut.trackId);
  QCOMPARE(left->sourceRate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(right->sourceRate.denominator, static_cast<std::int64_t>(1));
  const auto leftSource = frames(left->sourceStart, left->sourceEnd);
  const auto rightSource = frames(right->sourceStart, right->sourceEnd);
  const auto leftRecord = frames(left->recordStart, left->recordEnd);
  const auto rightRecord = frames(right->recordStart, right->recordEnd);
  std::set<std::int64_t> sourceOverlap;
  std::set_intersection(leftSource.begin(), leftSource.end(), rightSource.begin(), rightSource.end(),
                        std::inserter(sourceOverlap, sourceOverlap.end()));
  std::set<std::int64_t> recordOverlap;
  std::set_intersection(leftRecord.begin(), leftRecord.end(), rightRecord.begin(), rightRecord.end(),
                        std::inserter(recordOverlap, recordOverlap.end()));
  QVERIFY(sourceOverlap.empty());
  QVERIFY(recordOverlap.empty());
  QCOMPARE(leftSource.size() + rightSource.size(), static_cast<std::size_t>(48));
  QCOMPARE(leftRecord.size() + rightRecord.size(), static_cast<std::size_t>(48));
  QCOMPARE(*leftSource.begin(), static_cast<std::int64_t>(10));
  QCOMPARE(*leftSource.rbegin(), static_cast<std::int64_t>(25));
  QCOMPARE(*rightSource.rbegin(), static_cast<std::int64_t>(57));
  QCOMPARE(later->instanceId, cut.laterId);
  QCOMPARE(later->recordStart, static_cast<std::int64_t>(60));
  QCOMPARE(later->recordEnd, static_cast<std::int64_t>(72));
  QCOMPARE(later->sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(later->sourceEnd, static_cast<std::int64_t>(12));
}

void SplitTests::otherTracksDownstreamMarks() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 3).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 9).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 4).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 30).ok());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 3).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 9).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 4).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 30).ok());
  auto created = scratch.session.split(cut.instanceId, 16);
  QVERIFY(created.ok());
  auto other = scratch.session.readInstance(cut.otherInstanceId);
  QVERIFY(other.ok());
  QCOMPARE(other.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(other.value.recordEnd, static_cast<std::int64_t>(48));
  QCOMPARE(other.value.sourceStart, static_cast<std::int64_t>(5));
  QCOMPARE(other.value.sourceEnd, static_cast<std::int64_t>(53));
  QCOMPARE(other.value.trackId, cut.otherTrackId);
  auto later = scratch.session.readInstance(cut.laterId);
  QCOMPARE(later.value.recordStart, static_cast<std::int64_t>(60));
  QCOMPARE(later.value.recordEnd, static_cast<std::int64_t>(72));
  auto sourceMarks = scratch.session.sourceMarks(cut.representationId);
  auto recordMarks = scratch.session.recordMarks(cut.sequenceId);
  QCOMPARE(*sourceMarks.value.markIn, static_cast<std::int64_t>(3));
  QCOMPARE(*sourceMarks.value.markOut, static_cast<std::int64_t>(9));
  QCOMPARE(*recordMarks.value.markIn, static_cast<std::int64_t>(4));
  QCOMPARE(*recordMarks.value.markOut, static_cast<std::int64_t>(30));
  auto sequence = scratch.session.readSequence(cut.sequenceId);
  QCOMPARE(sequence.value.name, std::string("Reel"));
  QCOMPARE(sequence.value.rate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(sequence.value.rate.denominator, static_cast<std::int64_t>(1));
  std::int64_t maximum = 0;
  for (const aether::TimelineInstance& row : scratch.session.listInstances(cut.trackId).value) {
    maximum = std::max(maximum, row.recordEnd);
  }
  QCOMPARE(maximum, static_cast<std::int64_t>(72));
  auto tables = scratch.session.storeForTest().listUserTables();
  QVERIFY(tables.ok());
  QCOMPARE(tables.value.size(), static_cast<std::size_t>(11));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}

void SplitTests::oneUndoRedoReopen() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  auto created = scratch.session.split(cut.instanceId, 16);
  QVERIFY(created.ok());
  const std::string newId = created.value.instanceId;
  QVERIFY(scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  auto restored = scratch.session.listInstances(cut.trackId);
  QCOMPARE(restored.value.size(), static_cast<std::size_t>(2));
  auto original = scratch.session.readInstance(cut.instanceId);
  QVERIFY(original.ok());
  QCOMPARE(original.value.sourceStart, static_cast<std::int64_t>(10));
  QCOMPARE(original.value.sourceEnd, static_cast<std::int64_t>(58));
  QCOMPARE(original.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(original.value.recordEnd, static_cast<std::int64_t>(48));
  QVERIFY(!scratch.session.readInstance(newId).ok());
  QVERIFY(scratch.session.redo().ok());
  auto again = scratch.session.readInstance(newId);
  QVERIFY(again.ok());
  QCOMPARE(again.value.sourceStart, static_cast<std::int64_t>(26));
  QCOMPARE(again.value.recordStart, static_cast<std::int64_t>(16));
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.recordEnd, static_cast<std::int64_t>(16));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  QVERIFY(scratch.session.readInstance(newId).ok());
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.sourceEnd, static_cast<std::int64_t>(26));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));
  QVERIFY(scratch.session.undo().error.code == aether::ErrorCode::NothingToUndo);
}

void SplitTests::failedWriteAddsNoUndo() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(scratch.session.split(cut.instanceId, 16).ok());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(scratch.session.canRedo());
  auto refused = scratch.session.split(cut.instanceId, 0);
  QVERIFY(!refused.ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.recordEnd, static_cast<std::int64_t>(48));

  scratch.session.storeForTest().debugArmNextMutationFailure();
  auto armed = scratch.session.split(cut.instanceId, 16);
  QVERIFY(!armed.ok());
  QCOMPARE(armed.error.message, std::string("write failed and was rolled back"));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(2));
  QVERIFY(scratch.session.redo().ok());
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.recordEnd, static_cast<std::int64_t>(16));
}

void SplitTests::windowSplitUsesSession() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 8).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 20).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1680, 1200);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QVERIFY(workspace != nullptr);
  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QVERIFY(sequences != nullptr);
  QVERIFY(sequences->count() > 0);
  QTest::mouseClick(sequences->viewport(), Qt::LeftButton, Qt::NoModifier,
                    sequences->visualItemRect(sequences->item(0)).center());
  auto* instances = window.findChild<QListWidget*>("instanceList");
  QVERIFY(instances != nullptr);
  int instanceRow = -1;
  for (int index = 0; index < instances->count(); ++index) {
    if (instances->item(index)->text().contains("track 0 start 0")) {
      instanceRow = index;
    }
  }
  QVERIFY(instanceRow >= 0);
  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  const QString picked = instances->item(instanceRow)->data(Qt::UserRole).toString();
  const QPoint point = workspace->blockCenter(picked);
  QVERIFY(!point.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, point);
  QCOMPARE(workspace->selectedInstanceId().toStdString(), cut.instanceId);

  auto* frame = window.findChild<QLineEdit*>("splitFrame");
  auto* shown = window.findChild<QLabel*>("splitFrameShown");
  auto* button = window.findChild<QPushButton*>("splitApply");
  auto* recordFrame = window.findChild<QLineEdit*>("recordMarkFrame");
  QVERIFY(frame != nullptr);
  QVERIFY(shown != nullptr);
  QVERIFY(button != nullptr);
  QVERIFY(button->text() == "Split");
  recordFrame->setText("8");
  frame->setText("0");
  QCOMPARE(shown->text(), QString("0"));
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(workspace->rejectionText().contains("start or end"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(2));

  frame->setText("16");
  QCOMPARE(shown->text(), QString("16"));
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(workspace->rejectionText().isEmpty());
  auto rows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(3));
  const auto* left = findRecord(rows.value, 0);
  const auto* right = findRecord(rows.value, 16);
  QVERIFY(left != nullptr);
  QVERIFY(right != nullptr);
  QCOMPARE(left->instanceId, cut.instanceId);
  QCOMPARE(left->sourceEnd, static_cast<std::int64_t>(26));
  QCOMPARE(right->sourceStart, static_cast<std::int64_t>(26));
  QCOMPARE(right->recordStart, static_cast<std::int64_t>(16));
  QVERIFY(workspace->inspectorText().contains("record [0,16)"));
  QVERIFY(workspace->inspectorText().contains("source [10,26)"));
  auto marks = scratch.session.recordMarks(cut.sequenceId);
  QCOMPARE(*marks.value.markIn, static_cast<std::int64_t>(8));
  QCOMPARE(*marks.value.markOut, static_cast<std::int64_t>(20));
  QVERIFY(scratch.session.canUndo());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}
