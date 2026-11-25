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

// Timeline workspace. It calls ProjectSession. It does not include the store
// and it does not write project.db itself.

#include "aether/app/project_session.hpp"

#include <QHash>
#include <QPoint>
#include <QStringList>
#include <QWidget>

#include <cstdint>

class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace aether {

class TimelineCanvas;
class TrackHeaderColumn;
class TimelineNavigator;

// The picture under the display playhead. The index is the stored source
// offset plus the sequence frames into that instance. It is not a new time.
struct PlayheadPicture {
  bool covered = false;
  QString path;
  std::int64_t mediaIndex = -1;
  QString note;
};

class TimelineWorkspace : public QWidget {
  Q_OBJECT

 public:
  explicit TimelineWorkspace(ProjectSession& session, QWidget* parent = nullptr);

  void reload();
  QString selectedSequenceId() const;
  QStringList trackIdsInOrder() const;
  QString selectedInstanceId() const;
  QString inspectorText() const;
  QString rejectionText() const;
  QString rulerCaption() const;
  QString recordMarkReadoutText() const;
  QStringList extractParticipantIds() const;

  bool selectSequence(const QString& sequenceId);
  bool selectInstance(const QString& instanceId);
  void clearInstanceSelection();
  void beginPlacement(const QString& objectId, const QString& trackId, std::int64_t recordFrame);
  void setOverwriteSource(const QString& objectId, const QString& representationId);
  QPoint blockCenter(const QString& instanceId) const;
  // QApplication does not deliver a Drop unless a drag is already active.
  // This calls the canvas event handler directly, which is the drop path.
  bool deliverTimelineEvent(QEvent* event);
  bool blockIsSelected(const QString& instanceId) const;
  void setMoveRecordStartText(const QString& text);
  void setTrimText(const QString& sourceStart,
                   const QString& sourceEnd,
                   const QString& recordStart,
                   const QString& recordEnd);
  std::int64_t playheadFrame() const;
  void setPlayheadFrame(std::int64_t frame);
  std::int64_t contentEndFrame() const;
  PlayheadPicture pictureAtPlayhead() const;

 public slots:
  void applyMove();
  void applyTrim();
  void applyOverwrite();
  void applySplit();
  void applyLift();
  void applyExtract();
  void undoEdit();
  void redoEdit();
  void markRecordIn();
  void markRecordOut();

 signals:
  void selectionChanged();
  void recordMarksChanged();
  void historyChanged();
  void timelinePlayRequested();
  void timelineStopRequested();
  void playheadChanged();

 private:
  void reloadSequences();
  void showSelectedSequence();
  void refreshCanvas();
  void showInspector();
  void fillFieldsFromSelection();
  void explain(const std::string& message);
  void syncUndo();
  void clearEditFields();
  void refreshRecordMarks();
  void refreshOverwriteTracks();
  void updateSequenceTab();
  void syncTrackChrome();
  QHash<QString, QString> fileNamesByObject() const;
  void clearRecordIn();
  void clearRecordOut();
  void clearRecordBoth();
  void applyRecordPoint(bool isIn);
  void clearRecordPoint(bool clearIn, bool clearOut);
  void publishExtractRange();
  void refreshExtractTracks(bool sequenceChanged);
  void acceptExtractFrame(bool ok, std::int64_t frame, const QString& reason);
  void updateVisibleEnd(const std::vector<TimelineInstance>& rows);
  void resetRangeInteraction();

  ProjectSession& session_;
  QListWidget* sequences_ = nullptr;
  QListWidget* instances_ = nullptr;
  QLabel* inspector_ = nullptr;
  QLabel* rejection_ = nullptr;
  QLabel* rulerCaption_ = nullptr;
  QLineEdit* moveStart_ = nullptr;
  QLineEdit* trimSourceStart_ = nullptr;
  QLineEdit* trimSourceEnd_ = nullptr;
  QLineEdit* trimRecordStart_ = nullptr;
  QLineEdit* trimRecordEnd_ = nullptr;
  QPushButton* applyMoveButton_ = nullptr;
  QPushButton* applyTrimButton_ = nullptr;
  QPushButton* undoButton_ = nullptr;
  QPushButton* redoButton_ = nullptr;
  QComboBox* overwriteTrack_ = nullptr;
  QComboBox* overwriteRate_ = nullptr;
  QPushButton* overwriteButton_ = nullptr;
  QLineEdit* splitFrame_ = nullptr;
  QLabel* splitFrameShown_ = nullptr;
  QPushButton* splitButton_ = nullptr;
  QString overwriteObjectId_;
  QString overwriteRepresentationId_;
  QLineEdit* recordMarkFrame_ = nullptr;
  QLabel* recordMarkReadout_ = nullptr;
  QLabel* recordMarkReason_ = nullptr;
  QPushButton* sourceTab_ = nullptr;
  QPushButton* sequenceTab_ = nullptr;
  QLabel* timelineClock_ = nullptr;
  TimelineCanvas* canvas_ = nullptr;
  TrackHeaderColumn* trackHeaders_ = nullptr;
  TimelineNavigator* navigator_ = nullptr;
  QString displayedSequenceId_;
  QPushButton* extractRangeMode_ = nullptr;
  QLineEdit* extractStart_ = nullptr;
  QLineEdit* extractEnd_ = nullptr;
  QLabel* extractRangeReadout_ = nullptr;
  QLabel* visibleEnd_ = nullptr;
  QPushButton* extractButton_ = nullptr;
  bool extractAwaitingEnd_ = false;
  bool reloading_ = false;
};

}  // namespace aether
