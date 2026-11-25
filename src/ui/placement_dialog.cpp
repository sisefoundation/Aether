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

#include "aether/ui/placement_dialog.hpp"

#include "aether/core/sequence/sequence.hpp"
#include "aether/ui/frame_label.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aether {
namespace {

struct NamedRate {
  const char* label;
  FrameRate rate;
};

const NamedRate kRates[] = {
    {"24/1", {24, 1}},
    {"25/1", {25, 1}},
    {"24000/1001", {24000, 1001}},
    {"30000/1001", {30000, 1001}},
    {"60000/1001", {60000, 1001}},
};

}  // namespace

PlacementDialog::PlacementDialog(ProjectSession& session, QWidget* parent)
    : QDialog(parent), session_(session) {
  setObjectName("placementDialog");
  setWindowTitle("Place on timeline");
  setModal(true);
  setStyleSheet(
      "QDialog, QWidget { background: #161616; color: #d8d8d8; }"
      "QLabel { color: #d8d8d8; }"
      "QLineEdit, QComboBox { background: #1e1e1e; color: #d8d8d8; border: 1px solid #2c2c2c; padding: 4px; }"
      "QPushButton { background: #2a2a2a; color: #d8d8d8; border: 1px solid #3a3a3a; padding: 6px 10px; }");
  resize(520, 460);

  auto* layout = new QVBoxLayout(this);
  auto* title = new QLabel("Place a registered reference on the sequence", this);
  title->setObjectName("dialogTitle");
  layout->addWidget(title);

  auto* form = new QFormLayout();
  file_ = new QLabel(this);
  file_->setObjectName("placementFile");
  file_->setWordWrap(true);
  form->addRow("Media", file_);
  sequence_ = new QLabel(this);
  sequence_->setObjectName("placementSequence");
  sequence_->setWordWrap(true);
  form->addRow("Sequence", sequence_);
  track_ = new QLabel(this);
  track_->setObjectName("placementTrack");
  track_->setWordWrap(true);
  tracks_ = new QComboBox(this);
  tracks_->setObjectName("placementTrackChoice");
  form->addRow("Track", track_);
  form->addRow("", tracks_);
  recordIn_ = new QLineEdit(this);
  recordIn_->setObjectName("placementRecordIn");
  form->addRow("Record in, sequence frames", recordIn_);
  sourceRate_ = new QComboBox(this);
  sourceRate_->setObjectName("placementSourceRate");
  sourceRate_->addItem("Choose a source rate");
  for (const NamedRate& named : kRates) {
    sourceRate_->addItem(named.label);
  }
  form->addRow("Source rate, not probed", sourceRate_);
  sourceStart_ = new QLineEdit(this);
  sourceStart_->setObjectName("placementSourceStart");
  form->addRow("Source in, source frames", sourceStart_);
  sourceEnd_ = new QLineEdit(this);
  sourceEnd_->setObjectName("placementSourceEnd");
  form->addRow("Source out, source frames", sourceEnd_);
  offer_ = new QLabel(this);
  offer_->setObjectName("placementMarkOffer");
  offer_->setWordWrap(true);
  form->addRow("Source marks", offer_);
  validation_ = new QLabel(this);
  validation_->setObjectName("placementValidation");
  validation_->setWordWrap(true);
  form->addRow("Record span", validation_);
  layout->addLayout(form);

  message_ = new QLabel(this);
  message_->setObjectName("placementMessage");
  message_->setWordWrap(true);
  layout->addWidget(message_);

  auto* place = new QPushButton("Place", this);
  place->setObjectName("placementCommit");
  layout->addWidget(place);
  connect(place, &QPushButton::clicked, this, &PlacementDialog::commit);
  connect(sourceRate_, &QComboBox::currentIndexChanged, this, &PlacementDialog::refreshValidation);
  connect(recordIn_, &QLineEdit::textChanged, this, &PlacementDialog::refreshValidation);
  connect(sourceStart_, &QLineEdit::textChanged, this, &PlacementDialog::refreshValidation);
  connect(sourceEnd_, &QLineEdit::textChanged, this, &PlacementDialog::refreshValidation);
  connect(tracks_, &QComboBox::currentIndexChanged, this, &PlacementDialog::refreshValidation);
}

