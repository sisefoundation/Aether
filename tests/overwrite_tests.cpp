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

#include "overwrite_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/persistence/project_store.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QToolButton>

#include <algorithm>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>

namespace {

bool openPanelMenu(QWidget* window, const char* name) {
  auto* button = window->findChild<QToolButton*>(name);
  if (button == nullptr || button->menu() == nullptr) {
    return false;
  }
  if (!button->menu()->isVisible()) {
    button->menu()->popup(button->mapToGlobal(QPoint(0, button->height())));
  }
  return QTest::qWaitFor([button] { return button->menu()->isVisible(); }, 1000);
}

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    parent = std::filesystem::temp_directory_path() / ("aether-overwrite-" + token);
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
    output << "overwrite-bytes";
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
  std::string sequenceId;
  std::string trackId;
  std::string otherTrackId;
  std::string otherInstanceId;
};

bool prepare(Scratch& scratch, Cut* cut, std::int64_t recordEnd) {
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
  auto placed = scratch.session.placeInstance(cut->sequenceId, cut->trackId, cut->objectId, {24, 1}, 0, recordEnd, 0,
                                              recordEnd);
  auto held = scratch.session.placeInstance(cut->sequenceId, cut->otherTrackId, cut->objectId, {24, 1}, 5, 5 + recordEnd,
                                            0, recordEnd);
  if (!placed.ok() || !held.ok()) {
    return false;
  }
  cut->otherInstanceId = held.value.instanceId;
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

#define EXPECT_SPAN(row, ss, se, rs, re)                                                                              \
  do {                                                                                                                \
    QCOMPARE((row).sourceStart, static_cast<std::int64_t>(ss));                                                      \
    QCOMPARE((row).sourceEnd, static_cast<std::int64_t>(se));                                                        \
    QCOMPARE((row).recordStart, static_cast<std::int64_t>(rs));                                                      \
    QCOMPARE((row).recordEnd, static_cast<std::int64_t>(re));                                                        \
    QCOMPARE((row).sourceRate.numerator, static_cast<std::int64_t>(24));                                             \
    QCOMPARE((row).sourceRate.denominator, static_cast<std::int64_t>(1));                                            \
  } while (0)

}  // namespace

void OverwriteTests::inclusiveMarkBecomesHalfOpenSpan() {
  auto span = aether::exclusiveEndFromInclusiveMark(10, 18);
  QVERIFY(span.ok());
  QCOMPARE(span.value, static_cast<std::int64_t>(19));
  auto oneFrame = aether::exclusiveEndFromInclusiveMark(10, 10);
  QVERIFY(oneFrame.ok());
  QCOMPARE(oneFrame.value, static_cast<std::int64_t>(11));
  auto reversed = aether::exclusiveEndFromInclusiveMark(10, 9);
  QVERIFY(!reversed.ok());
  QCOMPARE(reversed.error.message, std::string("Mark Out is before Mark In"));
  auto overflow = aether::exclusiveEndFromInclusiveMark(0, std::numeric_limits<std::int64_t>::max());
  QVERIFY(!overflow.ok());
  QCOMPARE(overflow.error.message, std::string("inclusive Mark Out does not fit"));

  aether::TimelineInstance replacement;
  replacement.trackId = "track";
  replacement.sourceStart = 4;
  replacement.sourceEnd = 4;
  replacement.recordStart = 0;
  replacement.recordEnd = 0;
  auto zero = aether::planSameRateOverwrite(replacement, {});
  QVERIFY(!zero.ok());
  QCOMPARE(zero.error.message, std::string("a frame span must be a half-open range of non-negative int64 frames"));
}

