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

#include "ui_integration_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/project_dialog.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTest>
#include <QTimer>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    parent = std::filesystem::temp_directory_path() / ("aether-ui-integration-" + token);
    media = std::filesystem::temp_directory_path() / ("aether-ui-integration-media-" + token);
    std::filesystem::create_directories(parent);
    std::filesystem::create_directories(media);
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ignored;
    std::filesystem::remove_all(parent, ignored);
    std::filesystem::remove_all(media, ignored);
  }

  std::filesystem::path writeMedia(const std::string& name, const std::string& bytes) const {
    const auto path = media / name;
    std::ofstream output(path, std::ios::binary);
    output << bytes;
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
  std::filesystem::path media;
};

QAction* findAction(QWidget& window, const QString& text) {
  const auto actions = window.findChildren<QAction*>();
  for (QAction* action : actions) {
    if (action->text() == text) {
      return action;
    }
  }
  return nullptr;
}

void dismissModalSoon() {
  QTimer::singleShot(0, [] {
    if (QWidget* modal = QApplication::activeModalWidget()) {
      modal->close();
    }
  });
}

int databaseCount(const std::filesystem::path& directory) {
  int count = 0;
  std::error_code ignored;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, ignored)) {
    if (entry.path().filename() == "project.db") {
      ++count;
    }
  }
  return count;
}

}  // namespace

void UiIntegrationTests::createDialogWritesTheProjectAndEntersTheWorkspace() {
  Scratch scratch;
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "the project dialog did not open";
      return;
    }
    dialog->findChild<QLineEdit*>("projectName")->setText("KeepRate");
    dialog->findChild<QLineEdit*>("parentFolder")->setText(QString::fromStdString(scratch.parent.string()));
    auto* rate = dialog->findChild<QComboBox*>("frameRate");
    rate->setCurrentIndex(0);
    if (rate->currentText() != "24/1") {
      failure = "frame rate combo is not 24/1";
      dialog->reject();
      return;
    }
    dialog->findChild<QPushButton*>("createProject")->click();
    if (dialog->result() != QDialog::Accepted) {
      auto* message = dialog->findChild<QLabel*>("projectDialogMessage");
      failure = message == nullptr ? QString("create did not accept") : message->text();
      dialog->reject();
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(scratch.session.isOpen());
  QCOMPARE(scratch.session.info().name, std::string("KeepRate"));
  QCOMPARE(databaseCount(scratch.parent / "KeepRate"), 1);
  auto sequences = scratch.session.listSequences();
  QVERIFY(sequences.ok());
  QCOMPARE(sequences.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(sequences.value[0].rate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(sequences.value[0].rate.denominator, static_cast<std::int64_t>(1));
  QCOMPARE(window.findChild<QLabel*>("shellTitle")->text(), QString("KeepRate"));
  QCOMPARE(window.findChild<QLabel*>("statusLine")->text(), QString("Open"));
  auto* sequencesList = window.findChild<QListWidget*>("sequenceList");
  QCOMPARE(sequencesList->count(), 1);
  QVERIFY(sequencesList->currentItem()->text().contains("24/1"));
  QVERIFY(window.findChild<QLabel*>("sequenceRate")->text().contains("Selected sequence KeepRate  24/1"));
  QCOMPARE(window.findChild<QLabel*>("emptyBin")->text(), QString("This bin has no media."));
  window.close();
}

void UiIntegrationTests::failedCreateRestoresTheOpenProject() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.parent / "KeepRate", "KeepRate").ok());
  QVERIFY(scratch.session.createSequence("KeepRate", {24, 1}).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "the project dialog did not open";
      return;
    }
    dialog->findChild<QLineEdit*>("projectName")->setText("KeepRate");
    dialog->findChild<QLineEdit*>("parentFolder")->setText(QString::fromStdString(scratch.parent.string()));
    dialog->findChild<QPushButton*>("createProject")->click();
    if (dialog->result() == QDialog::Accepted) {
      failure = "a duplicate create was accepted";
    } else if (!scratch.session.isOpen() || scratch.session.info().name != "KeepRate") {
      failure = "the open project was not restored";
    }
    dialog->reject();
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QCOMPARE(scratch.session.info().name, std::string("KeepRate"));
  QCOMPARE(window.findChild<QLabel*>("shellTitle")->text(), QString("KeepRate"));
  QCOMPARE(databaseCount(scratch.parent / "KeepRate"), 1);
  window.close();
}