void PlacementDialog::setRequest(const QString& sequenceId,
                                const QString& objectId,
                                const QString& trackId,
                                std::int64_t recordFrame) {
  sequenceId_ = sequenceId;
  objectId_ = objectId;
  fixedTrackId_ = trackId;
  createdId_.clear();
  online_ = false;
  tracks_->clear();
  recordIn_->setText(QString::number(recordFrame));
  sourceRate_->setCurrentIndex(0);
  sourceStart_->clear();
  sourceEnd_->clear();
  offer_->clear();
  message_->clear();

  auto sequence = session_.readSequence(sequenceId.toStdString());
  if (!sequence.ok()) {
    sequence_->setText(QString::fromStdString(sequence.error.message));
    file_->setText("No media selected.");
    refreshValidation();
    return;
  }
  sequence_->setText(QString::fromStdString(sequence.value.name + "  " + std::to_string(sequence.value.rate.numerator) +
                                            "/" + std::to_string(sequence.value.rate.denominator) +
                                            " sequence frames"));

  BinAssetFact found;
  bool have = false;
  auto bins = session_.listBins();
  if (bins.ok()) {
    for (const BinRecord& bin : bins.value) {
      auto assets = session_.listBinAssets(bin.binId);
      if (!assets.ok()) {
        continue;
      }
      for (const BinAssetFact& asset : assets.value) {
        if (asset.objectId == objectId.toStdString()) {
          found = asset;
          have = true;
        }
      }
    }
  }
  if (!have) {
    file_->setText("The selected reference is not in this project.");
    offer_->setText("The selected reference is not in this project.");
  } else {
    online_ = found.presence == "online";
    file_->setText(QString::fromStdString(found.fileName + "\npresence " + found.presence +
                                          "\ntechnical metadata " + found.technicalMetadata +
                                          "\nmedia " + found.mediaId));
    auto marks = session_.sourceMarks(found.representationId);
    if (!marks.ok()) {
      offer_->setText(QString::fromStdString(marks.error.message));
    } else if (marks.value.markIn.has_value() && marks.value.markOut.has_value()) {
      offer_->setText(QString::fromStdString(
          "Offered source marks, source frames: Mark In " + std::to_string(*marks.value.markIn) + ", Mark Out " +
          std::to_string(*marks.value.markOut) +
          ". These are points. Displayed inclusive OUT is not settled, so they are not copied into Source in or "
          "Source out and they are not a half-open edit range. The source rate is still chosen here. The file was "
          "not probed."));
    } else if (marks.value.markIn.has_value() || marks.value.markOut.has_value()) {
      offer_->setText(
          "One source mark is stored. A single point is not offered as a source span. Source in and source out stay "
          "empty. The file was not probed.");
    } else {
      offer_->setText(
          "No source mark pair is stored for this representation. Source in and source out stay empty. The file was "
          "not probed.");
    }
  }

  auto tracks = session_.listTracks(sequenceId.toStdString());
  if (!fixedTrackId_.isEmpty()) {
    QString label = "Selected track";
    if (tracks.ok()) {
      for (const TrackRecord& track : tracks.value) {
        if (QString::fromStdString(track.trackId) == fixedTrackId_) {
          label = QString::fromStdString("track " + std::to_string(track.ordinal));
        }
      }
    }
    track_->setText(label);
    tracks_->hide();
  } else if (tracks.ok() && !tracks.value.empty()) {
    for (const TrackRecord& track : tracks.value) {
      tracks_->addItem(QString::fromStdString("track " + std::to_string(track.ordinal)),
                       QString::fromStdString(track.trackId));
    }
    track_->setText("Stored order. No track kind is stored.");
    tracks_->show();
  } else {
    track_->setText("No track is stored. Place creates one track in stored order.");
    tracks_->hide();
  }
  refreshValidation();
}

QString PlacementDialog::createdInstanceId() const { return createdId_; }

