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

#include "mark_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/core/project/project_info.hpp"
#include "aether/ui/frame_label.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QApplication>
#include <QImage>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>
#include <QScrollBar>
#include <QTest>
#include <QTimer>

#include <filesystem>
#include <fstream>

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
    parent = std::filesystem::temp_directory_path() / ("aether-marks-" + token);
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
    output << "mark-bytes";
    output.close();
    return path;
  }

  std::string representationNamed(const std::string& fileName) {
    auto bins = session.listBins();
    if (!bins.ok()) {
      return {};
    }
    for (const aether::BinRecord& bin : bins.value) {
      auto assets = session.listBinAssets(bin.binId);
      if (!assets.ok()) {
        continue;
      }
      for (const aether::BinAssetFact& asset : assets.value) {
        if (asset.fileName == fileName) {
          return asset.representationId;
        }
      }
    }
    return {};
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
};

}  // namespace

void MarkTests::sourcePointsStayWithTheRepresentation() {
  Scratch scratch;
  const auto first = scratch.writeMedia("a.mov");
  const auto second = scratch.writeMedia("b.mov");
  QVERIFY(scratch.session.createProject(scratch.parent / "Marks", "Marks").ok());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  QCOMPARE(std::string(aether::kPrototypeVersion), std::string("slice-0a"));
  auto imported = scratch.session.importReferences({first.string(), second.string()});
  QVERIFY(imported.ok());
  const std::string repA = scratch.representationNamed("a.mov");
  const std::string repB = scratch.representationNamed("b.mov");
  QVERIFY(!repA.empty());
  QVERIFY(repA != repB);

  QVERIFY(scratch.session.setSourceMarkIn(repA, 10).ok());
  auto points = scratch.session.sourceMarks(repA);
  QVERIFY(points.ok());
  QVERIFY(points.value.markIn.has_value());
  QCOMPARE(*points.value.markIn, static_cast<std::int64_t>(10));
  QVERIFY(!points.value.markOut.has_value());
  QVERIFY(!scratch.session.canUndo());

  auto equal = scratch.session.setSourceMarkOut(repA, 10);
  QVERIFY(!equal.ok());
  QCOMPARE(equal.error.message, std::string("equal Mark In and Mark Out are an incomplete range"));
  QVERIFY(scratch.session.sourceMarks(repA).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markIn, static_cast<std::int64_t>(10));
  QVERIFY(!scratch.session.sourceMarks(repA).value.markOut.has_value());

  auto before = scratch.session.setSourceMarkOut(repA, 4);
  QVERIFY(!before.ok());
  QCOMPARE(before.error.message, std::string("Mark Out is before Mark In"));
  QVERIFY(!scratch.session.sourceMarks(repA).value.markOut.has_value());

  QVERIFY(scratch.session.setSourceMarkOut(repA, 18).ok());
  QVERIFY(scratch.session.sourceMarks(repA).value.markOut.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markOut, static_cast<std::int64_t>(18));

  auto negative = scratch.session.setSourceMarkIn(repA, -1);
  QVERIFY(!negative.ok());
  QCOMPARE(negative.error.message, std::string("a frame must be a non-negative integer"));
  QVERIFY(scratch.session.sourceMarks(repA).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markIn, static_cast<std::int64_t>(10));

  auto missing = scratch.session.setSourceMarkIn("missing-representation", 3);
  QVERIFY(!missing.ok());
  QCOMPARE(missing.error.message, std::string("the representation is not in this project"));
  QVERIFY(scratch.session.sourceMarks(repA).value.markOut.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markOut, static_cast<std::int64_t>(18));

  QVERIFY(scratch.session.setSourceMarkIn(repB, 3).ok());
  QVERIFY(scratch.session.sourceMarks(repA).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markIn, static_cast<std::int64_t>(10));
  QVERIFY(scratch.session.sourceMarks(repB).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repB).value.markIn, static_cast<std::int64_t>(3));
  QVERIFY(!scratch.session.sourceMarks(repB).value.markOut.has_value());

  QVERIFY(scratch.session.clearSourceMarkIn(repA).ok());
  QVERIFY(!scratch.session.sourceMarks(repA).value.markIn.has_value());
  QVERIFY(scratch.session.sourceMarks(repA).value.markOut.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markOut, static_cast<std::int64_t>(18));
  QVERIFY(scratch.session.clearSourceMarks(repB).ok());
  QVERIFY(!scratch.session.sourceMarks(repB).value.markIn.has_value());
  QVERIFY(!scratch.session.canUndo());
  QVERIFY(scratch.session.listSequences().value.empty());

  const auto directory = scratch.session.info().directory;
  QVERIFY(scratch.session.closeProject().ok());
  QVERIFY(scratch.session.openProject(directory).ok());
  auto restored = scratch.session.sourceMarks(repA);
  QVERIFY(restored.ok());
  QVERIFY(!restored.value.markIn.has_value());
  QVERIFY(!restored.value.markOut.has_value());
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}

