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

class QComboBox;
class QLabel;
class QLineEdit;

namespace aether {

// Create and open go through ProjectSession. This dialog does not write
// project.db itself and does not store a raster, pixel aspect, or colour space.
class ProjectDialog : public QDialog {
  Q_OBJECT

 public:
  ProjectDialog(ProjectSession& session, QWidget* parent = nullptr);

  void refreshRates();
  bool openDirectory(const QString& directory);

 private slots:
  void browseParent();
  void createNew();
  void openExisting();

 private:
  void restorePrevious(const std::string& directory, bool restore, const std::string& failure);
  ProjectSession& session_;
  QLabel* storedRate_ = nullptr;
  QLineEdit* name_ = nullptr;
  QLineEdit* parentFolder_ = nullptr;
  QComboBox* rate_ = nullptr;
  QLabel* message_ = nullptr;
};

}  // namespace aether
