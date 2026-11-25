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

#include "timeline_workspace_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/ui/frame_label.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QTest>
#include <QWidget>

#include <filesystem>

namespace {

class Scratch {
 public:
  Scratch() : session(logger), workspace(session) {
    const auto id = aether::makeStableId();
    directory = std::filesystem::temp_directory_path() / ("aether-slice1b-" + (id ? *id : std::string("tmp")));
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
  }

  bool openCut() {
    if (!session.createProject(directory, "Cut").ok()) {
      return false;
    }
    auto bin = session.createBin("Bin");
    auto asset = session.createAsset("picture", "{}");
    if (!bin.ok() || !asset.ok()) {
      return false;
    }
    auto clip = session.createMasterClip("Shot", asset.value.mediaId, bin.value.binId, "{}");
    if (!clip.ok()) {
      return false;
    }
    masterId = clip.value.objectId;
    mediaId = asset.value.mediaId;
    return true;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  aether::TimelineWorkspace workspace;
  std::filesystem::path directory;
  std::string masterId;
  std::string mediaId;
};

bool reopen(Scratch& scratch) {
  if (!scratch.session.closeProject().ok() || !scratch.session.openProject(scratch.directory).ok()) {
    return false;
  }
  scratch.workspace.reload();
  return true;
}

}  // namespace

void TimelineWorkspaceTests::selectionRefreshAndTrackOrder() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto first = scratch.session.createSequence("First", {24, 1});
  auto second = scratch.session.createSequence("Second", {25, 1});
  QVERIFY(first.ok());
  QVERIFY(second.ok());
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(first.value.sequenceId)));
  QCOMPARE(scratch.workspace.selectedSequenceId(), QString::fromStdString(first.value.sequenceId));
  QVERIFY(scratch.session.renameSequence(first.value.sequenceId, "First Cut").ok());
  scratch.workspace.reload();
  QCOMPARE(scratch.workspace.selectedSequenceId(), QString::fromStdString(first.value.sequenceId));
  auto* list = scratch.workspace.findChild<QListWidget*>("sequenceList");
  QVERIFY(list != nullptr);
  QVERIFY(list->currentItem()->text().contains("First Cut"));

  auto trackA = scratch.session.createTrack(first.value.sequenceId);
  auto trackB = scratch.session.createTrack(first.value.sequenceId);
  QVERIFY(trackA.ok());
  QVERIFY(trackB.ok());
  QVERIFY(scratch.session.orderTrack(trackB.value.trackId, 0).ok());
  scratch.workspace.reload();
  const QStringList order = scratch.workspace.trackIdsInOrder();
  QCOMPARE(order.size(), 2);
  QCOMPARE(order.at(0), QString::fromStdString(trackB.value.trackId));
  QCOMPARE(order.at(1), QString::fromStdString(trackA.value.trackId));
}

void TimelineWorkspaceTests::positionDurationAndInspector() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {24, 1}, 2, 8, 4, 10);
  QVERIFY(placed.ok());
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  auto* instances = scratch.workspace.findChild<QListWidget*>("instanceList");
  QVERIFY(instances->currentItem()->text().contains("start 4"));
  QVERIFY(instances->currentItem()->text().contains("duration 6"));
  const QString inspector = scratch.workspace.inspectorText();
  QVERIFY(inspector.contains("duration 6 frames"));
  QVERIFY(inspector.contains(QString::fromStdString(scratch.mediaId)));
  QVERIFY(inspector.contains("record [4,10)"));
  QVERIFY(inspector.contains("source [2,8)"));
}

void TimelineWorkspaceTests::validAndInvalidMove() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {24, 1}, 0, 10, 0, 10);
  auto other = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                              {24, 1}, 10, 20, 20, 30);
  QVERIFY(placed.ok());
  QVERIFY(other.ok());
  QVERIFY(reopen(scratch));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(other.value.instanceId)));
  scratch.workspace.setMoveRecordStartText("0");
  scratch.workspace.findChild<QPushButton*>("applyMove")->click();
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(other.value.instanceId).value.recordStart, static_cast<std::int64_t>(20));
  QVERIFY(scratch.workspace.rejectionText().contains("overlap"));

  scratch.workspace.setMoveRecordStartText("40");
  scratch.workspace.findChild<QPushButton*>("applyMove")->click();
  QVERIFY(scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(other.value.instanceId).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.readInstance(other.value.instanceId).value.recordEnd, static_cast<std::int64_t>(50));
  QCOMPARE(scratch.session.readInstance(other.value.instanceId).value.sourceStart, static_cast<std::int64_t>(10));
  QVERIFY(scratch.workspace.rejectionText().isEmpty());
}

