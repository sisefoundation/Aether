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
// See the License for the specific language governing the permissions and
// limitations under the License.

#include "ui_completion_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/ui/frame_label.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/project_dialog.hpp"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    parent = std::filesystem::temp_directory_path() / ("aether-ui-completion-" + (id ? *id : std::string("tmp")));
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
    output << "completion-bytes";
    output.close();
    return path;
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
};

struct Cut {
  std::filesystem::path directory;
  std::string sequenceId;
  std::string otherSequenceId;
  std::string trackId;
  std::string instanceId;
};

bool prepare(Scratch& scratch, Cut* cut) {
  cut->directory = scratch.parent / "ExtractUiCut";
  if (!scratch.session.createProject(cut->directory, "ExtractUiCut").ok()) {
    return false;
  }
  auto imported = scratch.session.importReferences({scratch.writeMedia("shot.mov").string()});
  if (!imported.ok() || imported.value.size() != 1) {
    return false;
  }
  auto sequence = scratch.session.createSequence("Reel", {24, 1});
  auto other = scratch.session.createSequence("Other", {24, 1});
  if (!sequence.ok() || !other.ok()) {
    return false;
  }
  cut->sequenceId = sequence.value.sequenceId;
  cut->otherSequenceId = other.value.sequenceId;
  auto track = scratch.session.createTrack(sequence.value.sequenceId);
  if (!track.ok() || !scratch.session.createTrack(other.value.sequenceId).ok()) {
    return false;
  }
  cut->trackId = track.value.trackId;
  auto placed = scratch.session.placeInstance(sequence.value.sequenceId, track.value.trackId,
                                              imported.value.front().objectId, {24, 1}, 0, 40, 0, 40);
  if (!placed.ok()) {
    return false;
  }
  cut->instanceId = placed.value.instanceId;
  return scratch.session.closeProject().ok();
}

void dismissDialogSoon() {
  QTimer::singleShot(0, [] {
    if (auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) {
      dialog->reject();
    }
  });
}

QAction* actionNamed(QWidget& window, const QString& text) {
  const auto actions = window.findChildren<QAction*>();
  for (QAction* action : actions) {
    if (action->text() == text) {
      return action;
    }
  }
  return nullptr;
}

void clickRuler(QWidget* canvas, int frame) {
  const QPoint point(aether::kTimelineOriginPx + frame * aether::kTimelinePixelsPerFrame, 8);
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, point);
}

bool clickSequence(QListWidget* sequences, const QString& name) {
  if (sequences == nullptr) {
    return false;
  }
  for (int index = 0; index < sequences->count(); ++index) {
    if (!sequences->item(index)->text().contains(name)) {
      continue;
    }
    const QRect row = sequences->visualItemRect(sequences->item(index));
    QTest::mouseClick(sequences->viewport(), Qt::LeftButton, Qt::NoModifier, row.center());
    return true;
  }
  return false;
}

}  // namespace