bool PlacementDialog::chosenSourceRate(FrameRate* rate) const {
  const int index = sourceRate_->currentIndex() - 1;
  if (index < 0 || index >= static_cast<int>(sizeof(kRates) / sizeof(kRates[0]))) {
    return false;
  }
  *rate = kRates[index].rate;
  return true;
}

bool PlacementDialog::readFrames(std::int64_t* recordStart,
                                std::int64_t* sourceStart,
                                std::int64_t* sourceEnd) const {
  const IntegerFrameParse record = parseIntegerFrame(recordIn_->text().toStdString());
  const IntegerFrameParse start = parseIntegerFrame(sourceStart_->text().toStdString());
  const IntegerFrameParse end = parseIntegerFrame(sourceEnd_->text().toStdString());
  if (!record.ok || !start.ok || !end.ok) {
    return false;
  }
  *recordStart = record.frame;
  *sourceStart = start.frame;
  *sourceEnd = end.frame;
  return true;
}

QString PlacementDialog::selectedTrackId() const {
  if (!fixedTrackId_.isEmpty()) {
    return fixedTrackId_;
  }
  if (tracks_->count() == 0 || tracks_->currentIndex() < 0) {
    return {};
  }
  return tracks_->currentData().toString();
}

void PlacementDialog::refreshValidation() {
  if (!online_) {
    validation_->setText("Offline references are not placed. Technical metadata is Not probed.");
    return;
  }
  FrameRate sourceRate;
  std::int64_t recordStart = 0;
  std::int64_t sourceStart = 0;
  std::int64_t sourceEnd = 0;
  if (!chosenSourceRate(&sourceRate) || !readFrames(&recordStart, &sourceStart, &sourceEnd)) {
    validation_->setText(
        "Source in and source out are source frames at the chosen rate. The file was not probed, so no duration is "
        "filled in.");
    return;
  }
  auto sequence = session_.readSequence(sequenceId_.toStdString());
  if (!sequence.ok()) {
    validation_->setText(QString::fromStdString(sequence.error.message));
    return;
  }
  auto end = identityRecordEnd(sequence.value.rate, sourceRate, sourceStart, sourceEnd, recordStart);
  if (!end.ok()) {
    validation_->setText(QString::fromStdString(end.error.message));
    return;
  }
  validation_->setText(QString::fromStdString(
      "Record [" + std::to_string(recordStart) + "," + std::to_string(end.value) +
      ") in sequence frames. One source frame occupies one sequence frame. This is not a speed change."));
}

void PlacementDialog::commit() {
  message_->clear();
  if (file_->text().contains("not in this project")) {
    message_->setText("The selected reference is not in this project.");
    return;
  }
  if (!online_) {
    message_->setText("This reference is offline. It is not placed, and it is not relinked.");
    return;
  }
  FrameRate sourceRate;
  if (!chosenSourceRate(&sourceRate)) {
    message_->setText("Choose a source rate. The file was not probed.");
    refreshValidation();
    return;
  }
  std::int64_t recordStart = 0;
  std::int64_t sourceStart = 0;
  std::int64_t sourceEnd = 0;
  if (!readFrames(&recordStart, &sourceStart, &sourceEnd)) {
    message_->setText("a frame must be a non-negative integer");
    refreshValidation();
    return;
  }
  auto sequence = session_.readSequence(sequenceId_.toStdString());
  if (!sequence.ok()) {
    message_->setText(QString::fromStdString(sequence.error.message));
    return;
  }
  auto end = identityRecordEnd(sequence.value.rate, sourceRate, sourceStart, sourceEnd, recordStart);
  if (!end.ok()) {
    message_->setText(QString::fromStdString(end.error.message));
    refreshValidation();
    return;
  }
  auto placed = session_.placeInstance(sequenceId_.toStdString(), selectedTrackId().toStdString(),
                                       objectId_.toStdString(), sourceRate, sourceStart, sourceEnd, recordStart,
                                       end.value);
  if (!placed.ok()) {
    message_->setText(QString::fromStdString(placed.error.message));
    refreshValidation();
    return;
  }
  createdId_ = QString::fromStdString(placed.value.instanceId);
  accept();
}

}  // namespace aether