void TimelineWorkspaceTests::validAndInvalidTrim() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {24, 1}, 0, 10, 0, 10);
  QVERIFY(placed.ok());
  QVERIFY(reopen(scratch));
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  scratch.workspace.setTrimText("0", "0", "0", "10");
  scratch.workspace.findChild<QPushButton*>("applyTrim")->click();
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.sourceEnd, static_cast<std::int64_t>(10));
  QVERIFY(!scratch.workspace.rejectionText().isEmpty());

  scratch.workspace.setTrimText("1", "7", "3", "9");
  scratch.workspace.findChild<QPushButton*>("applyTrim")->click();
  QVERIFY(scratch.session.canUndo());
  auto trimmed = scratch.session.readInstance(placed.value.instanceId);
  QCOMPARE(trimmed.value.sourceStart, static_cast<std::int64_t>(1));
  QCOMPARE(trimmed.value.sourceEnd, static_cast<std::int64_t>(7));
  QCOMPARE(trimmed.value.recordStart, static_cast<std::int64_t>(3));
  QCOMPARE(trimmed.value.recordEnd, static_cast<std::int64_t>(9));
}

void TimelineWorkspaceTests::integerFramesRejectFractions() {
  QCOMPARE(aether::formatFrameLabel(0, {24, 1}), std::string("00:00:00:00"));
  QCOMPARE(aether::formatFrameLabel(24, {24, 1}), std::string("00:00:01:00"));
  QCOMPARE(aether::formatFrameLabel(25, {30000, 1001}), std::string("00:00:00:25"));
  QCOMPARE(aether::formatFrameLabel(30, {30000, 1001}), std::string("00:00:01:00"));
  QVERIFY(!aether::parseIntegerFrame("1.5").ok);
  QVERIFY(!aether::parseIntegerFrame("-1").ok);
  QVERIFY(aether::parseIntegerFrame("0").ok);

  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {30000, 1001});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {30000, 1001}, 0, 8, 0, 8);
  QVERIFY(placed.ok());
  QVERIFY(reopen(scratch));
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.rulerCaption().contains("00:00:00:00"));
  QVERIFY(scratch.workspace.rulerCaption().contains("Drop-frame flag absent"));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  scratch.workspace.setMoveRecordStartText("1.5");
  scratch.workspace.applyMove();
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(0));
  QVERIFY(scratch.workspace.rejectionText().contains("integer"));
}

void TimelineWorkspaceTests::undoRedoAndFailedCommandAddsNothing() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {24, 1}, 0, 8, 0, 8);
  QVERIFY(placed.ok());
  QVERIFY(reopen(scratch));
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  scratch.workspace.setMoveRecordStartText("12");
  scratch.workspace.findChild<QPushButton*>("applyMove")->click();
  QVERIFY(scratch.session.canUndo());
  scratch.workspace.undoEdit();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(0));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.canRedo());
  scratch.workspace.redoEdit();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(12));

  QVERIFY(reopen(scratch));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  scratch.workspace.setMoveRecordStartText("nope");
  scratch.workspace.applyMove();
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(12));
}

void TimelineWorkspaceTests::closeReopenKeepsCommittedEdit() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {24, 1}, 0, 4, 0, 4);
  QVERIFY(placed.ok());
  QVERIFY(reopen(scratch));
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  scratch.workspace.setMoveRecordStartText("6");
  scratch.workspace.applyMove();
  QVERIFY(reopen(scratch));
  QVERIFY(!scratch.session.canUndo());
  auto kept = scratch.session.readInstance(placed.value.instanceId);
  QCOMPARE(kept.value.recordStart, static_cast<std::int64_t>(6));
  QCOMPARE(kept.value.recordEnd, static_cast<std::int64_t>(10));
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.selectInstance(QString::fromStdString(placed.value.instanceId)));
  QVERIFY(scratch.workspace.inspectorText().contains("record [6,10)"));
}

void TimelineWorkspaceTests::emptySequenceAndEmptyTrack() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Empty", {25, 1});
  QVERIFY(sequence.ok());
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(scratch.workspace.trackIdsInOrder().isEmpty());
  QCOMPARE(scratch.workspace.inspectorText(), QString("No instance selected."));
  QVERIFY(scratch.workspace.rulerCaption().contains("Drop-frame flag absent"));
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  QVERIFY(track.ok());
  scratch.workspace.reload();
  QCOMPARE(scratch.workspace.trackIdsInOrder().size(), 1);
  auto* instances = scratch.workspace.findChild<QListWidget*>("instanceList");
  QCOMPARE(instances->count(), 0);
  QCOMPARE(scratch.workspace.selectedInstanceId(), QString());
}