void UiIntegrationTests::openCloseAndFailedOpenKeepOneProject() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.parent / "KeepRate", "KeepRate").ok());
  QVERIFY(scratch.session.createSequence("KeepRate", {24, 1}).ok());
  QVERIFY(scratch.session.closeProject().ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "the project dialog did not open";
      return;
    }
    if (!dialog->openDirectory(QString::fromStdString((scratch.parent / "KeepRate").string()))) {
      auto* message = dialog->findChild<QLabel*>("projectDialogMessage");
      failure = message == nullptr ? QString("open failed") : message->text();
      dialog->reject();
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  auto opened = scratch.session.listSequences();
  QVERIFY(opened.ok());
  QCOMPARE(opened.value[0].rate.numerator, static_cast<std::int64_t>(24));
  QCOMPARE(opened.value[0].rate.denominator, static_cast<std::int64_t>(1));
  QVERIFY(window.findChild<QLabel*>("sequenceRate")->text().contains("24/1"));

  QAction* closeProject = findAction(window, "Close Project");
  QVERIFY(closeProject != nullptr);
  QTimer::singleShot(0, [] {
    if (QWidget* modal = QApplication::activeModalWidget()) {
      modal->close();
    }
  });
  closeProject->trigger();
  QVERIFY(!scratch.session.isOpen());
  QCOMPARE(window.findChild<QLabel*>("statusLine")->text(), QString("Closed"));
  QCOMPARE(window.findChild<QListWidget*>("binList")->count(), 0);
  QCOMPARE(window.findChild<QListWidget*>("assetList")->count(), 0);
  QCOMPARE(window.findChild<QListWidget*>("sequenceList")->count(), 0);
  QCOMPARE(window.findChild<QLabel*>("assetInspector")->text(), QString("No media selected."));
  QCOMPARE(window.findChild<QLabel*>("shellTitle")->text(), QString("Aether"));
  QCOMPARE(window.windowTitle(), QString("Aether"));

  QVERIFY(scratch.session.openProject(scratch.parent / "KeepRate").ok());
  window.findChild<QLabel*>("statusLine")->setText("stale");
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "the project dialog did not open for the failed open";
      return;
    }
    if (dialog->openDirectory(QString::fromStdString(scratch.parent.string()))) {
      failure = "a parent folder was accepted as a project";
    } else if (!scratch.session.isOpen() || scratch.session.info().name != "KeepRate") {
      failure = "the failed open did not restore KeepRate";
    }
    dialog->reject();
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QCOMPARE(window.findChild<QLabel*>("shellTitle")->text(), QString("KeepRate"));
  QVERIFY(window.findChild<QLabel*>("sequenceRate")->text().contains("KeepRate"));
  QVERIFY(window.findChild<QLabel*>("sequenceRate")->text().contains("24/1"));
  QCOMPARE(window.findChild<QListWidget*>("sequenceList")->count(), 1);
  window.close();
}