void MarkTests::recordPointsDoNotTouchTheCutOrUndo() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("cut.mov");
  QVERIFY(scratch.session.createProject(scratch.parent / "Record", "Record").ok());
  QVERIFY(scratch.session.createSequence("Picture", {24, 1}).ok());
  QVERIFY(scratch.session.createSequence("Sound", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  auto sequences = scratch.session.listSequences();
  QVERIFY(sequences.ok());
  QCOMPARE(sequences.value.size(), static_cast<std::size_t>(2));
  const std::string pictureId = sequences.value[0].name == "Picture" ? sequences.value[0].sequenceId
                                                                     : sequences.value[1].sequenceId;
  const std::string soundId = sequences.value[0].name == "Sound" ? sequences.value[0].sequenceId
                                                                 : sequences.value[1].sequenceId;
  auto placed = scratch.session.placeInstance(pictureId, "", imported.value.front().objectId, {24, 1}, 0, 8, 2, 10);
  QVERIFY(placed.ok());
  QVERIFY(scratch.session.canUndo());
  const std::string instanceId = placed.value.instanceId;
  const std::string trackId = placed.value.trackId;

  QVERIFY(scratch.session.setRecordMarkIn(pictureId, 5).ok());
  QVERIFY(scratch.session.setRecordMarkOut(pictureId, 12).ok());
  auto kept = scratch.session.readInstance(instanceId);
  QVERIFY(kept.ok());
  QCOMPARE(kept.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(kept.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(kept.value.recordStart, static_cast<std::int64_t>(2));
  QCOMPARE(kept.value.recordEnd, static_cast<std::int64_t>(10));
  QCOMPARE(scratch.session.listInstances(trackId).value.size(), static_cast<std::size_t>(1));

  QVERIFY(scratch.session.moveInstance(instanceId, 20).ok());
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(12));
  QVERIFY(scratch.session.trimInstance(instanceId, 1, 8, 20, 27).ok());
  QCOMPARE(scratch.session.readInstance(instanceId).value.sourceStart, static_cast<std::int64_t>(1));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(12));

  auto equal = scratch.session.setRecordMarkIn(pictureId, 12);
  QVERIFY(!equal.ok());
  QCOMPARE(equal.error.message, std::string("equal Mark In and Mark Out are an incomplete range"));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));

  QVERIFY(scratch.session.setRecordMarkOut(soundId, 9).ok());
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(12));
  QVERIFY(scratch.session.recordMarks(soundId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(soundId).value.markOut, static_cast<std::int64_t>(9));
  QVERIFY(!scratch.session.recordMarks(soundId).value.markIn.has_value());

  auto missing = scratch.session.setRecordMarkIn("missing-sequence", 1);
  QVERIFY(!missing.ok());
  QCOMPARE(missing.error.message, std::string("the sequence is not in this project"));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));

  QVERIFY(scratch.session.undo().ok());
  QCOMPARE(scratch.session.readInstance(instanceId).value.sourceStart, static_cast<std::int64_t>(0));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));
  QVERIFY(scratch.session.undo().ok());
  QCOMPARE(scratch.session.readInstance(instanceId).value.recordStart, static_cast<std::int64_t>(2));
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.readInstance(instanceId).ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(12));
  QVERIFY(scratch.session.redo().ok());
  QVERIFY(scratch.session.readInstance(instanceId).ok());
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(12));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
}