void TimelineWorkspaceTests::viewControlsDoNotWriteTheCut() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  auto placed = scratch.session.insertInstance(sequence.value.sequenceId, track.value.trackId, scratch.masterId,
                                               {24, 1}, 0, 8, 0, 8);
  QVERIFY(placed.ok());
  QVERIFY(reopen(scratch));
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  QVERIFY(!scratch.session.canUndo());
  scratch.workspace.resize(1200, 800);
  scratch.workspace.show();
  QVERIFY(QTest::qWaitForWindowExposed(&scratch.workspace));

  auto* zoom = scratch.workspace.findChild<QSlider*>("timelineZoom");
  QVERIFY(zoom != nullptr);
  QCOMPARE(zoom->value(), aether::kTimelinePixelsPerFrame);
  zoom->setValue(8);
  auto* overview = scratch.workspace.findChild<QWidget*>("timelineNavigator");
  QVERIFY(overview != nullptr);
  QVERIFY(overview->isVisible());
  const QPoint strip(24, overview->height() / 2);
  QTest::mousePress(overview, Qt::LeftButton, Qt::NoModifier, strip);
  QTest::mouseMove(overview, strip + QPoint(70, 0));
  QTest::mouseRelease(overview, Qt::LeftButton, Qt::NoModifier, strip + QPoint(70, 0));

  auto* headers = scratch.workspace.findChild<QWidget*>("timelineTrackHeaders");
  QVERIFY(headers != nullptr);
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, QPoint(124, 40));
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, QPoint(150, 40));

  auto* canvas = scratch.workspace.findChild<QWidget*>("timelineCanvas");
  QVERIFY(canvas != nullptr);
  const QPoint playhead(aether::kTimelineOriginPx, 40);
  QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, playhead);
  QTest::mouseMove(canvas, playhead + QPoint(48, 0));
  QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier, playhead + QPoint(48, 0));

  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordEnd, static_cast<std::int64_t>(8));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.workspace.rulerCaption().contains("Sequence frame 0 label"));
  QVERIFY(scratch.workspace.blockCenter(QString::fromStdString(placed.value.instanceId)).x() >
          aether::kTimelineOriginPx);
}

void TimelineWorkspaceTests::laneStackPresentsVideoAboveAudio() {
  Scratch scratch;
  QVERIFY(scratch.openCut());
  auto sequence = scratch.session.createSequence("Main", {24, 1});
  QVERIFY(sequence.ok());
  auto first = scratch.session.createTrack(sequence.value.sequenceId);
  auto second = scratch.session.createTrack(sequence.value.sequenceId);
  auto third = scratch.session.createTrack(sequence.value.sequenceId);
  QVERIFY(first.ok());
  QVERIFY(second.ok());
  QVERIFY(third.ok());
  scratch.workspace.reload();
  QVERIFY(scratch.workspace.selectSequence(QString::fromStdString(sequence.value.sequenceId)));
  scratch.workspace.resize(1280, 860);
  scratch.workspace.show();
  QVERIFY(QTest::qWaitForWindowExposed(&scratch.workspace));

  auto* headers = scratch.workspace.findChild<QWidget*>("timelineTrackHeaders");
  QVERIFY(headers != nullptr);
  const QString firstId = QString::fromStdString(first.value.trackId);
  const QString secondId = QString::fromStdString(second.value.trackId);
  const QString thirdId = QString::fromStdString(third.value.trackId);
  const auto clickName = [headers](int row) {
    const QPoint name(94, 28 + row * 48 + 24);
    QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, name);
  };
  clickName(0);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList{thirdId});
  QCOMPARE(headers->accessibleDescription(), QString("Extract tracks V2"));
  clickName(1);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList({firstId, thirdId}));
  QCOMPARE(headers->accessibleDescription(), QString("Extract tracks V1, V2"));
  clickName(2);
  QCOMPARE(scratch.workspace.extractParticipantIds(), QStringList({firstId, secondId, thirdId}));
  QCOMPARE(headers->accessibleDescription(), QString("Extract tracks V1, A1, V2"));
  QVERIFY(!headers->accessibleDescription().contains("track "));
}