void OverwriteTests::refusesMissingReversedZeroAndUnequal() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 48));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  const auto tables = scratch.session.storeForTest().listUserTables();
  QVERIFY(tables.ok());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));

  auto missing = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!missing.ok());
  QCOMPARE(missing.error.message, std::string("Mark In and Mark Out are required"));
  QVERIFY(!scratch.session.canUndo());

  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 10).ok());
  auto reversed = scratch.session.setSourceMarkOut(cut.representationId, 4);
  QVERIFY(!reversed.ok());
  QCOMPARE(reversed.error.message, std::string("Mark Out is before Mark In"));
  QVERIFY(!scratch.session.sourceMarks(cut.representationId).value.markOut.has_value());
  auto stillMissing =
      scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!stillMissing.ok());
  QCOMPARE(stillMissing.error.message, std::string("Mark In and Mark Out are required"));
  QVERIFY(!scratch.session.canUndo());

  auto equal = scratch.session.setSourceMarkOut(cut.representationId, 10);
  QVERIFY(!equal.ok());
  QCOMPARE(equal.error.message, std::string("equal Mark In and Mark Out are an incomplete range"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(1));

  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 17).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 3).ok());
  auto unequal = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!unequal.ok());
  QCOMPARE(unequal.error.message, std::string("source duration and record duration differ"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.front().recordEnd, static_cast<std::int64_t>(48));
  QCOMPARE(scratch.session.storeForTest().listUserTables().value, tables.value);

  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 0).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, std::numeric_limits<std::int64_t>::max()).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, std::numeric_limits<std::int64_t>::max()).ok());
  auto overflow = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!overflow.ok());
  QCOMPARE(overflow.error.message, std::string("inclusive Mark Out does not fit"));
  QVERIFY(!scratch.session.canUndo());
}

void OverwriteTests::refusesRateExtentTrackAndAsset() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 48));
  auto bare = scratch.session.createTrack(cut.sequenceId);
  QVERIFY(bare.ok());
  const std::string bareTrackId = bare.value.trackId;
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 0).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 7).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 7).ok());

  auto mixed = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {25, 1});
  QVERIFY(!mixed.ok());
  QCOMPARE(mixed.error.message, std::string("source rate and sequence rate differ; no duration mapping is authorised"));
  QVERIFY(!scratch.session.canUndo());

  QVERIFY(scratch.session.clearRecordMarks(cut.sequenceId).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 41).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 48).ok());
  auto past = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!past.ok());
  QCOMPARE(past.error.message, std::string("record end is past the current maximum record end"));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.front().recordEnd, static_cast<std::int64_t>(48));

  auto noSequence =
      scratch.session.overwrite("missing-sequence", cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!noSequence.ok());
  QCOMPARE(noSequence.error.message, std::string("sequence was not found"));

  auto emptyTrack = scratch.session.overwrite(cut.sequenceId, "", cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!emptyTrack.ok());
  QCOMPARE(emptyTrack.error.message, std::string("choose a track"));

  auto badTrack = scratch.session.overwrite(cut.sequenceId, "missing-track", cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!badTrack.ok());
  QCOMPARE(badTrack.error.message, std::string("track is not in the sequence"));

  auto noAsset = scratch.session.overwrite(cut.sequenceId, cut.trackId, "missing-object", cut.representationId, {24, 1});
  QVERIFY(!noAsset.ok());
  QCOMPARE(noAsset.error.message, std::string("master clip was not found"));

  auto noRepresentation = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, "missing-rep", {24, 1});
  QVERIFY(!noRepresentation.ok());
  QCOMPARE(noRepresentation.error.message, std::string("the representation is not on this asset"));

  auto emptyRange =
      scratch.session.overwrite(cut.sequenceId, bareTrackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!emptyRange.ok());
  QCOMPARE(emptyRange.error.message, std::string("the destination track has no record end"));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.listInstances(bareTrackId).value.empty());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}