void MarkTests::windowFollowsSelectionAndScroll() {
  Scratch scratch;
  const auto first = scratch.writeMedia("a.mov");
  const auto second = scratch.writeMedia("b.mov");
  QVERIFY(scratch.session.createProject(scratch.parent / "Window", "Window").ok());
  QVERIFY(scratch.session.createSequence("Picture", {24, 1}).ok());
  QVERIFY(scratch.session.createSequence("Sound", {24, 1}).ok());
  auto imported = scratch.session.importReferences({first.string(), second.string()});
  QVERIFY(imported.ok());
  auto sequences = scratch.session.listSequences();
  const std::string pictureId = sequences.value[0].name == "Picture" ? sequences.value[0].sequenceId
                                                                     : sequences.value[1].sequenceId;
  const std::string soundId = sequences.value[0].name == "Sound" ? sequences.value[0].sequenceId
                                                                 : sequences.value[1].sequenceId;
  auto placed = scratch.session.placeInstance(pictureId, "", imported.value.front().objectId, {24, 1}, 0, 8, 2, 10);
  QVERIFY(placed.ok());
  const std::string instanceId = placed.value.instanceId;

  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1680, 1100);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QVERIFY(workspace != nullptr);
  auto* sequencesList = window.findChild<QListWidget*>("sequenceList");
  const int pictureRow = sequencesList->item(0)->text().contains("Picture") ? 0 : 1;
  QTest::mouseClick(sequencesList->viewport(), Qt::LeftButton, Qt::NoModifier,
                    sequencesList->visualItemRect(sequencesList->item(pictureRow)).center());
  QCOMPARE(workspace->selectedSequenceId(), QString::fromStdString(pictureId));

  auto* sourceFrame = window.findChild<QLineEdit*>("sourceMarkFrame");
  auto* sourceIn = window.findChild<QPushButton*>("sourceMarkIn");
  auto* sourceOut = window.findChild<QPushButton*>("sourceMarkOut");
  auto* sourceClear = window.findChild<QPushButton*>("sourceClearBoth");
  QVERIFY(sourceFrame != nullptr);
  auto* sourceTab = window.findChild<QPushButton*>("viewerSourceTab");
  QVERIFY(sourceTab != nullptr);
  sourceTab->click();
  QVERIFY(sourceIn->isVisible());
  auto clickViewer = [&window](QPushButton* button) {
    if (button == nullptr) {
      return false;
    }
    if (button->objectName().startsWith("sourceMark")) {
      auto* tab = window.findChild<QPushButton*>("viewerSourceTab");
      if (tab == nullptr) {
        return false;
      }
      tab->click();
    }
    if (!openPanelMenu(&window, "viewerPanelMenu") || !button->isVisible()) {
      return false;
    }
    QTest::mouseClick(button, Qt::LeftButton);
    return true;
  };
  auto* assets = window.findChild<QListWidget*>("assetList");
  int firstRow = assets->item(0)->text().contains("a.mov") ? 0 : 1;
  QTest::mouseClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier,
                    assets->visualItemRect(assets->item(firstRow)).center());
  sourceFrame->setText("10");
  QVERIFY(clickViewer(sourceIn));
  sourceFrame->setText("18");
  QVERIFY(clickViewer(sourceOut));
  const std::string repA = scratch.representationNamed("a.mov");
  const std::string repB = scratch.representationNamed("b.mov");
  auto stored = scratch.session.sourceMarks(repA);
  QVERIFY(stored.ok());
  QVERIFY(stored.value.markIn.has_value());
  QCOMPARE(*stored.value.markIn, static_cast<std::int64_t>(10));
  QVERIFY(stored.value.markOut.has_value());
  QCOMPARE(*stored.value.markOut, static_cast<std::int64_t>(18));
  const QString sourceReadout = window.findChild<QLabel*>("sourceMarkReadout")->text();
  QVERIFY(sourceReadout.contains("Mark In: 10 source frames"));
  QVERIFY(sourceReadout.contains("Mark Out: 18 source frames"));
  QVERIFY(sourceReadout.contains("Not probed"));
  QVERIFY(sourceReadout.contains("No source rate"));
  QVERIFY(sourceReadout.contains("No duration"));
  QVERIFY(sourceReadout.contains("not a half-open edit range"));
  QVERIFY(window.findChild<QLabel*>("sourceTimecode")->text().contains("Not probed"));

  sourceFrame->setText("10");
  QVERIFY(clickViewer(sourceOut));
  QVERIFY(scratch.session.sourceMarks(repA).value.markOut.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markOut, static_cast<std::int64_t>(18));
  QVERIFY(window.findChild<QLabel*>("sourceMarkReason")->text().contains("incomplete range"));
  sourceFrame->setText("1.5");
  QVERIFY(clickViewer(sourceIn));
  QVERIFY(scratch.session.sourceMarks(repA).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markIn, static_cast<std::int64_t>(10));
  QVERIFY(window.findChild<QLabel*>("sourceMarkReason")->text().contains("non-negative integer"));

  QVERIFY(clickViewer(sourceClear));
  QVERIFY(!scratch.session.sourceMarks(repA).value.markIn.has_value());
  QVERIFY(!scratch.session.sourceMarks(repA).value.markOut.has_value());
  sourceFrame->setText("10");
  QVERIFY(clickViewer(sourceIn));
  sourceFrame->setText("18");
  QVERIFY(clickViewer(sourceOut));

  QCOMPARE(assets->count(), 2);
  int otherRow = assets->item(0)->text().contains("b.mov") ? 0 : 1;
  const QRect otherRect = assets->visualItemRect(assets->item(otherRow));
  QTest::mouseClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier, otherRect.center());
  const QString otherReadout = window.findChild<QLabel*>("sourceMarkReadout")->text();
  QVERIFY(!otherReadout.contains("Mark In: 10 source frames"));
  QVERIFY(otherReadout.contains("Mark In: none"));
  QVERIFY(scratch.session.sourceMarks(repA).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(repA).value.markIn, static_cast<std::int64_t>(10));
  QVERIFY(sourceFrame->text().isEmpty());

  auto* recordFrame = window.findChild<QLineEdit*>("recordMarkFrame");
  auto* recordIn = window.findChild<QPushButton*>("recordMarkIn");
  auto* recordOut = window.findChild<QPushButton*>("recordMarkOut");
  auto* recordClear = window.findChild<QPushButton*>("recordClearBoth");
  recordFrame->setText("5");
  QVERIFY(clickViewer(recordIn));
  recordFrame->setText("400");
  QVERIFY(clickViewer(recordOut));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(400));
  QVERIFY(window.findChild<QLabel*>("recordMarkReadout")->text().contains("Mark In: 5 sequence frames"));
  QVERIFY(window.findChild<QLabel*>("recordMarkReadout")->text().contains("not a half-open edit range"));
  QVERIFY(window.findChild<QLabel*>("recordMarkMirror")->text().contains("Mark Out: 400 sequence frames"));
  QCOMPARE(scratch.session.readInstance(instanceId).value.recordStart, static_cast<std::int64_t>(2));

  auto* canvas = window.findChild<QWidget*>("timelineCanvas");
  const int rulerX = aether::kTimelineOriginPx + 6 * aether::kTimelinePixelsPerFrame;
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, QPoint(rulerX, 8));
  QCOMPARE(recordFrame->text(), QString("6"));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(5));
  QVERIFY(clickViewer(recordIn));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(6));

  const QPoint block = workspace->blockCenter(QString::fromStdString(instanceId));
  QVERIFY(!block.isNull());
  QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, block);
  QCOMPARE(workspace->selectedInstanceId(), QString::fromStdString(instanceId));
  QCOMPARE(recordFrame->text(), QString("6"));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(6));
  QVERIFY(workspace->blockIsSelected(QString::fromStdString(instanceId)));

  const std::optional<int> painted = aether::timelinePaintX(6, aether::kTimelinePixelsPerFrame, aether::kTimelineOriginPx);
  QVERIFY(painted.has_value());
  QCOMPARE(*painted, aether::kTimelineOriginPx + 6 * aether::kTimelinePixelsPerFrame);
  const QImage image = canvas->grab().toImage();
  QVERIFY(image.width() > *painted);
  const int sampleX = static_cast<int>(*painted * image.devicePixelRatio());
  const QRgb sample = image.pixel(sampleX, static_cast<int>(4 * image.devicePixelRatio()));
  QVERIFY2(qGreen(sample) > 150 && qGreen(sample) > qRed(sample), "mark stroke was not found at the converted x");

  auto* scroll = window.findChild<QScrollArea*>("timelineScroll");
  QVERIFY(scroll != nullptr);
  QScrollBar* bar = scroll->horizontalScrollBar();
  QVERIFY(bar->maximum() > 0);
  const int scrolled = bar->maximum();
  bar->setValue(scrolled);
  QCOMPARE(bar->value(), scrolled);
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(6));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markOut.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markOut, static_cast<std::int64_t>(400));
  QCOMPARE(*aether::timelinePaintX(6, aether::kTimelinePixelsPerFrame, aether::kTimelineOriginPx), *painted);

  QVERIFY(clickViewer(recordClear));
  QVERIFY(!scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QVERIFY(!scratch.session.recordMarks(pictureId).value.markOut.has_value());
  recordFrame->setText("6");
  QVERIFY(clickViewer(recordIn));
  recordFrame->setText("400");
  QVERIFY(clickViewer(recordOut));

  int soundRow = sequencesList->item(0)->text().contains("Sound") ? 0 : 1;
  const QRect soundRect = sequencesList->visualItemRect(sequencesList->item(soundRow));
  QTest::mouseClick(sequencesList->viewport(), Qt::LeftButton, Qt::NoModifier, soundRect.center());
  QCOMPARE(workspace->selectedSequenceId(), QString::fromStdString(soundId));
  QVERIFY(!window.findChild<QLabel*>("recordMarkReadout")->text().contains("Mark In: 6 sequence frames"));
  QVERIFY(window.findChild<QLabel*>("recordMarkReadout")->text().contains("Mark In: none"));
  QVERIFY(scratch.session.recordMarks(pictureId).value.markIn.has_value());
  QCOMPARE(*scratch.session.recordMarks(pictureId).value.markIn, static_cast<std::int64_t>(6));
  QCOMPARE(scratch.session.readInstance(instanceId).value.recordEnd, static_cast<std::int64_t>(10));
  QVERIFY(!scratch.session.canRedo());
  window.close();
}

