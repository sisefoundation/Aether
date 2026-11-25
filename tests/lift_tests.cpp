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

#include "lift_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/persistence/project_store.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QListWidget>
#include <QPushButton>
#include <QTest>

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    parent = std::filesystem::temp_directory_path() / ("aether-lift-" + token);
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
    output << "lift-bytes";
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
};

struct Reel {
  std::filesystem::path directory;
  std::string representationId;
  std::string objectId;
  std::string mediaId;
  std::string sequenceId;
  std::string trackId;
  std::string otherTrackId;
  std::string firstId;
  std::string middleId;
  std::string lastId;
  std::string otherId;
};

bool place(Scratch& scratch, Reel* reel, const std::string& trackId, std::int64_t sourceStart, std::int64_t sourceEnd,
           std::int64_t recordStart, std::int64_t recordEnd, std::string* id) {
  auto placed = scratch.session.placeInstance(reel->sequenceId, trackId, reel->objectId, {24, 1}, sourceStart, sourceEnd,
                                              recordStart, recordEnd);
  if (!placed.ok()) {
    return false;
  }
  *id = placed.value.instanceId;
  return true;
}

bool prepareTrio(Scratch& scratch, Reel* reel, const std::string& folder) {
  scratch.session.closeProject();
  reel->directory = scratch.parent / folder;
  if (!scratch.session.createProject(reel->directory, "LiftCut").ok()) {
    return false;
  }
  const auto media = scratch.writeMedia(folder + ".mov");
  auto imported = scratch.session.importReferences({media.string()});
  if (!imported.ok() || imported.value.size() != 1) {
    return false;
  }
  reel->representationId = imported.value.front().representationId;
  reel->objectId = imported.value.front().objectId;
  reel->mediaId = imported.value.front().mediaId;
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
  if (!place(scratch, reel, reel->trackId, 0, 12, 0, 12, &reel->firstId) ||
      !place(scratch, reel, reel->trackId, 0, 12, 12, 24, &reel->middleId) ||
      !place(scratch, reel, reel->trackId, 0, 12, 24, 36, &reel->lastId) ||
      !place(scratch, reel, reel->otherTrackId, 0, 36, 0, 36, &reel->otherId)) {
    return false;
  }
  return true;
}

bool reopen(Scratch& scratch, const Reel& reel) {
  return scratch.session.closeProject().ok() && scratch.session.openProject(reel.directory).ok();
}

const aether::TimelineInstance* findId(const std::vector<aether::TimelineInstance>& rows, const std::string& id) {
  for (const aether::TimelineInstance& row : rows) {
    if (row.instanceId == id) {
      return &row;
    }
  }
  return nullptr;
}

bool intersects(const std::vector<aether::TimelineInstance>& rows, std::int64_t start, std::int64_t end) {
  for (const aether::TimelineInstance& row : rows) {
    if (row.recordStart < end && start < row.recordEnd) {
      return true;
    }
  }
  return false;
}

std::int64_t furthest(const std::vector<aether::TimelineInstance>& rows) {
  std::int64_t end = 0;
  for (const aether::TimelineInstance& row : rows) {
    if (row.recordEnd > end) {
      end = row.recordEnd;
    }
  }
  return end;
}

void expectHeld(const aether::TimelineInstance& row, const std::string& id, const std::string& track,
                std::int64_t recordStart, std::int64_t recordEnd) {
  QCOMPARE(row.instanceId, id);
  QCOMPARE(row.trackId, track);
  QCOMPARE(row.recordStart, recordStart);
  QCOMPARE(row.recordEnd, recordEnd);
  QCOMPARE(row.sourceEnd - row.sourceStart, recordEnd - recordStart);
}

}  // namespace

void LiftTests::selectionRefused() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepareTrio(scratch, &reel, "Select"));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(!scratch.session.canUndo());

  auto missing = scratch.session.lift("missing-instance");
  QVERIFY(!missing.ok());
  QCOMPARE(missing.error.message, std::string("instance was not found"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(3));

  auto blank = scratch.session.lift("");
  QVERIFY(!blank.ok());
  QCOMPARE(blank.error.message, std::string("instance was not found"));
  QVERIFY(!scratch.session.canUndo());

  QVERIFY(scratch.session.closeProject().ok());
  auto closed = scratch.session.lift(reel.middleId);
  QVERIFY(!closed.ok());
  QCOMPARE(closed.error.message, std::string("project is not open"));
  QVERIFY(!scratch.session.canUndo());

  QVERIFY(scratch.session.openProject(reel.directory).ok());
  auto marks = scratch.session.recordMarks(reel.sequenceId);
  QVERIFY(marks.ok());
  QVERIFY(!marks.value.markIn.has_value());
  QVERIFY(!marks.value.markOut.has_value());
}