void UiCompletionTests::rangeModeAndFeedbackFollowTheLifecycle() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1280, 860);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  auto* mode = window.findChild<QPushButton*>("extractRangeMode");
  auto* start = window.findChild<QLineEdit*>("extractRangeStart");
  auto* end = window.findChild<QLineEdit*>("extractRangeEnd");
  auto* readout = window.findChild<QLabel*>("extractRangeReadout");
  auto* rejection = window.findChild<QLabel*>("rejection");
  auto* mark = window.findChild<QLineEdit*>("recordMarkFrame");
  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  QVERIFY(mode && start && end && readout && rejection && mark && canvas);
  QVERIFY(!mode->isChecked());
  QCOMPARE(start->text(), QString());
  QCOMPARE(end->text(), QString());
  QCOMPARE(readout->text(), QString("No record range."));
  QCOMPARE(rejection->text(), QString());

  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr || !dialog->openDirectory(QString::fromStdString(cut.directory.string()))) {
      failure = "the project did not open";
      if (dialog != nullptr) {
        dialog->reject();
      }
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(!mode->isChecked());
  QCOMPARE(start->text(), QString());
  QCOMPARE(rejection->text(), QString());
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));

  clickRuler(canvas, 12);
  QCOMPARE(mark->text(), QString("12"));
  QCOMPARE(start->text(), QString());

  QTest::mouseClick(mode, Qt::LeftButton);
  QVERIFY(mode->isChecked());
  clickRuler(canvas, 8);
  clickRuler(canvas, 24);
  QCOMPARE(start->text(), QString("8"));
  QCOMPARE(end->text(), QString("24"));
  QVERIFY(readout->text().contains("Start 8 inclusive. End 24 exclusive. D 16."));
  QCOMPARE(mark->text(), QString("12"));

  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QVERIFY(sequences != nullptr && sequences->count() == 2);
  QVERIFY(sequences->currentItem() != nullptr);
  const QString otherName = sequences->currentItem()->text().contains("Reel") ? QString("Other") : QString("Reel");
  QVERIFY(clickSequence(sequences, otherName));
  QVERIFY(sequences->currentItem()->text().contains(otherName));
  QVERIFY(!mode->isChecked());
  QCOMPARE(start->text(), QString());
  QCOMPARE(end->text(), QString());
  QCOMPARE(readout->text(), QString("No record range."));
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  clickRuler(canvas, 4);
  QCOMPARE(mark->text(), QString("4"));
  QCOMPARE(start->text(), QString());

  QVERIFY(clickSequence(sequences, "Reel"));
  QVERIFY(!mode->isChecked());
  QTest::mouseClick(mode, Qt::LeftButton);
  clickRuler(canvas, 8);
  clickRuler(canvas, 24);
  const QPoint trackName(94, 52);
  auto* headers = window.findChild<QWidget*>("timelineTrackHeaders");
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, trackName);
  auto* extract = window.findChild<QPushButton*>("extractApply");
  QTest::mouseClick(extract, Qt::LeftButton);
  QCOMPARE(rejection->text(), QString());
  QCOMPARE(start->text(), QString());
  QCOMPARE(end->text(), QString());
  QVERIFY(readout->text().contains("cleared"));
  QVERIFY(mode->isChecked());
  QVERIFY(scratch.session.canUndo());
  const auto extractedCount = scratch.session.listInstances(cut.trackId).value.size();
  QVERIFY(extractedCount > 1);

  QTest::mouseClick(mode, Qt::LeftButton);
  QVERIFY(!mode->isChecked());
  QTest::mouseClick(mode, Qt::LeftButton);
  clickRuler(canvas, 20);
  clickRuler(canvas, 30);
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, trackName);
  QTest::mouseClick(extract, Qt::LeftButton);
  QVERIFY(rejection->text().contains("at least one track"));
  QVERIFY(mode->isChecked());
  QCOMPARE(start->text(), QString("20"));
  QCOMPARE(end->text(), QString("30"));
  QVERIFY(scratch.session.extractRange(nullptr, nullptr, nullptr));

  dismissDialogSoon();
  actionNamed(window, "Close Project")->trigger();
  QVERIFY(!scratch.session.isOpen());
  QVERIFY(!mode->isChecked());
  QCOMPARE(start->text(), QString());
  QCOMPARE(end->text(), QString());
  QCOMPARE(rejection->text(), QString());
  QVERIFY(!rejection->text().contains("at least one track"));

  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr || !dialog->openDirectory(QString::fromStdString(cut.directory.string()))) {
      failure = "reopen failed";
      if (dialog != nullptr) {
        dialog->reject();
      }
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(!mode->isChecked());
  QCOMPARE(start->text(), QString());
  QCOMPARE(rejection->text(), QString());
  QVERIFY(!scratch.session.extractRange(nullptr, nullptr, nullptr));
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), extractedCount);
  clickRuler(canvas, 6);
  QCOMPARE(mark->text(), QString("6"));
  QCOMPARE(start->text(), QString());
  window.close();
}

void UiCompletionTests::timelineMenuOpensFromItsButton() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1280, 860);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  auto* button = window.findChild<QToolButton*>("timelinePanelMenu");
  QVERIFY(button != nullptr && button->menu() != nullptr);
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(button->menu()->isVisible());
  auto* undo = button->menu()->findChild<QPushButton*>("undoEdit");
  auto* redo = button->menu()->findChild<QPushButton*>("redoEdit");
  QVERIFY(undo != nullptr && redo != nullptr);
  QVERIFY(!undo->isEnabled());
  QVERIFY(!redo->isEnabled());

  auto* side = window.findChild<QToolButton*>("sidePanelMenu");
  QVERIFY(side != nullptr && side->menu() != nullptr && !side->menu()->actions().isEmpty());
  QVERIFY(!side->menu()->actions().first()->isEnabled());
  button->menu()->hide();

  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QVERIFY(clickSequence(sequences, "Other"));
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(button->menu()->isVisible());
  button->menu()->hide();

  dismissDialogSoon();
  actionNamed(window, "Close Project")->trigger();
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr || !dialog->openDirectory(QString::fromStdString(cut.directory.string()))) {
      failure = "reopen failed";
      if (dialog != nullptr) {
        dialog->reject();
      }
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QTest::mouseClick(button, Qt::LeftButton);
  QVERIFY(button->menu()->isVisible());
  QVERIFY(!undo->isEnabled());
  QTest::mouseClick(undo, Qt::LeftButton);
  QVERIFY(!scratch.session.canUndo());
  button->menu()->hide();
  window.close();
}

