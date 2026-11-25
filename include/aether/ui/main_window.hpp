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

#pragma once

#include "aether/app/project_session.hpp"
#include "aether/media/picture.hpp"
#include "aether/ui/probe_controller.hpp"
#include "aether/ui/timeline_workspace.hpp"

#include <QCloseEvent>
#include <QMainWindow>
#include <QStringList>

class QAction;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QStackedWidget;
class QTimer;

namespace aether {

class PictureView;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(ProjectSession& session, Logger& logger, QWidget* parent = nullptr);

  void showProjectDialog();
  void importPaths(const QStringList& paths);
  void syncFromSession();

 protected:
  void closeEvent(QCloseEvent* event) override;

 private slots:
  void createProject();
  void openProject();
  void closeProject();
  void binChosen();
  void assetChosen();
  void importMedia();
  void showMode(int index);
  void openClipInViewer(QListWidgetItem* item);
  void showSelection();
  void probeSelected();
  void showProbe();
  void syncProbeSelection(const std::string& representationId, const std::string& recordedPath);

 private:
  void refresh();
  void showViewerPage(int index);
  void loadSourcePicture(const QString& objectId);
  void playSource();
  void stopSource();
  void advanceSource();
  void placeLoadedSource();
  void playRecord();
  void stopRecord();
  void advanceRecord();
  void showRecordPicture();
  void updatePlayheadLabel();
  QString sourcePictureLabel(bool playing) const;
  std::string recordedPathForObject(const std::string& objectId) const;
  void showBinAssets();
  void showRates();
  void showError(const Error& error);
  void refreshSourceMarks();
  void showRecordMarks();
  void markSourceIn();
  void markSourceOut();
  void clearSourceIn();
  void clearSourceOut();
  void clearSourceBoth();
  void applySourcePoint(bool isIn);
  void clearSourcePoint(bool clearIn, bool clearOut);
  std::string selectedRepresentationId();
  std::vector<BinAssetFact> assetsForSelection(const std::string& binId, std::string* error);

  ProjectSession& session_;
  Logger& logger_;
  QLabel* nameLabel_ = nullptr;
  QLabel* statusLabel_ = nullptr;
  QLabel* emptyBinLabel_ = nullptr;
  QLabel* inspectorLabel_ = nullptr;
  QLabel* previewCard_ = nullptr;
  QLabel* sourceTimecode_ = nullptr;
  QLabel* recordTimecode_ = nullptr;
  QLineEdit* sourceMarkFrame_ = nullptr;
  QLabel* sourceMarkReadout_ = nullptr;
  QLabel* sourceMarkReason_ = nullptr;
  QLabel* recordMarkMirror_ = nullptr;
  QString entryRepresentationId_;
  QLabel* playheadLabel_ = nullptr;
  QLabel* rateLabel_ = nullptr;
  QLabel* probeStatus_ = nullptr;
  QPlainTextEdit* probeReport_ = nullptr;
  ProbeController* probeController_ = nullptr;
  std::string probeRepresentationId_;
  std::string probePath_;
  QListWidget* bins_ = nullptr;
  QListWidget* assets_ = nullptr;
  QListWidget* tags_ = nullptr;
  QStackedWidget* modes_ = nullptr;
  QStackedWidget* viewerStack_ = nullptr;
  QString loadedSourceObjectId_;
  PictureView* sourceView_ = nullptr;
  PictureView* recordView_ = nullptr;
  PictureReader sourceReader_;
  PictureReader recordReader_;
  QString recordPath_;
  QTimer* sourceTimer_ = nullptr;
  QTimer* recordTimer_ = nullptr;
  std::int64_t sourceIndex_ = 0;
  bool sourcePlaying_ = false;
  bool recordPlaying_ = false;
  TimelineWorkspace* workspace_ = nullptr;
  QAction* undoAction_ = nullptr;
  QAction* redoAction_ = nullptr;
  std::string rootBinId_;
  bool closing_ = false;
  bool rebuildingBins_ = false;
  bool updatingSelection_ = false;
};

}  // namespace aether