void LiftTests::firstLastOnlyMiddleAdjacent() {
  Scratch scratch;
  Reel firstReel;
  QVERIFY(prepareTrio(scratch, &firstReel, "First"));
  auto sequence = scratch.session.readSequence(firstReel.sequenceId);
  QVERIFY(sequence.ok());
  QVERIFY(scratch.session.setRecordMarkIn(firstReel.sequenceId, 2).ok());
  QVERIFY(scratch.session.setRecordMarkOut(firstReel.sequenceId, 8).ok());
  QVERIFY(scratch.session.setSourceMarkIn(firstReel.representationId, 1).ok());
  QVERIFY(scratch.session.setSourceMarkOut(firstReel.representationId, 4).ok());
  QVERIFY(scratch.session.lift(firstReel.firstId).ok());
  auto track = scratch.session.listInstances(firstReel.trackId);
  QCOMPARE(track.value.size(), static_cast<std::size_t>(2));
  QVERIFY(!intersects(track.value, 0, 12));
  QVERIFY(findId(track.value, firstReel.firstId) == nullptr);
  expectHeld(*findId(track.value, firstReel.middleId), firstReel.middleId, firstReel.trackId, 12, 24);
  expectHeld(*findId(track.value, firstReel.lastId), firstReel.lastId, firstReel.trackId, 24, 36);
  auto other = scratch.session.listInstances(firstReel.otherTrackId);
  QCOMPARE(other.value.size(), static_cast<std::size_t>(1));
  expectHeld(other.value.front(), firstReel.otherId, firstReel.otherTrackId, 0, 36);
  QCOMPARE(furthest(track.value), static_cast<std::int64_t>(36));
  QCOMPARE(furthest(other.value), static_cast<std::int64_t>(36));
  auto still = scratch.session.readSequence(firstReel.sequenceId);
  QCOMPARE(still.value.name, sequence.value.name);
  QCOMPARE(still.value.rate.numerator, sequence.value.rate.numerator);
  QCOMPARE(still.value.rate.denominator, sequence.value.rate.denominator);
  auto recordMarks = scratch.session.recordMarks(firstReel.sequenceId);
  QCOMPARE(*recordMarks.value.markIn, static_cast<std::int64_t>(2));
  QCOMPARE(*recordMarks.value.markOut, static_cast<std::int64_t>(8));
  auto sourceMarks = scratch.session.sourceMarks(firstReel.representationId);
  QCOMPARE(*sourceMarks.value.markIn, static_cast<std::int64_t>(1));
  QCOMPARE(*sourceMarks.value.markOut, static_cast<std::int64_t>(4));

  Reel lastReel;
  QVERIFY(prepareTrio(scratch, &lastReel, "Last"));
  QVERIFY(scratch.session.lift(lastReel.lastId).ok());
  track = scratch.session.listInstances(lastReel.trackId);
  QCOMPARE(track.value.size(), static_cast<std::size_t>(2));
  QVERIFY(!intersects(track.value, 24, 36));
  expectHeld(*findId(track.value, lastReel.firstId), lastReel.firstId, lastReel.trackId, 0, 12);
  expectHeld(*findId(track.value, lastReel.middleId), lastReel.middleId, lastReel.trackId, 12, 24);
  other = scratch.session.listInstances(lastReel.otherTrackId);
  expectHeld(other.value.front(), lastReel.otherId, lastReel.otherTrackId, 0, 36);
  QCOMPARE(furthest(other.value), static_cast<std::int64_t>(36));
  QCOMPARE(scratch.session.readSequence(lastReel.sequenceId).value.rate.numerator, static_cast<std::int64_t>(24));

  Reel middleReel;
  QVERIFY(prepareTrio(scratch, &middleReel, "Middle"));
  QVERIFY(scratch.session.lift(middleReel.middleId).ok());
  track = scratch.session.listInstances(middleReel.trackId);
  QCOMPARE(track.value.size(), static_cast<std::size_t>(2));
  QVERIFY(!intersects(track.value, 12, 24));
  expectHeld(*findId(track.value, middleReel.firstId), middleReel.firstId, middleReel.trackId, 0, 12);
  expectHeld(*findId(track.value, middleReel.lastId), middleReel.lastId, middleReel.trackId, 24, 36);
  QVERIFY(scratch.session.listInstances(middleReel.otherTrackId).value.size() == 1);

  Reel only;
  QVERIFY(prepareTrio(scratch, &only, "Only"));
  QVERIFY(scratch.session.lift(only.firstId).ok());
  QVERIFY(scratch.session.lift(only.middleId).ok());
  QVERIFY(scratch.session.lift(only.lastId).ok());
  track = scratch.session.listInstances(only.trackId);
  QVERIFY(track.value.empty());
  QVERIFY(!intersects(track.value, 0, 36));
  other = scratch.session.listInstances(only.otherTrackId);
  expectHeld(other.value.front(), only.otherId, only.otherTrackId, 0, 36);
  QCOMPARE(furthest(other.value), static_cast<std::int64_t>(36));
  QCOMPARE(scratch.session.readSequence(only.sequenceId).value.rate.denominator, static_cast<std::int64_t>(1));
  auto tables = scratch.session.storeForTest().listUserTables();
  QVERIFY(tables.ok());
  QCOMPARE(tables.value.size(), static_cast<std::size_t>(11));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));
}

