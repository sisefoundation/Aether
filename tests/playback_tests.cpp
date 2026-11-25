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

#include "playback_tests.hpp"

#include "aether/app/project_session.hpp"
#include "aether/core/project/ids.hpp"
#include "aether/media/picture.hpp"
#include "aether/persistence/logger.hpp"
#include "aether/ui/main_window.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTest>
#include <QTimer>

#include <filesystem>
#include <sys/stat.h>

namespace {

std::filesystem::path mxfPath() {
  return std::filesystem::path(AETHER_SOURCE_DIR) / "validation-fixtures/ui-integration/media/Normal_test.mxf";
}

struct FileStamp {
  off_t size = 0;
  time_t mtime = 0;
};

FileStamp stampOf(const std::filesystem::path& path) {
  struct stat st {};
  stat(path.c_str(), &st);
  return {st.st_size, st.st_mtime};
}

bool sampleDiffers(const std::vector<std::uint8_t>& left, const std::vector<std::uint8_t>& right) {
  const std::size_t count = std::min(left.size(), right.size());
  for (std::size_t index = 0; index < count; index += 1000) {
    if (left[index] != right[index]) {
      return true;
    }
  }
  return left.size() != right.size();
}

bool sampleHasColor(const std::vector<std::uint8_t>& rgb) {
  for (std::size_t index = 0; index < rgb.size(); index += 97) {
    if (rgb[index] != 0) {
      return true;
    }
  }
  return false;
}

class Scratch {
 public:
  Scratch() : session(logger) {
    const auto id = aether::makeStableId();
    const std::string token = id ? *id : std::string("tmp");
    parent = std::filesystem::temp_directory_path() / ("aether-playback-" + token);
    std::filesystem::create_directories(parent);
  }

  ~Scratch() {
    session.closeProject();
    std::error_code ignored;
    std::filesystem::remove_all(parent, ignored);
  }

  aether::MemoryLogger logger;
  aether::ProjectSession session;
  std::filesystem::path parent;
};

}  // namespace

void PlaybackTests::mxfPictureIsReadableAndUnchanged() {
  const auto path = mxfPath();
  QVERIFY(std::filesystem::is_regular_file(path));
  const FileStamp before = stampOf(path);
  const auto entriesBefore = std::distance(std::filesystem::directory_iterator(path.parent_path()),
                                           std::filesystem::directory_iterator());
  aether::PictureReader reader;
  std::string note;
  QVERIFY2(reader.open(path.string(), &note), note.c_str());
  std::int64_t microseconds = 0;
  std::string field;
  QVERIFY(reader.reportedDisplayIntervalUs(&microseconds, &field));
  QCOMPARE(field, std::string("r_frame_rate"));
  QVERIFY(microseconds > 0);
  const aether::DisplayPicture first = reader.pictureAt(0);
  QVERIFY2(first.present, first.note.c_str());
  QCOMPARE(first.width, 1920);
  QCOMPARE(first.height, 1080);
  QCOMPARE(first.mediaIndex, static_cast<std::int64_t>(0));
  QVERIFY(sampleHasColor(first.rgb));
  const aether::DisplayPicture second = reader.pictureAt(1);
  QVERIFY2(second.present, second.note.c_str());
  QCOMPARE(second.mediaIndex, static_cast<std::int64_t>(1));
  QVERIFY(sampleDiffers(first.rgb, second.rgb));
  reader.close();
  const FileStamp after = stampOf(path);
  QCOMPARE(after.size, before.size);
  QCOMPARE(after.mtime, before.mtime);
  QCOMPARE(std::distance(std::filesystem::directory_iterator(path.parent_path()), std::filesystem::directory_iterator()),
           entriesBefore);
}