void OverwriteTests::splitsCoverHeadTailAndInterior() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 48));
  const auto heldBefore = scratch.session.readInstance(cut.otherInstanceId);
  QVERIFY(heldBefore.ok());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());

  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 1000).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 1047).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 47).ok());
  auto covered = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(covered.ok());
  auto coveredRows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(coveredRows.value.size(), static_cast<std::size_t>(1));
  EXPECT_SPAN(coveredRows.value.front(), 1000, 1048, 0, 48);
  QCOMPARE(coveredRows.value.front().instanceId, covered.value.instanceId);
  QVERIFY(scratch.session.undo().ok());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.front().sourceStart, static_cast<std::int64_t>(0));
  QVERIFY(scratch.session.redo().ok());
  QVERIFY(scratch.session.undo().ok());

  QVERIFY(scratch.session.clearSourceMarks(cut.representationId).ok());
  QVERIFY(scratch.session.clearRecordMarks(cut.sequenceId).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 2).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 9).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 7).ok());
  auto head = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(head.ok());
  auto headRows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(headRows.value.size(), static_cast<std::size_t>(2));
  const auto* headNew = findRecord(headRows.value, 0);
  const auto* headKept = findRecord(headRows.value, 8);
  QVERIFY(headNew != nullptr);
  QVERIFY(headKept != nullptr);
  EXPECT_SPAN(*headNew, 2, 10, 0, 8);
  EXPECT_SPAN(*headKept, 8, 48, 8, 48);
  QVERIFY(scratch.session.undo().ok());

  QVERIFY(scratch.session.clearRecordMarks(cut.sequenceId).ok());
  QVERIFY(scratch.session.clearSourceMarks(cut.representationId).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 0).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 7).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 40).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 47).ok());
  auto tail = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(tail.ok());
  auto tailRows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(tailRows.value.size(), static_cast<std::size_t>(2));
  const auto* tailKept = findRecord(tailRows.value, 0);
  const auto* tailNew = findRecord(tailRows.value, 40);
  QVERIFY(tailKept != nullptr);
  QVERIFY(tailNew != nullptr);
  EXPECT_SPAN(*tailKept, 0, 40, 0, 40);
  EXPECT_SPAN(*tailNew, 0, 8, 40, 48);
  QVERIFY(scratch.session.undo().ok());

  QVERIFY(scratch.session.clearRecordMarks(cut.sequenceId).ok());
  QVERIFY(scratch.session.clearSourceMarks(cut.representationId).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 10).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 17).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 16).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 23).ok());
  auto interior = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(interior.ok());
  auto interiorRows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(interiorRows.value.size(), static_cast<std::size_t>(3));
  const auto* left = findRecord(interiorRows.value, 0);
  const auto* middle = findRecord(interiorRows.value, 16);
  const auto* right = findRecord(interiorRows.value, 24);
  QVERIFY(left != nullptr);
  QVERIFY(middle != nullptr);
  QVERIFY(right != nullptr);
  EXPECT_SPAN(*left, 0, 16, 0, 16);
  EXPECT_SPAN(*middle, 10, 18, 16, 24);
  EXPECT_SPAN(*right, 24, 48, 24, 48);
  QCOMPARE(middle->instanceId, interior.value.instanceId);
  QCOMPARE(left->instanceId == interior.value.instanceId, false);

  auto other = scratch.session.readInstance(cut.otherInstanceId);
  QVERIFY(other.ok());
  QCOMPARE(other.value.recordStart, heldBefore.value.recordStart);
  QCOMPARE(other.value.recordEnd, heldBefore.value.recordEnd);
  QCOMPARE(other.value.sourceStart, heldBefore.value.sourceStart);
  QCOMPARE(other.value.sourceEnd, heldBefore.value.sourceEnd);
  QCOMPARE(other.value.trackId, cut.otherTrackId);
  auto sequence = scratch.session.readSequence(cut.sequenceId);
  QCOMPARE(sequence.value.rate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(sequence.value.rate.denominator, static_cast<std::int64_t>(1));
  QCOMPARE(sequence.value.name, std::string("Reel"));
  std::int64_t maximum = 0;
  for (const aether::TimelineInstance& row : interiorRows.value) {
    maximum = std::max(maximum, row.recordEnd);
  }
  QCOMPARE(maximum, static_cast<std::int64_t>(48));
  auto marks = scratch.session.sourceMarks(cut.representationId);
  QCOMPARE(*marks.value.markIn, static_cast<std::int64_t>(10));
  QCOMPARE(*marks.value.markOut, static_cast<std::int64_t>(17));
}