void LiftTests::postSplitEmptyInterval() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepareTrio(scratch, &reel, "SplitRight"));
  QVERIFY(scratch.session.lift(reel.middleId).ok());
  QVERIFY(scratch.session.lift(reel.lastId).ok());
  auto split = scratch.session.split(reel.firstId, 6);
  QVERIFY(split.ok());
  const std::string rightId = split.value.instanceId;
  QVERIFY(rightId != reel.firstId);
  QVERIFY(scratch.session.lift(rightId).ok());
  auto track = scratch.session.listInstances(reel.trackId);
  QCOMPARE(track.value.size(), static_cast<std::size_t>(1));
  QVERIFY(!intersects(track.value, 6, 12));
  const auto* left = findId(track.value, reel.firstId);
  QVERIFY(left != nullptr);
  QCOMPARE(left->recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(left->recordEnd, static_cast<std::int64_t>(6));
  QCOMPARE(left->sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(left->sourceEnd, static_cast<std::int64_t>(6));
  QVERIFY(findId(track.value, rightId) == nullptr);
  auto other = scratch.session.listInstances(reel.otherTrackId);
  expectHeld(other.value.front(), reel.otherId, reel.otherTrackId, 0, 36);
  QCOMPARE(scratch.session.readSequence(reel.sequenceId).value.rate.numerator, static_cast<std::int64_t>(24));

  Reel kept;
  QVERIFY(prepareTrio(scratch, &kept, "SplitLeft"));
  QVERIFY(scratch.session.lift(kept.middleId).ok());
  QVERIFY(scratch.session.lift(kept.lastId).ok());
  auto pieces = scratch.session.split(kept.firstId, 4);
  QVERIFY(pieces.ok());
  QVERIFY(scratch.session.lift(kept.firstId).ok());
  track = scratch.session.listInstances(kept.trackId);
  QCOMPARE(track.value.size(), static_cast<std::size_t>(1));
  QVERIFY(!intersects(track.value, 0, 4));
  const auto* right = findId(track.value, pieces.value.instanceId);
  QVERIFY(right != nullptr);
  QCOMPARE(right->recordStart, static_cast<std::int64_t>(4));
  QCOMPARE(right->recordEnd, static_cast<std::int64_t>(12));
  QCOMPARE(right->sourceStart, static_cast<std::int64_t>(4));
  QCOMPARE(right->sourceEnd, static_cast<std::int64_t>(12));
  QCOMPARE(right->trackId, kept.trackId);
  QVERIFY(!scratch.session.readInstance(kept.firstId).ok());
  for (const aether::TimelineInstance& row : track.value) {
    QVERIFY(row.recordEnd > row.recordStart);
    QVERIFY(row.mediaId == kept.mediaId);
  }
}

void LiftTests::oneUndoRedoReopen() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepareTrio(scratch, &reel, "Undo"));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(!scratch.session.canUndo());
  auto before = scratch.session.readInstance(reel.middleId);
  QVERIFY(before.ok());
  QVERIFY(scratch.session.lift(reel.middleId).ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  QVERIFY(!scratch.session.readInstance(reel.middleId).ok());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  auto restored = scratch.session.readInstance(reel.middleId);
  QVERIFY(restored.ok());
  QCOMPARE(restored.value.instanceId, before.value.instanceId);
  QCOMPARE(restored.value.trackId, before.value.trackId);
  QCOMPARE(restored.value.mediaId, before.value.mediaId);
  QCOMPARE(restored.value.masterObjectId, before.value.masterObjectId);
  QCOMPARE(restored.value.sourceRate.numerator, before.value.sourceRate.numerator);
  QCOMPARE(restored.value.sourceRate.denominator, before.value.sourceRate.denominator);
  QCOMPARE(restored.value.sourceStart, before.value.sourceStart);
  QCOMPARE(restored.value.sourceEnd, before.value.sourceEnd);
  QCOMPARE(restored.value.recordStart, before.value.recordStart);
  QCOMPARE(restored.value.recordEnd, before.value.recordEnd);
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(3));
  QVERIFY(scratch.session.redo().ok());
  QVERIFY(!scratch.session.readInstance(reel.middleId).ok());
  auto afterRedo = scratch.session.listInstances(reel.trackId);
  QCOMPARE(afterRedo.value.size(), static_cast<std::size_t>(2));
  QVERIFY(!intersects(afterRedo.value, 12, 24));
  expectHeld(*findId(afterRedo.value, reel.firstId), reel.firstId, reel.trackId, 0, 12);
  expectHeld(*findId(afterRedo.value, reel.lastId), reel.lastId, reel.trackId, 24, 36);
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(reel.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  QVERIFY(!scratch.session.readInstance(reel.middleId).ok());
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(2));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));
  QVERIFY(scratch.session.undo().error.code == aether::ErrorCode::NothingToUndo);
}

