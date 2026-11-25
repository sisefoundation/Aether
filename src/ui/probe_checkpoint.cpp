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

#include "aether/app/project_session.hpp"
#include "aether/ui/main_window.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QFont>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCursor>
#include <QProcess>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace aether {
namespace {

void pump(int milliseconds) {
  QEventLoop loop;
  QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
  loop.exec();
}

bool waitForText(QLabel* label, const QString& text, int milliseconds) {
  QElapsedTimer timer;
  timer.start();
  while (timer.elapsed() < milliseconds) {
    QApplication::processEvents();
    if (label->text() == text) {
      return true;
    }
    QThread::msleep(20);
  }
  return label->text() == text;
}

bool selectNamed(QListWidget* list, const QString& name) {
  for (int index = 0; index < list->count(); ++index) {
    if (list->item(index)->text().contains(name)) {
      list->setCurrentRow(index);
      QApplication::processEvents();
      return true;
    }
  }
  return false;
}

void reveal(QPlainTextEdit* report, const QString& needle) {
  if (report == nullptr || needle.isEmpty()) {
    return;
  }
  const QTextCursor found = report->document()->find(needle);
  if (found.isNull()) {
    std::cerr << "checkpoint could not reveal " << needle.toStdString() << '\n';
    return;
  }
  report->setTextCursor(found);
  report->centerCursor();
  QApplication::processEvents();
  auto* bar = report->verticalScrollBar();
  if (bar != nullptr) {
    bar->setValue(std::max(0, bar->value() + report->cursorRect().top() - 6));
  }
  QApplication::processEvents();
}

bool saveWindow(QWidget* window, const std::string& path) {
  window->raise();
  window->activateWindow();
  pump(300);
  const QString target = QString::fromStdString(path);
  const QString id = QString::number(static_cast<qulonglong>(window->winId()));
  const int code = QProcess::execute("import", {"-window", id, target});
  if (code != 0 || QFileInfo(target).size() < 20000) {
    std::cerr << "live window capture failed for " << path << '\n';
    return false;
  }
  return true;
}

void writeGate(const std::string& path, const std::string& text) {
  std::ofstream output(path, std::ios::trunc);
  output << text;
}

}  // namespace