void PlaybackTests::sourceRecordAndTimelinePlaybackUseTheMxf() {
  const auto path = mxfPath();
  const FileStamp before = stampOf(path);
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.parent / "PlaybackCut", "PlaybackCut").ok());
  QVERIFY(scratch.session.createSequence("PlaybackCut", {25, 1}).ok());
  auto imported = scratch.session.importReferences({path.string()});
  QVERIFY(imported.ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.resize(1680, 1100);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* assets = window.findChild<QListWidget*>("assetList");
  QVERIFY(assets != nullptr);
  QPoint rowCenter;
  for (int index = 0; index < assets->count(); ++index) {
    if (assets->item(index)->text().contains("Normal_test.mxf")) {
      rowCenter = assets->visualItemRect(assets->item(index)).center();
    }
  }
  QVERIFY(!rowCenter.isNull());
  QTest::mousePress(assets->viewport(), Qt::LeftButton, Qt::NoModifier, rowCenter);
  QTest::mouseDClick(assets->viewport(), Qt::LeftButton, Qt::NoModifier, rowCenter);
  QTest::mouseRelease(assets->viewport(), Qt::LeftButton, Qt::NoModifier, rowCenter);
  auto* source = window.findChild<QFrame*>("sourceViewer");
  QVERIFY(source != nullptr);
  auto* sourceLabel = window.findChild<QLabel*>("sourceTimecode");
  QVERIFY2(source->accessibleDescription() == "picture", qPrintable(sourceLabel->text()));
  QVERIFY(sourceLabel->text().contains("Picture 0."));
  QVERIFY(sourceLabel->text().contains("not an editorial time"));
  QVERIFY(sourceLabel->text().contains("Stopped"));

  window.findChild<QPushButton*>("sourcePlay")->click();
  QVERIFY(sourceLabel->text().contains("Playing"));
  QTest::qWait(800);
  window.findChild<QPushButton*>("sourceStop")->click();
  QVERIFY(sourceLabel->text().contains("Stopped"));
  QVERIFY(!sourceLabel->text().contains("Picture 0."));
  QCOMPARE(source->accessibleDescription(), QString("picture"));

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
    dialog->findChild<QComboBox*>("placementSourceRate")->setCurrentIndex(2);
    dialog->findChild<QLineEdit*>("placementSourceStart")->setText("0");
    dialog->findChild<QLineEdit*>("placementSourceEnd")->setText("8");
    dialog->findChild<QPushButton*>("placementCommit")->click();
    if (dialog->result() != QDialog::Accepted) {
      failure = dialog->findChild<QLabel*>("placementMessage")->text();
      dialog->reject();
    }
  });
  window.findChild<QPushButton*>("sourcePlace")->click();
  QVERIFY2(failure.isEmpty(), qPrintable(failure));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  QVERIFY(workspace != nullptr);
  QCOMPARE(workspace->playheadFrame(), static_cast<std::int64_t>(0));
  QCOMPARE(workspace->contentEndFrame(), static_cast<std::int64_t>(8));
  auto* record = window.findChild<QFrame*>("sequenceViewer");
  QCOMPARE(record->accessibleDescription(), QString("picture"));
  auto* playhead = window.findChild<QLabel*>("playhead");
  QVERIFY(playhead->text().contains("00:00:00:00"));
  QVERIFY(playhead->text().contains("PlaybackCut"));
  QVERIFY(playhead->text().contains("Stopped"));

  window.findChild<QPushButton*>("timelinePlay")->click();
  QVERIFY(playhead->text().contains("Playing"));
  QTest::qWait(300);
  window.findChild<QPushButton*>("timelineStop")->click();
  QVERIFY(playhead->text().contains("Stopped"));
  QVERIFY(workspace->playheadFrame() > 0);
  QVERIFY(workspace->playheadFrame() < 8);
  QCOMPARE(record->accessibleDescription(), QString("picture"));
  QVERIFY(playhead->text().contains("PlaybackCut"));
  const FileStamp after = stampOf(path);
  QCOMPARE(after.size, before.size);
  QCOMPARE(after.mtime, before.mtime);
  window.close();
}

void PlaybackTests::playheadLabelFollowsTheStoredFrame() {
  Scratch scratch;
  QVERIFY(scratch.session.createProject(scratch.parent / "Label", "Label").ok());
  QVERIFY(scratch.session.createSequence("Reel", {24, 1}).ok());
  aether::MainWindow window(scratch.session, scratch.logger);
  window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  auto* workspace = window.findChild<aether::TimelineWorkspace*>();
  workspace->setPlayheadFrame(3);
  auto* playhead = window.findChild<QLabel*>("playhead");
  QVERIFY(playhead->text().contains("00:00:00:03"));
  QVERIFY(playhead->text().contains("Reel"));
  QVERIFY(playhead->text().contains("Stopped"));
  window.findChild<QPushButton*>("recordPlay")->click();
  QCOMPARE(window.findChild<QLabel*>("statusLine")->text(), QString("The timeline has no picture to play."));
  QVERIFY(playhead->text().contains("Stopped"));
  QCOMPARE(workspace->playheadFrame(), static_cast<std::int64_t>(3));
  window.close();
}