void LiftTests::failedWriteAddsNoUndo() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepareTrio(scratch, &reel, "Fail"));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(scratch.session.lift(reel.middleId).ok());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(scratch.session.canRedo());
  auto refused = scratch.session.lift("not-an-instance");
  QVERIFY(!refused.ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  QCOMPARE(scratch.session.readInstance(reel.middleId).value.recordStart, static_cast<std::int64_t>(12));

  scratch.session.storeForTest().debugArmNextMutationFailure();
  auto armed = scratch.session.lift(reel.middleId);
  QVERIFY(!armed.ok());
  QCOMPARE(armed.error.message, std::string("write failed and was rolled back"));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(3));
  QVERIFY(scratch.session.readInstance(reel.middleId).ok());
  QVERIFY(scratch.session.redo().ok());
  QVERIFY(!scratch.session.readInstance(reel.middleId).ok());
}

void LiftTests::windowLiftUsesSession() {
  Scratch scratch;
  Reel reel;
  QVERIFY(prepareTrio(scratch, &reel, "Window"));
  QVERIFY(reopen(scratch, reel));
  QVERIFY(scratch.session.setRecordMarkIn(reel.sequenceId, 2).ok());
  QVERIFY(scratch.session.setRecordMarkOut(reel.sequenceId, 8).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1680, 1200);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QVERIFY(workspace != nullptr);
  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QVERIFY(sequences != nullptr);
  QTest::mouseClick(sequences->viewport(), Qt::LeftButton, Qt::NoModifier,
                    sequences->visualItemRect(sequences->item(0)).center());
  auto* button = window.findChild<QPushButton*>("liftApply");
  QVERIFY(button != nullptr);
  QCOMPARE(button->text(), QString("Lift"));
  QVERIFY(button->shortcut().isEmpty());
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(workspace->rejectionText().contains("no instance is selected"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(reel.trackId).value.size(), static_cast<std::size_t>(3));

  auto* instances = window.findChild<QListWidget*>("instanceList");
  QVERIFY(instances != nullptr);
  int instanceRow = -1;
  for (int index = 0; index < instances->count(); ++index) {
    if (instances->item(index)->text().contains("track 0 start 12")) {
      instanceRow = index;
    }
  }
  QVERIFY(instanceRow >= 0);
  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  const QString picked = instances->item(instanceRow)->data(Qt::UserRole).toString();
  const QPoint point = workspace->blockCenter(picked);
  QVERIFY(!point.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, point);
  QCOMPARE(workspace->selectedInstanceId().toStdString(), reel.middleId);
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(workspace->rejectionText().isEmpty());
  QVERIFY(workspace->selectedInstanceId().isEmpty());
  QVERIFY(workspace->inspectorText().contains("No instance selected."));
  auto rows = scratch.session.listInstances(reel.trackId);
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(2));
  QVERIFY(!intersects(rows.value, 12, 24));
  QVERIFY(findId(rows.value, reel.middleId) == nullptr);
  QVERIFY(findId(rows.value, reel.firstId) != nullptr);
  QVERIFY(findId(rows.value, reel.lastId) != nullptr);
  QCOMPARE(scratch.session.listInstances(reel.otherTrackId).value.front().instanceId, reel.otherId);
  auto marks = scratch.session.recordMarks(reel.sequenceId);
  QCOMPARE(*marks.value.markIn, static_cast<std::int64_t>(2));
  QCOMPARE(*marks.value.markOut, static_cast<std::int64_t>(8));
  QVERIFY(scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(std::string(aether::kPrototypeVersion), std::string("slice-0a"));
}
