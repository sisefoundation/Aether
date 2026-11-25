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

#include "aether/ui/main_window.hpp"

#include "aether/ui/frame_label.hpp"
#include "aether/ui/project_dialog.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QKeySequence>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QMenu>
#include <QTimer>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <algorithm>
#include <set>

void initProductResources() { Q_INIT_RESOURCE(aether); }

namespace aether {
namespace {

const char* kAllMedia = "all-media";

const char* kStyle = R"(
QMainWindow, QDialog, QWidget { background: #1a1c20; color: #d5d7db; font-size: 20px; }
QLabel { color: #d5d7db; }
QLabel#shellTitle, QLabel#dialogTitle { font-size: 26px; color: #f3f4f6; }
QLabel#panelTitle { font-size: 22px; color: #c5c8ce; }
QLabel#muted, QLabel#modeUnavailable { color: #8b9098; }
QLabel#timelineClock { font-size: 28px; color: #f3f4f6; }
QLabel#recordTimecode, QLabel#sourceTimecode { font-size: 28px; color: #f3f4f6; }
QLabel#mediaPreview { background: #101114; color: #8b9098; border: 1px solid #2c3138; }
QListWidget, QLineEdit, QComboBox, QPlainTextEdit {
  background: #14161a; color: #d5d7db; border: 1px solid #2c3138; font-size: 20px; }
QListWidget::item { min-height: 32px; padding: 4px 8px; }
QListWidget::item:selected { background: #2e3640; color: #f3f4f6; }
QWidget#timelineCanvas, QWidget#timelineTrackHeaders { font-size: 18px; }
QPushButton, QToolButton {
  background: #2a2e35; color: #d5d7db; border: 1px solid #3a3f48;
  padding: 8px 16px; font-size: 18px;
}
QPushButton:hover, QToolButton:hover { background: #343a42; }
QPushButton:disabled { color: #6e7480; background: #22262c; }
QPushButton[roomTab="true"] {
  background: transparent; border: none; border-bottom: 2px solid transparent;
  padding: 8px 18px; color: #9aa1ab; font-size: 20px;
}
QPushButton[roomTab="true"]:checked { color: #f4f6f8; border-bottom: 2px solid #d64545; }
QPushButton[viewerTab="true"] {
  background: #2a2e35; color: #c5c8ce; border: 1px solid #3a3f48;
  border-bottom: 2px solid transparent; padding: 8px 18px; font-size: 20px;
}
QPushButton[viewerTab="true"]:checked { color: #f4f6f8; border-bottom: 2px solid #d64545; }
QFrame#sequenceViewer, QFrame#sourceViewer { background: #000000; border: 1px solid #111111; }
QFrame#projectPanel, QFrame#viewerPanel, QFrame#sidePanel, QFrame#timelinePanel {
  background: #1e2126; border: 1px solid #2a2e35;
}
QTabWidget::pane { border: none; background: #1e2126; }
QTabBar::tab { background: #1a1c20; color: #9aa1ab; padding: 8px 16px; font-size: 18px; }
QTabBar::tab:selected { color: #f3f4f6; background: #2a2e35; }
QMenuBar { background: #16181c; color: #d5d7db; font-size: 18px; }
QMenu { background: #1e2126; color: #d5d7db; border: 1px solid #3a3f48; font-size: 20px; }
QMenu::item { padding: 8px 18px; font-size: 20px; }
QSplitter::handle { background: #121418; }
)";

// The dark-wordmark file :/aether/aether-logo-dark-wordmark.png is the alternate
// for a light surface. It is not painted here. This workspace is black.
QPixmap markForDarkSurface() {
  initProductResources();
  static const QPixmap cached = [] {
  QImage image(QStringLiteral(":/aether/aether-logo-on-dark.png"));
  if (image.isNull()) {
    return QPixmap();
  }
  image = image.convertToFormat(QImage::Format_ARGB32);
  int minX = image.width();
  int minY = image.height();
  int maxX = -1;
  int maxY = -1;
  for (int y = 0; y < image.height(); ++y) {
    const auto* scan = reinterpret_cast<const QRgb*>(image.constScanLine(y));
    for (int x = 0; x < image.width(); ++x) {
      const QRgb pixel = scan[x];
      if (qAlpha(pixel) > 8 && (qRed(pixel) > 12 || qGreen(pixel) > 12 || qBlue(pixel) > 12)) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
      }
    }
  }
  if (maxX >= minX) {
    image = image.copy(minX, minY, maxX - minX + 1, maxY - minY + 1);
  }
  return QPixmap::fromImage(image);
  }();
  return cached;
}

QWidget* unavailablePage(const QString& mode, const QString& detail, QWidget* parent) {
  auto* page = new QWidget(parent);
  auto* layout = new QVBoxLayout(page);
  auto* label = new QLabel(mode + "\n" + detail, page);
  label->setObjectName("modeUnavailable");
  label->setAlignment(Qt::AlignCenter);
  label->setWordWrap(true);
  layout->addWidget(label);
  return page;
}

QWidget* menuForm(QMenu* menu) {
  auto* form = new QWidget(menu);
  auto* layout = new QVBoxLayout(form);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(4);
  form->setMinimumWidth(360);
  return form;
}

void addMenuForm(QMenu* menu, QWidget* form) {
  auto* action = new QWidgetAction(menu);
  action->setDefaultWidget(form);
  menu->addAction(action);
}

}  // namespace

class PictureView : public QFrame {
 public:
  explicit PictureView(QWidget* parent = nullptr) : QFrame(parent) {}

  void showPicture(const DisplayPicture& picture) {
    note_.clear();
    if (!picture.present || picture.width <= 0 || picture.height <= 0) {
      image_ = QImage();
      note_ = QString::fromStdString(picture.note.empty() ? "no picture" : picture.note);
    } else {
      const std::size_t bytes = static_cast<std::size_t>(picture.width) * static_cast<std::size_t>(picture.height) * 3;
      if (picture.rgb.size() < bytes) {
        image_ = QImage();
        note_ = "decoded picture is incomplete";
      } else {
        QImage image(picture.rgb.data(), picture.width, picture.height, picture.width * 3, QImage::Format_RGB888);
        image_ = image.copy();
      }
    }
    setAccessibleDescription(image_.isNull() ? QStringLiteral("empty") : QStringLiteral("picture"));
    update();
  }

  void showNote(const QString& note) {
    image_ = QImage();
    note_ = note;
    setAccessibleDescription(QStringLiteral("empty"));
    update();
  }

 protected:
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);
    if (!image_.isNull()) {
      const QImage scaled = image_.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
      const int x = (width() - scaled.width()) / 2;
      const int y = (height() - scaled.height()) / 2;
      painter.drawImage(x, y, scaled);
      return;
    }
    painter.setPen(QColor(220, 220, 220));
    painter.drawText(rect().adjusted(12, 12, -12, -12), Qt::AlignCenter | Qt::TextWordWrap, note_);
  }

 private:
  QImage image_;
  QString note_;
};

MainWindow::MainWindow(ProjectSession& session, Logger& logger, QWidget* parent)
    : QMainWindow(parent), session_(session), logger_(logger) {
  const QPixmap mark = markForDarkSurface();
  const QIcon icon(mark);
  QApplication::setWindowIcon(icon);
  setWindowIcon(icon);
  setWindowTitle("Aether");
  resize(3000, 1600);
  setStyleSheet(kStyle);

  auto* file = menuBar()->addMenu("File");
  file->addAction("New or Open Project", this, &MainWindow::showProjectDialog);
  file->addAction("Close Project", this, &MainWindow::closeProject);
  file->addAction("Import Media", this, &MainWindow::importMedia);

  auto* central = new QWidget(this);
  auto* root = new QVBoxLayout(central);
  root->setContentsMargins(8, 6, 8, 6);
  root->setSpacing(6);

  auto* titleRow = new QHBoxLayout();
  titleRow->setSpacing(8);
  auto* markLabel = new QLabel(central);
  markLabel->setObjectName("productMark");
  markLabel->setPixmap(mark.scaledToHeight(22, Qt::SmoothTransformation));
  titleRow->addWidget(markLabel);
  nameLabel_ = new QLabel("Aether", central);
  nameLabel_->setObjectName("shellTitle");
  titleRow->addWidget(nameLabel_);
  titleRow->addStretch(1);
  const char* roomNames[] = {"Edit", "Effects", "Audio", "Export"};
  const char* roomIds[] = {"roomEdit", "roomEffects", "roomAudio", "roomExport"};
  QList<QPushButton*> roomButtons;
  for (int index = 0; index < 4; ++index) {
    auto* button = new QPushButton(roomNames[index], central);
    button->setObjectName(roomIds[index]);
    button->setProperty("roomTab", true);
    button->setCheckable(true);
    button->setChecked(index == 0);
    button->setFlat(true);
    titleRow->addWidget(button);
    roomButtons.append(button);
  }
  for (int index = 0; index < roomButtons.size(); ++index) {
    QPushButton* button = roomButtons.at(index);
    connect(button, &QPushButton::clicked, this, [this, index, button, roomButtons]() {
      for (QPushButton* other : roomButtons) {
        other->setChecked(other == button);
      }
      showMode(index);
    });
  }
  titleRow->addStretch(1);
  root->addLayout(titleRow);

  modes_ = new QStackedWidget(central);
  workspace_ = new TimelineWorkspace(session_, modes_);
  modes_->addWidget(workspace_);
  auto* edit = menuBar()->addMenu("Edit");
  edit->setObjectName("editMenu");
  undoAction_ = edit->addAction("Undo");
  undoAction_->setObjectName("editUndo");
  undoAction_->setShortcut(QKeySequence::Undo);
  undoAction_->setShortcutContext(Qt::WindowShortcut);
  redoAction_ = edit->addAction("Redo");
  redoAction_->setObjectName("editRedo");
  redoAction_->setShortcut(QKeySequence::Redo);
  redoAction_->setShortcutContext(Qt::WindowShortcut);
  connect(workspace_, &TimelineWorkspace::timelinePlayRequested, this, [this]() { playRecord(); });
  connect(workspace_, &TimelineWorkspace::timelineStopRequested, this, [this]() { stopRecord(); });
  connect(workspace_, &TimelineWorkspace::playheadChanged, this, [this]() { showRecordPicture(); });
  connect(undoAction_, &QAction::triggered, workspace_, &TimelineWorkspace::undoEdit);
  connect(redoAction_, &QAction::triggered, workspace_, &TimelineWorkspace::redoEdit);
  connect(workspace_, &TimelineWorkspace::historyChanged, this, [this]() {
    undoAction_->setEnabled(session_.canUndo());
    redoAction_->setEnabled(session_.canRedo());
    showRecordPicture();
  });
  undoAction_->setEnabled(session_.canUndo());
  redoAction_->setEnabled(session_.canRedo());
  modes_->addWidget(unavailablePage("Effects", "There is no effects engine.", modes_));
  modes_->addWidget(unavailablePage(
      "Audio", "There are no sound tools. The sound build shape stays open.", modes_));
  modes_->addWidget(unavailablePage("Export", "There is no exporter.", modes_));

  auto* projectBody = workspace_->findChild<QWidget*>("projectBody");
  auto* projectLayout = qobject_cast<QVBoxLayout*>(projectBody->layout());
  int projectRow = 0;
  projectLayout->insertWidget(projectRow++, new QLabel("Bins", projectBody));
  bins_ = new QListWidget(projectBody);
  bins_->setObjectName("binList");
  bins_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  bins_->setSelectionMode(QAbstractItemView::SingleSelection);
  bins_->setMinimumHeight(88);
  projectLayout->insertWidget(projectRow++, bins_);
  projectLayout->insertWidget(projectRow++, new QLabel("Clips", projectBody));
  emptyBinLabel_ = new QLabel("No project is open.", projectBody);
  emptyBinLabel_->setObjectName("emptyBin");
  emptyBinLabel_->setWordWrap(true);
  projectLayout->insertWidget(projectRow++, emptyBinLabel_);
  assets_ = new QListWidget(projectBody);
  assets_->setObjectName("assetList");
  assets_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  assets_->setSelectionMode(QAbstractItemView::SingleSelection);
  assets_->setDragEnabled(true);
  assets_->setDragDropMode(QAbstractItemView::DragOnly);
  assets_->setContextMenuPolicy(Qt::CustomContextMenu);
  assets_->setMinimumHeight(120);
  projectLayout->insertWidget(projectRow++, assets_);
  previewCard_ = new QLabel(projectBody);
  previewCard_->setObjectName("mediaPreview");
  previewCard_->setMinimumHeight(72);
  previewCard_->setAlignment(Qt::AlignCenter);
  previewCard_->hide();
  projectLayout->insertWidget(projectRow++, previewCard_);
  auto* tagsEmpty = new QLabel("No tags are stored.", projectBody);
  tagsEmpty->setObjectName("muted");
  projectLayout->addWidget(tagsEmpty);

  auto* projectMenu = workspace_->findChild<QToolButton*>("projectPanelMenu")->menu();
  auto* projectForm = menuForm(projectMenu);
  auto* projectFormLayout = qobject_cast<QVBoxLayout*>(projectForm->layout());
  auto* importButton = new QPushButton("Import Media", projectForm);
  importButton->setObjectName("importMedia");
  projectFormLayout->addWidget(importButton);
  auto* probeButtons = new QHBoxLayout();
  auto* probeButton = new QPushButton("Probe", projectForm);
  probeButton->setObjectName("probeMedia");
  auto* reprobeButton = new QPushButton("Reprobe", projectForm);
  reprobeButton->setObjectName("reprobeMedia");
  probeButtons->addWidget(probeButton);
  probeButtons->addWidget(reprobeButton);
  projectFormLayout->addLayout(probeButtons);
  probeStatus_ = new QLabel("Not probed", projectForm);
  probeStatus_->setObjectName("probeStatus");
  probeStatus_->setWordWrap(true);
  projectFormLayout->addWidget(probeStatus_);
  probeReport_ = new QPlainTextEdit(projectForm);
  probeReport_->setObjectName("probeReport");
  probeReport_->setReadOnly(true);
  probeReport_->setFont(QFont(QStringLiteral("monospace"), 13));
  probeReport_->setMinimumHeight(140);
  probeReport_->setPlainText(
      "Not probed. The file is registered. No container, duration, frame rate, or frame count is known yet.");
  projectFormLayout->addWidget(probeReport_);
  addMenuForm(projectMenu, projectForm);

  auto* viewerBody = workspace_->findChild<QWidget*>("viewerBody");
  auto* viewerLayout = qobject_cast<QVBoxLayout*>(viewerBody->layout());
  viewerStack_ = new QStackedWidget(viewerBody);
  viewerStack_->setObjectName("viewerStack");

  auto* sourcePage = new QWidget(viewerStack_);
  auto* sourcePageLayout = new QVBoxLayout(sourcePage);
  sourcePageLayout->setContentsMargins(0, 0, 0, 0);
  sourcePageLayout->setSpacing(6);
  sourceView_ = new PictureView(sourcePage);
  sourceView_->setObjectName("sourceViewer");
  sourceView_->setMinimumHeight(220);
  sourceView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  sourceView_->showNote("No media loaded");
  sourcePageLayout->addWidget(sourceView_, 1);
  sourceTimecode_ = new QLabel("No media loaded", sourcePage);
  sourceTimecode_->setObjectName("sourceTimecode");
  sourceTimecode_->setAlignment(Qt::AlignHCenter);
  sourceTimecode_->setWordWrap(true);
  sourcePageLayout->addWidget(sourceTimecode_);
  auto* sourceMarks = new QHBoxLayout();
  sourceMarks->setSpacing(6);
  auto* sourceIn = new QPushButton("Mark In", sourcePage);
  sourceIn->setObjectName("sourceMarkIn");
  auto* sourceOut = new QPushButton("Mark Out", sourcePage);
  sourceOut->setObjectName("sourceMarkOut");
  sourceMarks->addWidget(sourceIn);
  sourceMarks->addWidget(sourceOut);
  auto* sourcePlay = new QPushButton("Play", sourcePage);
  sourcePlay->setObjectName("sourcePlay");
  auto* sourceStop = new QPushButton("Stop", sourcePage);
  sourceStop->setObjectName("sourceStop");
  auto* sourcePlace = new QPushButton("Place", sourcePage);
  sourcePlace->setObjectName("sourcePlace");
  sourceMarks->addWidget(sourcePlay);
  sourceMarks->addWidget(sourceStop);
  sourceMarks->addWidget(sourcePlace);
  sourceMarks->addStretch(1);
  sourcePageLayout->addLayout(sourceMarks);
  connect(sourcePlay, &QPushButton::clicked, this, [this]() { playSource(); });
  connect(sourceStop, &QPushButton::clicked, this, [this]() { stopSource(); });
  connect(sourcePlace, &QPushButton::clicked, this, [this]() { placeLoadedSource(); });
  connect(sourceIn, &QPushButton::clicked, this, &MainWindow::markSourceIn);
  connect(sourceOut, &QPushButton::clicked, this, &MainWindow::markSourceOut);

  auto* recordPage = new QWidget(viewerStack_);
  auto* recordPageLayout = new QVBoxLayout(recordPage);
  recordPageLayout->setContentsMargins(0, 0, 0, 0);
  recordPageLayout->setSpacing(6);
  recordView_ = new PictureView(recordPage);
  recordView_->setObjectName("sequenceViewer");
  recordView_->setMinimumHeight(220);
  recordView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  recordView_->showNote("No timeline picture");
  recordPageLayout->addWidget(recordView_, 1);
  recordTimecode_ = new QLabel("—", recordPage);
  recordTimecode_->setObjectName("recordTimecode");
  recordTimecode_->setAlignment(Qt::AlignHCenter);
  recordPageLayout->addWidget(recordTimecode_);
  playheadLabel_ = new QLabel("Playhead is a frame position. Stopped.", recordPage);
  playheadLabel_->setObjectName("playhead");
  playheadLabel_->setWordWrap(true);
  playheadLabel_->setAlignment(Qt::AlignHCenter);
  recordPageLayout->addWidget(playheadLabel_);
  auto* recordMarks = new QHBoxLayout();
  recordMarks->setSpacing(6);
  auto* recordIn = new QPushButton("Mark In", recordPage);
  recordIn->setObjectName("recordViewerMarkIn");
  auto* recordOut = new QPushButton("Mark Out", recordPage);
  recordOut->setObjectName("recordViewerMarkOut");
  recordMarks->addWidget(recordIn);
  recordMarks->addWidget(recordOut);
  auto* recordPlay = new QPushButton("Play", recordPage);
  recordPlay->setObjectName("recordPlay");
  auto* recordStop = new QPushButton("Stop", recordPage);
  recordStop->setObjectName("recordStop");
  recordMarks->addWidget(recordPlay);
  recordMarks->addWidget(recordStop);
  recordMarks->addStretch(1);
  recordPageLayout->addLayout(recordMarks);
  connect(recordPlay, &QPushButton::clicked, this, [this]() { playRecord(); });
  connect(recordStop, &QPushButton::clicked, this, [this]() { stopRecord(); });
  connect(recordIn, &QPushButton::clicked, workspace_, &TimelineWorkspace::markRecordIn);
  connect(recordOut, &QPushButton::clicked, workspace_, &TimelineWorkspace::markRecordOut);

  viewerStack_->addWidget(sourcePage);
  viewerStack_->addWidget(recordPage);
  viewerLayout->addWidget(viewerStack_, 1);
  auto* sourceTab = workspace_->findChild<QPushButton*>("viewerSourceTab");
  auto* recordTab = workspace_->findChild<QPushButton*>("viewerRecordTab");
  connect(sourceTab, &QPushButton::clicked, this, [this]() { showViewerPage(0); });
  connect(recordTab, &QPushButton::clicked, this, [this]() { showViewerPage(1); });
  sourceTimer_ = new QTimer(this);
  recordTimer_ = new QTimer(this);
  sourceTimer_->setTimerType(Qt::PreciseTimer);
  recordTimer_->setTimerType(Qt::PreciseTimer);
  connect(sourceTimer_, &QTimer::timeout, this, [this]() { advanceSource(); });
  connect(recordTimer_, &QTimer::timeout, this, [this]() { advanceRecord(); });
  showViewerPage(1);

  auto* viewerMenu = workspace_->findChild<QToolButton*>("viewerPanelMenu")->menu();
  auto* sourceForm = menuForm(viewerMenu);
  auto* sourceLayout = qobject_cast<QVBoxLayout*>(sourceForm->layout());
  auto* sourceUnit = new QLabel("source frames", sourceForm);
  sourceUnit->setObjectName("sourceMarkUnit");
  sourceLayout->addWidget(sourceUnit);
  sourceMarkFrame_ = new QLineEdit(sourceForm);
  sourceMarkFrame_->setObjectName("sourceMarkFrame");
  sourceMarkFrame_->setPlaceholderText("source frames");
  sourceLayout->addWidget(sourceMarkFrame_);
  auto* sourceButtons = new QGridLayout();
  auto addSourceButton = [this, sourceForm, sourceButtons](const char* text, const char* name, int row, int column,
                                                          void (MainWindow::*method)()) {
    auto* button = new QPushButton(text, sourceForm);
    button->setObjectName(name);
    sourceButtons->addWidget(button, row, column);
    connect(button, &QPushButton::clicked, this, method);
  };
  addSourceButton("Clear In", "sourceClearIn", 0, 0, &MainWindow::clearSourceIn);
  addSourceButton("Clear Out", "sourceClearOut", 0, 1, &MainWindow::clearSourceOut);
  addSourceButton("Clear Both", "sourceClearBoth", 0, 2, &MainWindow::clearSourceBoth);
  sourceLayout->addLayout(sourceButtons);
  sourceMarkReadout_ = new QLabel("No representation is selected.", sourceForm);
  sourceMarkReadout_->setObjectName("sourceMarkReadout");
  sourceMarkReadout_->setWordWrap(true);
  sourceLayout->addWidget(sourceMarkReadout_);
  sourceMarkReason_ = new QLabel(sourceForm);
  sourceMarkReason_->setObjectName("sourceMarkReason");
  sourceMarkReason_->setWordWrap(true);
  sourceLayout->addWidget(sourceMarkReason_);
  recordMarkMirror_ = new QLabel("No sequence is selected.", sourceForm);
  recordMarkMirror_->setObjectName("recordMarkMirror");
  recordMarkMirror_->setWordWrap(true);
  sourceLayout->addWidget(recordMarkMirror_);
  auto* sourceAction = new QWidgetAction(viewerMenu);
  sourceAction->setDefaultWidget(sourceForm);
  if (viewerMenu->actions().isEmpty()) {
    viewerMenu->addAction(sourceAction);
  } else {
    viewerMenu->insertAction(viewerMenu->actions().constFirst(), sourceAction);
  }

  auto* sideBody = workspace_->findChild<QWidget*>("sideBody");
  auto* sideLayout = qobject_cast<QVBoxLayout*>(sideBody->layout());
  auto* sideTabs = new QTabWidget(sideBody);
  sideTabs->setObjectName("sideTabs");
  auto* sideMenuButton = workspace_->findChild<QToolButton*>("sidePanelMenu");
  sideTabs->setCornerWidget(sideMenuButton, Qt::TopRightCorner);
  auto* settingsPage = new QWidget(sideTabs);
  auto* settingsLayout = new QVBoxLayout(settingsPage);
  auto* settingsNote = new QLabel(
      "No settings beyond the stored sequence rate. Raster, pixel aspect, colour, and stereoscopic values are not "
      "stored. Tracks stay in stored order. A video or audio kind is not stored. The header mark is a view label.",
      settingsPage);
  settingsNote->setObjectName("settingsNote");
  settingsNote->setWordWrap(true);
  settingsLayout->addWidget(settingsNote);
  settingsLayout->addStretch();
  auto* infoPage = new QWidget(sideTabs);
  auto* infoLayout = new QVBoxLayout(infoPage);
  rateLabel_ = new QLabel("No sequence rate is stored", infoPage);
  rateLabel_->setObjectName("sequenceRate");
  rateLabel_->setWordWrap(true);
  infoLayout->addWidget(rateLabel_);
  inspectorLabel_ = new QLabel("No media selected.", infoPage);
  inspectorLabel_->setObjectName("assetInspector");
  inspectorLabel_->setWordWrap(true);
  inspectorLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  inspectorLabel_->setAlignment(Qt::AlignTop);
  infoLayout->addWidget(inspectorLabel_, 1);
  auto* cuesPage = new QWidget(sideTabs);
  auto* cuesLayout = new QVBoxLayout(cuesPage);
  auto* cues = new QLabel("No sequence cues are stored.", cuesPage);
  cues->setObjectName("sequenceCues");
  cues->setWordWrap(true);
  cues->setAlignment(Qt::AlignCenter);
  cuesLayout->addWidget(cues);
  auto* exportPage = new QWidget(sideTabs);
  auto* exportLayout = new QVBoxLayout(exportPage);
  auto* exportIdle = new QLabel("Export does nothing.", exportPage);
  exportIdle->setObjectName("exportIdle");
  exportIdle->setWordWrap(true);
  exportIdle->setAlignment(Qt::AlignCenter);
  exportLayout->addWidget(exportIdle);
  sideTabs->addTab(settingsPage, "Settings");
  sideTabs->addTab(infoPage, "Info");
  sideTabs->addTab(cuesPage, "Sequence Cues");
  sideTabs->addTab(exportPage, "Export");
  sideLayout->addWidget(sideTabs);
  root->addWidget(modes_, 1);

  statusLabel_ = new QLabel("Closed", central);
  statusLabel_->setObjectName("statusLine");
  root->addWidget(statusLabel_);
  setCentralWidget(central);

  probeController_ = new ProbeController(this);
  connect(probeController_, &ProbeController::updated, this, &MainWindow::showProbe);
  connect(probeButton, &QPushButton::clicked, this, &MainWindow::probeSelected);
  connect(reprobeButton, &QPushButton::clicked, this, &MainWindow::probeSelected);
  connect(importButton, &QPushButton::clicked, this, &MainWindow::importMedia);
  connect(bins_, &QListWidget::currentRowChanged, this, &MainWindow::binChosen);
  connect(assets_, &QListWidget::currentRowChanged, this, &MainWindow::assetChosen);
  connect(assets_, &QListWidget::itemDoubleClicked, this, &MainWindow::openClipInViewer);
  connect(assets_, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
    QListWidgetItem* item = assets_->itemAt(pos);
    if (item == nullptr || !session_.isOpen()) {
      return;
    }
    QMenu menu(this);
    QAction* place = menu.addAction("Insert into Timeline");
    if (menu.exec(assets_->mapToGlobal(pos)) != place) {
      return;
    }
    workspace_->beginPlacement(item->data(Qt::UserRole).toString(), QString(), workspace_->playheadFrame());
    showRecordPicture();
  });
  connect(workspace_, &TimelineWorkspace::selectionChanged, this, &MainWindow::showSelection);
  connect(workspace_, &TimelineWorkspace::recordMarksChanged, this, &MainWindow::showRecordMarks);
  refresh();
}

void MainWindow::showProjectDialog() {
  if (closing_) {
    return;
  }
  ProjectDialog dialog(session_, this);
  dialog.refreshRates();
  if (dialog.exec() == QDialog::Accepted) {
    logger_.info("project", "window left the project dialog with a project open");
  }
  refresh();
}

void MainWindow::closeEvent(QCloseEvent* event) {
  closing_ = true;
  if (session_.isOpen()) {
    session_.closeProject();
  }
  QMainWindow::closeEvent(event);
}

void MainWindow::showMode(int index) {
  if (modes_ == nullptr || index < 0 || index >= modes_->count()) {
    return;
  }
  modes_->setCurrentIndex(index);
  if (index == 0) {
    statusLabel_->setText(session_.isOpen() ? "Open" : "Closed");
    return;
  }
  statusLabel_->setText("This room is not available.");
}

void MainWindow::showViewerPage(int index) {
  if (viewerStack_ != nullptr) {
    viewerStack_->setCurrentIndex(index);
  }
  if (workspace_ == nullptr) {
    return;
  }
  if (auto* sourceTab = workspace_->findChild<QPushButton*>("viewerSourceTab")) {
    sourceTab->setChecked(index == 0);
  }
  if (auto* recordTab = workspace_->findChild<QPushButton*>("viewerRecordTab")) {
    recordTab->setChecked(index == 1);
  }
}

void MainWindow::openClipInViewer(QListWidgetItem* item) {
  if (item == nullptr || !session_.isOpen() || viewerStack_ == nullptr) {
    return;
  }
  const QString objectId = item->data(Qt::UserRole).toString();
  if (!loadedSourceObjectId_.isEmpty() && objectId == loadedSourceObjectId_ && viewerStack_->currentIndex() == 0) {
    showViewerPage(1);
    return;
  }
  loadedSourceObjectId_ = objectId;
  QString name = item->text();
  const int splitAt = name.indexOf(QStringLiteral("  "));
  if (splitAt > 0) {
    name = name.left(splitAt);
  }
  if (auto* sourceTab = workspace_->findChild<QPushButton*>("viewerSourceTab")) {
    sourceTab->setText(name);
  }
  if (assets_->currentItem() != item) {
    assets_->setCurrentItem(item);
  }
  loadSourcePicture(objectId);
  showViewerPage(0);
}

std::vector<BinAssetFact> MainWindow::assetsForSelection(const std::string& binId, std::string* error) {
  std::vector<BinAssetFact> facts;
  if (binId != kAllMedia) {
    auto assets = session_.listBinAssets(binId);
    if (!assets.ok()) {
      *error = assets.error.message;
      return facts;
    }
    return assets.value;
  }
  auto bins = session_.listBins();
  if (!bins.ok()) {
    *error = bins.error.message;
    return facts;
  }
  std::set<std::string> seen;
  for (const BinRecord& bin : bins.value) {
    auto assets = session_.listBinAssets(bin.binId);
    if (!assets.ok()) {
      *error = assets.error.message;
      return {};
    }
    for (BinAssetFact& asset : assets.value) {
      if (!seen.insert(asset.objectId).second) {
        continue;
      }
      facts.push_back(std::move(asset));
    }
  }
  return facts;
}

void MainWindow::showSelection() {
  if (updatingSelection_) {
    return;
  }
  showRates();
  if (workspace_ != nullptr && !workspace_->selectedInstanceId().isEmpty()) {
    inspectorLabel_->setText(workspace_->inspectorText());
    previewCard_->hide();
    return;
  }
  assetChosen();
}

void MainWindow::showBinAssets() {
  rebuildingBins_ = true;
  assets_->clear();
  inspectorLabel_->setText("No media selected.");
  previewCard_->hide();
  if (!session_.isOpen() || bins_->currentItem() == nullptr) {
    emptyBinLabel_->setText(session_.isOpen() ? "Select a bin." : "No project is open.");
    rebuildingBins_ = false;
    refreshSourceMarks();
    return;
  }
  std::string error;
  const std::string binId = bins_->currentItem()->data(Qt::UserRole).toString().toStdString();
  const std::vector<BinAssetFact> assets = assetsForSelection(binId, &error);
  if (!error.empty()) {
    rebuildingBins_ = false;
    showError(Error{ErrorCode::Database, error});
    refreshSourceMarks();
    return;
  }
  if (assets.empty()) {
    emptyBinLabel_->setText("This bin has no media.");
    rebuildingBins_ = false;
    refreshSourceMarks();
    return;
  }
  emptyBinLabel_->setText("References only. Files are not copied.");
  for (const BinAssetFact& asset : assets) {
    auto* item = new QListWidgetItem(QString::fromStdString(asset.fileName + "  " + asset.presence), assets_);
    item->setData(Qt::UserRole, QString::fromStdString(asset.objectId));
    item->setData(Qt::UserRole + 1, QString::fromStdString(binId == kAllMedia ? std::string() : binId));
  }
  assets_->setCurrentRow(0);
  rebuildingBins_ = false;
}

void MainWindow::assetChosen() {
  if (updatingSelection_) {
    return;
  }
  if (!rebuildingBins_ && workspace_ != nullptr && !workspace_->selectedInstanceId().isEmpty() &&
      assets_->currentItem() != nullptr) {
    updatingSelection_ = true;
    workspace_->clearInstanceSelection();
    updatingSelection_ = false;
  }
  if (!session_.isOpen() || assets_->currentItem() == nullptr || bins_->currentItem() == nullptr) {
    inspectorLabel_->setText("No media selected.");
    previewCard_->hide();
    syncProbeSelection({}, {});
    if (workspace_ != nullptr) {
      workspace_->setOverwriteSource(QString(), QString());
    }
    refreshSourceMarks();
    return;
  }
  std::string error;
  std::string binId = assets_->currentItem()->data(Qt::UserRole + 1).toString().toStdString();
  if (binId.empty()) {
    binId = bins_->currentItem()->data(Qt::UserRole).toString().toStdString();
  }
  const std::string objectId = assets_->currentItem()->data(Qt::UserRole).toString().toStdString();
  const std::vector<BinAssetFact> assets = assetsForSelection(binId == kAllMedia ? std::string(kAllMedia) : binId, &error);
  if (!error.empty()) {
    showError(Error{ErrorCode::Database, error});
    refreshSourceMarks();
    return;
  }
  for (const BinAssetFact& asset : assets) {
    if (asset.objectId != objectId) {
      continue;
    }
    const std::string text = "file " + asset.fileName + "\nmedia " + asset.mediaId + "\nrepresentation " +
                             asset.representationId + "\nobject " + asset.objectId + "\npath " + asset.recordedPath +
                             "\nsize " + asset.sizeText + "\nmtime " + asset.mtimeUtc + "\npresence " + asset.presence +
                             "\nregistration " + asset.registrationStatus + "\ntechnical metadata " +
                             asset.technicalMetadata;
    inspectorLabel_->setText(QString::fromStdString(text));
    previewCard_->setText(QString::fromStdString(asset.fileName + "\nNo thumbnail. Not probed."));
    previewCard_->show();
    syncProbeSelection(asset.representationId, asset.recordedPath);
    if (workspace_ != nullptr) {
      workspace_->setOverwriteSource(QString::fromStdString(asset.objectId),
                                     QString::fromStdString(asset.representationId));
    }
    refreshSourceMarks();
    return;
  }
  inspectorLabel_->setText("No media selected.");
  previewCard_->hide();
  syncProbeSelection({}, {});
  if (workspace_ != nullptr) {
    workspace_->setOverwriteSource(QString(), QString());
  }
  refreshSourceMarks();
}

void MainWindow::importMedia() {
  if (!session_.isOpen()) {
    statusLabel_->setText("Open a project before importing media");
    return;
  }
  const QStringList picked = QFileDialog::getOpenFileNames(
      this, "Import Media", QString(),
      "Media references (*.mov *.mp4 *.mxf *.avi *.mkv *.mpg *.mpeg *.m4v *.wav *.aif *.aiff *.mp3 *.flac *.aac *.m4a)");
  importPaths(picked);
}

void MainWindow::importPaths(const QStringList& paths) {
  if (!session_.isOpen()) {
    statusLabel_->setText("Open a project before importing media");
    return;
  }
  if (paths.isEmpty()) {
    return;
  }
  std::vector<std::string> files;
  files.reserve(static_cast<std::size_t>(paths.size()));
  for (const QString& path : paths) {
    files.push_back(path.toStdString());
  }
  auto imported = session_.importReferences(files);
  if (!imported.ok()) {
    refresh();
    showError(imported.error);
    return;
  }
  logger_.info("media", "window registered media references");
  refresh();
}

std::string MainWindow::recordedPathForObject(const std::string& objectId) const {
  auto clip = session_.readMasterClip(objectId);
  if (!clip.ok()) {
    return {};
  }
  auto reps = session_.listRepresentations(clip.value.mediaId);
  if (!reps.ok()) {
    return {};
  }
  std::string fallback;
  for (const Representation& rep : reps.value) {
    if (rep.recordedPath.empty()) {
      continue;
    }
    if (rep.role == RepresentationRole::Original) {
      return rep.recordedPath;
    }
    if (fallback.empty()) {
      fallback = rep.recordedPath;
    }
  }
  return fallback;
}

QString MainWindow::sourcePictureLabel(bool playing) const {
  std::string spacing = " The container did not report a display interval.";
  std::int64_t microseconds = 0;
  std::string field;
  if (sourceReader_.reportedDisplayIntervalUs(&microseconds, &field)) {
    spacing = " Display spacing uses the container " + field + ". It is not an editorial time.";
  }
  return QString::fromStdString("Picture " + std::to_string(sourceIndex_) + "." + spacing +
                                (playing ? " Playing." : " Stopped."));
}

void MainWindow::loadSourcePicture(const QString& objectId) {
  stopSource();
  sourceReader_.close();
  sourceIndex_ = 0;
  const std::string path = recordedPathForObject(objectId.toStdString());
  std::string note;
  if (path.empty() || !sourceReader_.open(path, &note)) {
    const std::string reason = note.empty() ? std::string("The clip has no recorded path.") : note;
    if (sourceView_ != nullptr) {
      sourceView_->showNote(QString::fromStdString(reason));
    }
    if (sourceTimecode_ != nullptr) {
      sourceTimecode_->setText(QString::fromStdString(reason));
    }
    return;
  }
  const DisplayPicture picture = sourceReader_.pictureAt(0);
  if (!picture.present) {
    sourceReader_.close();
    const std::string reason = picture.note.empty() ? std::string("no picture") : picture.note;
    if (sourceView_ != nullptr) {
      sourceView_->showNote(QString::fromStdString(reason));
    }
    if (sourceTimecode_ != nullptr) {
      sourceTimecode_->setText(QString::fromStdString(reason));
    }
    return;
  }
  if (sourceView_ != nullptr) {
    sourceView_->showPicture(picture);
  }
  if (sourceTimecode_ != nullptr) {
    sourceTimecode_->setText(sourcePictureLabel(false));
  }
}

void MainWindow::playSource() {
  if (sourcePlaying_) {
    return;
  }
  if (!sourceReader_.isOpen()) {
    statusLabel_->setText("No source picture is open.");
    return;
  }
  std::int64_t microseconds = 0;
  std::string field;
  if (!sourceReader_.reportedDisplayIntervalUs(&microseconds, &field)) {
    statusLabel_->setText(QString::fromStdString(field));
    if (sourceTimecode_ != nullptr) {
      sourceTimecode_->setText(QString::fromStdString(field));
    }
    return;
  }
  sourcePlaying_ = true;
  if (sourceTimecode_ != nullptr) {
    sourceTimecode_->setText(sourcePictureLabel(true));
  }
  statusLabel_->setText("Source playing");
  sourceTimer_->start(std::max<int>(1, static_cast<int>(microseconds / 1000)));
}

void MainWindow::stopSource() {
  sourcePlaying_ = false;
  if (sourceTimer_ != nullptr) {
    sourceTimer_->stop();
  }
  if (sourceReader_.isOpen() && sourceTimecode_ != nullptr) {
    sourceTimecode_->setText(sourcePictureLabel(false));
  }
}

void MainWindow::advanceSource() {
  if (!sourcePlaying_ || !sourceReader_.isOpen()) {
    stopSource();
    return;
  }
  const DisplayPicture picture = sourceReader_.pictureAt(sourceIndex_ + 1);
  if (!picture.present) {
    stopSource();
    if (sourceView_ != nullptr) {
      sourceView_->showNote(QString::fromStdString(picture.note));
    }
    statusLabel_->setText(QString::fromStdString(picture.note.empty() ? "no picture" : picture.note));
    return;
  }
  sourceIndex_ = picture.mediaIndex;
  if (sourceView_ != nullptr) {
    sourceView_->showPicture(picture);
  }
  if (sourceTimecode_ != nullptr) {
    sourceTimecode_->setText(sourcePictureLabel(true));
  }
}

void MainWindow::placeLoadedSource() {
  if (workspace_ == nullptr || loadedSourceObjectId_.isEmpty()) {
    statusLabel_->setText("Open a clip in the source monitor before placing it.");
    return;
  }
  workspace_->beginPlacement(loadedSourceObjectId_, QString(), workspace_->playheadFrame());
  showViewerPage(1);
  showRecordPicture();
}

void MainWindow::updatePlayheadLabel() {
  if (playheadLabel_ == nullptr || recordTimecode_ == nullptr) {
    return;
  }
  if (!session_.isOpen() || workspace_ == nullptr) {
    recordTimecode_->setText("—");
    playheadLabel_->setText("Playhead is a frame position. Stopped.");
    return;
  }
  auto sequences = session_.listSequences();
  if (!sequences.ok() || sequences.value.empty()) {
    recordTimecode_->setText("No sequence");
    playheadLabel_->setText("No sequence. Playhead has no rate. Stopped.");
    return;
  }
  const QString selectedId = workspace_->selectedSequenceId();
  const SequenceRecord* selected = &sequences.value.front();
  for (const SequenceRecord& item : sequences.value) {
    if (QString::fromStdString(item.sequenceId) == selectedId) {
      selected = &item;
      break;
    }
  }
  const std::int64_t frame = workspace_->playheadFrame();
  const std::string label = formatFrameLabel(frame, selected->rate);
  const std::string shown = label.empty() ? "frame " + std::to_string(frame) : label;
  const char* state = recordPlaying_ ? "Playing." : "Stopped.";
  recordTimecode_->setText(QString::fromStdString(shown));
  playheadLabel_->setText(QString::fromStdString("Playhead " + shown + "  " + selected->name + ". " + state));
}

void MainWindow::showRecordPicture() {
  if (recordView_ == nullptr || workspace_ == nullptr) {
    return;
  }
  updatePlayheadLabel();
  if (!session_.isOpen()) {
    recordReader_.close();
    recordPath_.clear();
    recordView_->showNote("No timeline picture");
    return;
  }
  const PlayheadPicture hit = workspace_->pictureAtPlayhead();
  if (!hit.covered) {
    recordReader_.close();
    recordPath_.clear();
    recordView_->showNote(hit.note);
    if (recordPlaying_) {
      stopRecord();
    }
    return;
  }
  if (!recordReader_.isOpen() || recordPath_ != hit.path) {
    recordReader_.close();
    std::string note;
    if (!recordReader_.open(hit.path.toStdString(), &note)) {
      recordPath_.clear();
      recordView_->showNote(QString::fromStdString(note.empty() ? "The timeline picture did not open." : note));
      if (recordPlaying_) {
        stopRecord();
      }
      return;
    }
    recordPath_ = hit.path;
  }
  const DisplayPicture picture = recordReader_.pictureAt(hit.mediaIndex);
  if (!picture.present) {
    recordView_->showNote(QString::fromStdString(picture.note.empty() ? "no picture" : picture.note));
    if (recordPlaying_) {
      stopRecord();
    }
    return;
  }
  recordView_->showPicture(picture);
}

void MainWindow::playRecord() {
  if (recordPlaying_) {
    return;
  }
  if (workspace_ == nullptr || !session_.isOpen()) {
    statusLabel_->setText("No sequence is open.");
    return;
  }
  auto sequences = session_.listSequences();
  const SequenceRecord* selected = nullptr;
  if (sequences.ok()) {
    const QString selectedId = workspace_->selectedSequenceId();
    for (const SequenceRecord& item : sequences.value) {
      if (QString::fromStdString(item.sequenceId) == selectedId) {
        selected = &item;
        break;
      }
    }
  }
  if (selected == nullptr || !isAcceptedFrameRate(selected->rate) || selected->rate.numerator <= 0) {
    statusLabel_->setText("The stored sequence rate cannot pace playback.");
    return;
  }
  const std::int64_t end = workspace_->contentEndFrame();
  if (end <= 0) {
    statusLabel_->setText("The timeline has no picture to play.");
    if (recordView_ != nullptr) {
      recordView_->showNote("The timeline has no picture to play.");
    }
    return;
  }
  if (workspace_->playheadFrame() >= end) {
    workspace_->setPlayheadFrame(0);
  }
  const std::int64_t milliseconds =
      std::max<std::int64_t>(1, (1000 * selected->rate.denominator) / selected->rate.numerator);
  recordPlaying_ = true;
  showViewerPage(1);
  showRecordPicture();
  if (!recordPlaying_) {
    return;
  }
  statusLabel_->setText("Record playing");
  recordTimer_->start(static_cast<int>(milliseconds));
}

void MainWindow::stopRecord() {
  recordPlaying_ = false;
  if (recordTimer_ != nullptr) {
    recordTimer_->stop();
  }
  updatePlayheadLabel();
  if (session_.isOpen() && statusLabel_ != nullptr && statusLabel_->text() == "Record playing") {
    statusLabel_->setText("Stopped");
  }
}

void MainWindow::advanceRecord() {
  if (!recordPlaying_ || workspace_ == nullptr) {
    stopRecord();
    return;
  }
  const std::int64_t end = workspace_->contentEndFrame();
  const std::int64_t next = workspace_->playheadFrame() + 1;
  if (end <= 0 || next >= end) {
    stopRecord();
    statusLabel_->setText("Stopped");
    return;
  }
  workspace_->setPlayheadFrame(next);
  showRecordPicture();
}

void MainWindow::showRates() {
  if (!session_.isOpen()) {
    recordTimecode_->setText("—");
    playheadLabel_->setText("Playhead is a frame position. Stopped.");
    rateLabel_->setText("No sequence rate is stored");
    refreshSourceMarks();
    showRecordMarks();
    showRecordPicture();
    return;
  }
  auto sequences = session_.listSequences();
  if (!sequences.ok() || sequences.value.empty()) {
    recordTimecode_->setText("No sequence");
    playheadLabel_->setText("No sequence. Playhead has no rate. Stopped.");
    rateLabel_->setText("No sequence rate is stored");
    refreshSourceMarks();
    showRecordMarks();
    showRecordPicture();
    return;
  }
  const QString selectedId = workspace_ == nullptr ? QString() : workspace_->selectedSequenceId();
  const SequenceRecord* selected = nullptr;
  for (const SequenceRecord& item : sequences.value) {
    if (QString::fromStdString(item.sequenceId) == selectedId) {
      selected = &item;
      break;
    }
  }
  if (selected == nullptr) {
    selected = &sequences.value.front();
  }
  updatePlayheadLabel();
  QString rates = QString::fromStdString("Selected sequence " + selected->name + "  " +
                                         std::to_string(selected->rate.numerator) + "/" +
                                         std::to_string(selected->rate.denominator));
  rates += "\nStored sequence rate";
  for (const SequenceRecord& item : sequences.value) {
    rates += QString::fromStdString("\n" + item.name + "  " + std::to_string(item.rate.numerator) + "/" +
                                    std::to_string(item.rate.denominator));
  }
  rateLabel_->setText(rates);
  refreshSourceMarks();
  showRecordMarks();
  showRecordPicture();
}

void MainWindow::syncFromSession() { refresh(); }

void MainWindow::probeSelected() {
  if (probeController_ == nullptr || probeRepresentationId_.empty() || probePath_.empty()) {
    statusLabel_->setText("Select a representation before probing");
    return;
  }
  probeController_->probe(probeRepresentationId_, probePath_);
}

void MainWindow::showProbe() {
  if (probeStatus_ == nullptr || probeReport_ == nullptr || probeController_ == nullptr) {
    return;
  }
  probeStatus_->setText(probeController_->statusText());
  probeReport_->setPlainText(probeController_->reportText());
}

void MainWindow::syncProbeSelection(const std::string& representationId, const std::string& recordedPath) {
  probeRepresentationId_ = representationId;
  probePath_ = recordedPath;
  if (probeController_ != nullptr) {
    probeController_->showRepresentation(representationId);
  }
}

void MainWindow::refresh() {
  bins_->clear();
  assets_->clear();
  rootBinId_.clear();
  previewCard_->hide();
  probeRepresentationId_.clear();
  probePath_.clear();
  loadedSourceObjectId_.clear();
  stopSource();
  stopRecord();
  sourceReader_.close();
  recordReader_.close();
  recordPath_.clear();
  sourceIndex_ = 0;
  if (sourceView_ != nullptr) {
    sourceView_->showNote("No media loaded");
  }
  if (recordView_ != nullptr) {
    recordView_->showNote("No timeline picture");
  }
  if (auto* sourceTab = workspace_->findChild<QPushButton*>("viewerSourceTab")) {
    sourceTab->setText(QStringLiteral("Source"));
  }
  showViewerPage(1);
  if (probeController_ != nullptr) {
    probeController_->setProject(session_.isOpen() ? session_.info().projectId : std::string());
  }
  if (!session_.isOpen()) {
    setWindowTitle("Aether");
    nameLabel_->setText("Aether");
    statusLabel_->setText("Closed");
    emptyBinLabel_->setText("No project is open.");
    inspectorLabel_->setText("No media selected.");
    showRates();
    workspace_->reload();
    return;
  }
  setWindowTitle(QString::fromStdString("Aether — " + session_.info().name));
  nameLabel_->setText(QString::fromStdString(session_.info().name));
  statusLabel_->setText("Open");
  auto* allMedia = new QListWidgetItem("All Media", bins_);
  allMedia->setData(Qt::UserRole, QString::fromUtf8(kAllMedia));
  auto root = session_.ensureRootBin();
  if (!root.ok()) {
    showError(root.error);
    return;
  }
  auto bins = session_.listBins();
  if (!bins.ok()) {
    showError(bins.error);
    return;
  }
  for (const BinRecord& bin : bins.value) {
    auto* item = new QListWidgetItem(QString::fromStdString(bin.name), bins_);
    item->setData(Qt::UserRole, QString::fromStdString(bin.binId));
    if (bin.name == "Root") {
      rootBinId_ = bin.binId;
    }
  }
  bins_->setCurrentRow(0);
  showBinAssets();
  workspace_->reload();
  if (workspace_->selectedSequenceId().isEmpty()) {
    auto sequences = session_.listSequences();
    if (sequences.ok() && !sequences.value.empty()) {
      workspace_->selectSequence(QString::fromStdString(sequences.value.front().sequenceId));
    }
  }
  showRates();
}

void MainWindow::showError(const Error& error) {
  statusLabel_->setText(QString::fromStdString(std::string(errorCodeName(error.code)) + ": " + error.message));
  QMessageBox::warning(this, "Aether", QString::fromStdString(error.message));
}

void MainWindow::binChosen() {
  if (!session_.isOpen() || bins_->currentItem() == nullptr) {
    return;
  }
  const std::string binId = bins_->currentItem()->data(Qt::UserRole).toString().toStdString();
  if (binId != kAllMedia) {
    auto selected = session_.selectBin(binId);
    if (!selected.ok()) {
      showError(selected.error);
      return;
    }
  }
  showBinAssets();
}

void MainWindow::createProject() { showProjectDialog(); }

void MainWindow::openProject() { showProjectDialog(); }

void MainWindow::closeProject() {
  auto closed = session_.closeProject();
  if (!closed.ok()) {
    showError(closed.error);
  }
  refresh();
  if (!closing_) {
    showProjectDialog();
  }
}

namespace {

QString formatSourceMarks(const std::string& representationId, const MarkPoints& marks) {
  QString text = QString::fromStdString("Representation " + representationId + "\n");
  if (marks.markIn.has_value()) {
    text += QString::fromStdString("Mark In: " + std::to_string(*marks.markIn) + " source frames\n");
  } else {
    text += "Mark In: none\n";
  }
  if (marks.markOut.has_value()) {
    text += QString::fromStdString("Mark Out: " + std::to_string(*marks.markOut) + " source frames\n");
  } else {
    text += "Mark Out: none\n";
  }
  text += "Source frames. Not probed. No duration. No source rate.\n";
  if (marks.markIn.has_value() && marks.markOut.has_value()) {
    text += "Points only. Displayed inclusive OUT is not settled, so this pair is not a half-open edit range.";
  } else if (marks.markIn.has_value() || marks.markOut.has_value()) {
    text += "A single point is not an edit range.";
  } else {
    text += "No source marks are stored.";
  }
  return text;
}

}  // namespace

std::string MainWindow::selectedRepresentationId() {
  if (!session_.isOpen() || assets_ == nullptr || assets_->currentItem() == nullptr || bins_ == nullptr ||
      bins_->currentItem() == nullptr) {
    return {};
  }
  std::string error;
  std::string binId = assets_->currentItem()->data(Qt::UserRole + 1).toString().toStdString();
  if (binId.empty()) {
    binId = bins_->currentItem()->data(Qt::UserRole).toString().toStdString();
  }
  const std::string objectId = assets_->currentItem()->data(Qt::UserRole).toString().toStdString();
  const std::vector<BinAssetFact> assets =
      assetsForSelection(binId == kAllMedia ? std::string(kAllMedia) : binId, &error);
  if (!error.empty()) {
    return {};
  }
  for (const BinAssetFact& asset : assets) {
    if (asset.objectId == objectId) {
      return asset.representationId;
    }
  }
  return {};
}

void MainWindow::refreshSourceMarks() {
  const QString representationId = QString::fromStdString(selectedRepresentationId());
  const bool switched = representationId != entryRepresentationId_;
  if (switched) {
    entryRepresentationId_ = representationId;
    if (sourceMarkFrame_ != nullptr) {
      sourceMarkFrame_->clear();
    }
    if (sourceMarkReason_ != nullptr) {
      sourceMarkReason_->clear();
    }
  }
  if (sourceTimecode_ != nullptr && !sourceReader_.isOpen()) {
    sourceTimecode_->setText(representationId.isEmpty() ? "No media loaded"
                                                        : "Not probed. No source rate. No duration.");
  }
  if (sourceMarkReadout_ == nullptr) {
    return;
  }
  if (!session_.isOpen() || representationId.isEmpty()) {
    sourceMarkReadout_->setText("No representation is selected.");
    return;
  }
  auto marks = session_.sourceMarks(representationId.toStdString());
  if (!marks.ok()) {
    sourceMarkReadout_->setText(QString::fromStdString(marks.error.message));
    return;
  }
  sourceMarkReadout_->setText(formatSourceMarks(representationId.toStdString(), marks.value));
}

void MainWindow::showRecordMarks() {
  if (recordMarkMirror_ == nullptr) {
    return;
  }
  if (workspace_ == nullptr) {
    recordMarkMirror_->setText("No sequence is selected.");
    return;
  }
  recordMarkMirror_->setText(workspace_->recordMarkReadoutText());
}

void MainWindow::markSourceIn() { applySourcePoint(true); }

void MainWindow::markSourceOut() { applySourcePoint(false); }

void MainWindow::clearSourceIn() { clearSourcePoint(true, false); }

void MainWindow::clearSourceOut() { clearSourcePoint(false, true); }

void MainWindow::clearSourceBoth() { clearSourcePoint(true, true); }

void MainWindow::applySourcePoint(bool isIn) {
  const std::string representationId = selectedRepresentationId();
  refreshSourceMarks();
  if (representationId.empty()) {
    sourceMarkReason_->setText("the representation is not in this project");
    return;
  }
  const IntegerFrameParse parsed = parseIntegerFrame(sourceMarkFrame_->text().toStdString());
  if (!parsed.ok) {
    sourceMarkReason_->setText(QString::fromUtf8(parsed.reason));
    return;
  }
  const auto result = isIn ? session_.setSourceMarkIn(representationId, parsed.frame)
                           : session_.setSourceMarkOut(representationId, parsed.frame);
  refreshSourceMarks();
  sourceMarkReason_->setText(result.ok() ? QString() : QString::fromStdString(result.error.message));
}

void MainWindow::clearSourcePoint(bool clearIn, bool clearOut) {
  const std::string representationId = selectedRepresentationId();
  refreshSourceMarks();
  if (representationId.empty()) {
    sourceMarkReason_->setText("the representation is not in this project");
    return;
  }
  const Result<void> result = clearIn && clearOut ? session_.clearSourceMarks(representationId)
                               : clearIn          ? session_.clearSourceMarkIn(representationId)
                                                  : session_.clearSourceMarkOut(representationId);
  refreshSourceMarks();
  sourceMarkReason_->setText(result.ok() ? QString() : QString::fromStdString(result.error.message));
}

}  // namespace aether