bool runProbeCheckpoint(MainWindow& window, ProjectSession& session, const std::string& projectDir,
                        const std::string& shotDir, const std::vector<std::string>& imports) {
  std::filesystem::create_directories(shotDir);
  auto created = session.createProject(projectDir, "ProbeCut");
  if (!created.ok()) {
    std::cerr << "checkpoint create failed: " << created.error.message << '\n';
    return false;
  }
  auto imported = session.importReferences(imports);
  if (!imported.ok() || imported.value.empty()) {
    std::cerr << "checkpoint import failed\n";
    return false;
  }
  auto sequence = session.createSequence("Probe", FrameRate{24, 1});
  if (!sequence.ok()) {
    std::cerr << "checkpoint sequence failed: " << sequence.error.message << '\n';
    return false;
  }
  auto placed = session.placeInstance(sequence.value.sequenceId, "", imported.value.front().objectId, FrameRate{24, 1},
                                      0, 8, 0, 8);
  if (!placed.ok()) {
    std::cerr << "checkpoint place failed: " << placed.error.message << '\n';
    return false;
  }
  window.syncFromSession();
  window.setMaximumSize(1880, 1040);
  window.setGeometry(0, 0, 1860, 1000);
  window.show();
  window.setGeometry(0, 0, 1860, 1000);
  pump(300);

  auto* assets = window.findChild<QListWidget*>("assetList");
  auto* status = window.findChild<QLabel*>("probeStatus");
  auto* report = window.findChild<QPlainTextEdit*>("probeReport");
  auto* probe = window.findChild<QPushButton*>("probeMedia");
  auto* sourceFrame = window.findChild<QLineEdit*>("sourceMarkFrame");
  auto* sourceIn = window.findChild<QPushButton*>("sourceMarkIn");
  auto* sourceReadout = window.findChild<QLabel*>("sourceMarkReadout");
  auto* recordFrame = window.findChild<QLineEdit*>("recordMarkFrame");
  auto* recordIn = window.findChild<QPushButton*>("recordMarkIn");
  auto* instances = window.findChild<QListWidget*>("instanceList");
  if (assets == nullptr || status == nullptr || report == nullptr || probe == nullptr || sourceFrame == nullptr ||
      sourceIn == nullptr || sourceReadout == nullptr || recordFrame == nullptr || recordIn == nullptr ||
      instances == nullptr) {
    std::cerr << "checkpoint widgets missing\n";
    return false;
  }
  report->setFont(QFont(QStringLiteral("monospace"), 8));
  report->setMinimumHeight(220);

  if (!selectNamed(assets, "video.mkv") || status->text() != "Not probed") {
    std::cerr << "checkpoint A status " << status->text().toStdString() << '\n';
    return false;
  }
  if (!saveWindow(&window, shotDir + "/A-not-probed.png")) {
    return false;
  }

  const std::string gate = "/tmp/aether-slice1e-probe-gate";
  setenv("AETHER_PROBE_GATE", gate.c_str(), 1);
  writeGate(gate, "");
  probe->click();
  if (!waitForText(status, "Probing", 3000)) {
    std::cerr << "checkpoint did not reach Probing: " << status->text().toStdString() << '\n';
    return false;
  }
  sourceFrame->setText("4");
  sourceIn->click();
  QApplication::processEvents();
  if (!sourceReadout->text().contains("Mark In: 4")) {
    std::cerr << "checkpoint mark did not update while probing\n";
    writeGate(gate, "go\n");
    return false;
  }
  if (status->text() != "Probing" || !saveWindow(&window, shotDir + "/B-probing-responsive.png")) {
    writeGate(gate, "go\n");
    return false;
  }
  writeGate(gate, "go\n");
  if (!waitForText(status, "Probed", 10000) || !report->toPlainText().contains("video stream")) {
    std::cerr << "checkpoint video probe failed: " << report->toPlainText().left(400).toStdString() << '\n';
    return false;
  }
  reveal(report, "video stream");
  if (!saveWindow(&window, shotDir + "/C-video-metadata.png")) {
    return false;
  }

  if (!selectNamed(assets, "audio.wav")) {
    return false;
  }
  probe->click();
  if (!waitForText(status, "Probed", 10000) || !report->toPlainText().contains("audio stream")) {
    std::cerr << "checkpoint audio probe failed\n";
    return false;
  }
  reveal(report, "audio stream");
  if (!saveWindow(&window, shotDir + "/D-audio-metadata.png")) {
    return false;
  }

  if (!selectNamed(assets, "multi.mkv")) {
    return false;
  }
  probe->click();
  if (!waitForText(status, "Probed", 10000)) {
    return false;
  }
  const QString multi = report->toPlainText();
  if (!multi.contains("video stream") || !multi.contains("audio stream 1") || !multi.contains("audio stream 2") ||
      !multi.contains("subtitle stream")) {
    std::cerr << "checkpoint multi probe missing streams\n";
    return false;
  }
  reveal(report, "audio stream 1");
  if (!saveWindow(&window, shotDir + "/E-multiple-streams.png")) {
    return false;
  }

  if (!selectNamed(assets, "vfr.mp4")) {
    return false;
  }
  probe->click();
  if (!waitForText(status, "Probed", 10000)) {
    return false;
  }
  const QString conflict = report->toPlainText();
  if (!conflict.contains("frame rate candidates conflict") || !conflict.contains("verified source rate: no") ||
      !conflict.contains("not a proven frame rate")) {
    std::cerr << "checkpoint conflict text missing\n";
    return false;
  }
  reveal(report, "frame rate candidates conflict");
  if (!saveWindow(&window, shotDir + "/F-unknown-or-conflicting-timing.png")) {
    return false;
  }

  if (!selectNamed(assets, "malformed.mov")) {
    return false;
  }
  probe->click();
  if (!waitForText(status, "Failed", 10000) || !report->toPlainText().contains("error ")) {
    std::cerr << "checkpoint failure probe did not fail\n";
    return false;
  }
  reveal(report, "error ");
  if (!saveWindow(&window, shotDir + "/G-probe-failure.png")) {
    return false;
  }

  const std::string project = session.info().directory;
  if (!session.closeProject().ok()) {
    return false;
  }
  window.syncFromSession();
  if (!session.openProject(project).ok()) {
    return false;
  }
  window.syncFromSession();
  pump(200);
  if (instances->count() < 1 || !instances->item(0)->text().contains("duration 8")) {
    std::cerr << "checkpoint reopened timeline lost the placed clip\n";
    return false;
  }
  if (!selectNamed(assets, "video.mkv")) {
    return false;
  }
  sourceFrame->setText("11");
  sourceIn->click();
  recordFrame->setText("2");
  recordIn->click();
  QApplication::processEvents();
  if (!sourceReadout->text().contains("Mark In: 11")) {
    std::cerr << "checkpoint marks did not work after reopen\n";
    return false;
  }
  instances->setCurrentRow(0);
  QApplication::processEvents();
  return saveWindow(&window, shotDir + "/H-marks-bin-timeline-after-reopen.png");
}

}  // namespace aether