void UiIntegrationTests::switchingProjectsDropsStaleRows() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("only-a.mov", "aaaa");
  QVERIFY(scratch.session.createProject(scratch.parent / "ProjectA", "ProjectA").ok());
  QVERIFY(scratch.session.createSequence("ProjectA", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  auto bins = scratch.session.listBins();
  QVERIFY(bins.ok());
  auto assets = scratch.session.listBinAssets(bins.value.front().binId);
  QVERIFY(assets.ok());
  const std::string objectId = assets.value.front().objectId;
  const std::string mediaId = assets.value.front().mediaId;
  auto sequence = scratch.session.listSequences();
  auto track = scratch.session.createTrack(sequence.value.front().sequenceId);
  QVERIFY(track.ok());
  auto placed = scratch.session.insertInstance(sequence.value.front().sequenceId, track.value.trackId, objectId,
                                               {24, 1}, 0, 4, 0, 4);
  QVERIFY(placed.ok());
  const std::string instanceId = placed.value.instanceId;
  QVERIFY(scratch.session.closeProject().ok());

  QVERIFY(scratch.session.createProject(scratch.parent / "ProjectB", "ProjectB").ok());
  QVERIFY(scratch.session.createSequence("ProjectB", {25, 1}).ok());
  QVERIFY(scratch.session.createBin("OnlyB").ok());
  QVERIFY(scratch.session.closeProject().ok());

  QVERIFY(scratch.session.openProject(scratch.parent / "ProjectA").ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QVERIFY(workspace->selectInstance(QString::fromStdString(instanceId)));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains(QString::fromStdString(instanceId)));
  QVERIFY(window.findChild<QListWidget*>("assetList")->item(0)->text().contains("only-a.mov"));

  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "the project dialog did not open";
      return;
    }
    if (!dialog->openDirectory(QString::fromStdString((scratch.parent / "ProjectB").string()))) {
      failure = "ProjectB did not open";
      dialog->reject();
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QCOMPARE(scratch.session.info().name, std::string("ProjectB"));
  QCOMPARE(window.findChild<QLabel*>("shellTitle")->text(), QString("ProjectB"));
  auto* assetList = window.findChild<QListWidget*>("assetList");
  QCOMPARE(assetList->count(), 0);
  QVERIFY(!window.findChild<QLabel*>("assetInspector")->text().contains(QString::fromStdString(mediaId)));
  QVERIFY(!window.findChild<QLabel*>("assetInspector")->text().contains(QString::fromStdString(instanceId)));
  QVERIFY(!window.findChild<QLabel*>("assetInspector")->text().contains("only-a.mov"));
  auto* binList = window.findChild<QListWidget*>("binList");
  bool sawOnlyB = false;
  bool sawOnlyA = false;
  for (int index = 0; index < binList->count(); ++index) {
    const QString name = binList->item(index)->text();
    sawOnlyB = sawOnlyB || name == "OnlyB";
    sawOnlyA = sawOnlyA || name.contains("only-a");
  }
  QVERIFY(sawOnlyB);
  QVERIFY(!sawOnlyA);
  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QCOMPARE(sequences->count(), 1);
  QVERIFY(sequences->item(0)->text().contains("ProjectB"));
  QVERIFY(sequences->item(0)->text().contains("25/1"));
  QVERIFY(!sequences->item(0)->text().contains("ProjectA"));
  QCOMPARE(window.findChild<QListWidget*>("instanceList")->count(), 0);
  QCOMPARE(workspace->selectedInstanceId(), QString());
  auto rate = scratch.session.listSequences();
  QCOMPARE(rate.value.front().rate.numerator, static_cast<std::int64_t>(25));
  QCOMPARE(rate.value.front().rate.denominator, static_cast<std::int64_t>(1));
  window.close();
}

void UiIntegrationTests::importPopulatesTheBinAndFailureWritesNothing() {
  Scratch scratch;
  aether::MainWindow closed(scratch.session, scratch.logger);
  closed.show();
  QVERIFY(QTest::qWaitForWindowExposed(&closed));
  closed.findChild<QPushButton*>("importMedia")->click();
  QCOMPARE(closed.findChild<QLabel*>("statusLine")->text(), QString("Open a project before importing media"));
  closed.close();

  QVERIFY(scratch.session.createProject(scratch.parent / "Imports", "Imports").ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  QCOMPARE(window.findChild<QLabel*>("emptyBin")->text(), QString("This bin has no media."));
  const auto bad = scratch.writeMedia("notes.txt", "not-media");
  const auto picture = scratch.writeMedia("take.mov", "picture-bytes");
  const auto sizeBefore = std::filesystem::file_size(picture);
  dismissModalSoon();
  window.importPaths({QString::fromStdString(bad.string())});
  QCOMPARE(window.findChild<QListWidget*>("assetList")->count(), 0);
  QVERIFY(window.findChild<QLabel*>("statusLine")->text().contains("no records were written"));
  auto bins = scratch.session.listBins();
  QVERIFY(bins.ok());
  auto empty = scratch.session.listBinAssets(bins.value.front().binId);
  QVERIFY(empty.ok());
  QCOMPARE(empty.value.size(), static_cast<std::size_t>(0));

  window.importPaths({QString::fromStdString(picture.string())});
  auto* assets = window.findChild<QListWidget*>("assetList");
  QCOMPARE(assets->count(), 1);
  QVERIFY(assets->item(0)->text().contains("take.mov"));
  QVERIFY(assets->item(0)->text().contains("online"));
  const QString inspector = window.findChild<QLabel*>("assetInspector")->text();
  QVERIFY(inspector.contains("take.mov"));
  QVERIFY(inspector.contains("online"));
  QVERIFY(inspector.contains("registered"));
  QVERIFY(inspector.contains("Not probed"));
  QVERIFY(!window.findChild<QLabel*>("mediaPreview")->isHidden());
  QCOMPARE(std::filesystem::file_size(picture), sizeBefore);

  dismissModalSoon();
  window.importPaths({QString::fromStdString(picture.string())});
  QCOMPARE(assets->count(), 1);
  QVERIFY(window.findChild<QLabel*>("statusLine")->text().contains("exact recorded path"));
  auto once = scratch.session.listBinAssets(bins.value.front().binId);
  QCOMPARE(once.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(std::filesystem::file_size(picture), sizeBefore);
  window.close();
}

void UiIntegrationTests::sequenceSelectionRefreshesTheTimelineAndRate() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.parent / "Rates", "Rates").ok());
  auto first = scratch.session.createSequence("Picture", {24, 1});
  auto second = scratch.session.createSequence("Sound", {25, 1});
  QVERIFY(first.ok());
  QVERIFY(second.ok());
  auto track = scratch.session.createTrack(first.value.sequenceId);
  QVERIFY(track.ok());
  auto bin = scratch.session.createBin("Bin");
  auto asset = scratch.session.createAsset("picture", "{}");
  QVERIFY(bin.ok());
  QVERIFY(asset.ok());
  auto clip = scratch.session.createMasterClip("Shot", asset.value.mediaId, bin.value.binId, "{}");
  QVERIFY(clip.ok());
  auto placed = scratch.session.insertInstance(first.value.sequenceId, track.value.trackId, clip.value.objectId,
                                               {24, 1}, 0, 6, 0, 6);
  QVERIFY(placed.ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* list = window.findChild<QListWidget*>("sequenceList");
  QCOMPARE(list->count(), 2);
  QListWidgetItem* sound = nullptr;
  for (int index = 0; index < list->count(); ++index) {
    if (list->item(index)->text().contains("Sound")) {
      sound = list->item(index);
    }
  }
  QVERIFY(sound != nullptr);
  const QRect row = list->visualItemRect(sound);
  QVERIFY(row.isValid());
  QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier, row.center());
  QCOMPARE(list->currentItem()->text().left(5), QString("Sound"));
  QVERIFY(window.findChild<QLabel*>("sequenceRate")->text().contains("Selected sequence Sound  25/1"));
  QVERIFY(window.findChild<QLabel*>("playhead")->text().contains("Sound"));
  QCOMPARE(window.findChild<QListWidget*>("instanceList")->count(), 0);
  QVERIFY(!window.findChild<QLabel*>("assetInspector")->text().contains(QString::fromStdString(placed.value.instanceId)));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QCOMPARE(workspace->selectedSequenceId(), QString::fromStdString(second.value.sequenceId));
  QCOMPARE(workspace->trackIdsInOrder().size(), 0);
  window.close();
}

