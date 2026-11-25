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

#include "aether/ui/project_dialog.hpp"

#include "aether/core/sequence/sequence.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
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

std::string rateText(FrameRate rate) {
  return std::to_string(rate.numerator) + "/" + std::to_string(rate.denominator);
}

}  // namespace

ProjectDialog::ProjectDialog(ProjectSession& session, QWidget* parent)
    : QDialog(parent), session_(session) {
  setObjectName("projectDialog");
  setWindowTitle("Project");
  setModal(true);
  setStyleSheet(
      "QDialog, QWidget { background: #161616; color: #d8d8d8; }"
      "QLabel { color: #d8d8d8; }"
      "QLineEdit, QComboBox { background: #1e1e1e; color: #d8d8d8; border: 1px solid #2c2c2c; padding: 4px; }"
      "QPushButton { background: #2a2a2a; color: #d8d8d8; border: 1px solid #3a3a3a; padding: 6px 10px; }"
      "QTabWidget::pane { border: 1px solid #2c2c2c; }"
      "QTabBar::tab { background: #1a1a1a; color: #9a9a9a; padding: 6px 12px; }"
      "QTabBar::tab:selected { color: #f2f2f2; }");
  resize(560, 420);

  auto* layout = new QVBoxLayout(this);
  auto* title = new QLabel("Select or create a project", this);
  title->setObjectName("dialogTitle");
  layout->addWidget(title);

  storedRate_ = new QLabel(this);
  storedRate_->setObjectName("storedSequenceRate");
  storedRate_->setWordWrap(true);
  layout->addWidget(storedRate_);

  auto* tabs = new QTabWidget(this);
  tabs->setObjectName("projectTabs");

  auto* existing = new QWidget(tabs);
  auto* existingLayout = new QVBoxLayout(existing);
  auto* catalog = new QLabel(
      "No project catalog is stored. Open chooses a project folder through the existing open path.", existing);
  catalog->setWordWrap(true);
  existingLayout->addWidget(catalog);
  auto* openButton = new QPushButton("Open Project", existing);
  openButton->setObjectName("openExistingProject");
  existingLayout->addWidget(openButton);
  existingLayout->addStretch();
  tabs->addTab(existing, "Existing projects");

  auto* created = new QWidget(tabs);
  auto* form = new QFormLayout(created);
  name_ = new QLineEdit(created);
  name_->setObjectName("projectName");
  form->addRow("Project name", name_);
  rate_ = new QComboBox(created);
  rate_->setObjectName("frameRate");
  for (const NamedRate& named : kRates) {
    rate_->addItem(named.label, QString::fromUtf8(named.label));
  }
  form->addRow("Frame rate", rate_);
  auto* note = new QLabel(
      "The rate is stored on the new sequence as that reduced rational. Raster, pixel aspect, colour space, and "
      "stereoscopic settings are not stored.",
      created);
  note->setWordWrap(true);
  form->addRow(note);
  auto* folderRow = new QHBoxLayout();
  parentFolder_ = new QLineEdit(created);
  parentFolder_->setObjectName("parentFolder");
  auto* browse = new QPushButton("Browse", created);
  browse->setObjectName("browseParent");
  folderRow->addWidget(parentFolder_);
  folderRow->addWidget(browse);
  form->addRow("Parent folder", folderRow);
  auto* createButton = new QPushButton("Create Project", created);
  createButton->setObjectName("createProject");
  form->addRow(createButton);
  tabs->addTab(created, "New Project");
  tabs->setCurrentIndex(1);
  layout->addWidget(tabs);

  message_ = new QLabel(this);
  message_->setObjectName("projectDialogMessage");
  message_->setWordWrap(true);
  layout->addWidget(message_);

  connect(browse, &QPushButton::clicked, this, &ProjectDialog::browseParent);
  connect(createButton, &QPushButton::clicked, this, &ProjectDialog::createNew);
  connect(openButton, &QPushButton::clicked, this, &ProjectDialog::openExisting);
  refreshRates();
}

void ProjectDialog::refreshRates() {
  if (!session_.isOpen()) {
    storedRate_->setText("No project is open. Opening a project does not change its sequence rate.");
    return;
  }
  auto sequences = session_.listSequences();
  if (!sequences.ok()) {
    storedRate_->setText(QString::fromStdString(sequences.error.message));
    return;
  }
  if (sequences.value.empty()) {
    storedRate_->setText("No sequence rate is stored. This dialog will not invent one for an open project.");
    return;
  }
  QString text = "Stored sequence rate, shown and not changed:";
  for (const SequenceRecord& sequence : sequences.value) {
    text += QString::fromStdString("\n" + sequence.name + "  " + rateText(sequence.rate));
  }
  storedRate_->setText(text);
}

void ProjectDialog::browseParent() {
  const QString directory = QFileDialog::getExistingDirectory(this, "Choose a parent folder");
  if (!directory.isEmpty()) {
    parentFolder_->setText(directory);
  }
}

void ProjectDialog::restorePrevious(const std::string& directory, bool restore, const std::string& failure) {
  if (restore) {
    auto restored = session_.openProject(directory);
    if (!restored.ok()) {
      message_->setText(QString::fromStdString(failure + " The previous project could not be restored: " +
                                               restored.error.message));
      refreshRates();
      return;
    }
  }
  message_->setText(QString::fromStdString(failure));
  refreshRates();
}

void ProjectDialog::createNew() {
  const QString name = name_->text().trimmed();
  const QString parent = parentFolder_->text().trimmed();
  if (name.isEmpty() || parent.isEmpty()) {
    message_->setText("A project name and a parent folder are required.");
    return;
  }
  const int index = rate_->currentIndex();
  if (index < 0 || index >= static_cast<int>(sizeof(kRates) / sizeof(kRates[0]))) {
    message_->setText("Choose one of the accepted frame rates.");
    return;
  }
  const FrameRate rate = kRates[index].rate;
  std::string previous;
  const bool restore = session_.isOpen();
  if (restore) {
    previous = session_.info().directory;
    auto closed = session_.closeProject();
    if (!closed.ok()) {
      message_->setText(QString::fromStdString(closed.error.message));
      return;
    }
  }
  const QString folder = parent + "/" + name;
  auto created = session_.createProject(folder.toStdString(), name.toStdString());
  if (!created.ok()) {
    restorePrevious(previous, restore, created.error.message);
    return;
  }
  auto sequence = session_.createSequence(name.toStdString(), rate);
  if (!sequence.ok()) {
    message_->setText(QString::fromStdString(sequence.error.message));
    refreshRates();
    return;
  }
  accept();
}

bool ProjectDialog::openDirectory(const QString& directory) {
  const QString folder = directory.trimmed();
  if (folder.isEmpty()) {
    message_->setText("A project folder is required.");
    return false;
  }
  std::string previous;
  const bool restore = session_.isOpen();
  if (restore) {
    previous = session_.info().directory;
    auto closed = session_.closeProject();
    if (!closed.ok()) {
      message_->setText(QString::fromStdString(closed.error.message));
      return false;
    }
  }
  auto opened = session_.openProject(folder.toStdString());
  if (!opened.ok()) {
    restorePrevious(previous, restore, opened.error.message);
    return false;
  }
  accept();
  return true;
}

void ProjectDialog::openExisting() {
  const QString directory = QFileDialog::getExistingDirectory(this, "Open project");
  if (directory.isEmpty()) {
    return;
  }
  openDirectory(directory);
}

}  // namespace aether
