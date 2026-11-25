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

#include "timeline_placement_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/core/sequence/sequence.hpp"
#include "aether/ui/frame_label.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/project_dialog.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDataStream>
#include <QDialog>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QVariant>

#include <filesystem>
#include <fstream>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    parent = std::filesystem::temp_directory_path() / ("aether-placement-" + token);
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
    output << "placement-bytes";
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
};

void sendModelDrop(aether::TimelineWorkspace* workspace, const QString& objectId, const QPointF& pos, QEvent::Type type) {
  QByteArray encoded;
  QDataStream stream(&encoded, QIODevice::WriteOnly);
  QMap<int, QVariant> roles;
  roles.insert(Qt::UserRole, objectId);
  stream << 0 << 0 << roles;
  QMimeData mime;
  mime.setData("application/x-qabstractitemmodeldatalist", encoded);
  if (type == QEvent::DragMove) {
    QDragMoveEvent event(pos.toPoint(), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    workspace->deliverTimelineEvent(&event);
    return;
  }
  QDropEvent event(pos, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
  workspace->deliverTimelineEvent(&event);
}

bool dragMoveAccepted(aether::TimelineWorkspace* workspace, const QString& objectId, const QPointF& pos) {
  QByteArray encoded;
  QDataStream stream(&encoded, QIODevice::WriteOnly);
  QMap<int, QVariant> roles;
  roles.insert(Qt::UserRole, objectId);
  stream << 0 << 0 << roles;
  QMimeData mime;
  mime.setData("application/x-qabstractitemmodeldatalist", encoded);
  QDragMoveEvent event(pos.toPoint(), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
  workspace->deliverTimelineEvent(&event);
  return event.isAccepted();
}

QAction* findAction(QWidget& window, const QString& text) {
  const auto actions = window.findChildren<QAction*>();
  for (QAction* action : actions) {
    if (action->text() == text) {
      return action;
    }
  }
  return nullptr;
}

}  // namespace

void TimelinePlacementTests::identityRecordEndAndPointerMapping() {
  auto same = aether::identityRecordEnd({24, 1}, {24, 1}, 0, 8, 3);
  QVERIFY(same.ok());
  QCOMPARE(same.value, static_cast<std::int64_t>(11));
  auto mixed = aether::identityRecordEnd({24, 1}, {25, 1}, 0, 8, 0);
  QVERIFY(!mixed.ok());
  QCOMPARE(mixed.error.message, std::string("source rate and sequence rate differ; no duration mapping is authorised"));
  auto fractional = aether::identityRecordEnd({24000, 1001}, {24000, 1001}, 10, 14, 2);
  QVERIFY(fractional.ok());
  QCOMPARE(fractional.value, static_cast<std::int64_t>(6));
  auto swapped = aether::identityRecordEnd({24000, 1001}, {24, 1}, 0, 8, 0);
  QVERIFY(!swapped.ok());
  QVERIFY(!aether::identityRecordEnd({24, 1}, {24, 1}, 4, 4, 0).ok());
  QVERIFY(!aether::identityRecordEnd({24, 1}, {24, 1}, -1, 4, 0).ok());

  const aether::CanvasFrameHit origin = aether::frameAtCanvasX(aether::kTimelineOriginPx, aether::kTimelinePixelsPerFrame,
                                                               aether::kTimelineOriginPx);
  QVERIFY(origin.ok);
  QCOMPARE(origin.frame, static_cast<std::int64_t>(0));
  const aether::CanvasFrameHit boundary =
      aether::frameAtCanvasX(aether::kTimelineOriginPx + aether::kTimelinePixelsPerFrame, aether::kTimelinePixelsPerFrame,
                            aether::kTimelineOriginPx);
  QVERIFY(boundary.ok);
  QCOMPARE(boundary.frame, static_cast<std::int64_t>(1));
  const aether::CanvasFrameHit between = aether::frameAtCanvasX(
      aether::kTimelineOriginPx + aether::kTimelinePixelsPerFrame + 1, aether::kTimelinePixelsPerFrame,
      aether::kTimelineOriginPx);
  QVERIFY(between.ok);
  QCOMPARE(between.frame, static_cast<std::int64_t>(1));
  const aether::CanvasFrameHit before =
      aether::frameAtCanvasX(aether::kTimelineOriginPx - 1, aether::kTimelinePixelsPerFrame, aether::kTimelineOriginPx);
  QVERIFY(!before.ok);
  QCOMPARE(before.reason, "the pointer is before frame 0");
  const aether::CanvasFrameHit zoomed = aether::frameAtCanvasX(48 + 8 + 3, 8, 48);
  QVERIFY(zoomed.ok);
  QCOMPARE(zoomed.frame, static_cast<std::int64_t>(1));
  QVERIFY(!aether::frameAtCanvasX(48, 0, 48).ok);
  QCOMPARE(std::string(aether::kPrototypeVersion), std::string("slice-0a"));
}

void TimelinePlacementTests::placeCommandPersistsAndRejectionLeavesTheCut() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("clip.mov");
  const auto bytes = std::filesystem::file_size(picture);
  QVERIFY(scratch.session.createProject(scratch.parent / "Cut", "Cut").ok());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QVERIFY(scratch.session.createSequence("Cut", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  QCOMPARE(imported.value.size(), static_cast<std::size_t>(1));
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.parent / "Cut").ok());
  QVERIFY(!scratch.session.canUndo());
  auto sequence = scratch.session.listSequences();
  QVERIFY(sequence.ok());
  const std::string sequenceId = sequence.value.front().sequenceId;

  auto missing = scratch.session.placeInstance(sequenceId, "", "missing-object", {24, 1}, 0, 8, 0, 8);
  QVERIFY(!missing.ok());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.listTracks(sequenceId).value.empty());

  auto mixed = scratch.session.placeInstance(sequenceId, "", imported.value.front().objectId, {25, 1}, 0, 8, 0, 8);
  QVERIFY(!mixed.ok());
  QCOMPARE(mixed.error.message, std::string("source rate and sequence rate differ; no duration mapping is authorised"));
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.listTracks(sequenceId).value.empty());

  auto unequal = scratch.session.placeInstance(sequenceId, "", imported.value.front().objectId, {24, 1}, 0, 8, 0, 7);
  QVERIFY(!unequal.ok());
  QCOMPARE(unequal.error.message, std::string("record end is not the same-rate source span"));
  QVERIFY(!scratch.session.canUndo());

  auto placed = scratch.session.placeInstance(sequenceId, "", imported.value.front().objectId, {24, 1}, 0, 8, 0, 8);
  QVERIFY(placed.ok());
  QCOMPARE(placed.value.sequenceId, sequenceId);
  QCOMPARE(placed.value.mediaId, imported.value.front().mediaId);
  QCOMPARE(placed.value.masterObjectId, imported.value.front().objectId);
  QCOMPARE(placed.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(placed.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(placed.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(placed.value.recordEnd, static_cast<std::int64_t>(8));
  QCOMPARE(placed.value.sourceRate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(placed.value.sourceRate.denominator, static_cast<std::int64_t>(1));
  auto representations = scratch.session.listRepresentations(placed.value.mediaId);
  QVERIFY(representations.ok());
  QCOMPARE(representations.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(representations.value.front().representationId, imported.value.front().representationId);
  auto tracks = scratch.session.listTracks(sequenceId);
  QCOMPARE(tracks.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(placed.value.trackId, tracks.value.front().trackId);

  auto overlap = scratch.session.placeInstance(sequenceId, placed.value.trackId, imported.value.front().objectId, {24, 1},
                                              8, 12, 4, 8);
  QVERIFY(!overlap.ok());
  QVERIFY(overlap.error.message.find("overlap") != std::string::npos);
  QCOMPARE(scratch.session.listInstances(placed.value.trackId).value.size(), static_cast<std::size_t>(1));

  const std::string instanceId = placed.value.instanceId;
  const std::string trackId = placed.value.trackId;
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.readInstance(instanceId).ok());
  QVERIFY(scratch.session.listTracks(sequenceId).value.empty());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.redo().ok());
  auto restored = scratch.session.readInstance(instanceId);
  QVERIFY(restored.ok());
  QCOMPARE(restored.value.trackId, trackId);
  QCOMPARE(restored.value.recordEnd, static_cast<std::int64_t>(8));
  QCOMPARE(scratch.session.listTracks(sequenceId).value.size(), static_cast<std::size_t>(1));

  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.parent / "Cut").ok());
  QVERIFY(!scratch.session.canUndo());
  auto kept = scratch.session.readInstance(instanceId);
  QVERIFY(kept.ok());
  QCOMPARE(kept.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(kept.value.recordStart, static_cast<std::int64_t>(0));
  auto again = scratch.session.placeInstance(sequenceId, trackId, imported.value.front().objectId, {24, 1}, 0, 4, 0, 4);
  QVERIFY(!again.ok());
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.readInstance(instanceId).value.recordEnd, static_cast<std::int64_t>(8));

  auto other = scratch.session.createSequence("Other", {25, 1});
  QVERIFY(other.ok());
  auto crossed = scratch.session.placeInstance(other.value.sequenceId, trackId, imported.value.front().objectId, {25, 1},
                                              0, 4, 0, 4);
  QVERIFY(!crossed.ok());
  QVERIFY(crossed.error.message.find("track") != std::string::npos);
  QVERIFY(scratch.session.listTracks(other.value.sequenceId).value.empty());
  QCOMPARE(scratch.session.readInstance(instanceId).value.mediaId, imported.value.front().mediaId);
  QCOMPARE(std::filesystem::file_size(picture), bytes);
  auto unknown = scratch.session.placeInstance("missing-sequence", "", imported.value.front().objectId, {24, 1}, 0, 4, 0,
                                              4);
  QVERIFY(!unknown.ok());
  QCOMPARE(unknown.error.message, std::string("sequence was not found"));
}

void TimelinePlacementTests::dropPlacesEditsUndoesAndReopens() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("drop.mov");
  const auto bytes = std::filesystem::file_size(picture);
  QVERIFY(scratch.session.createProject(scratch.parent / "Drop", "Drop").ok());
  QVERIFY(scratch.session.createSequence("Drop", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.parent / "Drop").ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  QVERIFY(workspace != nullptr);
  QVERIFY(canvas != nullptr);
  const QString objectId = QString::fromStdString(imported.value.front().objectId);
  QCOMPARE(window.findChild<QListWidget*>("assetList")->count(), 1);
  QVERIFY(window.findChild<QListWidget*>("assetList")->item(0)->text().contains("drop.mov"));
  QVERIFY(!workspace->selectedSequenceId().isEmpty());
  QCOMPARE(workspace->trackIdsInOrder().size(), 0);

  QCOMPARE(window.windowTitle(), QString("Aether — Drop"));
  QVERIFY(!window.windowIcon().isNull());
  QVERIFY(!window.findChild<QLabel*>("productMark")->pixmap().isNull());
  QVERIFY(!dragMoveAccepted(workspace, objectId, QPointF(47, 40)));
  sendModelDrop(workspace, objectId, QPointF(47, 40), QEvent::Drop);
  QVERIFY2(workspace->rejectionText().contains("before frame 0"), qPrintable(workspace->rejectionText()));
  QVERIFY(scratch.session.listTracks(workspace->selectedSequenceId().toStdString()).value.empty());

  QVERIFY(!dragMoveAccepted(workspace, objectId, QPointF(56, 10)));
  sendModelDrop(workspace, objectId, QPointF(56, 10), QEvent::Drop);
  QVERIFY(workspace->rejectionText().contains("not on a track"));

  QVERIFY(dragMoveAccepted(workspace, objectId, QPointF(56, 40)));
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr || dialog->objectName() != "placementDialog") {
      failure = "placement dialog did not open";
      if (dialog != nullptr) {
        dialog->reject();
      }
      return;
    }
    if (dialog->findChild<QLineEdit*>("placementRecordIn")->text() != "2") {
      failure = "record in was " + dialog->findChild<QLineEdit*>("placementRecordIn")->text();
      dialog->reject();
      return;
    }
    if (!dialog->findChild<QLabel*>("placementFile")->text().contains("Not probed")) {
      failure = dialog->findChild<QLabel*>("placementFile")->text();
      dialog->reject();
      return;
    }
    dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(1);
    dialog->findChild<QLineEdit*>("placementSourceStart")->setText("0");
    dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("8");
    if (!dialog->findChild<QLabel*>("placementValidation")->text().contains("Record [2,10)")) {
      failure = dialog->findChild<QLabel*>("placementValidation")->text();
      dialog->reject();
      return;
    }
    dialog->findChild<QPushButton*>("placementCommit")->click();
    if (dialog->result() != QDialog::Accepted) {
      failure = dialog->findChild<QLabel*>("placementMessage")->text();
      dialog->reject();
    }
  });
  sendModelDrop(workspace, objectId, QPointF(56, 40), QEvent::Drop);
  QVERIFY2(failure.isEmpty(), qPrintable(failure));

  const QString instanceId = workspace->selectedInstanceId();
  QVERIFY(!instanceId.isEmpty());
  QVERIFY(workspace->blockIsSelected(instanceId));
  auto stored = scratch.session.readInstance(instanceId.toStdString());
  QVERIFY(stored.ok());
  QCOMPARE(stored.value.recordStart, static_cast<std::int64_t>(2));
  QCOMPARE(stored.value.recordEnd, static_cast<std::int64_t>(10));
  QCOMPARE(stored.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(stored.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(stored.value.sourceRate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(stored.value.mediaId, imported.value.front().mediaId);
  QCOMPARE(workspace->trackIdsInOrder().size(), 1);
  const QString inspector = window.findChild<QLabel*>("assetInspector")->text();
  QVERIFY(inspector.contains("record [2,10)"));
  QVERIFY(inspector.contains("source [0,8)"));
  QVERIFY(inspector.contains("24/1"));
  QVERIFY(window.findChild<QPushButton*>("undoEdit")->isEnabled());

  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "overlap dialog did not open";
      return;
    }
    if (dialog->findChild<QLineEdit*>("placementRecordIn")->text() != "2") {
      failure = "between-frame record in was " + dialog->findChild<QLineEdit*>("placementRecordIn")->text();
      dialog->reject();
      return;
    }
    dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(1);
    dialog->findChild<QLineEdit*>("placementSourceStart")->setText("0");
    dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("8");
    dialog->findChild<QPushButton*>("placementCommit")->click();
    if (dialog->result() == QDialog::Accepted) {
      failure = "an overlapping drop was accepted";
    } else if (!dialog->findChild<QLabel*>("placementMessage")->text().contains("overlap")) {
      failure = dialog->findChild<QLabel*>("placementMessage")->text();
    }
    dialog->reject();
  });
  sendModelDrop(workspace, objectId, QPointF(57, 40), QEvent::Drop);
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QCOMPARE(scratch.session.listInstances(stored.value.trackId).value.size(), static_cast<std::size_t>(1));
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.recordStart, static_cast<std::int64_t>(2));

  workspace->setMoveRecordStartText("20");
  window.findChild<QPushButton*>("applyMove")->click();
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.recordStart, static_cast<std::int64_t>(20));
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.recordEnd, static_cast<std::int64_t>(28));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [20,28)"));
  workspace->setTrimText("1", "8", "20", "27");
  window.findChild<QPushButton*>("applyTrim")->click();
  auto trimmed = scratch.session.readInstance(instanceId.toStdString());
  QCOMPARE(trimmed.value.sourceStart, static_cast<std::int64_t>(1));
  QCOMPARE(trimmed.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(trimmed.value.recordEnd, static_cast<std::int64_t>(27));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("source [1,8)"));

  window.findChild<QPushButton*>("undoEdit")->click();
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.recordEnd, static_cast<std::int64_t>(28));
  window.findChild<QPushButton*>("undoEdit")->click();
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.recordStart, static_cast<std::int64_t>(2));
  window.findChild<QPushButton*>("undoEdit")->click();
  QVERIFY(!scratch.session.readInstance(instanceId.toStdString()).ok());
  QCOMPARE(workspace->trackIdsInOrder().size(), 0);
  QVERIFY(!window.findChild<QPushButton*>("undoEdit")->isEnabled());
  QVERIFY(window.findChild<QPushButton*>("redoEdit")->isEnabled());
  window.findChild<QPushButton*>("redoEdit")->click();
  window.findChild<QPushButton*>("redoEdit")->click();
  window.findChild<QPushButton*>("redoEdit")->click();
  auto redone = scratch.session.readInstance(instanceId.toStdString());
  QVERIFY(redone.ok());
  QCOMPARE(redone.value.sourceStart, static_cast<std::int64_t>(1));
  QCOMPARE(redone.value.recordStart, static_cast<std::int64_t>(20));
  QCOMPARE(redone.value.recordEnd, static_cast<std::int64_t>(27));
  QVERIFY(window.findChild<QPushButton*>("undoEdit")->isEnabled());

  QAction* closeProject = findAction(window, "Close Project");
  QVERIFY(closeProject != nullptr);
  QTimer::singleShot(0, [] {
    if (QWidget* modal = QApplication::activeModalWidget()) {
      modal->close();
    }
  });
  closeProject->trigger();
  QVERIFY(!scratch.session.isOpen());
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "reopen dialog did not open";
      return;
    }
    if (!dialog->openDirectory(QString::fromStdString((scratch.parent / "Drop").string()))) {
      failure = "reopen failed";
      dialog->reject();
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(!scratch.session.canUndo());
  auto kept = scratch.session.readInstance(instanceId.toStdString());
  QVERIFY(kept.ok());
  QCOMPARE(kept.value.sourceStart, static_cast<std::int64_t>(1));
  QCOMPARE(kept.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(kept.value.recordStart, static_cast<std::int64_t>(20));
  QCOMPARE(kept.value.recordEnd, static_cast<std::int64_t>(27));
  workspace = window.findChild<aether::TimelineWorkspace*>();
  canvas = window.findChild<QWidget*>("timelineCanvas");
  const QPoint block = workspace->blockCenter(instanceId);
  QVERIFY(!block.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, block);
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [20,27)"));
  QCOMPARE(std::filesystem::file_size(picture), bytes);
  window.close();
}

void TimelinePlacementTests::menuMixedRateOfflineAndBadTarget() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("menu.mov");
  QVERIFY(scratch.session.createProject(scratch.parent / "Menu", "Menu").ok());
  QVERIFY(scratch.session.createSequence("Menu", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* assets = window.findChild<QListWidget*>("assetList");
  QVERIFY(assets->count() == 1);
  const QRect row = assets->visualItemRect(assets->item(0));
  QVERIFY(row.isValid());
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
    if (menu == nullptr) {
      failure = "Insert menu did not open";
      return;
    }
    QAction* place = nullptr;
    for (QAction* action : menu->actions()) {
      if (action->text() == "Insert into Timeline") {
        place = action;
      }
    }
    if (place == nullptr) {
      failure = "Insert into Timeline is missing";
      menu->close();
      return;
    }
    menu->setActiveAction(place);
    QTimer::singleShot(0, [&] {
      auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
      if (dialog == nullptr || dialog->objectName() != "placementDialog") {
        failure = "insert dialog did not open";
        if (dialog != nullptr) {
          dialog->reject();
        }
        return;
      }
      dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(2);
      dialog->findChild<QLineEdit*>("placementRecordIn")->setText("0");
      dialog->findChild<QLineEdit*>("placementSourceStart")->setText("0");
      dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("8");
      if (!dialog->findChild<QLabel*>("placementValidation")->text().contains("no duration mapping is authorised")) {
        failure = dialog->findChild<QLabel*>("placementValidation")->text();
        dialog->reject();
        return;
      }
      dialog->findChild<QPushButton*>("placementCommit")->click();
      if (dialog->result() == QDialog::Accepted) {
        failure = "a mixed-rate insert was accepted";
      } else if (!dialog->findChild<QLabel*>("placementMessage")->text().contains("no duration mapping is authorised")) {
        failure = dialog->findChild<QLabel*>("placementMessage")->text();
      }
      dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(1);
      dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("4");
      dialog->findChild<QPushButton*>("placementCommit")->click();
      if (dialog->result() != QDialog::Accepted) {
        failure = dialog->findChild<QLabel*>("placementMessage")->text();
        dialog->reject();
      }
    });
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
    QApplication::sendEvent(menu, &press);
    QApplication::sendEvent(menu, &release);
  });
  const QPoint menuPos = row.center();
  QContextMenuEvent menuEvent(QContextMenuEvent::Mouse, menuPos, assets->viewport()->mapToGlobal(menuPos));
  QApplication::sendEvent(assets->viewport(), &menuEvent);
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  const QString instanceId = workspace->selectedInstanceId();
  QString placedDetail = "selected " + instanceId;
  auto placedTracks = scratch.session.listTracks(workspace->selectedSequenceId().toStdString());
  if (placedTracks.ok()) {
    placedDetail += " tracks " + QString::number(static_cast<int>(placedTracks.value.size()));
    if (!placedTracks.value.empty()) {
      auto placedRows = scratch.session.listInstances(placedTracks.value.front().trackId);
      if (placedRows.ok()) {
        placedDetail += " rows " + QString::number(static_cast<int>(placedRows.value.size()));
      }
    }
  }
  auto* placedList = window.findChild<QListWidget*>("instanceList");
  placedDetail += " list " + QString::number(placedList == nullptr ? -1 : placedList->count());
  QVERIFY2(!instanceId.isEmpty(), qPrintable(placedDetail));
  auto stored = scratch.session.readInstance(instanceId.toStdString());
  QVERIFY(stored.ok());
  QCOMPARE(stored.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(stored.value.recordEnd, static_cast<std::int64_t>(4));
  QCOMPARE(stored.value.sourceEnd, static_cast<std::int64_t>(4));
  QCOMPARE(stored.value.sourceRate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(workspace->trackIdsInOrder().size(), 1);
  QVERIFY(scratch.session.canUndo());
  window.findChild<QPushButton*>("undoEdit")->click();
  QVERIFY(!scratch.session.readInstance(instanceId.toStdString()).ok());
  QCOMPARE(scratch.session.listTracks(workspace->selectedSequenceId().toStdString()).value.size(), static_cast<std::size_t>(0));
  window.findChild<QPushButton*>("redoEdit")->click();
  stored = scratch.session.readInstance(instanceId.toStdString());
  QVERIFY(stored.ok());
  QCOMPARE(workspace->trackIdsInOrder().size(), 1);

  std::filesystem::rename(picture, scratch.parent / "menu-offline.mov");
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "offline dialog did not open";
      return;
    }
    if (!dialog->findChild<QLabel*>("placementFile")->text().contains("offline")) {
      failure = dialog->findChild<QLabel*>("placementFile")->text();
      dialog->reject();
      return;
    }
    dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(1);
    dialog->findChild<QLineEdit*>("placementSourceStart")->setText("0");
    dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("4");
    dialog->findChild<QPushButton*>("placementCommit")->click();
    if (dialog->result() == QDialog::Accepted) {
      failure = "an offline reference was placed";
    } else if (!dialog->findChild<QLabel*>("placementMessage")->text().contains("offline")) {
      failure = dialog->findChild<QLabel*>("placementMessage")->text();
    }
    dialog->reject();
  });
  sendModelDrop(workspace, QString::fromStdString(imported.value.front().objectId), QPointF(48 + 16, 40), QEvent::Drop);
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QCOMPARE(scratch.session.listInstances(stored.value.trackId).value.size(), static_cast<std::size_t>(1));
  auto bins = scratch.session.listBins();
  auto assetsAfter = scratch.session.listBinAssets(bins.value.front().binId);
  QCOMPARE(assetsAfter.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(assetsAfter.value.front().presence, std::string("offline"));
  QCOMPARE(assetsAfter.value.front().technicalMetadata, std::string("Not probed"));

  QTimer::singleShot(0, [&] {
    if (QWidget* modal = QApplication::activeModalWidget()) {
      failure = "a drop past the track opened a dialog";
      modal->close();
    }
  });
  sendModelDrop(workspace, QString::fromStdString(imported.value.front().objectId), QPointF(80, 28 + 48 + 8), QEvent::Drop);
  QApplication::processEvents();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(workspace->rejectionText().contains("not on a track"));
  QCOMPARE(scratch.session.readInstance(instanceId.toStdString()).value.recordEnd, static_cast<std::int64_t>(4));
  window.close();
}