void UiCompletionTests::editMenuAndShortcutsUndoAnExtract() {
  Scratch scratch;
  Cut cut;
  QVERIFY(prepare(scratch, &cut));
  QVERIFY(scratch.session.openProject(cut.directory).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1280, 860);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));

  auto* undoAction = window.findChild<QAction*>("editUndo");
  auto* redoAction = window.findChild<QAction*>("editRedo");
  auto* editMenu = window.findChild<QMenu*>("editMenu");
  QVERIFY(undoAction && redoAction && editMenu);
  QCOMPARE(undoAction->shortcut(), QKeySequence(QKeySequence::Undo));
  QCOMPARE(redoAction->shortcut(), QKeySequence(QKeySequence::Redo));
  QVERIFY(!undoAction->isEnabled());
  QVERIFY(!redoAction->isEnabled());

  auto* mode = window.findChild<QPushButton*>("extractRangeMode");
  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  auto* headers = window.findChild<QWidget*>("timelineTrackHeaders");
  auto* extract = window.findChild<QPushButton*>("extractApply");
  auto* start = window.findChild<QLineEdit*>("extractRangeStart");
  auto* sequences = window.findChild<QListWidget*>("sequenceList");
  QVERIFY(clickSequence(sequences, "Reel"));
  QTest::mouseClick(mode, Qt::LeftButton);
  clickRuler(canvas, 8);
  clickRuler(canvas, 24);
  QTest::mouseClick(headers, Qt::LeftButton, Qt::NoModifier, QPoint(94, 52));
  QTest::mouseClick(extract, Qt::LeftButton);
  QVERIFY(scratch.session.canUndo());
  QVERIFY(undoAction->isEnabled());
  QVERIFY(!redoAction->isEnabled());
  const auto extractedCount = scratch.session.listInstances(cut.trackId).value.size();
  QVERIFY(extractedCount > 1);

  auto* bar = window.menuBar();
  QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->actionGeometry(editMenu->menuAction()).center());
  QVERIFY(editMenu->isVisible());
  const QRect undoRect = editMenu->actionGeometry(undoAction);
  QTest::mouseClick(editMenu, Qt::LeftButton, Qt::NoModifier, undoRect.center());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(redoAction->isEnabled());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(1));
  QCOMPARE(scratch.session.readInstance(cut.instanceId).value.recordEnd, static_cast<std::int64_t>(40));

  QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->actionGeometry(editMenu->menuAction()).center());
  QVERIFY(editMenu->isVisible());
  QTest::mouseClick(editMenu, Qt::LeftButton, Qt::NoModifier, editMenu->actionGeometry(redoAction).center());
  QVERIFY(scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), extractedCount);

  start->setFocus();
  QVERIFY(start->hasFocus());
  QTest::keyClick(start, Qt::Key_9);
  QCOMPARE(start->text(), QString("9"));
  QTest::keySequence(start, QKeySequence(QKeySequence::Undo));
  QCOMPARE(start->text(), QString());
  QVERIFY(scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), extractedCount);

  canvas->setFocus();
  QTest::keySequence(canvas, QKeySequence(QKeySequence::Undo));
  QVERIFY(!scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), static_cast<std::size_t>(1));
  QTest::keySequence(canvas, QKeySequence(QKeySequence::Redo));
  QVERIFY(scratch.session.canUndo());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), extractedCount);

  QVERIFY(clickSequence(sequences, "Other"));
  QVERIFY(undoAction->isEnabled());
  QVERIFY(clickSequence(sequences, "Reel"));

  dismissDialogSoon();
  actionNamed(window, "Close Project")->trigger();
  QString failure;
  QTimer::singleShot(0, [&] {
    auto* dialog = qobject_cast<aether::ProjectDialog*>(QApplication::activeModalWidget());
    if (dialog == nullptr || !dialog->openDirectory(QString::fromStdString(cut.directory.string()))) {
      failure = "reopen failed";
      if (dialog != nullptr) {
        dialog->reject();
      }
    }
  });
  window.showProjectDialog();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  QVERIFY(!undoAction->isEnabled());
  QVERIFY(!redoAction->isEnabled());
  QCOMPARE(scratch.session.listInstances(cut.trackId).value.size(), extractedCount);
  window.close();
}