void OverwriteTests::emptyIntersectionWritesAndOtherTracksStay() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 20));
  auto later = scratch.session.placeInstance(cut.sequenceId, cut.trackId, cut.objectId, {24, 1}, 0, 18, 30, 48);
  QVERIFY(later.ok());
  const std::string laterId = later.value.instanceId;
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(!scratch.session.canUndo());

  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 1).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 8).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 20).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 27).ok());
  const auto before = scratch.session.listInstances(cut.trackId);
  QCOMPARE(before.value.size(), static_cast<std::size_t>(2));
  auto wrote = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(wrote.ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  auto rows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(3));
  const auto* gap = findRecord(rows.value, 20);
  const auto* first = findRecord(rows.value, 0);
  const auto* tail = findRecord(rows.value, 30);
  QVERIFY(gap != nullptr);
  QVERIFY(first != nullptr);
  QVERIFY(tail != nullptr);
  EXPECT_SPAN(*gap, 1, 9, 20, 28);
  EXPECT_SPAN(*first, 0, 20, 0, 20);
  EXPECT_SPAN(*tail, 0, 18, 30, 48);
  QCOMPARE(tail->instanceId, laterId);
  QCOMPARE(first->recordStart, static_cast<std::int64_t>(0));
  auto other = scratch.session.readInstance(cut.otherInstanceId);
  EXPECT_SPAN(other.value, 5, 25, 0, 20);
  std::int64_t maximum = 0;
  for (const aether::TimelineInstance& row : rows.value) {
    maximum = std::max(maximum, row.recordEnd);
  }
  QCOMPARE(maximum, static_cast<std::int64_t>(48));
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(2));
}

void OverwriteTests::oneUndoRedoAndReopen() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 48));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 10).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 17).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 16).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 23).ok());
  auto wrote = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(wrote.ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(1));
  EXPECT_SPAN(scratch.session.listInstances(cut.trackId).value.front(), 0, 48, 0, 48);
  QVERIFY(scratch.session.redo().ok());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(3));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(!scratch.session.canRedo());
  QVERIFY(!scratch.session.sourceMarks(cut.representationId).value.markIn.has_value());
  auto reopened = scratch.session.listInstances(cut.trackId);
  QCOMPARE(reopened.value.size(), static_cast<std::size_t>(3));
  const auto* middle = findRecord(reopened.value, 16);
  QVERIFY(middle != nullptr);
  EXPECT_SPAN(*middle, 10, 18, 16, 24);
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(scratch.session.info().prototypeVersion, std::string("slice-0a"));
  QVERIFY(scratch.session.undo().error.code == aether::ErrorCode::NothingToUndo);
}

void OverwriteTests::failedWriteAddsNoUndo() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 48));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  QVERIFY(scratch.session.setSourceMarkIn(cut.representationId, 0).ok());
  QVERIFY(scratch.session.setSourceMarkOut(cut.representationId, 7).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 7).ok());
  auto first = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(first.ok());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(scratch.session.canRedo());
  QVERIFY(scratch.session.clearRecordMarks(cut.sequenceId).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 3).ok());
  auto refused = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!refused.ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  EXPECT_SPAN(scratch.session.listInstances(cut.trackId).value.front(), 0, 48, 0, 48);

  scratch.session.storeForTest().debugArmNextMutationFailure();
  QVERIFY(scratch.session.clearRecordMarks(cut.sequenceId).ok());
  QVERIFY(scratch.session.setRecordMarkIn(cut.sequenceId, 0).ok());
  QVERIFY(scratch.session.setRecordMarkOut(cut.sequenceId, 7).ok());
  auto armed = scratch.session.overwrite(cut.sequenceId, cut.trackId, cut.objectId, cut.representationId, {24, 1});
  QVERIFY(!armed.ok());
  QCOMPARE(armed.error.message, std::string("write failed and was rolled back"));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  EXPECT_SPAN(scratch.session.listInstances(cut.trackId).value.front(), 0, 48, 0, 48);
  QVERIFY(scratch.session.redo().ok());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(2));
}