void UiIntegrationTests::clickingABlockSelectsItAndEditsPersist() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("block.mov", "block-bytes");
  const auto sizeBefore = std::filesystem::file_size(picture);
  QVERIFY(scratch.session.createProject(scratch.parent / "Edit", "Edit").ok());
  QVERIFY(scratch.session.createSequence("Edit", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  auto bins = scratch.session.listBins();
  auto assets = scratch.session.listBinAssets(bins.value.front().binId);
  QVERIFY(assets.ok());
  auto sequence = scratch.session.listSequences();
  auto track = scratch.session.createTrack(sequence.value.front().sequenceId);
  QVERIFY(track.ok());
  auto placed = scratch.session.insertInstance(sequence.value.front().sequenceId, track.value.trackId,
                                               assets.value.front().objectId, {24, 1}, 0, 8, 0, 8);
  auto other = scratch.session.insertInstance(sequence.value.front().sequenceId, track.value.trackId,
                                              assets.value.front().objectId, {24, 1}, 8, 16, 20, 28);
  QVERIFY(placed.ok());
  QVERIFY(other.ok());
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(scratch.parent / "Edit").ok());
  QVERIFY(!scratch.session.canUndo());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  QVERIFY(canvas != nullptr);
  const QPoint point = workspace->blockCenter(QString::fromStdString(placed.value.instanceId));
  QVERIFY(!point.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, point);
  QCOMPARE(workspace->selectedInstanceId(), QString::fromStdString(placed.value.instanceId));
  QVERIFY(workspace->blockIsSelected(QString::fromStdString(placed.value.instanceId)));
  QVERIFY(!workspace->blockIsSelected(QString::fromStdString(other.value.instanceId)));
  const QString selected = window.findChild<QLabel*>("assetInspector")->text();
  QVERIFY(selected.contains("instance " + QString::fromStdString(placed.value.instanceId)));
  QVERIFY(selected.contains("record [0,8)"));
  QVERIFY(selected.contains("Not probed") == false);
  QVERIFY(window.findChild<QLabel*>("settingsNote")->text().contains(
      "Raster, pixel aspect, colour, and stereoscopic values are not stored."));
  QVERIFY(window.findChild<QLabel*>("mediaPreview")->isHidden());

  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
  QCOMPARE(workspace->selectedInstanceId(), QString::fromStdString(placed.value.instanceId));

  workspace->setMoveRecordStartText("20");
  window.findChild<QPushButton*>("applyMove")->click();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(0));
  QVERIFY(workspace->rejectionText().contains("overlap"));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [0,8)"));
  QVERIFY(!window.findChild<QPushButton*>("undoEdit")->isEnabled());

  workspace->setMoveRecordStartText("1.5");
  window.findChild<QPushButton*>("applyMove")->click();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(0));

  workspace->setMoveRecordStartText("40");
  window.findChild<QPushButton*>("applyMove")->click();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordEnd, static_cast<std::int64_t>(48));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [40,48)"));
  QVERIFY(window.findChild<QPushButton*>("undoEdit")->isEnabled());
  QVERIFY(!window.findChild<QPushButton*>("redoEdit")->isEnabled());

  window.findChild<QPushButton*>("undoEdit")->click();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(0));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [0,8)"));
  QVERIFY(window.findChild<QPushButton*>("redoEdit")->isEnabled());
  window.findChild<QPushButton*>("redoEdit")->click();
  QCOMPARE(scratch.session.readInstance(placed.value.instanceId).value.recordStart, static_cast<std::int64_t>(40));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [40,48)"));

  workspace->setTrimText("1", "5", "40", "44");
  window.findChild<QPushButton*>("applyTrim")->click();
  auto trimmed = scratch.session.readInstance(placed.value.instanceId);
  QCOMPARE(trimmed.value.sourceStart, static_cast<std::int64_t>(1));
  QCOMPARE(trimmed.value.sourceEnd, static_cast<std::int64_t>(5));
  QCOMPARE(trimmed.value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(trimmed.value.recordEnd, static_cast<std::int64_t>(44));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [40,44)"));
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("source [1,5)"));

  QAction* closeProject = findAction(window, "Close Project");
  QTimer::singleShot(0, [] {
    if (QWidget* modal = QApplication::activeModalWidget()) {
      modal->close();
    }
  });
  closeProject->trigger();
  QVERIFY(!scratch.session.isOpen());
  QCOMPARE(window.findChild<QLabel*>("assetInspector")->text(), QString("No media selected."));
  QCOMPARE(window.findChild<QListWidget*>("instanceList")->count(), 0);

  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr) {
      failure = "reopen dialog did not open";
      return;
    }
    if (!dialog->openDirectory(QString::fromStdString((scratch.parent / "Edit").string()))) {
      failure = "reopen failed";
      dialog->reject();
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(!scratch.session.canUndo());
  auto kept = scratch.session.readInstance(placed.value.instanceId);
  QCOMPARE(kept.value.recordStart, static_cast<std::int64_t>(40));
  QCOMPARE(kept.value.recordEnd, static_cast<std::int64_t>(44));
  QCOMPARE(kept.value.sourceStart, static_cast<std::int64_t>(1));
  workspace = window.findChild<aether::TimelineWorkspace*>();
  canvas = window.findChild<QWidget*>("timelineCanvas");
  QCOMPARE(window.findChild<QLineEdit*>("moveRecordStart")->text(), QString());
  QCOMPARE(window.findChild<QLineEdit*>("trimRecordEnd")->text(), QString());
  const QPoint again = workspace->blockCenter(QString::fromStdString(placed.value.instanceId));
  QVERIFY(!again.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, again);
  QVERIFY(window.findChild<QLabel*>("assetInspector")->text().contains("record [40,44)"));
  QVERIFY(window.findChild<QListWidget*>("assetList")->item(0)->text().contains("block.mov"));
  QCOMPARE(std::filesystem::file_size(picture), sizeBefore);
  window.close();
}