void MarkTests::doubleClickLoadsSourceWithoutWriting() {
  Scratch scratch;
  const auto first = scratch.writeMedia("a.mov");
  const auto second = scratch.writeMedia("b.mov");
  QVERIFY(scratch.session.createProject(scratch.parent / "Viewer", "Viewer").ok());
  QVERIFY(scratch.session.createSequence("Picture", {24, 1}).ok());
  auto imported = scratch.session.importReferences({first.string(), second.string()});
  QVERIFY(imported.ok());
  auto sequences = scratch.session.listSequences();
  QVERIFY(sequences.ok());
  const std::string pictureId = sequences.value.front().sequenceId;
  auto placed = scratch.session.placeInstance(pictureId, "", imported.value.front().objectId, {24, 1}, 0, 8, 2, 10);
  QVERIFY(placed.ok());
  auto tracks = scratch.session.listTracks(pictureId);
  QVERIFY(tracks.ok());
  QCOMPARE(tracks.value.size(), static_cast<std::size_t>(1));
  const std::string trackId = tracks.value.front().trackId;

  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1680, 1100);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* assets = window.findChild<QListWidget*>("assetList");
  auto* stack = window.findChild<QStackedWidget*>("viewerStack");
  auto* sourceTab = window.findChild<QPushButton*>("viewerSourceTab");
  auto* recordTab = window.findChild<QPushButton*>("viewerRecordTab");
  QVERIFY(assets != nullptr && stack != nullptr && sourceTab != nullptr && recordTab != nullptr);
  QCOMPARE(stack->currentIndex(), 1);
  QVERIFY(recordTab->isChecked());

  auto rowFor = [assets](const QString& name) {
    for (int index = 0; index < assets->count(); ++index) {
      if (assets->item(index)->text().contains(name)) {
        return assets->visualItemRect(assets->item(index)).center();
      }
    }
    return QPoint();
  };
  const QPoint firstPoint = rowFor("a.mov");
  const QPoint secondPoint = rowFor("b.mov");
  QVERIFY(!firstPoint.isNull());
  QVERIFY(!secondPoint.isNull());

  auto doubleClickRow = [](QWidget* viewport, const QPoint& point) {
    QTest::mousePress(viewport, Qt::LeftButton, Qt::NoModifier, point);
    QTest::mouseDClick(viewport, Qt::LeftButton, Qt::NoModifier, point);
    QTest::mouseRelease(viewport, Qt::LeftButton, Qt::NoModifier, point);
  };
  doubleClickRow(assets->viewport(), firstPoint);
  QCOMPARE(stack->currentIndex(), 0);
  QVERIFY(sourceTab->isChecked());
  QVERIFY(sourceTab->text().contains("a.mov"));
  auto* sourceViewer = window.findChild<QFrame*>("sourceViewer");
  QVERIFY(sourceViewer != nullptr);
  QVERIFY(sourceViewer->isVisible());

  doubleClickRow(assets->viewport(), firstPoint);
  QCOMPARE(stack->currentIndex(), 1);
  QVERIFY(recordTab->isChecked());
  QVERIFY(!sourceTab->isChecked());

  doubleClickRow(assets->viewport(), secondPoint);
  QCOMPARE(stack->currentIndex(), 0);
  QVERIFY(sourceTab->isChecked());
  QVERIFY(sourceTab->text().contains("b.mov"));

  auto rows = scratch.session.listInstances(trackId);
  QVERIFY(rows.ok());
  QCOMPARE(rows.value.size(), static_cast<std::size_t>(1));
  QCOMPARE(rows.value.front().recordStart, static_cast<std::int64_t>(2));
  QCOMPARE(rows.value.front().recordEnd, static_cast<std::int64_t>(10));
  QCOMPARE(rows.value.front().sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(rows.value.front().sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(scratch.session.info().schemaVersion, 2);
  window.close();
}

void MarkTests::placementOffersPointsAndStillPlaces() {
  Scratch scratch;
  const auto picture = scratch.writeMedia("offer.mov");
  QVERIFY(scratch.session.createProject(scratch.parent / "Offer", "Offer").ok());
  QVERIFY(scratch.session.createSequence("Offer", {24, 1}).ok());
  auto imported = scratch.session.importReferences({picture.string()});
  QVERIFY(imported.ok());
  const std::string rep = scratch.representationNamed("offer.mov");
  QVERIFY(scratch.session.setSourceMarkIn(rep, 100).ok());
  QVERIFY(scratch.session.setSourceMarkOut(rep, 140).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  const QString objectId = QString::fromStdString(imported.value.front().objectId);
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
    if (!dialog->findChild<QLineEdit*>("placementSourceStart")->text().isEmpty() ||
        !dialog->findChild<QLineEdit*>("placementSourceEnd")->text().isEmpty()) {
      failure = "source span was filled from marks";
      dialog->reject();
      return;
    }
    const QString offer = dialog->findChild<QLabel*>("placementMarkOffer")->text();
    if (!offer.contains("Mark In 100") || !offer.contains("Mark Out 140") || !offer.contains("not copied") ||
        !offer.contains("not a half-open edit range") || !offer.contains("not probed")) {
      failure = offer;
      dialog->reject();
      return;
    }
    dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(2);
    dialog->findChild<QLineEdit*>("placementSourceStart")->setText("0");
    dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("8");
    if (!dialog->findChild<QLabel*>("placementValidation")->text().contains("no duration mapping is authorised")) {
      failure = dialog->findChild<QLabel*>("placementValidation")->text();
      dialog->reject();
      return;
    }
    dialog->findChild<QPushButton*>("placementCommit")->click();
    if (dialog->result() == QDialog::Accepted) {
      failure = "a mixed-rate place was accepted";
      return;
    }
    dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(1);
    dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("8");
    if (!dialog->findChild<QLabel*>("placementValidation")->text().contains("Record [0,8)")) {
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
  workspace->beginPlacement(objectId, QString(), 0);
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  const QString instanceId = workspace->selectedInstanceId();
  QVERIFY(!instanceId.isEmpty());
  auto row = scratch.session.readInstance(instanceId.toStdString());
  QVERIFY(row.ok());
  QCOMPARE(row.value.sourceStart, static_cast<std::int64_t>(0));
  QCOMPARE(row.value.sourceEnd, static_cast<std::int64_t>(8));
  QCOMPARE(row.value.recordStart, static_cast<std::int64_t>(0));
  QCOMPARE(row.value.recordEnd, static_cast<std::int64_t>(8));
  QCOMPARE(row.value.sourceRate.numerator, static_cast<std::int64_t>(24));
  QVERIFY(scratch.session.sourceMarks(rep).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(rep).value.markIn, static_cast<std::int64_t>(100));
  QVERIFY(scratch.session.sourceMarks(rep).value.markOut.has_value());
  QCOMPARE(*scratch.session.sourceMarks(rep).value.markOut, static_cast<std::int64_t>(140));
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.undo().ok());
  QVERIFY(!scratch.session.readInstance(instanceId.toStdString()).ok());
  QVERIFY(scratch.session.canUndo());
  QVERIFY(scratch.session.sourceMarks(rep).value.markIn.has_value());
  QCOMPARE(*scratch.session.sourceMarks(rep).value.markIn, static_cast<std::int64_t>(100));
  window.close();
}