void OverwriteTests::windowOverwriteUsesSession() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut, 16));
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1680, 1200);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QVERIFY(workspace != nullptr);
  auto* assets = window.findChild<QListWidget*>("assetList");
  QVERIFY(assets != nullptr);
  QVERIFY(assets->count() > 0);
  QTest::mouseClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier, assets->visualItemRect(assets->item(0)).center());
  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QTest::mouseClick(sequences->viewport(), Qt::LeftButton, Qt::NoModifier,
                    sequences->visualItemRect(sequences->item(0)).center());

  auto* sourceTab = window.findChild<QPushButton*>("viewerSourceTab");
  QVERIFY(sourceTab != nullptr);
  sourceTab->click();
  auto* sourceFrame = window.findChild<QLineEdit*>("sourceMarkFrame");
  sourceFrame->setText("0");
  QVERIFY(openPanelMenu(&window, "viewerPanelMenu"));
  QTest::mouseClick(window.findChild<QPushButton*>("sourceMarkIn"), Qt::LeftButton);
  sourceFrame->setText("7");
  QVERIFY(openPanelMenu(&window, "viewerPanelMenu"));
  QTest::mouseClick(window.findChild<QPushButton*>("sourceMarkOut"), Qt::LeftButton);
  auto* recordFrame = window.findChild<QLineEdit*>("recordMarkFrame");
  recordFrame->setText("0");
  QVERIFY(openPanelMenu(&window, "viewerPanelMenu"));
  QTest::mouseClick(window.findChild<QPushButton*>("recordMarkIn"), Qt::LeftButton);
  recordFrame->setText("7");
  QVERIFY(openPanelMenu(&window, "viewerPanelMenu"));
  QTest::mouseClick(window.findChild<QPushButton*>("recordMarkOut"), Qt::LeftButton);
  window.findChild<QToolButton*>("viewerPanelMenu")->menu()->close();

  auto* track = window.findChild<QComboBox*>("overwriteTrack");
  auto* rate = window.findChild<QComboBox*>("overwriteRate");
  auto* button = window.findChild<QPushButton*>("overwriteApply");
  QVERIFY(track != nullptr);
  QVERIFY(rate != nullptr);
  QVERIFY(button != nullptr);
  QVERIFY(button->text() == "Overwrite");
  int trackIndex = -1;
  for (int index = 0; index < track->count(); ++index) {
    if (track->itemData(index).toString().toStdString() == cut.trackId) {
      trackIndex = index;
    }
  }
  QVERIFY(trackIndex > 0);
  track->setCurrentIndex(0);
  rate->setCurrentIndex(1);
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(workspace->rejectionText().contains("choose a track"));
  QVERIFY(scratch.session.listInstances(cut.trackId).value.size() == 1);

  track->setCurrentIndex(trackIndex);
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(workspace->rejectionText().isEmpty());
  auto rows = scratch.session.listInstances(cut.trackId);
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(2));
  const auto* replacement = findRecord(rows.value, 0);
  const auto* kept = findRecord(rows.value, 8);
  QVERIFY(replacement != nullptr);
  QVERIFY(kept != nullptr);
  EXPECT_SPAN(*replacement, 0, 8, 0, 8);
  EXPECT_SPAN(*kept, 8, 16, 8, 16);
  QVERIFY(workspace->inspectorText().contains("record [0,8)"));
  QVERIFY(workspace->inspectorText().contains("source [0,8)"));
  QVERIFY(scratch.session.canUndo());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}
