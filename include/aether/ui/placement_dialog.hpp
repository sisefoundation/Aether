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

#include <QDialog>
#include <QString>

#include <cstdint>

class QComboBox;
class QLabel;
class QLineEdit;

namespace aether {

// Confirms one placeInstance call. It does not write project.db itself.
class PlacementDialog : public QDialog {
  Q_OBJECT

 public:
  PlacementDialog(ProjectSession& session, QWidget* parent = nullptr);

  void setRequest(const QString& sequenceId,
                  const QString& objectId,
                  const QString& trackId,
                  std::int64_t recordFrame);
  QString createdInstanceId() const;

 private slots:
  void refreshValidation();
  void commit();

 private:
  bool chosenSourceRate(FrameRate* rate) const;
  bool readFrames(std::int64_t* recordStart, std::int64_t* sourceStart, std::int64_t* sourceEnd) const;
  QString selectedTrackId() const;

  ProjectSession& session_;
  QString sequenceId_;
  QString objectId_;
  QString fixedTrackId_;
  QString createdId_;
  bool online_ = false;
  QLabel* file_ = nullptr;
  QLabel* sequence_ = nullptr;
  QLabel* track_ = nullptr;
  QComboBox* tracks_ = nullptr;
  QLineEdit* recordIn_ = nullptr;
  QComboBox* sourceRate_ = nullptr;
  QLineEdit* sourceStart_ = nullptr;
  QLineEdit* sourceEnd_ = nullptr;
  QLabel* offer_ = nullptr;
  QLabel* validation_ = nullptr;
  QLabel* message_ = nullptr;
};

}  // namespace aether
